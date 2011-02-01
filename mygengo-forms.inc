<?php

/*
This file is part of myGengo Drupal plugin.

    myGengo Drupal plugin is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    myGengo Drupal plugin is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with myGengo Drupal plugin.  If not, see <http://www.gnu.org/licenses/>.
*/

// admin/config/mygengo
// uses mygengo_account()
function mygengo_admin()
{
	$form = array();
	drupal_add_js(drupal_get_path('module','mygengo') . "/ajax-account.js",'file');;

	// acc info
	$form['mygengo_acc_info'] = array(
		'#type' => 'fieldset',
		'#title' => t('Account information'));

	$form['mygengo_acc_info']['mygengo_acc_info_balace'] = array(
		'#type' => "markup",
		'#markup' =>	'<div id="mygengo_account_div"><div class="ajax-progress ajax-progress-throbber"><div class="throbber">&nbsp;</div>' .
									t('Loading...') .
									'</div><br></div>');

	// pub key
	$form['public'] = array(
		'#type' => 'textfield',
		'#title' => t('API key'),
		'#default_value' => variable_get('mygengo_cred_public'),
		'#size' => 60,
		'#required' => TRUE);

	// priv key
	$form['private'] = array(
		'#type' => 'textfield',
		'#title' => t('Private key'),
		'#default_value' => variable_get('mygengo_cred_private'),
		'#size' => 60,
		'#required' => TRUE);

	$form['auto_appr'] = array(
		'#type' => 'checkbox',
		'#title' => t('Auto-approve all jobs'),
		'#default_value' => variable_get("mygengo_auto_appr"));

	$form['poll'] = array(
		'#type' => 'item',
		'#markup' => l(t('Rebuild database'),'mygengo/poll'),
		'#description' => t('Deletes the local database of jobs and comments and fetches a new one from myGengo. Under normal operation this shouldn\'t be necessary. Use with caution!'));
	
	$form['submit'] = array(
		'#type' => 'submit',
		'#value' => t('Save configuration'));

	return $form;
}

function mygengo_admin_submit($form,&$form_state)
{
	$new_api = $form_state['values']['public'];
	$new_priv = $form_state['values']['private'];

	$old_api = variable_get('mygengo_cred_public');
	$old_priv = variable_get('mygengo_cred_private');

	if($new_api != $old_api || $new_priv != $old_priv)
	{
		variable_set('mygengo_cred_public',$new_api);
		variable_set('mygengo_cred_private',$new_priv);

		try
		{
			mygengo_poll();
		}
		catch(Exception $e)
		{
			
			drupal_set_message(t('Cerdientials wrong'),'error');
			variable_set('mygengo_cred_public',$old_api);
			variable_set('mygengo_cred_private',$old_priv);

			return;
		}
	}

	variable_set('mygengo_auto_appr',$form_state['values']['auto_appr']);
	drupal_set_message(t('Saved'));
}

// nukes the database and fetches a new one from myGengo
function mygengo_poll()
{
	mygengo_plug_poll();
	
	drupal_set_message(t('Database rebuild'));
	drupal_goto('');
}

// submit a raw translation job
function mygengo_translate_raw()
{
	$names = array();
	$langs = array();
	global $language_content;

	if(mygengo_plug_fetch_language_pairs($names,$langs))
	{
		drupal_add_js(drupal_get_path('module','mygengo') . "/mygengo.js",'file');;
	  drupal_add_js(array('mygengo' => array(
			'langs' => $langs,
			'default_lang' => $language_content->language)), 'setting');
	}
	else
		drupal_set_message(t("Failed to fetch language data from myGengo. Try again later."),'error');
	
	variable_set('mygengo_language_names',$names);

	$form = array('#cache' => FALSE);

	$form['summary'] = array(
		'#type' => 'textfield',
		'#default_value' => '',
		'#required' => TRUE,
		'#title' => t('Summary'));

	$form['content'] = array(
		'#type' => 'textarea',
		'#default_value' => '',
		'#required' => TRUE,
		'#attributes' => array(
			'onchange' => 'javascript:event_content();',
			'onkeyup' => 'javascript:event_content();'));

	return array_merge($form,mygengo_translate_form($names));
}

function mygengo_translate_raw_submit($form,&$form_state)
{
	$names = variable_get('mygengo_language_names');

	if($form_state['values']['content'] != '')
	{	
		$tgt = json_decode($form_state['values']['hidden_tbl']);
		foreach($tgt as &$t)
		{
			$resp = mygengo_plug_post_job($form_state['values']['src_lang'],
																		$t->lc,
																		$t->tier,
																		$form_state['values']['content'],
																		$form_state['values']['comment'],
																		$form_state['values']['summary'],
																		$form_state['values']['auto_approve']);
			if($resp['opstat'] == 'ok')
			{
				drupal_set_message(t("Submitted translation job form " . $names[$form_state['values']['src_lang']] . " to " . $names[$t->lc] . "."));
				mygengo_plug_proc_job($resp['response']['job']);
				mygengo_plug_thread($resp['response']['job']['job_id']);
				$form_state['redirect'] = '';
			}
			else
				drupal_set_message(t("Failed to submit translation job form " . $names[$form_state['values']['src_lang']] . " to " . $names[$t->lc] . ". Error: '" . $resp['err']['msg'] . "'"),'error');
		}
		if(count($tgt) == 0)
			drupal_set_message(t("No target language selected."),'error');
	}
	else
		drupal_set_message(t("Nothing to translate"),'error');
}

// build a form used for raw and node translation
function mygengo_translate_form($opts)
{
	$form['src_lang'] = array(
		'#type' => 'select',
		'#title' => t('Source language'),
		'#options' => $opts,
		'#DANGEROUS_SKIP_CHECK'=>true,
		'#attributes' => array('onchange' => array('javascript:event_src_select();')),
		);
	
	$form['tbl_lang'] = array(
		'#type' => 'markup',
		'#markup' => theme_table(array(
			'header' => array(t('Language'),t('Tier'),t('Cost'),t('Command')),
			'rows' => array(),
			'attributes' => array(),
			'caption' => t('Target languages'),
			'colgroups' => array(),
			'sticky' => FALSE,
			'empty' => t('Add target languages with the menu below.'))),
		'#prefix' => '<div id="trans-list-div">',
		'#suffix' => '</div>');

	$form['tgt_set'] = array(
		'#type' => 'fieldset',
		'#title' => t('Add target languages'));

	$form['tgt_set']['tgt_lang'] = array(
		'#type' => 'select',
		'#title' => t('Target language'),
		'#options' => $opts,
		'#DANGEROUS_SKIP_CHECK'=>true,
		'#attributes' => array('onchange' => array('javascript:event_tgt_select();')));

	$form['tgt_set']['tgt_tier'] = array(
		'#type' => 'radios',
		'#title' => t('Tier'),
		'#options' => array("machine" => "Machine", 
												"standard" => "Standard",
												"pro" => "Pro",
												"ultra" => "Ultra"));

	$form['tgt_set']['tgt_add'] = array(
		'#type' => 'markup',
		'#markup' => '<a href="javascript:add_language();">' . t('Add target language' . '</a>'));

	$form['comment'] = array(
		'#type' => 'textarea',
		'#title' => t('Job comment'),
		'#description' => t('Comment for the translator. Comments give context and direction to the translator. ' .
												'Jobs that have comments are usually picked up and translated more quickly than those that don\'t.'));

	$form['auto_approve'] = array(
		'#type' => 'checkbox',
		'#title' => t('Auto-approve and publish jobs'),
		'#default_value' => variable_get("mygengo_auto_appr"));

	$form['cost_lb'] = array(
		'#type' => 'item',
		'#markup' => '<span style="font-size: x-large;"><b>' . t('Price: ') . '</b></span><b><i><span style="font-size: x-large;" id="cost_field">XXX</span></i></b>');

	$form['hidden_tbl'] = array(
		'#type' => 'hidden',
		'#default_value' => '');

	$form['submit'] = array(
		'#type' => 'submit',
		'#value' => t('Submit job'));
	
	return $form;
}

// overview block
function mygengo_dashboard()
{
	$items = "";
	$names = variable_get('mygengo_language_names');

	foreach(db_query('SELECT * FROM {mygengo_jobs}')->fetchAllAssoc('jid') as $j)
		$items .= mygengo_detail(get_object_vars($j),$names);

	if($items == "")
		$items = '<span style="font-size: large; float: center;"><i>' . t('No jobs submitted.') . '</i></span>';

	return $items;
}

// renders single job for the dashboard
function mygengo_detail($j,$names,$btns=TRUE)
{
	$cmnts = db_select('mygengo_comments','c')
						->fields('c',array('cid'))
						->condition('jid',$j['jid'])
						->countQuery()->execute()->fetchField();
	$cmnts_unr = db_select('mygengo_comments','c')
								->fields('c',array('cid'))
								->condition('jid',$j['jid'])
								->condition('new','1')
								->countQuery()->execute()->fetchField();
	$opt = '';
	$tier;
		
	switch($j['status'])
	{
		case "available":
			$opt = l(t('Cancel'),'mygengo/cancel/' . $j['jid']);
			break;
		case "reviewable":
			$opt = l(t('Review'),'mygengo/review/' . $j['jid']);
			break;
		case "approved":
			$opt = l(t('View'),'mygengo/view/' . $j['jid']);
	}

	switch($j['tier'])
	{
		case 'standard':
			$tier = t('Standard');
			break;
		case 'pro':
			$tier = t('Pro');
			break;
		case 'ultra':
		case 'ultra_pro':
			$tier = t('Ultra');
			break;
		case 'machine':
			$tier = t('Machine');
	}

	$tbl = array(
		'header' => array(),
		'rows' => array(
			array(
				'data' => array(
					array('style' => 'min-width: 90px;', 'data' => t('Job #') . $j['jid']),																				// job id
					array('style' => 'min-width: 70px;', 'data' => ucwords($j['status'])),																				// status
					array('style' => 'min-width: 120px;', 'data' => l($cmnts . t(' comments') . ($cmnts_unr > 0 ? ' (' . $cmnts_unr . ' new)' : '') ,'mygengo/comments/' . $j['jid'])),	// comments
					sprintf('%.2f',$j['credits']) . t(' credits'),																																// credits
					t('Updated ') . format_date($j['atime'],'medium'),																														// updated
					''),																																																					// btn1
				'no_striping' => TRUE),
			array(
				'data' => array(
					$j['slug'],																																// summary
					$tier,																																		// tier
					$names[$j['lc_src']] . ' > ' . $names[$j['lc_tgt']],											// src, tgt lang
					$j['unit_count'] . ' ' . ($j['unit_count'] > 1 ? t('words') : t('word')),	// units
					t('Ordered ') . format_date($j['ctime'],'medium'),												// order ts
					$opt),																																		// btn2
				'no_striping' => TRUE)),
		'attributes' => array(),
		'caption' => "",
		'colgroups' => array(),
		'sticky' => FALSE,
		'empty' => "");

	if($btns == FALSE)
	{
		unset($tbl['rows'][0]['data'][5]);
		unset($tbl['rows'][1]['data'][5]);
	}

	return theme_table($tbl);
}

function mygengo_review_submit($form,&$form_state)
{
	$resp = mygengo_plug_put_approve(
		$form_state['values']['jid'],
		intval($form_state['values']['rating']),
		$form_state['values']['trans'],
		$form_state['values']['mygengo'],
		$form_state['values']['public']);
	
	if($resp['opstat'] != 'ok')
		drupal_set_message(t('Failed to approve job. Internal error!'));
	else
	{
		mygengo_plug_update($form_state['values']['jid']);
		$form_state['redirect'] = '';
		drupal_set_message(t('Job approved.'));
	}
}

// review form
function mygengo_review($form,&$form_state,$job)
{
	if($job['status'] != 'reviewable')
		drupal_set_message(t('Job #' ) . $job['jid'] . t(' can\'t be reviewed, yet.'),'error');

	drupal_set_title('Review job #' . $job['jid']);	

	$form['review'] = array(
		'#type' => 'form',
		'#value' => t('Job review'),
		'#description' => t('Approve translation'),
		'#cache' => FALSE);

	$form['review']['frame'] = array(
		'#type' => 'fieldset',
		'#title' => 'Preview');

	$form['review']['frame']['gif'] = array(
		'#type' => 'markup',
		'#markup' => theme_image(array('path' => url('mygengo/img/' . $job['jid']),'attributes' => array(),'alt' => $job['body_tgt'])));

	$form['review']['src'] = array(
		'#type' => 'item',
		'#title' => t('Original text'),
		'#markup' => '<p>' . $job['body_src'] . '</p>');

	$form['review']['rating'] = array(
		'#type' => 'radios',
		'#title' => t('Please rate the translation'),
		'#options' => array(
			'1' => '1 Bad',
			'2' => '2',
			'3' => '3',
			'4' => '4',
			'5' => '5 Great'),
		'#required' => TRUE);

	$form['review']['feedback'] = array(
		'#type' => 'fieldset',
		'#title' => t('Translation feedback'));
	
	$form['review']['feedback']['trans'] = array(
		'#type' => 'textarea',
		'#title' => t('Feedback for translator'),
		'#default_value' => '');

	$form['review']['feedback']['mygengo'] = array(
		'#type' => 'textarea',
		'#title' => t('Feedback for myGengo'),
		'#default_value' => '');

	$form['review']['feedback']['public'] = array(
		'#type' => 'checkbox',
		'#title' => t('Yes, you can use this translation as a public example of myGengo\'s service.'),
		'#description' => t('Can myGengo use this translation publicly in his examples?'));

	$form['review']['approve'] = array(
		'#type' => 'submit',
		'#value' => t('Approve'));

	$form['review']['options'] = array(
		'#type' => 'markup',
		'#markup' => 	'<span style="float: right;">' .
									l(t('Reject'),'mygengo/reject/' . $job['jid']) .
									'&nbsp;|&nbsp;' .
									l(t('Request corrections'),'mygengo/correct/' . $job['jid']) .
									'</span>');
	
	$form['review']['jid'] = array(
		'#type' => 'hidden',
		'#value' => $job['jid']);
		
	return $form;
}

// returns the prefiew gif. used with review form
function mygengo_img($job)
{
	drupal_add_http_header('Content-type','image/gif');
	echo mygengo_plug_get_preview($job['jid']);
}

// cancel a job in the 'available' status
function mygengo_cancel($job)
{
	if($job['status'] == 'available')
	{
		$json = mygengo_plug_cancel($job['jid']);
		if($json['opstat'] == 'ok')
		{
			db_delete('mygengo_jobs')->condition('jid',$job['jid'])->execute();
			drupal_set_message(t("Job canceled"));
		}
		else
			drupal_set_message(t("Job couldn't be canceled:") . $json['err']['msg'],'error');
	}
	else
		drupal_set_message(t("You can only cancel a job if it has not been started already by a translator."),'error');
	drupal_goto('');
}

// view a approved translation and sumit a feedback
function mygengo_view($form,&$form_state,$job)
{
	$json = mygengo_plug_get_feedback($job['jid']);

	drupal_set_title('View job');
	$form['block'] = array(
		'#type' => 'markup',
		'#markup' => mygengo_detail($job,variable_get('mygengo_language_names'),FALSE));

	$form['body'] = array(
		'#type' => 'item',
		'#title' => t('Body'),
		'#markup' => '<p>' . $job['body_tgt'] . '</p>');

	if($json['opstat'] == 'ok')
	{
		$form['feedback'] = array(
			'#type' => 'fieldset',
			'#title' => t('Translation feedback'));
	
		$form['feedback']['rating'] = array(
			'#type' => 'item',
			'#title' => t('Rating'),
			'#markup' => $json['response']['feedback']['rating']);

		if($json['response']['feedback']['for_translator'])
		{
			$form['feedback']['trans'] = array(
				'#type' => 'item',
				'#title' => t('Feedback for translator'),
				'#markup' => '<p>'. $json['response']['feedback']['for_translator'] . '</p>');
		}
	}
	return $form;
}

// read/post comments
function mygengo_comments($form,&$form_state,$job)
{
	drupal_set_title('Comments on job #' . $job['jid']);

	$i = 0;
	$db = db_query('SELECT * FROM {mygengo_comments} WHERE jid = :jid ORDER BY ctime',array(':jid' => $job['jid']))->fetchAll();
	foreach($db as &$cmnt)
	{
		$bgrd = ($cmnt->author == 'customer' ? 'rgba(22,22,22,ff)' : 'rgba(0,0,0,0)');
		$form['cmnt_' . $i] = array(
			'#type' => 'markup',
			'#markup' => theme_table(array(
				'header' => array(),
				'rows' => array(
					array(
						'data' => array($cmnt->body),
						'style' => 'font-size: large; background: '.$bgrd.' !important',
						'no_striping' => TRUE),
					array(
						'data' => array(t(ucwords($cmnt->author)) . '&nbsp;|&nbsp;' . format_date($cmnt->ctime,'medium')),
						'style' => 'background: rgba(0,0,0,0) !important',
						'no_striping' => TRUE)),
				'attributes' => array('style' => 'background: rgba(22,22,22,0) !important;'),
				'caption' => "",
				'colgroups' => array(),
				'sticky' => FALSE,
				'empty' => "")));
		$i += 1;
	}

	$form['post'] = array(
		'#type' => 'fieldset',
		'#title' => t('New comment'));

	$form['post']['body'] = array(
		'#type' => 'textarea',
		'#description' => t('Your comment'),
		'#default_value' => '');

	$form['post']['jid'] = array(
		'#type' => 'hidden',
		'#value' => $job['jid']);
	
	$form['post']['submit'] = array(
		'#type' => 'submit',
		'#description' => t('Requesting personal contact details from translators is prohibited.'),
		'#value' => t('Submit comment'));
	
	db_update('mygengo_comments')->fields(array('new' => '0'))->condition('jid',$job['jid'])->execute(); 
	return $form;
}

function mygengo_comments_submit($form,&$form_state)
{
	if($form_state['values']['body'] != '')
	{
		$json = mygengo_plug_post_comment($form_state['values']['jid'],$form_state['values']['body']);
		if($json['opstat'] == 'ok')
		{
			drupal_set_message(t('Comment posted'));
			mygengo_plug_thread($form_state['values']['jid']);
		}
		else
			drupal_set_message(t('Failed to post comment'),'error');
		drupal_goto('');
	}
	else
		drupal_set_message(t('Please write a comment'),'error');
}

// reject form
function mygengo_reject($form,&$form_state,$job)
{
	drupal_set_title(t('Reject job #') . $job['jid']);
	
	$form['intro'] = array(
		'#type' => 'markup',
		'#markup' => t('<p>Please use rejections sparingly, and only as a last resort.</p><p>If you\'re not happy with a translation, you can reject and cancel the job. However, before you receive your full refund, the myGengo Quality Control team will review your request and determine whether or not it was a fair rejection. You also have the option to pass the translation along to another translator if you don\'t want to cancel the job.</p><p>We\'re people too (and so are our translators). So please try to explain things as calmly as possible if things go wrong - as the Beatles say \'We can work it out\' :)</p><p>Please read the <a href="http://mygengo.com/help/faqs">FAQ</a> for more informations.</p>'));

	$form['follow_up'] = array(
		'#type' => 'radios',
		'#title' => t('Would you like to cancel the translation?'),
		'#options' => array(
			'cancel' => t('Yes, please cancel and refund me'),
			'requeue' => t('No, please have another translator finish the job')),
		'#required' => TRUE);
	
	$form['reason'] = array(
		'#type' => 'radios',
		'#title' => t('Rejection reason'),
		'#options' => array(
			'quality' => t('Poor quality of the translation'),
			'incomplete' => t('Missing or incomplete translation'),
			'other' => t('Other (please describe below)')),
		'#required' => TRUE);

	$form['comment'] = array(
		'#type' => 'textarea',
		'#title' => t('Feedback for original translator'));

	$form['captcha_img'] = array(
		'#type' => 'markup',
		'#markup' => theme_image(array(
			'path' => $job['captcha_url'],
			'attributes' => array())));
	
	$form['captcha'] = array(
		'#type' => 'textfield',
		'#title' => t('Please enter the text above (to confirm you\'re human)'),
		'#required' => TRUE);

	$form['submit'] = array(
		'#type' => 'submit',
		'#value' => t('Reject translation'));

	return $form;
}

function mygengo_reject_submit($form,&$form_state)
{
	if($form_state['values']['comment'] == '')
	{
		drupal_set_message(t('Please supply a feedback'),'error');
		return;
	}
	else
	{
		$json = mygengo_plug_reject($form_state['build_info']['args'][0]['jid'],
																$form_state['values']['follow_up'],
																$form_state['values']['reason'],
																$form_state['values']['comment'],
																$form_state['values']['captcha']);
		if($json['opstat'] == 'ok')
		{
			mygengo_plug_update($form_state['build_info']['args'][0]['jid']);
			drupal_set_message(t('Rejection submitted'));
			drupal_goto('');
		}
		else if($json['err']['code'] == 2401)
		{
			drupal_set_message(t('Invalid captcha'),'error');
			db_update('mygengo_jobs')
				->fields(array('captcha_url' => $json['err']['msg'][1]))
				->condition('jid',$form_state['build_info']['args'][0]['jid'])
				->execute();
		}
		else
			drupal_set_message(t('Failed to submit rejection form'),'error');
	}
}

// request correction form
function mygengo_correct($form,&$form_state,$job)
{
	drupal_set_title(t('Request correction for job #') . $job['jid']);
	
	$form['intro'] = array(
		'#type' => 'markup',
		'#markup' => t('<p>If you find a few small mistakes in the translation - or the translator has not fully responded to your comment requests, please select the request corrections option and explain what changes need to be done.</p><p>If you think that corrections will not be enough, then please reject the translation and either choose to pass the job onto another translator, or request a refund. If you do choose to reject a job, please give a detailed explanation of the reason. This information will be helpful for us in improving our services, as well as good feedback for the translator.</p><p>Please read the <a href="http://mygengo.com/help/faqs">FAQ</a> for more informations.</p>'));

	$form['comment'] = array(
		'#type' => 'textarea',
		'#title' => t('Use this space to make a formal correction request'));

	$form['submit'] = array(
		'#type' => 'submit',
		'#value' => t('Request corrections from translator'));
	
	return $form;
}

function mygengo_correct_submit($form,&$form_state)
{
	if($form_state['values']['comment'] == '')
	{
		drupal_set_message(t('Please supply a request'),'error');
		return;
	}
	else
	{
		$json = mygengo_plug_correct($form_state['build_info']['args'][0]['jid'],
																$form_state['values']['comment']);
		if($json['opstat'] == 'ok')
		{
			mygengo_plug_update($form_state['build_info']['args'][0]['jid']);
			drupal_set_message(t('Correction request submitted'));		
			drupal_goto('');
		}
		else
			drupal_set_message(t('Failed to submit correction form'),'error');
	}
}

// load the content of a node into the raw translation form
// called by the links in the content menu
function mygengo_translate_node($form,&$form_state,$node)
{
	drupal_set_title(t('Translate with myGengo'));

	$names = array();
	$langs = array();
	global $language_content;

	if(mygengo_plug_fetch_language_pairs($names,$langs))
	{
		drupal_add_js(drupal_get_path('module','mygengo') . "/mygengo.js",'file');;
	  drupal_add_js(array('mygengo' => array(
			'langs' => $langs,
			'default_lang' => $language_content->language)), 'setting');
	}
	else
		drupal_set_message(t("Failed to fetch language data from myGengo. Try again later."),'error');
	
	variable_set('mygengo_language_names',$names);

	$form = array('#cache' => FALSE);

	$form['summary'] = array(
		'#type' => 'textfield',
		'#default_value' => $node->title,
		'#required' => TRUE,
		'#title' => t('Summary'));

	$cnt = "";
	if(array_key_exists($language_content->language,$node->body))
		$cnt = $node->body[$language_content->language][0]['value'];
	else if(array_key_exists('und',$node->body))
		$cnt = $node->body['und'][0]['value'];
	else
	{
		$e = current($node->body);
		$cnt = $e[0]['value'];
	}

	$form['content'] = array(
		'#type' => 'textarea',
		'#default_value' => $cnt,
		'#required' => TRUE,
		'#attributes' => array(
			'onchange' => 'javascript:event_content();',
			'onkeyup' => 'javascript:event_content();'));

	return array_merge($form,mygengo_translate_form($names));
}

function mygengo_translate_node_submit($form,&$state)
{
	return mygengo_translate_raw_submit($form,$state);
}
