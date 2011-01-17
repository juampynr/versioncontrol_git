<?php
// $Id$

$view = new view;
$view->name = 'vc_git_project_global_commits';
$view->description = 'Commitlog Global Commit Log';
$view->tag = 'VersionControl Core';
$view->view_php = '';
$view->base_table = 'versioncontrol_operations';
$view->is_cacheable = FALSE;
$view->api_version = 2;
$view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */
$handler = $view->new_display('default', 'Defaults', 'default');
$handler->override_option('fields', array(
  'title' => array(
    'label' => '',
    'alter' => array(
      'alter_text' => 0,
      'text' => '',
      'make_link' => 0,
      'path' => '',
      'link_class' => '',
      'alt' => '',
      'prefix' => '',
      'suffix' => '',
      'target' => '',
      'help' => '',
      'trim' => 0,
      'max_length' => '',
      'word_boundary' => 1,
      'ellipsis' => 1,
      'html' => 0,
      'strip_tags' => 0,
    ),
    'empty' => '',
    'hide_empty' => 0,
    'empty_zero' => 0,
    'link_to_node' => 1,
    'exclude' => 1,
    'id' => 'title',
    'table' => 'node',
    'field' => 'title',
    'relationship' => 'none',
  ),
  'attribution' => array(
    'label' => '',
    'alter' => array(
      'alter_text' => 0,
      'text' => '',
      'make_link' => 0,
      'path' => '',
      'link_class' => '',
      'alt' => '',
      'prefix' => '',
      'suffix' => '',
      'target' => '',
      'help' => '',
      'trim' => 0,
      'max_length' => '',
      'word_boundary' => 1,
      'ellipsis' => 1,
      'html' => 0,
      'strip_tags' => 0,
    ),
    'empty' => '',
    'hide_empty' => 0,
    'empty_zero' => 0,
    'exclude' => 1,
    'id' => 'attribution',
    'table' => 'versioncontrol_operations',
    'field' => 'attribution',
    'relationship' => 'none',
  ),
  'vc_op_id' => array(
    'label' => 'Operation ID',
    'alter' => array(
      'alter_text' => 0,
      'text' => '',
      'make_link' => 0,
      'path' => '',
      'link_class' => '',
      'alt' => '',
      'prefix' => '',
      'suffix' => '',
      'target' => '',
      'help' => '',
      'trim' => 0,
      'max_length' => '',
      'word_boundary' => 1,
      'ellipsis' => 1,
      'strip_tags' => 0,
      'html' => 0,
    ),
    'empty' => '',
    'hide_empty' => 0,
    'empty_zero' => 0,
    'set_precision' => FALSE,
    'precision' => 0,
    'decimal' => '.',
    'separator' => ',',
    'prefix' => '',
    'suffix' => '',
    'exclude' => 1,
    'id' => 'vc_op_id',
    'table' => 'versioncontrol_operations',
    'field' => 'vc_op_id',
    'relationship' => 'none',
  ),
  'date' => array(
    'label' => 'Date',
    'alter' => array(
      'alter_text' => 0,
      'text' => '',
      'make_link' => 0,
      'path' => '',
      'link_class' => '',
      'alt' => '',
      'prefix' => '',
      'suffix' => '',
      'target' => '',
      'help' => '',
      'trim' => 0,
      'max_length' => '',
      'word_boundary' => 1,
      'ellipsis' => 1,
      'html' => 0,
      'strip_tags' => 0,
    ),
    'empty' => '',
    'hide_empty' => 0,
    'empty_zero' => 0,
    'date_format' => 'custom',
    'custom_date_format' => 'F j, Y G:i',
    'link' => 1,
    'exclude' => 1,
    'id' => 'date',
    'table' => 'versioncontrol_operations',
    'field' => 'date',
    'relationship' => 'none',
  ),
  'name' => array(
    'label' => '',
    'alter' => array(
      'alter_text' => 0,
      'text' => '',
      'make_link' => 0,
      'path' => '',
      'link_class' => '',
      'alt' => '',
      'prefix' => '',
      'suffix' => '',
      'target' => '',
      'help' => '',
      'trim' => 0,
      'max_length' => '',
      'word_boundary' => 1,
      'ellipsis' => 1,
      'html' => 0,
      'strip_tags' => 0,
    ),
    'empty' => '',
    'hide_empty' => 0,
    'empty_zero' => 0,
    'exclude' => 1,
    'id' => 'name',
    'table' => 'versioncontrol_labels',
    'field' => 'name',
    'relationship' => 'none',
  ),
  'revision' => array(
    'label' => 'Revision',
    'alter' => array(
      'alter_text' => 0,
      'text' => '',
      'make_link' => 0,
      'path' => '',
      'link_class' => '',
      'alt' => '',
      'prefix' => '',
      'suffix' => '',
      'target' => '',
      'help' => '',
      'trim' => 0,
      'max_length' => '',
      'word_boundary' => 0,
      'ellipsis' => 0,
      'html' => 0,
      'strip_tags' => 0,
    ),
    'empty' => '',
    'hide_empty' => 0,
    'empty_zero' => 0,
    'exclude' => 1,
    'id' => 'revision',
    'table' => 'versioncontrol_operations',
    'field' => 'revision',
    'relationship' => 'none',
  ),
  'nothing' => array(
    'label' => '',
    'alter' => array(
      'text' => '<div class="commit-global">
  <h3>[title]: [date]</h3>
  <div class="commit-info">Commit <strong>[revision]</strong> on <strong>[name]</strong></div>
  <div class="attribtution">[attribution]</div>
</div>',
      'make_link' => 0,
      'path' => '#',
      'link_class' => 'global-author',
      'alt' => '',
      'prefix' => '',
      'suffix' => '',
      'target' => '',
      'help' => '',
      'trim' => 0,
      'max_length' => '',
      'word_boundary' => 1,
      'ellipsis' => 1,
      'html' => 0,
      'strip_tags' => 0,
    ),
    'empty' => '',
    'hide_empty' => 0,
    'empty_zero' => 0,
    'exclude' => 0,
    'id' => 'nothing',
    'table' => 'views',
    'field' => 'nothing',
    'relationship' => 'none',
  ),
  'view' => array(
    'label' => '',
    'alter' => array(
      'alter_text' => FALSE,
      'text' => '',
      'make_link' => FALSE,
      'path' => '',
      'link_class' => '',
      'alt' => '',
      'prefix' => '',
      'suffix' => '',
      'target' => '',
      'help' => '',
      'trim' => FALSE,
      'max_length' => '',
      'word_boundary' => TRUE,
      'ellipsis' => TRUE,
      'html' => FALSE,
      'strip_tags' => FALSE,
    ),
    'empty' => '',
    'hide_empty' => 0,
    'empty_zero' => 0,
    'view' => 'commitlog_commit_items',
    'display' => 'block_1',
    'arguments' => '[vc_op_id]',
    'query_aggregation' => 1,
    'exclude' => 0,
    'id' => 'view',
    'table' => 'views',
    'field' => 'view',
    'relationship' => 'none',
  ),
  'message' => array(
    'label' => '',
    'alter' => array(
      'alter_text' => 0,
      'text' => '',
      'make_link' => 0,
      'path' => '',
      'link_class' => '',
      'alt' => '',
      'prefix' => '',
      'suffix' => '',
      'target' => '',
      'help' => '',
      'trim' => 0,
      'max_length' => '',
      'word_boundary' => 1,
      'ellipsis' => 1,
      'html' => 0,
      'strip_tags' => 0,
    ),
    'empty' => '',
    'hide_empty' => 0,
    'empty_zero' => 0,
    'issue_tracker_url' => '',
    'exclude' => 1,
    'id' => 'message',
    'table' => 'versioncontrol_operations',
    'field' => 'message',
    'relationship' => 'none',
  ),
  'nothing_1' => array(
    'label' => '',
    'alter' => array(
      'text' => '<pre>[message]</pre>',
      'make_link' => 0,
      'path' => '',
      'link_class' => '',
      'alt' => '',
      'prefix' => '',
      'suffix' => '',
      'target' => '',
      'help' => '',
      'trim' => 0,
      'max_length' => '',
      'word_boundary' => 1,
      'ellipsis' => 1,
      'html' => 0,
      'strip_tags' => 0,
    ),
    'empty' => '',
    'hide_empty' => 0,
    'empty_zero' => 0,
    'exclude' => 0,
    'id' => 'nothing_1',
    'table' => 'views',
    'field' => 'nothing',
    'relationship' => 'none',
  ),
));
$handler->override_option('sorts', array(
  'date' => array(
    'order' => 'DESC',
    'granularity' => 'second',
    'id' => 'date',
    'table' => 'versioncontrol_operations',
    'field' => 'date',
    'relationship' => 'none',
  ),
));
$handler->override_option('access', array(
  'type' => 'none',
));
$handler->override_option('cache', array(
  'type' => 'none',
));
$handler->override_option('use_pager', '1');
$handler->override_option('style_plugin', 'list');
$handler->override_option('style_options', array(
  'grouping' => '',
  'type' => 'ul',
));
