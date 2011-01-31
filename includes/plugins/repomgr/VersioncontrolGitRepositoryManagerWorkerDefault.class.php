<?php

class VersioncontrolGitRepositoryManagerWorkerDefault implements VersioncontrolGitRepositoryManagerWorkerInterface {

  protected $repository;

  public function setRepository(VersioncontrolRepository $repository) {
    $this->repository = $repository;
  }

  public function init() {
    ;
  }

  public function create() {
    ;
  }

  public function delete() {
    ;
  }
}

