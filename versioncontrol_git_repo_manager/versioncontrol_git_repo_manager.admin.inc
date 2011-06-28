<?php

/**
 * @file
 *   Provides the administrative interface for git repository management.
 */

/**
 * Provide the settings form.
 */
function versioncontrol_git_repo_manager_settings_form() {
  $form['versioncontrol_git_repo_manager_repository_base_path'] = array(
    '#title' => t('Base Repository Path'),
    '#description' => t('The path on disk where this repository resides.  This folder must actually exist.'),
    '#type' => 'textfield',
    '#default_value' => variable_get('versioncontrol_git_repo_manager_repository_base_path', '/git'),
  );
  if (module_exists('versioncontrol_project')) {
    $form['versioncontrol_git_repo_manager_manage_repos_for_projects'] = array(
      '#title' => t('Manage git repositories for projects.'),
      '#description' => t('If this checkbox is checked, the repository manager will create and delete repositories on disk corresponding to the project shortname.  If the basepath is set to /git and someone creates a project with the shortname \'foo\', a bare repository will be created at \'/git/foo.git\'.'),
      '#type' => 'checkbox',
      '#default_value' => variable_get('versioncontrol_git_repo_manager_manage_repos_for_projects', FALSE),
    );
  }
  return system_settings_form($form);
}
