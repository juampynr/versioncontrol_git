<?php
/**
 * @file
 * Repository Url handler class for gitphp.
 */

/**
 * Base class for URL handlers.
 */
class VersioncontrolRepositoryUrlHandlerGitphp extends VersioncontrolRepositoryUrlHandler {

  public function getItemViewUrl(&$item) {
    $placeholders = array(
      '%repo_name' => $this->repository->name,
      '%path'     => substr($item->path, 1),
      '%revision' => $item->blob_hash,
    );

    $label = $item->getSelectedLabel();
    if (isset($label->type) && $label->type == VERSIONCONTROL_LABEL_BRANCH) {
      $current_branch = $label->name;
    }
    if (isset($current_branch)) {
      $placeholders['%branch'] = '&hb=' . $current_branch;
    }
    else {
      $placeholders['%branch'] = '';
    }

    $view_url = $item->isFile()
      ? $this->getTemplateUrl('file_view')
      : $this->getTemplateUrl('directory_view');

    return strtr($view_url, $placeholders);
  
}

}
