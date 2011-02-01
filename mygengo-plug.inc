<?php
// $Id$

define('MYGENGO_URL_BASE','http://api.mygengo.com/v1/');

define('MYGENGO_URL_BALANCE',MYGENGO_URL_BASE . 'account/balance');
define('MYGENGO_URL_GET_JOBS',MYGENGO_URL_BASE . 'translate/jobs');
define('MYGENGO_URL_JOB',MYGENGO_URL_BASE . 'translate/job/%s');
define('MYGENGO_URL_GET_COMMENTS',MYGENGO_URL_BASE . 'translate/job/%s/comments');
define('MYGENGO_URL_GET_PREVIEW',MYGENGO_URL_BASE . 'translate/job/%s/preview');
define('MYGENGO_URL_GET_FEEDBACK',MYGENGO_URL_BASE . 'translate/job/%s/feedback');
define('MYGENGO_URL_GET_LANGUAGES',MYGENGO_URL_BASE . 'translate/service/languages');
define('MYGENGO_URL_GET_LANGUAGE_PAIRS',MYGENGO_URL_BASE . 'translate/service/language_pairs');
define('MYGENGO_URL_POST_JOB',MYGENGO_URL_BASE . 'translate/job');
define('MYGENGO_URL_POST_COMMENT',MYGENGO_URL_BASE . 'translate/job/%s/comment');

/**
 * Build GET query
 */
function mygengo_plug_build($opts) {
	$opts['ts'] = gmdate('U');
	$opts['format'] = 'json';
	$opts['api_key'] = variable_get('mygengo_cred_public');
	ksort($opts);

	$ret = http_build_query($opts);
	$opts['api_sig'] = hash_hmac('sha1',$ret,variable_get('mygengo_cred_private'));

	return http_build_query($opts);
}

/** 
 * Make a generic GET request to the myGengo API
 */
function mygengo_plug_fetch($m,$url,$opts) {
	if ($m == "GET") {
		$resp = drupal_http_request($url . "?" . mygengo_plug_build($opts),array(
			'method' => 'GET',
			'headers' => array(
			'Accept' => 'application/json')));
		$resp = $resp->data;
	}
	elseif($m == "POST") {
		$resp = drupal_http_request(MYGENGO_URL_POST_JOB,array(
		'method' => 'POST',
		'data' => http_build_query($opts),//substr($tmp,0,strlen($tmp)-1),
		'headers' => array(
			'Accept' => 'application/json',
			'Content-Type' => 'application/x-www-form-urlencoded')));
	}
	
	$json = drupal_json_decode($resp);

	if($json == NULL) {
		$json = $resp;
	}

	return $json;
}

/**
 * GET account/balance
 */
function mygengo_plug_balance() {
	return mygengo_plug_fetch("GET",MYGENGO_URL_BALANCE,array());
}

/**
 * GET translate/jobs
 */
function mygengo_plug_get_jobs() {
	return mygengo_plug_fetch("GET",MYGENGO_URL_GET_JOBS,array());
}

/**
 * GET translate/job/id
 */
function mygengo_plug_get_job($id) {
	return mygengo_plug_fetch("GET",sprintf(MYGENGO_URL_JOB,$id),array());
}

/**
 * GET translate/service/languages
 */
function mygengo_plug_get_languages() {
	return mygengo_plug_fetch("GET",MYGENGO_URL_GET_LANGUAGES,array());
}

/**
 * GET translate/service/language_pairs
 */
function mygengo_plug_get_language_pairs() {
	return mygengo_plug_fetch("GET",MYGENGO_URL_GET_LANGUAGE_PAIRS,array());
}

/**
 * GET translate/job/id/feedback
 */
function mygengo_plug_get_feedback($jid) {
	return mygengo_plug_fetch("GET",sprintf(MYGENGO_URL_GET_FEEDBACK,$jid),array());
}

/**
 * GET translate/job/id/comments
 */
function mygengo_plug_get_comments($jid) {
	return mygengo_plug_fetch("GET",sprintf(MYGENGO_URL_GET_COMMENTS,$jid),array());
}

/**
 * POST translate/job
 */
function mygengo_plug_post_job($lc_src,$lc_tgt,$tier,$body,$comment,$summary,$auto_appr) {
	$opts = array('api_key' => variable_get('mygengo_cred_public'), 'ts' => gmdate('U'), 'data' => json_encode(array(
		'job' => array(
			'body_src' => $body,
			'lc_src' => $lc_src,
			'lc_tgt' => $lc_tgt,
			'tier' => $tier,
			'slug' => $summary,
			'comment' => $comment,
			'auto_approve' => $auto_appr))));
	ksort($opts);
	$opts['api_sig'] = hash_hmac('sha1',json_encode($opts),variable_get('mygengo_cred_private'));

	$resp = drupal_http_request(MYGENGO_URL_POST_JOB,array(
		'method' => 'POST',
		'data' => http_build_query($opts),
		'headers' => array(
			'Accept' => 'application/json',
			'Content-Type' => 'application/x-www-form-urlencoded')));
	
	return drupal_json_decode($resp->data);
}

/**
 * POST translate/job/id/comment
 */
function mygengo_plug_post_comment($jid,$comment) {
	$opts = array('api_key' => variable_get('mygengo_cred_public'), 'ts' => gmdate('U'), 'data' => json_encode(array('body' => $comment)));
	ksort($opts);
	$opts['api_sig'] = hash_hmac('sha1',json_encode($opts),variable_get('mygengo_cred_private'));

	$resp = drupal_http_request(sprintf(MYGENGO_URL_POST_COMMENT,$jid),array(
		'method' => 'POST',
		'data' => http_build_query($opts),
		'headers' => array(
			'Accept' => 'application/json',
			'Content-Type' => 'application/x-www-form-urlencoded')));

	return drupal_json_decode($resp->data);
}

/**
 * GET translate/job/id/preview
 */
function mygengo_plug_get_preview($jid) {
	return mygengo_plug_fetch("GET",sprintf(MYGENGO_URL_GET_PREVIEW,$jid),array());
}

/**
 * Generic PUT translate/job/id
 */
function mygengo_plug_put_job($jid,$act,$args) {
	$opts = array(
		'api_key' => variable_get('mygengo_cred_public'),
		'ts' => gmdate('U'),
		'_method' => 'PUT', 
		'data' => json_encode(array_merge(array('action' => $act),$args)));
	
	ksort($opts);
	$opts['api_sig'] = hash_hmac('sha1',http_build_query($opts),variable_get('mygengo_cred_private'));

	$resp = drupal_http_request(sprintf(MYGENGO_URL_JOB,$jid) . '?' . http_build_query($opts),array(
		'method' => 'GET',
		'headers' => array(
			'Accept' => 'application/json',
			'Content-Type' => 'application/x-www-form-urlencoded')));
	
	return drupal_json_decode($resp->data);
}

/**
 * Approve PUT translate/job/id
 */
function mygengo_plug_put_approve($jid,$rating,$for_trans,$for_mygengo,$public) {
	return mygengo_plug_put_job($jid,'approve',array(
		'rating' => $rating, 
		'for_translator' => $for_trans,
		'for_mygengo' => $for_mygengo,
		'public' => $public));
}

/**
 * Fetch the current thread for a job
 */
function mygengo_plug_thread($jid) {
	$cmnts = mygengo_plug_get_comments($jid);
	if($cmnts['opstat'] == 'ok') {
		foreach($cmnts['response']['thread'] as &$c) {
			$c['job_id'] = $jid;
			mygengo_plug_proc_comment($c);
		}
	}
}
/**
 * Update the local copy of a job
 */
function mygengo_plug_update($jid) {
	$json = mygengo_plug_get_job($jid);
	if($json['opstat'] == 'ok') {
		mygengo_plug_proc_job($json['response']['job']);
	}
}

/** 
 * Put the json reply of a comment into the database
 */
function mygengo_plug_proc_comment($json) {
	db_merge('mygengo_comments')->key(array('ctime' => $json['ctime'],'body' => $json['body']))->fields(array(
		'jid' => $json['job_id'],
		'body' => $json['body'],
		'ctime' => $json['ctime'],
		'author' => $json['author']))->execute();
}

/**
 * Put job payload into the database
 */
function mygengo_plug_proc_job($json) {
	$jid = $json['job_id'];
	unset($json['job_id']);
	unset($json['mt']);
	unset($json['callback_url']);
	unset($json['auto_approve']);
	unset($json['eta']);
	$json['atime'] = gmdate('U');

	db_merge('mygengo_jobs')->key(array('jid' => $jid))->fields($json)->execute();
}

/**
 * Process the language(_pairs) into a workable format.
 *
 * Used by mygengo_translate_form()
 */
function mygengo_plug_fetch_language_pairs(&$names,&$langs) {
	$json_l = mygengo_plug_get_languages();
	$json_p = mygengo_plug_get_language_pairs();

	if ($json_l['opstat'] == 'ok' and $json_p['opstat'] == 'ok') 	{
		foreach($json_l['response'] as &$i) {
			$langs[$i['lc']] = $i;
			$names[$i['lc']] = $i['language'];
		}

		foreach($json_p['response'] as &$l) {
			$langs[$l['lc_src']]['lc_tgt'][$l['lc_tgt']][] = $l;
		}

		return TRUE;
	}
	else {
		return FALSE;
	}
}

/**
 * Lower end of mygengo_poll. Polls myGengo's db.
 */
function mygengo_plug_poll() {
	$db = db_query('SELECT jid FROM {mygengo_jobs}')->fetchAllAssoc('jid');
	foreach($db as &$jid) {
		db_delete('mygengo_jobs')->condition('jid',$jid->jid)->execute();
		db_delete('mygengo_comments')->condition('jid',$jid->jid)->execute();
	}

	$json = mygengo_plug_get_jobs();

	if ($json['opstat'] == 'ok') {
		foreach($json['response'] as &$job) {
			$jd = mygengo_plug_get_job($job['job_id']);
			if($jd['opstat'] == 'ok') {
				mygengo_plug_proc_job($jd['response']['job']);
			}
			mygengo_plug_thread($job['job_id']);
		}
	}
	else {
		throw new Exception;
	}
}

/**
 * DELETE translate/job/id
 */
function mygengo_plug_cancel($jid) {
	$opts = array(
		'api_key' => variable_get('mygengo_cred_public'),
		'_method' => 'DELETE',
		'ts' => gmdate('U'));
	ksort($opts);
	$opts['api_sig'] = hash_hmac('sha1',http_build_query($opts),variable_get('mygengo_cred_private'));
	
	$resp = drupal_http_request(sprintf(MYGENGO_URL_JOB,$jid) . '?' . http_build_query($opts),array(
		'method' => 'GET',
		'headers' => array(
			'Accept' => 'application/json',
			'Content-Type' => 'application/x-www-form-urlencoded')));

	return drupal_json_decode($resp->data);
}

/** 
 * Reject PUT translate/job/id
 */
function mygengo_plug_reject($jid,$follow_up,$reason,$comment,$captcha) {
	return mygengo_plug_put_job($jid,'reject',array(
		'reason' => $reason,
		'comment' => $comment,
		'captcha' => $captcha,
		'follow_up' => $follow_up));
}

/**
 * Revise PUT translate/job/id
 */
function mygengo_plug_correct($jid,$comment) {
	return mygengo_plug_put_job($jid,'revise',array('comment' => $comment));
}
