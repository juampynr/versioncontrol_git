<?php

class VersioncontrolGitRepositoryManagerWorkerDefault implements VersioncontrolGitRepositoryManagerWorkerInterface {

  protected $repository;

  protected $templateDir;

  public function setRepository(VersioncontrolRepository $repository) {
    $this->repository = $repository;
  }

  public function create() {
    $this->init();
    $this->save();
  }

  public function init() {
    exec('mkdir -p ' . escapeshellarg($this->repository->root), $output, $return);
    if ($return) {
      // mkdir failed for some reason, throw exception
      throw new Exception('Failed to mkdir for intended repository location "' . $this->repository->root . '"; mkdir exited with code ' . $return, E_ERROR);
    }

    // Create the repository on disk. Init with a template dir if one exists
    if (!empty($this->templateDir) && file_exists($this->templateDir)) {
      $return = $this->passthru('init --template ' . $this->templateDir, TRUE);
    }
    else {
      $return = $this->passthru('init', TRUE);
    }

    if ($return) {
      // init failed for some reason, throw exception
      throw new Exception('Git repository initialization failed with code ' . $return, E_ERROR);
    }

    return TRUE;
  }

  public function reInit(array $flush) {
    if (!empty($flush)) {
      $flush = count($flush) > 1 ? '{' . implode(',', $flush) . '}' : array_shift($flush);
      exec("rm -rf {$this->repository->root}/$flush");
    }
    $this->init();
  }

  public function configSet($name, $value, $type = NULL) {
    $cmd = 'config --file ';
    if (!is_null($type) && in_array($type, array('int', 'bool', 'path'))) {
      $cmd .= "--$type ";
    }
    $cmd .= escapeshellarg($name) . ' ' . escapeshellarg($value);
    $return = $this->passthru($cmd);
    return empty($return);
  }

  public function delete() {
    exec('rm -rf ' . escapeshellarg($this->repository->root), $output, $return);
    if ($return) {
      // Deletion failed, throw an error.
      throw new Exception('Git repository deletion failed with code ' . $return . ' and error message \'' . implode(' ', $output) . '\'', E_ERROR);
    }

    $this->repository->delete();
    return TRUE;
  }

  public function move($target) {
    exec('mv ' . escapeshellarg($this->repository->root) . ' ' . escapeshellarg($target), $output, $return);

    if ($return) {
      // move failed for some reason, throw exception
      throw new Exception('Relocating Git repository on disk failed with code ' . $return . ' and error message \'' . implode(' ', $output) . '\'', E_ERROR);
    }

    $this->repository->root = $target;
  }

  public function save() {
    $this->repository->save();
    return TRUE;
  }

  public function setDescription($description) {
    file_put_contents($this->repository->root . '/description', $description);
    return TRUE;
  }

  public function passthru($command, $exception = FALSE) {
    $prepend = "GIT_DIR={$this->repository->root} " . _versioncontrol_git_get_binary_path() . ' ';
    $command = escapeshellcmd($prepend . $command);
    exec($command, $output, $return);
    if ($exception && $return) {
      throw new Exception ("Command '$command' exited with status $return and the following output: " . implode(' ', $output), E_ERROR);
    }
    return $return;
  }
}

