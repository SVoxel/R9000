function goto_next(cf, wl_login)
{
	if(cf.choose[0].checked)
	{	
		var ssid_bgn = document.getElementById("ESSID").value;
		var ssid_an = document.getElementById("ESSID_an").value;
		var ssid_ad = document.getElementById("ESSID_ad").value;

		if(ssid_bgn == "" || ssid_an == "" || ssid_ad == "")
		{
			alert(ssid_null);
			return false;
		}

		for(i=0;i<ssid_bgn.length;i++)
		{
			if(isValidChar_space(ssid_bgn.charCodeAt(i))==false)
			{
				alert(ssid_not_allowed);
				return false;
			}
		}

		for(i=0;i<ssid_an.length;i++)
		{
			if(isValidChar_space(ssid_an.charCodeAt(i))==false)
			{
				alert(ssid_not_allowed);
				return false;
			}
		}
		
		for(i=0; i<ssid_ad.length; i++)
		{
			if(isValidChar_space(ssid_ad.charCodeAt(i))==false)
			{
				alert(ssid_not_allowed);
				return false;
			}
			
			var code = ssid_ad.charCodeAt(i);
			if(code == "34" || code == "38" || code == "39" || code == "40" || code == "41" || code == "42" || code == "59" || code == "60" || code == "62")
			{
				alert("60G wireless ssid doesn't support these characters \"&'()*;<>");
				return false;
			}
		}

		if(checkpsk(cf.passphrase_an, cf.wla_sec_wpaphrase_len)== false)
			return;
		if( checkpsk(cf.passphrase, cf.wl_sec_wpaphrase_len)== false)
			return;
		if( checkpsk(cf.passphrase_ad, cf.wig_sec_wpaphrase_len, "ad")== false)
			return
		cf.wl_hidden_wpa_psk.value = cf.passphrase.value;
		cf.wla_hidden_wpa_psk.value = cf.passphrase_an.value;
		cf.wig_hidden_wpa_psk.value = cf.passphrase_ad.value;
		cf.method="post";
		if(wl_login == 1)
			cf.action="/apply.cgi?/BRS_ap_detect_01_04.html timestamp="+ts;
		else
			cf.action="/apply.cgi?/BRS_01_checkNet_ping.html timestamp="+ts;
		cf.submit_flag.value="wl_ssid_password";
	
		cf.submit();
	}
	else if(cf.choose[1].checked)
		this.location.href="BRS_01_checkNet_ping.html";
	else
	{
		alert(bh_warning_info);
		return false;
	}	
}

function goback()
{	
	var pre_url = document.referrer;
	var pre_url_info = pre_url.split("/");
	
	if(pre_url_info[3] == "BRS_ap_detect_01_router_01.html")
		this.location.href = "BRS_ap_detect_01_router_01.html";
	else if(pre_url_info[3] == "BRS_00_02_ap_select.html")
		this.location.href = "BRS_00_02_ap_select.html";
	else if(typeof top.brs_pre_url != "undefined")
		this.location.href = top.brs_pre_url;
	else
		return false;
}

