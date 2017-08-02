function click_backup()
{
	if(degrade_version != "" && degrade_feature != "") {
		if(degrade_feature == "QoS" && qos_status == "1") {
			var disable_qos = confirm("This config file is applicable to firmware which is up to version " + degrade_version + ". If you still want to restore this config file to lower " + degrade_version + " version, please disable " + degrade_feature + " function. Do you need we disable " + degrade_feature + " function?");
			if(disable_qos) {
				document.forms[0].action="/backup.cgi?/BAK_backup.htm timestamp="+dts;
				document.forms[0].submit();
				return;
			}
		}
	}
	document.forms[0].action="/backup.cgi?/BAK_backup.htm timestamp="+ts;
	document.forms[0].submit();
}

function check_restore()
{
	cf=document.forms[0];
	if (cf.mtenRestoreCfg.value.length == 0)
	{
		alert("$filename_null");
		return false;
	}
	var filestr=cf.mtenRestoreCfg.value;
	var file_format=filestr.substr(filestr.lastIndexOf(".")+1);
	if (file_format!="cfg")
	{
		alert("$not_correct_file"+"cfg");
		return false;
	} 

	if (confirm("$ask_for_restore"))
	{	
		cf.action="/restore.cgi?/restore_process.htm timestamp="+ts;
		top_left_nolink();
		top.enable_action=0;
		cf.submit();
	}
	else 
		return false;
}

function click_yesfactory()
{
	cf=document.forms[0];
	cf.action="/apply.cgi?/pls_wait_factory_reboot.html timestamp="+ts;
	top_left_nolink();
	top.enable_action=0;
	cf.submit();
}
