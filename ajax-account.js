jQuery(document).ready(function () 
{ 
	jQuery.ajax({
   			type: "GET",
				url: "?q=mygengo/account",
   			success: function(msg){
					jQuery('#mygengo_account_div').replaceWith(msg);
   			}
 		});
});
