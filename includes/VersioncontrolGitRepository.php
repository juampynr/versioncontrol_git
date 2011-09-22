<?php

class VersioncontrolGitRepository extends VersioncontrolRepository {

  /**
   * State flag indicating whether or not the GIT_DIR variable has been pushed
   * into the environment.
   *
   * Used to prevent multiple unnecessary calls to putenv(). Will be obsoleted
   * by the introduction of a cligit library.
   *
   * @var bool
   */
  public $envSet = FALSE;

  /**
   * Working tree path, which means the root path of a clone of this repository
   *
   * Used for adding, committing and pushing
   *
   * @var string 
   */
  public $work_tree = '';


  /**
   * Ensure environment variables are set for interaction with the repository on
   * disk.
   *
   * Hopefully temporary, until we can get a proper cligit library written.
   */
  public function setEnv() {
    if (!$this->envSet) {
      $root = escapeshellcmd($this->root);
      putenv("GIT_DIR=$root");
      $this->envSet = TRUE;
    }
  }

  /**
   * Return working tree environment variables to use them in commands
   * that deal with a work tree (a clone of a git repository)
   *
   * @return
   *   string git variables such as "--work-tree=/home/test-repo"
   */
  public function getWorkTreeEnv() {
    return '--git-dir=' . $this->work_tree . '/.git --work-tree=' . $this->work_tree;
  }
  
  public function purgeData($bypass = TRUE) {
    if (empty($bypass)) {
      foreach ($this->loadBranches() as $branch) {
        $branch->delete();
      }
      foreach ($this->loadTags() as $tag) {
        $tag->delete();
      }
      foreach ($this->loadCommits() as $commit) {
        $commit->delete();
      }
    }
    else {
      $label_ids = db_select('versioncontrol_labels', 'vl')
        ->fields('vl', array('label_id'))
        ->condition('vl.repo_id', $this->repo_id)
        ->execute()->fetchAll(PDO::FETCH_COLUMN);

      if (!empty($label_ids)) {
        db_delete('versioncontrol_operation_labels')
          ->condition('label_id', $label_ids)
          ->execute();
      }

      $op_ids = db_select('versioncontrol_operations', 'vco')
        ->fields('vco', array('vc_op_id'))
        ->condition('vco.repo_id', $this->repo_id)
        ->execute()->fetchAll(PDO::FETCH_COLUMN);

      if (!empty($op_ids)) {
        db_delete('versioncontrol_git_operations')
          ->condition('vc_op_id', $op_ids)
          ->execute();
      }

      $ir_ids = db_select('versioncontrol_item_revisions', 'vir')
        ->fields('vir', array('item_revision_id'))
        ->condition('vir.repo_id', $this->repo_id)
        ->execute()->fetchAll(PDO::FETCH_COLUMN);

      if (!empty($ir_ids)) {
        db_delete('versioncontrol_git_item_revisions')
          ->condition('item_revision_id', $ir_ids)
          ->execute();
      }

      db_delete('versioncontrol_operations')
        ->condition('repo_id', $this->repo_id)
        ->execute();

      db_delete('versioncontrol_labels')
        ->condition('repo_id', $this->repo_id)
        ->execute();

      db_delete('versioncontrol_item_revisions')
        ->condition('repo_id', $this->repo_id)
        ->execute();

      module_invoke_all('versioncontrol_repository_bypass_purge', $this);
    }
  }

  public function fetchLogs() {
    // Set a hefty timeout, in case it ends up being a long fetch
    if (!ini_get('safe_mode')) {
      set_time_limit(3600);
    }
    require_once drupal_get_path('module', 'versioncontrol_git') .'/versioncontrol_git.log.inc';
    return _versioncontrol_git_log_update_repository($this);
  }

  /**
   * Invoke git to fetch a list of local branches in the repository, including
   * the SHA1 of the current branch tip and the branch name.
   */
  public function fetchBranches() {
    $branches = array();

    $data = array(
      'repo_id' => $this->repo_id,
      'label_id' => NULL,
    );
    $logs = $this->exec('show-ref --heads');
    while (($branchdata = next($logs)) !== FALSE) {
      list($data['tip'], $data['name']) = explode(' ', trim($branchdata));
      $data['name'] = substr($data['name'], 11);
      $branches[$data['name']] = new VersioncontrolBranch($this->backend);
      $branches[$data['name']]->build($data);
    }

    return $branches;
  }

  /**
   * Invoke git to fetch a list of local tags in the repository, including
   * the SHA1 of the commit to which the tag is attached.
   */
  public function fetchTags() {
    $tags = array();
    $data = array(
      'repo_id' => $this->repo_id,
      'label_id' => NULL,
    );

    $logs = $this->exec('show-ref --tags');
    while (($tagdata = next($logs)) !== FALSE) {
      list($data['tip'], $data['name']) = explode(' ', trim($tagdata));
      $data['name'] = substr($data['name'], 10);
      $tags[$data['name']] = new VersioncontrolTag();
      $tags[$data['name']]->build($data);
    }

    return $tags;
  }

  public function fetchCommits($branch_name = NULL) {
    $logs = $this->exec('rev-list --reverse ' . (empty($branch_name) ? '--all' : $branch_name));
    $commits = array();
    while (($line = next($logs)) !== FALSE) {
      $commits[] = trim($line);
    }
    return $commits;
  }

  /**
   * Clones the repository to a given path
   *
   * @param $path
   *   A string with the full destination path
   *
   * @return
   *   array log containing the output
   */
  public function cloneRepository($path) {
    exec('git clone ' . $this->root . ' ' . $path, $logs, $return);
    $this->work_tree = $path;
    return $logs;
  }

  /**
   * Adds selected files to the git index 
   *
   * @param $pattern
   *   A string with the pattern to add files
   *
   * @return
   *   array log containing the output
   */
  public function add($pattern) {
    $output = exec('git ' . $this->getWorkTreeEnv() . ' add ' . $this->work_tree .'/' . $pattern, $logs, $return);
    return $output;
  }

  /**
   * Commits added files
   *
   * @param $message
   *   A string with the commit message description
   *
   * @return
   *   array log containing the output
   */
  public function commit($message) {
    $output = exec('git ' . $this->getWorkTreeEnv() . ' commit -m "' . $message . '"', $logs, $return);
    return $output;
  }

  /**
   * Pushes commits back to the default origin
   *
   * @return
   *   array log containing the output
   */
  public function push() {
    $output = exec('git ' . $this->getWorkTreeEnv() . ' push', $logs, $return);
    return $output;
  }

  /**
   * Execute a Git command using the root context and the command to be
   * executed.
   *
   * @param string $command
   *   Command to execute.
   * @return mixed
   *  Logged output from the command; an array of either strings or file
   *  pointers.
   */
  protected function exec($command) {
    if (!$this->envSet) {
      $this->setEnv();
    }
    $logs = array();
    $git_bin = _versioncontrol_git_get_binary_path();
    exec(escapeshellcmd("$git_bin $command"), $logs);
    array_unshift($logs, '');
    reset($logs); // Reset the array pointer, so that we can use next().
    return $logs;
  }

  /**
   * Verify if the repository root points to a valid Git repository.
   *
   * @return boolean
   *   TRUE if the repository path is a bare Git Repository.
   */
  public function isValidGitRepo() {
    // do not use exec() method to get the shell return code
    if (!$this->envSet) {
      $this->setEnv();
    }
    $logs = array();
    $git_bin = _versioncontrol_git_get_binary_path();
    exec(escapeshellcmd("$git_bin ls-files"), $logs, $shell_return);
    if ($shell_return != 0) {
      return FALSE;
    }
    return TRUE;
  }

}
