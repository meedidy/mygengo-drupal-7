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

jQuery(document).ready(function ()
{
	jQuery('#edit-src-lang').empty();
	for(var i in Drupal.settings.mygengo.langs)
	{
		var lang = Drupal.settings.mygengo.langs[i];
		var str = '<option value="' + lang.lc + '"';
		if(lang.lc == Drupal.settings.mygengo.default_lang)
			str += ' selected="1"';
		str += '>' + lang.language + '</option>';

		jQuery('#edit-src-lang').append(str);
	}

	Drupal.settings.mygengo.costs = new Array();

	event_costs();
	event_src_select();
});

/*
 * utility functions
 */
function language(lc)
{
	return Drupal.settings.mygengo.langs[lc];
}

function current_slang()
{
	var src = jQuery('#edit-src-lang')[0];
	var opt = src[src.selectedIndex];

	return language(opt['value']);
}

function current_tlang()
{
	var tgt = jQuery('#edit-tgt-lang')[0];
	var opt = tgt[tgt.selectedIndex];

	return current_slang().lc_tgt[opt['value']];
}

function tgt_language(lc,tier)
{
	var tmp = current_slang().lc_tgt[lc];
	for (var i in tmp)
	{
		if(tmp[i].tier == tier)
			return tmp[i];
	}
	return null;
}

function content()
{
	var area = jQuery('#edit-content')[0];
	if(area)
		return area.value;
	else
		return "";
}

function words()
{
	var txt = content()
	var r = 0;

	var a = txt.replace(/\s/g,' ');
	a = a.split(' ');
	for (var z = 0; z < a.length; z++)
		if(a[z].length > 0)
			r++;

	return r;
}

/*
 * event callbacks
 */
function event_src_select()
{
	var tgt = jQuery('#edit-tgt-lang');
	var tbl = jQuery('#trans-list-div table tbody');
	var lctgt = current_slang().lc_tgt;

	tgt.empty();
	tbl.empty();
	Drupal.settings.mygengo.costs = new Array;
	event_costs();

	event_content();
	
	for(var i in lctgt)
	{
		var lang = lctgt[i];
		tgt.append('<option value="' + lang[0].lc_tgt + '">' + Drupal.settings.mygengo.langs[lang[0].lc_tgt].language + '</option>');
	}

	event_tgt_select();
}

function event_tgt_select()
{
	var tier_mch = jQuery('#edit-tgt-tier input[value=machine]');
	var tier_std = jQuery('#edit-tgt-tier input[value=standard]');
	var tier_pro = jQuery('#edit-tgt-tier input[value=pro]');
	var tier_ultra = jQuery('#edit-tgt-tier input[value=ultra]');
	var c_tlang = current_tlang();

	tier_mch.attr('disabled','disabled');
	tier_std.attr('disabled','disabled');
	tier_pro.attr('disabled','disabled');
	tier_ultra.attr('disabled','disabled');

	tier_mch.checked = false;
	tier_std.checked = false;
	tier_pro.checked = false;
	tier_ultra.checked = false;
	
	for(var i in c_tlang)
	{
		switch(c_tlang[i].tier)
		{
			case "machine":
				tier_mch.removeAttr('disabled');
				break;
			case "standard":
				tier_std.removeAttr('disabled');
				break;
			case "pro":
				tier_pro.removeAttr('disabled');
				break;
			case "ultra":
				tier_ultra.removeAttr('disabled');
		}
	}
}

function event_content()
{
	var slang = current_slang();
	var cnt;
	var sum = 0.0;

	if(slang.unit_type == "word")
		cnt = words();
	if(slang.unit_type == "character")
		cnt = content().length;

	for(var i in Drupal.settings.mygengo.costs)
	{
		var c = Drupal.settings.mygengo.costs[i];
		var tr = jQuery('#lang-' + c['lc'] + '');
		var td_cost = tr.children(':nth-child(3)');
		var tlang = tgt_language(c['lc'],c['tier']);

		sum += cnt * tlang.unit_price;

		td_cost.replaceWith('<td>' + (cnt * tlang.unit_price).toFixed(2) + '</td>');
	}

	jQuery('#cost_field').html(sum.toFixed(2));
}

function event_costs()
{
	hid = jQuery('input[name=hidden_tbl]');
	hid.attr('value',JSON.stringify(Drupal.settings.mygengo.costs));
}		

/*
 * button callbacks
 */
function add_language()
{
	var src = jQuery('#edit-src-lang')[0];
	var tgt = jQuery('#edit-tgt-lang')[0];
	var sopt = src[src.selectedIndex];

	if(tgt.selectedIndex == -1)
		return;

	var topt = tgt[tgt.selectedIndex];
	var tier = jQuery('#edit-tgt-tier input:checked');

	if(!tier[0])
		return;

	var tier_lb = jQuery('label[for=' + tier[0].id + ']');
	var tbl = jQuery('#trans-list-div table tbody');

	if(topt && !topt.disabled && !tier.attr('disabled'))
	{
		var c = {lc: tgt['value'], tier: tier[0]['value']};
		Drupal.settings.mygengo.costs.push(c);
		event_costs();

		tbl.append('<tr id="lang-' + tgt['value'] + '">' + 
							 '<td>' + Drupal.settings.mygengo.langs[topt['value']].language + '</td>' +
							 '<td value="' + tier[0]['value'] + '">' + jQuery(tier_lb[0].childNodes.item(0)).text() + '</td>' +
							 '<td>0.00</td>' +
							 '<td><a href="javascript:del_language(\'' + tgt['value'] + '\');">remove</a></td></tr>');
		topt.disabled = true;
		tgt.selectedIndex = -1;
	}

	event_content();
}

function del_language(lang)
{
	var td = jQuery('#lang-' + lang);
	var tgt = jQuery('#edit-tgt-lang option[value=' + lang + ']')[0];

	var i = 0;
	while(i < Drupal.settings.mygengo.costs.length)
	{
		if(Drupal.settings.mygengo.costs[i]['lc'] == lang)
			Drupal.settings.mygengo.costs.splice(i,1);
		i++;
	}

	td.remove();
	tgt.disabled = false;
	event_content();
	event_costs();
}
