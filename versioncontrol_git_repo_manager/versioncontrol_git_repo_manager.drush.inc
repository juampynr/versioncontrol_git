<?php

/**
 * @file
 *
 * Provides drush commands to access the repository manager.
 */

/**
 * Implementation of hook_drush_command().
 */
function versioncontrol_git_repo_manager_drush_command() {
  return array(
    'git-create' => array(
      'aliases' => array('gc'),
      'description' => 'Create a git repository on disk and add it to versioncontrol module\'s list of repositories.',
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
      'arguments' => array(
        'repository name' => 'The name of the repository to create.',
        'repository path' => 'Repository at which to initialize the bare repository.',
      ),
      'options' => array(
        '--nid' => 'project-nid',
      ),
    ),
    'git-delete' => array(
      'aliases' => array('gd'),
      'description' => 'Delete a git repository from the local disk and from versioncontrol module.',
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
      'arguments' => array(
        'repository id' => 'The id of the repository to delete.',
      ),
    ),
    'git-list-repos' => array(
      'aliases' => array('gl'),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
      'description' => 'List git repositories tracked by Drupal\'s versioncontrol',
    ),
    'git-queue-process' => array(
      'aliases' => array('gp'),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
      'description' => 'Process enqueued repository creations and deletions.',
    ),
  );
}

/**
 * Create a git repository on disk and add it to versioncontrol module's list of repositories.
 *
 * @param $repository_name
 *   The title of the repository to create in vcs_api.
 * @param $repository_path
 *   The path of the repository to create in vcs_api and on disk.
 */
function drush_versioncontrol_git_repo_manager_git_create($repository_name, $repository_path) {
  // Create versioncontrol entity data.
  $data['repository_data']['name'] = $repository_name;
  $data['repository_data']['vcs'] = 'git';
  $data['repository_data']['backend'] = 'git';
  $data['repository_data']['root'] = $repository_path;
  $data['nid'] = drush_get_option('project-nid', 0);
  _versioncontrol_git_repo_manager_create_git_repository($data);
  drush_print(t('Git repository @name created', array('@name' => $repository_name)));
}

/**
 *  List git repositories tracked by versioncontrol.
 */
function drush_versioncontrol_git_repo_manager_git_list_repos() {
  $rows = array();
  $rows[] = array(
    t('Id'),
    t('Name'),
    t('Path'),
  );
  $result = db_query("SELECT repo_id, name, root FROM {versioncontrol_repositories} WHERE vcs='git'");
  while ($item = db_fetch_object($result)) {
    $rows[] = array(
      $item->repo_id,
      $item->name,
      $item->root,
    );
  }
  drush_print_table($rows, TRUE);
}


/**
 *  Delete a git repository from the local disk and from versioncontrol module.
 */
function drush_versioncontrol_git_repo_manager_git_delete($repository_id) {
  if ($repository = versioncontrol_repository_load($repository_id)) {
    _versioncontrol_git_repo_manager_delete_git_repository(array('repo_id' => $repository_id));
    drush_print(t('Versioncontrol Repository @repo_name was deleted.', array('@repo_name' => $repository->name)));
  }
  else {
    drush_print(t('Repository @repo_id was not found.', array('@repo_id' => $repository_id)));
  }
}

/**
 * Process an item in the git_repo_manager queue.
 */
function drush_versioncontrol_git_repo_manager_git_queue_process() {
  drupal_queue_include();
  $queue = DrupalQueue::get('versioncontrol_git_repo_manager');
  drush_print(t("@number git repository operations currently in the queue.\n", array('@number' => $queue->numberOfItems())));
  if ($item = $queue->claimItem($lease_time = 60)) {
    $data = $item->data;
    switch($data['operation']) {
      case 'create':
        _versioncontrol_git_repo_manager_create_git_repository($item->data['data']);
        drush_print(t("Job processed.  Git repository created at path @path.\n", array('@path' => $item->data['data']['repository_data']['root'])));
        break;
      case 'delete':
        _versioncontrol_git_repo_manager_delete_git_repository($item->data['data']);
        drush_print(t("Job processed.  Git repository @repo_id deleted.\n", array('@repo_id' => $item->data['data']['repo_id'])));
        break;
    }
    $queue->deleteItem($item);
  }
}
