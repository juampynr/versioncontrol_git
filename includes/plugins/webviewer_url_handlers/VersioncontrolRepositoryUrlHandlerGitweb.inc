<?php
/**
 * @file
 * Repository Url handler class for gitweb.
 */

/**
 * Base class for URL handlers.
 */
class VersioncontrolRepositoryUrlHandlerGitweb extends VersioncontrolRepositoryUrlHandler {

  /**
   * Gitweb do not need to have a hash base parameter to show an item
   * log history.
   */
  public function getItemLogViewUrl($item) {
    $placeholders = array(
      '%repo_name' => $this->repository->name,
      '%path'     => substr($item->path, 1),
      '%revision' => $item->revision,
    );

    $label = $item->getSelectedLabel();
    if (isset($label->type) && $label->type == VERSIONCONTROL_LABEL_BRANCH) {
      $current_branch = $label->name;
    }
    if (isset($current_branch)) {
      $placeholders['%branch'] = ';hb=' . $current_branch;
    }
    else {
      $placeholders['%branch'] = '';
    }

    if ($item->isFile()) {
      return strtr($this->getTemplateUrl('file_log_view'), $placeholders);
    }
    else { // directory
      return strtr($this->getTemplateUrl('directory_log_view'), $placeholders);
    }
  }

  /**
   * Gitweb do not need to have a hash base parameter to show an item
   * content.
   */
  public function getItemViewUrl($item) {
    // Removed items has not blob hash
    if (is_null($item->blob_hash)) {
      return '';
    }
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
      $placeholders['%branch'] = ';hb=' . $current_branch;
    }
    else {
      $placeholders['%branch'] = '';
    }

    $view_url = $item->isFile()
      ? $this->getTemplateUrl('file_view')
      : $this->getTemplateUrl('directory_view');

    return strtr($view_url, $placeholders);
  }

  /**
   * Gitweb use blobs for diff.
   */
  public function getDiffUrl($file_item_new, $file_item_old) {
    $placeholders = array(
      '%repo_name' => $this->repository->name,
      '%path'         => substr($file_item_new->path, 1),
      '%new_blob_hash' => $file_item_new->blob_hash,
      '%old_path'     => $file_item_old->path,
      '%old_blob_hash' => $file_item_old->blob_hash,
      '%new_revision' => $file_item_new->revision,
      '%old_revision' => $file_item_old->revision,
    );

    return strtr($this->getTemplateUrl('diff'), $placeholders);
  }
}
