<?php

interface VersioncontrolGitRepositoryManagerWorkerInterface extends VersioncontrolRepositoryManagerWorkerInterface {

  /**
   * (Re-)Init a repository a repository on disk.]
   *
   * If called on an existing repository, this does not overwrite data - it
   * merely ensures the latest version of the template directory is used by the
   * repository.
   */
  public function init();

  /**
   * Set the contents of the git repo description file ($GIT_DIR/description).
   */
  public function setDescription($description);

  /**
   * Relocate the repository on disk to the new target location, then optionally
   * update the repository record in the database.
   */
  public function move($target);
}

