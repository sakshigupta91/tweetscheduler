<?php

function tweet_scheduler_form($form, &$form_state) {
  $form = array();

  global $user;
  module_load_include('inc', 'twitter');
  $twitter_accounts = module_invoke('twitter', 'twitter_accounts', $user);
  $options = array();
  foreach ($twitter_accounts as $option) {
    $options[$option->screen_name] = $option->screen_name;
  }
  
  if (count($options) > 0) {
    $form['tweet_text'] = array(
      '#type' => 'textarea',
      '#rows' => 10,
      '#columns' => 40,
      '#default_value' => '',
      '#title' => t('Tweet it!'),
      '#required' => TRUE,
    );
    $form['tweet_scheduler_select'] = array(
      '#type' => 'radios',
      '#title' => 'Schedule options',
      '#default_value' => '0',
      '#options' => array(
        '0' => t('Send this tweet to twitter immediately'),
        '1' => t('Schedule this tweet'),
        '2' => t('Periodic Tweets'),
      ),
      '#ajax' => array(
        'callback' => 'tweet_scheduler_form_callback',
        'event' => 'change',
        'wrapper' => 'tweet_scheduler_replace_form',
        'effect'=> 'fade',
      ),
    );
    
    $form_state['#tweet_scheduler_account'] = $user;
    $form_state['#tweet_scheduler_options_count'] = count($options);
    
    $form['tweet_scheduler_ajax_form']['open'] = array(
      '#markup' => '<div id="tweet_scheduler_replace_form">',
    );
    
    if(isset($form_state['values']['tweet_scheduler_select']) &&  $form_state['values']['tweet_scheduler_select'] == '1') {
      $form['tweet_scheduler_ajax_form']['date_time'] = array(
        '#type' => 'date_popup',
        '#date_format' => 'Y-m-d H:i:s',
      );
    }
    
    $form['tweet_scheduler_ajax_form']['tweet_scheduler_twitter_account'] = array(
      '#type' => 'select',
      '#title' => t('Choose an account to which tweets from Tweet Scheduler should be posted'),
      '#description' => t('Tweets will be posted to the account by Tweet Scheduler automatically '),
      '#options' => $options,
    );
    
    $submit_text = t('Publish');
    if(isset($form_state['values']['tweet_scheduler_select']) && $form_state['values']['tweet_scheduler_select'] == '1') {
      $submit_text = t('Schedule'); 
    }
    else if(isset($form_state['values']['tweet_scheduler_select']) && $form_state['values']['tweet_scheduler_select'] == '2') {
      $submit_text = t('Save'); 
    }
    
    $form['tweet_scheduler_ajax_form']['tweet_submit'] = array(
      '#type' => 'submit',
      '#value' => $submit_text,
    );
    
    $form['tweet_scheduler_ajax_form']['close'] = array(
      '#markup' => '</div>',
    );
  }
  else {
    $form['twitter_account_setup'] = array(
      '#markup' => t('No twitter account is available, set up a twitter account <a href="@add_twitter_account">here</a>.', array('@add_twitter_account' => url('user/1/edit/twitter'))),
    );
  }
  
  return $form;
}
function tweet_scheduler_form_callback($form, $form_state) {
  return $form['tweet_scheduler_ajax_form'];
}

function tweet_scheduler_form_validate(&$form, &$form_state) {
  $tweet_text = $form_state['values']['tweet_text'];
  if (strlen($tweet_text) > 140) {
    form_set_error('tweet_text', t('Tweet can not be more than 140 characters'));
  }
}

function tweet_scheduler_form_submit(&$form, &$form_state) {
  switch($form_state['values']['tweet_scheduler_select']['options']) {
    case 0:
      tweet_scheduler_post_to_twitter($form_state['values']['tweet_text'], $form_state['values']['tweet_scheduler_twitter_account']);
      break;
    case 1:
      $scheduled = 1;
      $scheduled_time = strtotime($form_state['values']['date_time']);
      $user_name = $form_state['values']['tweet_scheduler_twitter_account'];
      $post = $form_state['values']['tweet_text'];
      global $user;
      $nid = db_insert('tweet_scheduler')
      ->fields(array(
        'screen_name' => $user_name,
        'text' => $post,
        'scheduled_timestamp' => $scheduled_time,
        'scheduled' => $scheduled,
        'uid' => $user->uid,
      ))
      ->execute();
      break;
    case 2:
      $scheduled = 2;
      global $user;
      $account = user_load($user->uid);
      $interval_time = $account->data['tweet_scheduler_interval_time'];
      $time = time() + $interval_time*60*60;
      $query = db_select('tweet_scheduler', 'a')
            ->fields('a', array('scheduled', 'scheduled_timestamp'))
            ->condition('scheduled', '2', '=')
            ->execute()
            ->fetchAssoc();
      $scheduled_time = ($query['scheduled_timestamp']) ? $query['scheduled_timestamp'] : $time;
      $user_name = $form_state['values']['tweet_scheduler_twitter_account'];
      $post = $form_state['values']['tweet_text'];
      global $user;
      $nid = db_insert('tweet_scheduler')
      ->fields(array(
        'screen_name' => $user_name,
        'text' => $post,
        'scheduled' => $scheduled,
        'uid' => $user->uid,
        'scheduled_timestamp' => $scheduled_time,
      ))
      ->execute();
      break;
  }
}