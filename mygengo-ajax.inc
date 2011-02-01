<?php
// $Id$

/**
 * Fetch account balance for admin/config/mygengo
 * 
 */
function mygengo_account() {
	$json = mygengo_plug_balance();

	if ($json['opstat'] == 'ok') {
		echo '<b>Balance:</b> '  . $json['response']['credits'] . ' credits<br>';
		echo "<a href='https://mygengo.com/account/top_up'>Top up</a>";
	}
	else {
		echo 'Error from server: <i>' . $json['err']['msg'] . '</i>';
	}
}

/**
 * Callback for myGengo server
 * 
 * The myGengo server POSTs updates for jobs or comments that are inserted into the database.
 */
function mygengo_callback() {
	if (isset($_POST['job'])) {
		$json = drupal_json_decode($_POST['job']);
		mygengo_plug_proc_job($json);
	}

	if (isset($_POST['comment'])) {	
		// mygengo doesn't send author field
		// WORKAROUND is to receck the whole thread
		$json = drupal_json_decode($_POST['comment']);

		mygengo_plug_thread($json['job_id']);
	}

	echo '<html><body>go away!</body></html>';
}
