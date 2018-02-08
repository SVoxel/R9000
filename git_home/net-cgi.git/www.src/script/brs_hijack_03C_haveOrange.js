function initPage()
{
	//buttons left
	var btns_div1 = document.getElementById("back");
	btns_div1.value = bh_back_mark;

	if( master == "admin" )
	btns_div1.onclick = function()
	{
		return goBack();
	}
	else
		btns_div1.className = "grey_short_btn";
	
	
	//buttons right
	var btns_div2 = document.getElementById("next");
	btns_div2.value = bh_next_mark;
	if( master == "admin" )
	btns_div2.onclick = function()
	{
		return check_orange();
	}
	else
		btns_div2.className = "grey_short_btn";
}

function goBack()
{
	if(top.dsl_enable_flag == "0")
		this.location.href = "BRS_02_genieHelp.html";
	else	
	{
		if(top.location.href.indexOf("BRS_index.htm") > -1)
			this.location.href = "BRS_ISP_country_help.html";
		else
			this.location.href = "DSL_WIZ_sel.htm";
	}	
	return true;
}

function check_orange()
{
	var cf = document.forms[0];

	if(cf.orange_login.value=="")
	{
		alert("$login_name_null");
		return false;
	}
	for(var i=0;i<cf.orange_login.value.length;i++)
	{
		if(isValidChar(cf.orange_login.value.charCodeAt(i))==false)
		{
			alert("$loginname_not_allowed");
			return false;
		}
	}

	if(cf.enable_orange.checked == true)
	{
		cf.hidden_enable_orange.value = "1";
	} else {
		cf.hidden_enable_orange.value = "0";
	}
	top.orange_apply_flag="1";
	cf.submit();
}

addLoadEvent(initPage);
