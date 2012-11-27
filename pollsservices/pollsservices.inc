<?php
//pollsservices.inc
/**
* Callback for creating pollsservices
*
* @param int $page
* @param object $data
* return object
*/
function _polls_service_index($page, $parameters) {
  global $user;

  $news = array();
  $res = db_query("Select node.nid as nid, node.title as title, node.created as created, users.uid as author_uid, users.name as author_name, field_data_field_firstname.field_firstname_value as author_firstname, field_data_field_lastname.field_lastname_value as author_lastname, file_managed.uri as author_avatar_uri, file_managed_logo.uri as logo, 0 as comments_count from node inner join users on node.uid = users.uid inner join file_managed on file_managed.fid = users.picture left join field_data_field_logo on field_data_field_logo.entity_id = node.nid left join file_managed as file_managed_logo on file_managed_logo.fid = field_data_field_logo.field_logo_fid Inner Join field_data_field_firstname on field_data_field_firstname.entity_id = users.uid inner join field_data_field_lastname on field_data_field_lastname.entity_id = users.uid Where type='poll' ORDER BY node.created DESC");

 foreach($res as $item) {
    $item->author_avatar_uri = file_create_url($item->author_avatar_uri);
    $news[] = $item;
  }

  return $news;
}
function _polls_service_access($op, $args) {
  global $user;
  $access = TRUE;

  return $access;
}