<?php
// $Id$
/**
 * @file
 * Git backend for Version Control API - Provides Git commit information and
 * account management as a pluggable backend.
 * FIXME add an action to operation_labels
 *
 * Copyright 2008 by Jimmy Berry ("boombatower", http://drupal.org/user/214218)
 * Copyright 2009-2010 by Cornelius Riemenschneider ("CorniI", http://drupal.org/user/136353)
 */

/**
 * Actually update the repository by fetching commits and other stuff
 * directly from the repository, invoking the git executable.
 * @param VersioncontrolGitRepository $repository
 * @return
 *   TRUE if the logs were updated, or FALSE if fetching and updating the logs
 *   failed for whatever reason.
 *
 * FIXME while largely ported to the new entities system, this is still not 100%
 * done
 */
function _versioncontrol_git_log_update_repository(VersioncontrolGitRepository &$repository) {
  if (!$repository->isValidGitRepo()) {
    drupal_set_message(t('The repository %name at <code>@root</code> is not a valid Git bare repository.', array('%name' => $repository->name, '@root' => $repository->root)), 'error');
    return FALSE;
  }

  $root = escapeshellcmd($repository->root);
  putenv("GIT_DIR=$root");
  if ($repository->locked != 0) {
    drupal_set_message(t('This repository is locked, there is already a fetch in progress. If this is not the case, press the clear lock button.'), 'error');
    return FALSE;
  }
  $repository->updateLock();
  $repository->update();

$autoinc_base = variable_get('versioncontrol_git_autoincrement_base', 0);
$autoinc_step = variable_get('versioncontrol_git_autoincrement_step', 1);

  // Is this a fresh import?
  $commits_in_db = $repository->loadCommits(array(), array(), array('may cache' => FALSE));
  if (empty($commits_in_db)) {
    $repo_id = $repository->repo_id;

    $file_names = _version_control_fast_export();

    $marks = _versioncontrol_git_log_process_marks($file_names[2]);
    $logs = _versioncontrol_git_log_process_logfile($file_names[1], $marks);
    $numlines = _versioncontrol_git_log_process_numstat($file_names[0]);

    $values = array();
    $start = db_result(db_query('SELECT MAX(vc_op_id) FROM {versioncontrol_operations}'));
    $query = db_insert('versioncontrol_operations')->fields(array('type', 'repo_id', 'date', 'author', 'author_uid', 'committer', 'committer_uid', 'revision', 'message'));
    if (!is_numeric($start) || $start == 0) {
      $start = $autoinc_base;
    }
    foreach ($logs as $log) {
      $start += $autoinc_step;
      $values = array(
        'type' => 1,
        'repo_id' => $repo_id,
        'date' => $log->date,
        'author' => $log->author,
        'author_uid' => 0,
        'committer' => $log->committer,
        'committer_uid' => 0,
        'revision' => $log->revision,
        'message' => $log->msg,
      );
      $log->vc_op_id = $start;
      $query->values($values);
    }
    if ($values) {
      $query->execute();
    }
    $end = db_result(db_query('SELECT MAX(vc_op_id) FROM {versioncontrol_operations} WHERE repo_id = '. $repo_id));
    if ($start != $end) {
      watchdog('vc_git', "Concurrency problem during inital import of {versioncontrol_operations} for repo id #$repo_id. Expected $start, got $end. Recovering.");
      $result = db_query("SELECT vc_op_id, revision FROM {versioncontrol_operations} WHERE repo_id = $repo_id ORDER BY vc_op_id ASC");
      foreach ($logs as $log) {
        $r = db_fetch_array($result);
        if ($r['revision'] == $log->revision) {
          $log->vc_op_id = $r['vc_op_id'];
        }
        else {
          drupal_set_message("Mismatch when correcting {versioncontrol_operations} autoincrement mapping.");
        }
      }
    }

    $values = array();
    $start = db_result(db_query('SELECT MAX(item_revision_id) FROM {versioncontrol_item_revisions}')); //Make this happen in the same transaction.
    if (!is_numeric($start) || $start == 0) {
      $start = $autoinc_base;
    }
    $query = db_insert('versioncontrol_item_revisions')->fields(array('vc_op_id', 'repo_id', 'path', 'revision', 'type', 'source_item_revision_id', 'action', 'line_changes_added', 'line_changes_removed'));
    $source_id = 0;
    foreach ($logs as $log) {
      foreach($log->files as $hash => $file) {
        $start += $autoinc_step;
        $log->files[$hash]['rev_id'] = $start;
        $log->name_block->updateAncestor($log->revision, $file['name'], $start);
        $type = $file['type'] == VERSIONCONTROL_ACTION_DELETED ? VERSIONCONTROL_ITEM_FILE_DELETED : VERSIONCONTROL_ITEM_FILE;
        //define('VERSIONCONTROL_ITEM_DIRECTORY',         2); Git doesn't track directories so these are never used.
        //define('VERSIONCONTROL_ITEM_DIRECTORY_DELETED', 4);
        if (isset($numlines[$log->revision][$file['name']][0])) {
          $lines_added = $numlines[$log->revision][$file['name']][0];
          $lines_removed = $numlines[$log->revision][$file['name']][1];
        }
        else {
          $lines_added = 0;
          $lines_removed = 0;
        }
        $values = array(
          'vc_op_id' => $log->vc_op_id,
          'repo_id' => $repo_id,
          'path' => $file['name'],
          'revision' => $log->revision,
          'type' => $type,
          'source_item_revision_id' => $log->name_block->getAncestor( $file['name'], $log->revision),
          'action' => $file['type'],
          'line_changes_added' => $lines_added,
          'line_changes_removed' => $lines_removed,
        );
        $query->values($values);
      }
    }
    if ($values) {
      $query->execute();
    }
    $end = db_result(db_query('SELECT MAX(item_revision_id) FROM {versioncontrol_item_revisions} WHERE repo_id = '. $repo_id));
    if ($start != $end) {
      watchdog('vc_git', "Concurrency error during inital import of {versioncontrol_item_revisions} for repo id #$repo_id. Expected $start, got $end. Recovering.");
      $logs[0]->name_block->history = array();
      $result = db_query("SELECT item_revision_id, path FROM {versioncontrol_item_revisions} WHERE repo_id = $repo_id ORDER BY item_revision_id ASC");
      foreach ($logs as $log) {
        foreach($log->files as $hash => $file) {
          $r = db_fetch_array($result);
          if ($r['path'] == $file['name']) {
            $log->files[$hash]['rev_id'] = $r['item_revision_id'];
            $log->name_block->updateAncestor($log->revision, $file['name'], $r['item_revision_id']);
          }
          else {
            drupal_set_message("Mismatch when correcting {versioncontrol_item_revisions} autoincrement mapping.");
          }
        }
      }
    }

    $query = db_insert('versioncontrol_git_item_revisions')->fields(array('item_revision_id', 'blob_hash'));
    $values = array();
    foreach ($logs as $log) {
      foreach(array_keys($log->files) as $hash) {
        $values = array(
          'item_revision_id' => $log->files[$hash]['rev_id'],
          'blob_hash' => $hash,
        );
        $query->values($values);
      }
    }
    if ($values) {
      $query->execute();
    }


    $query = db_insert('versioncontrol_git_operations')->fields(array('vc_op_id', 'author_name', 'committer_name', 'parent_commit', 'merge'));
    $values = array();
    foreach ($logs as $log) {
      $merge = empty($log->merge) ? 0 : 1; // We don't reference the additional merge parents by commit.
      $values = array(
        'vc_op_id' => $log->vc_op_id,
        'author_name' => $log->author,
        'committer_name' => $log->committer,
        'parent_commit' => $log->from,
        'merge' => $merge,
      );
      $query->values($values);
    }
    if ($values) {
      $query->execute();
    }


    //db_query('LOCK TABLES {versioncontrol_labels} WRITE');
    $values = '';
    $start = db_result(db_query('SELECT MAX(label_id) FROM {versioncontrol_labels}'));
    if (!is_numeric($start) || $start == 0) {
      $start = $autoinc_base;
    }
    $query = db_insert('versioncontrol_labels')->fields(array('repo_id', 'name', 'type', 'data'));
    $log = $logs[0];
    $names = $log->name_block;
    foreach (array_keys($names->names) as $name) {
      $start += $autoinc_step;
      $names->names[$name]['label_id'] = $start; //Save the new label_id in the shared nameBlock for later.
      $values = array(
        'repo_id' => $repo_id,
        'name' => $name,
        'type' => $log->type,
        'data' => '',
      );
      $query->values($values);
    }
    if ($values) {
      $query->execute();
    }
    $end = db_result(db_query('SELECT MAX(label_id) FROM {versioncontrol_labels} WHERE repo_id = '. $repo_id));
    if ($start != $end) {
      watchdog('vc_git', "Concurrency error during inital import of {versioncontrol_labels} for repo id #$repo_id. Expected $start, got $end. Recovering.");
      $result = db_query("SELECT label_id, name FROM {versioncontrol_labels} WHERE repo_id = $repo_id ORDER BY label_id ASC");
      $log = $logs[0];
      $names = $log->name_block;
      foreach (array_keys($names->names) as $name) {
        $r = db_fetch_array($result);
        if ($r['name'] == $name) {
          $names->names[$name]['label_id'] = $r['label_id'];
        }
        else {
          drupal_set_message("Mismatch when correcting {versioncontrol_labels} autoincrement mapping.");
        }
      }
    }

    $log = $logs[0];
    $names = $log->name_block;
    foreach (array_keys($names->names) as $name) {
      if ($names->names[$name]['type'] == VERSIONCONTROL_LABEL_BRANCH) {
        $label_obj = (object) array('name' => $name);
        $revs = _versioncontrol_git_log_get_commits_in_label($label_obj);
        foreach ($revs as $rev) {
          $label_map[$rev][] = $name;
        }
      }
    }

    $query = db_insert('versioncontrol_operation_labels')->fields(array('vc_op_id', 'label_id', 'action'));
    $values = '';
    foreach ($logs as $log) {
      if (empty($label_map[$log->revision])) {
        continue;
      }
      // If the same commit appears in 10 branches we should have 10 rows.
      foreach ($label_map[$log->revision] as $name) {
        $values = array(
          'vc_op_id' => $log->vc_op_id,
          'label_id' => $log->name_block->names[$name]['label_id'],
          'action' => 2, // It's always two on the live db. Column will be removed.
        );
        $query->values($values);
      }
    }
    unset($label_map);
    if ($values) {
      $query->execute();
    }

    $repository->updated = time();
    $repository->updateLock(0);
    $repository->update();

    return TRUE;
  }

  // 1. Process branches

  // Fetch branches from the repo and load them from the db.
  $branches_in_repo = $repository->fetchBranches();
  $branches_in_db = $repository->loadBranches(array(), array(), array('may cache' => FALSE));
  $branches_in_db_by_name = array();
  foreach ($branches_in_db as $branch) { // Branches get keyed on id; key on name instead.
    $branches_in_db_by_name[$branch->name] = $branch;
  }
  unset($branches_in_db);
  // Determine whether we've got branch changes to make.
  $branches_new = array_diff_key($branches_in_repo, $branches_in_db_by_name);
  $branches_deleted = array_diff_key($branches_in_db_by_name, $branches_in_repo);
  // Insert new branches in the repository. Later all commits in these new
  // branches will be updated.
  // Unfortunately we can't say anything about the branch author at this time.
  // The post-update hook could do this, though.
  // We also can't insert a VCOperation which adds the branch, because
  // we don't know anything about the branch. This is all stuff a hook could
  // figure out.
  foreach($branches_new as $branch) {
    $branch->insert();
  }
  // reload branches, after they was inserted
  $branches_in_db = $repository->loadBranches();
  $branches_in_db_by_name = array();
  foreach ($branches_in_db as $branch) {
    $branches_in_db_by_name[$branch->name] = $branch;
  }
  // Deleted branches are removed, commits in them are not!
  foreach($branches_deleted as $branch) {
    $branch->delete();
  }
  // 2. Process commits
  // Fetch commits from the repo and load them from the db.
  $commits_in_repo_hashes = $repository->fetchCommits();
  $commits_in_db = $repository->loadCommits(array(), array(), array('may cache' => FALSE));
  $commits_in_db_hashes = array();
  foreach ($commits_in_db as $commit) {
    $commits_in_db_hashes[] = $commit->revision;
  }
  unset($commits_in_db);
  $commits_new = array_diff($commits_in_repo_hashes, $commits_in_db_hashes);
  // Insert new commits in the database.
  foreach ($commits_new as $hash) {
    $command = "show --numstat --summary --pretty=format:\"%H%n%P%n%an%n%ae%n%cn%n%ce%n%ct%n%s%n%b%nENDOFOUTPUTGITMESSAGEHERE\" " . escapeshellarg($hash);
    $output = _versioncontrol_git_log_exec($command);
    _versioncontrol_git_log_parse_and_insert_commit($repository, $output, $branches_in_db_by_name);
  }
  // 3. Process tags
  // Insert new tags in the database.
  $tags_in_repo = $repository->fetchTags();
  $tags_in_db = $repository->loadTags();
  $tags_in_db_by_name = array();
  foreach ($tags_in_db as $tag) {
    $tags_in_db_by_name[$tag->name] = $tag;
  }
  unset($tags_in_db);
  // Deleting tags is *not* supported. Read the manual if you want to know why...
  // Check for new tags.
  $tags_new = array_diff_key($tags_in_repo, $tags_in_db_by_name);
  if (!empty($tags_new)) {
    _versioncontrol_git_log_process_tags($repository, $tags_new);
  }

  // Update repository updated field. Displayed on administration interface for documentation purposes.
  $repository->updated = time();
  $repository->updateLock(0);
  $repository->update();
  return TRUE;
}
/// All general functions
function _version_control_fast_export() {
  $git_bin = variable_get('versioncontrol_git_binary_path', 'git');
  if ($errors = _versioncontrol_git_binary_check_path($git_bin)) {
    watchdog('vc_git', '!errors', array('!errors' => implode('<br />', $errors)), WATCHDOG_ERROR);
    return array();
  }
  $tmp_mark = tempnam(file_directory_temp(), 'vclpm');
  $tmp_log = tempnam(file_directory_temp(), 'vclpl');
  $cmd = "fast-export --no-data --tag-of-filtered-object=drop --signed-tags=verbatim --export-marks=$tmp_mark --all";
  exec(escapeshellcmd("$git_bin $cmd") ." > $tmp_log", $logs);
  $tmp_stat = tempnam(file_directory_temp(), 'vclps');
  $cmd = "log --numstat --pretty=format:%H";
  exec(escapeshellcmd("$git_bin $cmd") ." > $tmp_stat", $logs);

  return array($tmp_stat, $tmp_log, $tmp_mark);
}
/**
 * Execute a Git command using the root context and the command to be executed.
 * @param string $command Command to execute.
 * @return mixed Logged output from the command in either array of file pointer form.
 */
function _versioncontrol_git_log_exec($command) {
  $logs = array();
  $git_bin = variable_get('versioncontrol_git_binary_path', 'git');
  if ($errors = _versioncontrol_git_binary_check_path($git_bin)) {
    watchdog('vc_git', '!errors', array('!errors' => implode('<br />', $errors)), WATCHDOG_ERROR);
    return array();
  }
  exec(escapeshellcmd("$git_bin $command"), $logs);
  array_unshift($logs, ''); // FIXME doing it this way is just wrong.
  reset($logs); // Reset the array pointer, so that we can use next().
  return $logs;
}
/// All commit related function

/**
 * Returns an array of all branches a given commit is in.
 * @param string $revision
 * @param array $branch_label_list
 * @return VersioncontrolBranch
 */
function _versioncontrol_git_log_get_branches_of_commit($revision, $branch_label_list) {
  $exec = 'branch --contains ' . escapeshellarg($revision);
  $logs = _versioncontrol_git_log_exec($exec);
  $branches = array();
  while (($line = next($logs)) !== FALSE) {
    $line = trim($line);
    if($line[0] == '*') {
      $line = substr($line, 2);
    }
    $branches[] = $branch_label_list[$line];
  }
  return $branches;
}

/**
 * This function returns all commits in the repository
 * @param $repository
 * @return array An array of strings with all commit id's in it
 */
function _versioncontrol_git_log_get_commits_in_repo($repository) {
  $logs = _versioncontrol_git_log_exec("rev-list --all");
  $commits = array();
  while (($line = next($logs)) !== FALSE) {
    $commits[] = trim($line);
  }
  return $commits;
}

/**
 * This function returns all commits in the given branch.
 * @param VersioncontrolBranch $branch
 * @return array An array of strings with all commit id's in it
 */
function _versioncontrol_git_log_get_commits_in_label($label) {
  $logs = _versioncontrol_git_log_exec("rev-list " . escapeshellarg($label->name) . " --");
  $commits = array();
  while (($line = next($logs)) !== FALSE) {
    $commits[] = trim($line);
  }
  return $commits;
}

/**
 * A function to fill in the source_item for a specific VersioncontrolItem.
 *
 * Now VCS API assumes there is only one source item, so merges can not be
 * tracked propertly there, and we are neither tracking on git backend for
 * now.
 * For merges we are choosing the first parent git-log  show.
 *
 * @param VersioncontrolItem &$item
 * @param array $parents The parent commit(s)
 * @return none
 */
function _versioncontrol_git_fill_source_item(&$item, $parents, $inc_data) {
  $data = array(
    'type' => VERSIONCONTROL_ITEM_FILE,
    'repository' => $inc_data['repository'],
    'path' => $item->path,
  );

  $path_stripped = substr($item->path, 1);
  // using -5 to let detect merges until 4 parents, merging more than 4 parents in one operation is insane!
  // use also --first-parent to retrieve only one parent for the current support of VCS API
  $cmd = 'log --first-parent --follow --pretty=format:"%H" -5 ' . escapeshellarg($item->revision) . ' -- ' . escapeshellarg($path_stripped);
  $prev_revisions = _versioncontrol_git_log_exec($cmd);

  next($prev_revisions); // grab our hash out
  if (($parent_hash = next($prev_revisions)) !== FALSE) { // get the first parent hash
    $data['revision'] = trim($parent_hash);
    // just fill an object from scratch
    $source_item = new VersioncontrolGitItem($item->getBackend());
    $source_item->build($data);
    $item->setSourceItem($source_item);
  }
  //TODO unify the way to fail
}

/**
 * Takes parts of the output of git log and returns all affected OperationItems for a commit.
 * @param array $logs
 * @param string $line
 * @param string $revision
 * @param array $parents The parent commit(s)
 * @param bool $merge
 * @return array All items affected by a commit.
 */
function _versioncontrol_git_parse_items(&$logs, &$line, $data, $parents) {
  $op_items = array();

  // Parse the diffstat for the changed files.
  do {
    if (!preg_match('/^(\S+)' . "\t" . '(\S+)' . "\t" . '(.+)$/', $line, $matches)) {
      break;
    }
    $path = '/'. $matches[3];
    $op_items[$path] = new VersioncontrolGitItem($data['backend']);
    $data['path'] = $path;
    $op_items[$path]->build($data);
    unset($data['path']);

    if (is_numeric($matches[1]) && is_numeric($matches[2])) {
      $op_items[$path]->line_changes_added = $matches[1];
      $op_items[$path]->line_changes_removed = $matches[2];
    }

    // extract blob
    $command = 'ls-tree -r ' . escapeshellarg($data['revision']) . ' ' . escapeshellarg($matches[3]);
    $lstree_lines = _versioncontrol_git_log_exec($command);
    $blob_hash = _versioncontrol_git_parse_item_blob($lstree_lines);
    $op_items[$path]->blob_hash = $blob_hash;
  } while (($line = next($logs)) !== FALSE);

  // Parse file actions.
  do {
    if (!preg_match('/^ (\S+) (\S+) (\S+) (.+)$/', $line, $matches)) {
      break;
    }
    // We also can get 'mode' here if someone changes the file permissions.
    if ($matches[1] == 'create') {
      $op_items['/'. $matches[4]]->action = VERSIONCONTROL_ACTION_ADDED;
      // extract blob
      $command = 'ls-tree -r ' . escapeshellarg($data['revision']) . ' ' . escapeshellarg($matches[4]);
      $lstree_lines = _versioncontrol_git_log_exec($command);
      $blob_hash = _versioncontrol_git_parse_item_blob($lstree_lines);
      $op_items['/'. $matches[4]]->blob_hash = $blob_hash;
    }
    else if ($matches[1] == 'delete') {
      $op_items['/'. $matches[4]]->action = VERSIONCONTROL_ACTION_DELETED;
      $op_items['/'. $matches[4]]->type = VERSIONCONTROL_ITEM_FILE_DELETED;
    }
  } while (($line = next($logs)) !== FALSE);

  // Fill in the source_items for non-added items
  foreach ($op_items as $path => &$item) {
    if ($item->action != VERSIONCONTROL_ACTION_ADDED) {
      _versioncontrol_git_fill_source_item($item, $parents, $data);
    }
  }
  return $op_items;
}

/**
 * Parse ls-tree with one commit hash and one item.
 */
function _versioncontrol_git_parse_item_blob($lines) {
  $line = next($lines);
  // output: <mode> SP <type> SP <object> TAB <file>
  $info = explode("\t", $line);
  $info = array_shift($info);
  $info = explode(' ', $info);
  $blob_hash = array_pop($info);
  return $blob_hash;
}

/**
 * Parse the output of 'git log' and insert a commit based on its data.
 *
 * @param VersioncontrolRepository $repository
 * @param array $logs The output of 'git log' to parse
 * @param array $branch_label_list An associative list of branchname => VersioncontrolBranch
 */
function _versioncontrol_git_log_parse_and_insert_commit(VersioncontrolRepository $repository, $logs, $branch_label_list) {
  // Get commit hash (vcapi's "revision")
  $revision = trim(next($logs));

  // Get parent commit hash(es)
  $parents = explode(' ', trim(next($logs)));
  if ($parents[0] == '') {
    $parents = array();
  }
  $merge = isset($parents[1]); // Multiple parents indicates a merge

  // Get author data
  $author_name = trim(next($logs));
  $author_email = trim(next($logs));
  // Get committer data
  $committer_name = trim(next($logs));
  $committer_email = trim(next($logs));

  // Get date as timestamp
  $date = trim(next($logs));

  // Get revision message.
  // TODO: revisit!
  $message = '';
  $i = 0;
  while (($line = trim(next($logs))) !== FALSE) {
    if ($line == 'ENDOFOUTPUTGITMESSAGEHERE') {
      if (substr($message, -2) === "\n\n") {
        $message = substr($message, 0, strlen($message) - 1);
      }
      break;
    }
    if ($i == 1) {
      $message .= "\n";
    }
    $message .= $line ."\n";
    $i++;
  }

  // This is either a (kind of) diffstat for each modified file or a list of
  // file actions like moved, created, deleted, mode changed.
  $line = next($logs);

  // build the data array to be used for the commit op
  $op_data = array(
    'type' => VERSIONCONTROL_OPERATION_COMMIT,
    'revision' => $revision,
    'author' => $author_email,
    'author_name' => $author_name,
    'committer' => $committer_email,
    'committer_name' => $committer_name,
    'parent_commit' => reset($parents),
    'merge' => $merge,
    'date' => $date,
    'message' => $message,
    'repository' => $repository,
  );

  $op = new VersioncontrolGitOperation($repository->getBackend());
  $op->build($op_data);
  $op->labels = _versioncontrol_git_log_get_branches_of_commit($revision, $branch_label_list);
  $op->insert(array('map users' => TRUE));

  $item_action = $merge ? VERSIONCONTROL_ACTION_MERGED : VERSIONCONTROL_ACTION_MODIFIED;
  // build the data array to be used as default values for the item revision
  $default_item_data = array(
    // pass backend in data array to avoid just another param to parse
    // item function
    'backend' => $repository->getBackend(),
    'repository' => $repository,
    'vc_op_id' => $op->vc_op_id,
    'type' => VERSIONCONTROL_ITEM_FILE,
    'revision' => $revision,
    'action' => $item_action,
  );

  // Parse in the raw data and create VersioncontrolGitItem objects.
  $op_items = _versioncontrol_git_parse_items($logs, $line, $default_item_data, $parents);
  $op->itemRevisions = $op_items;
  $op->save(array('nested' => TRUE));
}

/// Tag related functions

/**
 * Returns a list of all existing tags in the database.
 * @param VersioncontrolRepository $repository
 * @return array An array of tag names.
 */
function _versioncontrol_git_log_get_tags_in_db($repository) {
  return _versioncontrol_git_log_get_labels_in_db($repository, VERSIONCONTROL_LABEL_TAG);
}

/**
 * Get all tags present in the repository.
 * @return array
 */
function _versioncontrol_git_log_get_tags_in_repo() {
  $log = _versioncontrol_git_log_exec('show-ref --tags'); // Query tags.
  $tags = array();
  while (($line = next($log)) !== FALSE) {
  	// the output of git show-ref --tags looks like this
  	// 94a5915923d5a9a6af935e4055c95582fd1a1136 refs/tags/DRUPAL-5--1-0
  	// to get 'DRUPAL-5--1-0', we have to skip 51 chars.
    $tags[] = substr(trim($line), 51);
  }
  return $tags;
}

/**
 * Returns a string with fully qualified tag names from an array of tag names.
 * @param array $tags
 * @return string
 */
function _versioncontrol_git_get_tag_string($tags) {
  $tag_string = '';
  // $tag_string is a list of fully qualified tag names
  foreach ($tags as $tag) {
    $tag_string .= escapeshellarg("refs/tags/{$tag->name}") . ' ';
  }
  return $tag_string;
}

/**
 * Returns a list of tag names with the tagged commits.
 * Handles annotated tags.
 * @param array $tags An array of tag names
 * @return array A list of all tags with the respective tagged commit.
 */
function _versioncontrol_git_log_get_tag_commit_list($tags) {
  if(empty($tags)) {
    return array();
  }
  $tag_string = _versioncontrol_git_get_tag_string($tags);
  $exec = "show-ref -d $tag_string";
  $tag_commit_list_raw = _versioncontrol_git_log_exec($exec);
  $tag_commit_list = array();
  $tags_annotated = array();
  foreach($tag_commit_list_raw as $tag_commit_line) {
  	if($tag_commit_line == '') {
  	  continue;
  	}
  	$tag_commit = substr($tag_commit_line, 0, 40);
  	// annotated tag mark
  	// 9c70f55549d3f4e70aaaf30c0697f704d02e9249 refs/tags/tag^{}
  	if (substr($tag_commit_line, -3, 3) == '^{}') {
  	  $tag_name = substr($tag_commit_line, 51, -3);
      $tags_annotated[$tag_name] = $tag_commit;
  	}
  	// Simple tags
  	// 9c70f55549d3f4e70aaaf30c0697f704d02e9249 refs/tags/tag
  	else {
      $tag_name = substr($tag_commit_line, 51);
  	}
    $tag_commit_list[$tag_name] = $tag_commit;
  }
  // Because annotated tags show up twice in the output of git show-ref, once
  // with a 'tag' object and once with a commit-id we will go through them and
  // adjust the array so we just keep the commits.
  foreach($tags_annotated as $tag_name => $tag_commit) {
  	$tag_commit_list[$tag_name] = $tag_commit;
  }
  return $tag_commit_list;
}

/**
 * Does all processing to insert the tags in $tags_new in the database.
 * @param VersioncontrolGitRepository $repository
 * @param array $tags_new All new tags.
 * @return none
 */
function _versioncontrol_git_log_process_tags($repository, $tags_new) {
  if (empty($tags_new)) {
    return;
  }

  // get a list of all tag names with the corresponding commit.
  $tag_commit_list = _versioncontrol_git_log_get_tag_commit_list($tags_new);
  $format = '%(objecttype)%0a%(objectname)%0a%(refname)%0a%(taggername) %(taggeremail)%0a%(taggerdate)%0a%(contents)ENDOFGITTAGOUTPUTMESAGEHERE';
  foreach($tag_commit_list as $tag_name => $tag_commit) {
    $exec = "for-each-ref --format=\"$format\" refs/tags/" . escapeshellarg($tag_name);
  	$logs_tag_msg = _versioncontrol_git_log_exec($exec);

    $tag_ops = $repository->loadCommits(array(), array('revision' => $tag_commit));
    $tagged_commit_op = reset($tag_ops);
  	// Get the specific tag data for annotated vs not annotated tags.
    if ($logs_tag_msg[1] == 'commit') {
      // simple tag
      // [2] is tagged commit [3] tagname [4] and [5] empty [6] commit log message
      // We get the tagger, the tag_date and the tag_message from the tagged commit.
      $tagger = $tagged_commit_op->author;
      $tag_date = $tagged_commit_op->date + 1;
      $message = $tagged_commit_op->message;
    }
    else if($logs_tag_msg[1] == 'tag') {
      // annotated tag
      // [2] is the tagged commit [3] tag name
      $tagger = $logs_tag_msg[4];
      $tag_date = strtotime($logs_tag_msg[5]);
      // Get the tag message
      $message = '';
      $i = 0;
      while (true) {
      	$line = $logs_tag_msg[$i + 6];
      	if($logs_tag_msg[$i + 7] == 'ENDOFGITTAGOUTPUTMESAGEHERE') {
      		$message .= $line;
      		break;
      	}
      	$message .= $line ."\n";
        $i++;
      }
    }
    else {
      watchdog('vc_git', 'Serious problem in tag parsing, please check that you\'re using a supported version of git!');
    }

    $tag_data = array(
      'name' => $tag_name,
      'repository' => $repository,
      'repo_id' => $repository->repo_id,
    );

    $tag = $repository->getBackend()->buildEntity('tag', $tag_data + array('action' => VERSIONCONTROL_ACTION_ADDED));
    $tag->insert();
  }
}

function _versioncontrol_git_log_process_marks($mark_file) {
  $lines = file($mark_file);
  $marks = array();
  foreach ($lines as $line) {
    $e = explode(' ', $line);
    $marks[intval(ltrim($e[0], ':'))] = trim($e[1]);
  }
  unlink($mark_file);
  return $marks;
}


function _versioncontrol_git_log_process_numstat($stats_file) {
  $lines = file($stats_file);
  $stats = array();
  $hash = GIT_NULL_REV;
  $mark = array();
  foreach ($lines as $line) {
    if (empty($line)) {
      continue;
    }
    if (preg_match('/^([a-f,0-9]{40})$/', $line, $mark)) {
      $hash = $mark[1];
      continue;
    }
    if (preg_match('/^(\d+)\t(\d+)\t(.+)$/', $line, $mark)) {
      $name = '/'. $mark[3];
      $stats[$hash][$name] = array($mark[1], $mark[2]);
    }
  }
  unlink($stats_file);
  return $stats;
}

function _versioncontrol_git_log_process_logfile($log_file, &$marks) {
  $fp = fopen($log_file, 'rb');
  $name_block = new nameBlock();
  $log = array();
  $data_length = 0;
  while (($line = fgets($fp)) !== FALSE) {
    $mark = array();
    if (empty($line)) {
      continue;
    }
    do {
      $repeat = FALSE;
      // Add in tagger.
      if ($data_length) {
        if ($data_length >= strlen($line)) {
          $data_length -= $working->addMsg($line);
        }
        else {
          $data_length = 0;
          $len = $working->addMsg($line);
          $line = substr($line, $len);
          $repeat = TRUE;
        }
      }
      elseif (preg_match('/^commit\b/', $line)) {
        if (isset($working)) {
          $working->updateCommit();
          $log[] = $working;
        }
        $working = new logBlock();
        $working->name_block = $name_block;
        $working->setCommit($line);
      }
      elseif (preg_match('/(.) (\d+) ([a-f,0-9]{40}) (.+)$/', $line, $mark)) {
        $working->addFile($mark[1], $mark[2], $mark[3], $mark[4]);
      }
      elseif (preg_match('/^data (\d+)$/', $line, $mark)) {
        $data_length = $mark[1];
      }
      elseif (preg_match('/^mark :(\d+)$/', $line, $mark)) {
        $working->revision = $marks[$mark[1]];
      }
      elseif (preg_match('/^from :(\d+)$/', $line, $mark)) {
        $working->from = $marks[$mark[1]];
      }
      elseif (preg_match('/^author[^<]*<([^>]+)> (\d+)/', $line, $mark)) {
        $working->author = $mark[1];
        $working->date = $mark[2];
      }
      elseif (preg_match('/^committer[^<]*<([^>]+)> (\d+)/', $line, $mark)) {
        $working->committer = $mark[1];
      }
      elseif (preg_match('/^merge :(\d+)$/', $line, $mark)) {
        $working->merge[] = $marks[$mark[1]];
      }
    } while ($repeat);
  }
  $working->updateCommit();
  $log[] = $working;
  fclose($fp);
  unlink($log_file);
  return $log;
}

class logBlock {
  public $files = array();
  public $commit = ''; //This actually holds branch/tag names.
  public $type = 0;
  public $revision = '';
  public $date = 0;
  public $author = '';
  public $committer = '';
  public $msg = '';
  public $from = GIT_NULL_REV;
  public $vc_op_id;
  public $name_block;
  public $merge = array();

  function addMsg($line) {
    $this->msg .= $line;
    return strlen($line);
  }

  function setCommit($commit) {
    $parts = explode(' ', $commit);
    $ref = trim($parts[1]);
    $path = explode('/', $ref);
    $this->commit = array_pop($path);
    // We've popped the name off so if any remaining piece says tag we have a tag.
    if (in_array('tags', $path)) {
      $this->type = VERSIONCONTROL_LABEL_TAG;
      // Annotated tags are style of tag: and have a tagger.
    }
    else {
      $this->type = VERSIONCONTROL_LABEL_BRANCH;
    }
  }

  function updateCommit() {
    $this->name_block->add($this->commit, $this->type);
  }

  function addfile($type, $mode, $hash, $name) {
    $name = '/'. $name; // Remove this later.
    switch ($type) {
      case 'M':
        $type = VERSIONCONTROL_ACTION_MODIFIED;
        break;
      case 'A':
        $type = VERSIONCONTROL_ACTION_ADDED;
        break;
      case 'D':
        $type = VERSIONCONTROL_ACTION_DELETED;
        break;
      case 'R':
        $type = VERSIONCONTROL_ACTION_MOVED;
        break;
      case 'C':
        $type = VERSIONCONTROL_ACTION_COPIED;
        break;
      case '?':
        $type = VERSIONCONTROL_ACTION_MERGED;
        break;
      case '?':
        $type = VERSIONCONTROL_ACTION_REPLACED;
        break;
      default:
        $type = VERSIONCONTROL_ACTION_OTHER;
    }
    $this->files[$hash] = array('name' => $name, 'type' => $type, 'mode' => $mode);
    $this->name_block->addAncestor($name, $this->revision);
  }
}

class nameBlock {
  public $names = array();
  public $ancestor = array();
  public $history = array();

  function add($name, $type) {
    if (!array_key_exists($name, $this->names)) {
      $this->names[$name]['type'] = $type; // Will be updated with a label_id
      $this->names[$name]['label_id'] = 0; // Will be updated with a label_id
    }
  }

  function addAncestor($name, $hash) {
    if (array_key_exists($name, $this->ancestor)) {
      if (array_key_exists($hash, $this->ancestor[$name])) {
        return;
      }
      $keys = array_keys($this->ancestor[$name]);
      $last = end($keys);
    }
    else {
      $this->ancestor[$name][$hash] = 0;
      return;
    }
    $this->ancestor[$name][$hash] = $last;
  }

  function updateAncestor($hash, $name, $id) {
    $this->history[$name][$hash] = $id;
  }

  function getAncestor($name, $rev) {
    if (array_key_exists($name, $this->history)) {
      $last_rev = $this->ancestor[$name][$rev];
      if ($last_rev && array_key_exists($last_rev, $this->history[$name])) {
        return $this->history[$name][$last_rev];
      }
    }
    return 0;
  }
}
