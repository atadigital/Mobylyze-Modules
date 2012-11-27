<?php
function _fbmod_access($op, $args) {
global $user;
$access = FALSE;
 
switch ($op) {
case "registered":
// only the owner can update it's own profile
if(user_is_logged_in()) {
  $access = TRUE;
}
break;
}
 
return $access;
}
 
/**
 Function to import the image
 */
function fbmod_import_picture($fbuser, $account) {
// Retrieve the user's picture from Facebook and save it locally.
  if ($account->uid) {
    $picture_directory = file_default_scheme() . '://' . variable_get('user_picture_path', 'pictures');
    if(file_prepare_directory($picture_directory, FILE_CREATE_DIRECTORY)){
      $picture_result = drupal_http_request('https://graph.facebook.com/' . $fbuser->id . '/picture?type=large');
      $picture_path = file_stream_wrapper_uri_normalize($picture_directory . '/picture-' . $account->uid . '-' . REQUEST_TIME . '.jpg');
      $picture_file = file_save_data($picture_result->data, $picture_path, FILE_EXISTS_REPLACE);
 
      // Check to make sure the picture isn't too large for the site settings.
      $max_dimensions = variable_get('user_picture_dimensions', '85x85');
      file_validate_image_resolution($picture_file, $max_dimensions);
 
      // Update the user record.
      $picture_file->uid = $account->uid;
      $picture_file = file_save($picture_file);
      file_usage_add($picture_file, 'user', 'user', $account->uid);
      db_update('users')
        ->fields(array(
        'picture' => $picture_file->fid,
        ))
        ->condition('uid', $account->uid)
        ->execute();
    }
  }
}
 
/**
 * Performs a login using an external service
 */
function _fbmod_loginService($service, $id, $accesstoken) {
  global $user;
  switch($service) {
    case "facebook":
    include_once(drupal_get_path("module", "fboauth") . "/includes/fboauth.fboauth.inc");
    $fbuser = fboauth_graph_query('me', $accesstoken);
    if($id == $fbuser->id) {
      $uid = fboauth_uid_load($fbuser->id);
      // If the user has logged in before, load their account and login.  
      if ($uid && $account = user_load($uid)) {
        fbmod_import_picture($fbuser, $account); // function to import picture
        return fbmod_login_external($account);
      }
    
      // If the Facebook e-mail address matches an existing account, bind them
      // together and log in as that account.
    elseif (!empty($fbuser->email) && ($account = user_load_by_mail($fbuser->email))) {
      fboauth_save($account->uid, $fbuser->id);
      fbmod_import_picture($fbuser, $account);
 
      // Logins will be denied if the user's account is blocked.
      if ($ret = fbmod_login_external($account)) {
        return $ret;
      }
    }
    else {
      if ($user->uid) {
        fboauth_save($user->uid, $fbuser->id);
        drupal_set_message(t('You\'ve connected your account with Facebook.'));
      }
      // Register a new user only if allowed.
      elseif (variable_get('user_register', 1)) {
        $account = fboauth_create_user($fbuser);
        // Load the account fresh just to have a fully-loaded object.
        $account = user_load($account->uid);
 
        // If the account requires administrator approval the new account will
        // have a status of '0' and not be activated yet.
        if ($account->status == 0) {
          _user_mail_notify('register_pending_approval', $account);
        }
        // Log in the user if no approval is required.
        elseif ($ret = fbmod_login_external($account)) {
          return $ret;
        }
        // If the login fails, fboauth_login_user() throws its own error message.
      }
    // Since user's can't create new accounts on their own, show an error.
      else {
        return "Account creation not allowed";
      }
    }
  }
}
}
 
/**
 * Similar to _user_resource_login()
 */
function fbmod_login_external($account) {
  global $user;
 
  if ($account->status) {
    $form_state['uid'] = $account->uid;
    user_login_submit(array(), $form_state);
 
    $return = new stdClass();
    $return->sessid = session_id();
    $return->session_name = session_name();
    services_remove_user_data($user);
    $return->user = $user;
    return $return;
  }
  else {
    // drupal_set_message(t('The username %name has not been activated or is blocked.', array('%name' => $account->name)), 'error');
  }
}