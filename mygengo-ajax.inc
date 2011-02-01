<?php

/*
This file is part of myGengo Drupal plugin.

    myGengo Drupal plugin is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

// fetch account balance for admin/config/mygengo
function mygengo_account()
{
	$json = mygengo_plug_balance();

	if($json['opstat'] == 'ok')
	{
		echo "<b>Balance:</b> "  . $json['response']['credits'] . " credits<br>";
		echo "<a href='https://mygengo.com/account/top_up'>Top up</a>";
	}
	else
	{
		echo "Error from server: <i>" . $json['err']['msg'] . "</i>";
	}
}

// callback for myGengo server
function mygengo_callback()
{
	if(isset($_POST['job']))
	{
		$json = drupal_json_decode($_POST['job']);
		mygengo_plug_proc_job($json);
	}

	if(isset($_POST['comment']))
	{	
		// mygengo doesn't send author field
		// WORKAROUND is to receck the whole thread
		$json = drupal_json_decode($_POST['comment']);

		mygengo_plug_thread($json['job_id']);
	}

	echo "<html><body>go away!</body></html>";
}
