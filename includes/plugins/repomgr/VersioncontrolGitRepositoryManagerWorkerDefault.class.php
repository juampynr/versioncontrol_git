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
    // if mkdir fails for some reason, it'll error out
    $this->proc_open('mkdir -p ' . escapeshellarg($this->repository->root), TRUE);

    // Create the repository on disk. Init with a template dir if one exists
    if (!empty($this->templateDir) && file_exists($this->templateDir)) {
      $return = $this->passthru('init --template ' . $this->templateDir, TRUE);
    }
    else {
      $return = $this->passthru('init', FALSE);
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
      $command = "rm -rf " . escapeshellarg($this->repository->root) . "/$flush";

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
    $command = 'rm -rf ' . escapeshellarg($this->repository->root);
    // This'll error out if deletion fails.
    $this->proc_open($command, TRUE);

    $this->repository->delete();
    return TRUE;
  }

  public function move($target) {
    $command = 'mv ' . escapeshellarg($this->repository->root) . ' ' . escapeshellarg($target);
    $this->proc_open($command, TRUE);
    $this->repository->root = $target;
    return TRUE;
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
    // $command = escapeshellcmd($prepend . $command);
    $command = _versioncontrol_git_get_binary_path() . ' ' . $command;

    $env = array(
      'GIT_DIR' => $this->repository->root,
    );
    return $this->proc_open($command, $exception, NULL, $env);
  }

  /**
   * Actually run the command, with full output control using the goodies of
   * proc_open().
   *
   * This is fully generic, so can also be used for git commands - we do all the
   * escaping and limiting to a particular dir in the public-facing passthru()
   * method, which then calls this.
   */
  protected function proc_open($command, $exception, $cwd = NULL, $env = array()) {
    $descriptor_spec = array(
      1 => array('pipe', 'w'),
      2 => array('pipe', 'w'),
    );

    $process = proc_open($command, $descriptor_spec, $pipes, $cwd, $env);
    if (is_resource($process)) {
      $stdout = stream_get_contents($pipes[1]);
      fclose($pipes[1]);
      $stderr = stream_get_contents($pipes[2]);
      fclose($pipes[2]);

      $return_code = proc_close($process);

      if ($return_code) {
        $vars = array(
          '%command' => $command,
          '%code' => $return_code,
          '%stdout' => $stdout,
          '%stderr' => $stderr,
        );
        $text = "Invocation of '%command' exited with return code %code";
        watchdog('vc_git', $text . ", emitting stdout:\n%stdout\n\nand stderr:\n%stderr", $vars, WATCHDOG_ERROR);
        if ($exception) {
          throw new Exception(strtr($text, array_slice($vars, 0, 2)), E_ERROR);
        }
      }

      return $return_code;
    }

    return FALSE;
  }
}

