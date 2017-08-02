function load_sel_hdd()
{
	for(i=0; i<usb_drive_num; i++) {
		var str = eval("usb_drive" + i);
		var each_info = str.split(",");
		if(pri_hdd_id == each_info[0]) {
			pri_sel_hdd = pri_hdd;
			pri_sel_hdd_id = each_info[0];
		}
		if(bak_hdd_id == each_info[0]) {
			bak_sel_hdd = bak_hdd;
			bak_sel_hdd_id = each_info[0];
		}
	}
}

function clear_sel_hdd(flag)
{
	if(!document.getElementById(flag + "_drive_table"))
		return;
	var row_length = document.getElementById(flag + "_drive_table").rows.length - 1;
	var have_seled = 0;
	
	for (i = 0; i < row_length; i++)
	{
		if(row_length == 1)
			var obj = "cf." + flag + "_sel_drive";
		else
			var obj = "cf." + flag + "_sel_drive[i]";
		if (eval(obj + ".checked == true"))
		{	
			have_seled = 1;
		}
	}
	if(have_seled == 0)
	{
		eval(flag + "_sel_hdd = ''");
		eval(flag + "_sel_hdd_id = ''");
	}
}

function display_control()
{
	if(usb_drive_num == "0")
	{
		set_display("", "pri_error", "bak_error");
		set_display("none", "tip_div", "input_path");
	}
	else
	{
		if(pri_hdd == "")
		{
			set_display("", "pri_warn");
			set_display("none", "pri_error");
		}
		else if(pri_sel_hdd == "")
		{
			set_display("", "pri_error");
			set_display("none", "pri_warn");
		}
		if(bak_hdd == "")
		{
			set_display("", "bak_warn");
			set_display("none", "bak_error");
		}
		else if(bak_sel_hdd == "")
		{
			set_display("", "bak_error");
			set_display("none", "bak_warn");
		}
		if(pri_sel_hdd != "")
			set_display("none", "pri_warn", "pri_error");
		if(bak_sel_hdd != "")
			set_display("none", "bak_warn", "bak_error");
		if(pri_sel_hdd != "" && bak_sel_hdd != "")
			set_display("none", "tip_div");
		else
			set_display("", "tip_div");
		if(!document.getElementById("pri_drive_table"))
		{
			set_display("none", "pri_warn", "pri_error");
			set_display("", "pri_another");
		}
		else
			set_display("none", "pri_another");
		if(!document.getElementById("bak_drive_table"))
		{
			set_display("none", "bak_warn", "bak_error", "input_path");
			set_display("", "bak_another");
		}
		else
		{	
			set_display("none", "bak_another");
			set_display("", "input_path");
		}
	}
}

function disabled_control()
{
	if(status == "3")
	{
		set_disabled(true, "apply");
		cf.apply.className = "new_greyapply_bt ui-corner-all";
	}
	else if(master == "admin")
	{
		set_disabled(false, "apply");
		cf.apply.className = "new_apply_bt ui-corner-all";
	}
}

function load_status()
{
	if(status == 1)
	{
		set_display("", "bak_good");
		set_display("none", "bak_faild", "bak_sync");
	}
	else if(status == 2)
	{
		set_display("", "bak_faild");
		set_display("none", "bak_good", "bak_sync");
	}
	else if(status == 3 && (usb_drive_num != "0" || pri_sel_hdd_id != "" || bak_sel_hdd_id != ""))
	{
			set_display("", "bak_sync");
			set_display("none", "bak_good", "bak_faild");
			document.getElementById("progress_bar").style.width = parseInt(progress)/2 + "px";
			document.getElementById("progress_num").innerHTML = progress + "%";
	}
}

function click_apply() {
	if(pri_sel_hdd == "") {
		alert("Please select your primary drive.");
		return false;
	}
	if(bak_sel_hdd == "") {
		alert("Please select your backup drive.");
		return false;
	}
	if(entered == 0) {
		alert("Please enter the USB backup directory.")
		return false;
	}
	
	var day = "";
	if (cf.backup_sche[0].checked == true)
		cf.hid_backup_schedule.value = "0";
	else if (cf.backup_sche[1].checked == true) {
		cf.hid_backup_schedule.value = "1";
		for (i = 1; i <= 7; i++) {
			if (eval("cf.day" + i + ".checked")) {
				if (day == "")
					day = day + i + "";
				else
					day = day + "," + i;
			}
		}
	}
	cf.hid_backup_day.value = day;
	if(cf.am_pm.options[1].selected == true)
		cf.hid_backup_hour.value = parseInt(cf.hour.value) + 12;
	else
		cf.hid_backup_hour.value = cf.hour.value;
	cf.hid_backup_minute.value = cf.minute.value;
	cf.hid_backup_ampm.value = cf.am_pm.value;

	if (cf.enable_auto_clean.checked == true)
		cf.hid_enable_auto_clean.value = "1";
	else
		cf.hid_enable_auto_clean.value = "0";
	cf.hid_clean_day.value = cf.auto_clean_day.value;
	cf.hid_clean_clock.value = cf.auto_clean_clock.value;
	
	cf.hid_pri_hdd.value = pri_sel_hdd;
	cf.hid_pri_hdd_id.value = pri_sel_hdd_id;
	cf.hid_bak_hdd.value = bak_sel_hdd;
	cf.hid_bak_hdd_id.value = bak_sel_hdd_id;
	cf.hid_bak_path.value = bak_sel_hdd + "/" + cf.backup_path.value;

	if(pri_hdd_id != "" && pri_hdd_id != pri_sel_hdd_id)
	{
		usb_change_alert();
		return false;
	}
	cf.submit();
}

function print_drive_info(flag) {
	var jump = 0;
	
	if(usb_drive_num == 0)
		return;
	tableHTML = "";
	tableHTML += '<TABLE class="drive_table" id="' + flag + '_drive_table" cellpadding="0" cellspacing="0">'
	tableHTML += '<TR class="title_line">';
	tableHTML += '<TD style="width:3%" class="no_right"></TD>';
	tableHTML += '<TD>Disk Number</TD>';
	tableHTML += '<TD>Volume Name</TD>';
	tableHTML += '<TD>Serias Number</TD>';
	tableHTML += '<TD>Size</TD>';
	tableHTML += '<TD class="no_right">USB Slot</TD>';
	tableHTML += '</TR>';

	for (i = 0, j = 0; i < usb_drive_num; i++) {
		var str = eval("usb_drive" + i);
		var each_info = str.split(",");
		if ((flag == "pri" && bak_sel_hdd_id == each_info[0]) || (flag == "bak" && pri_sel_hdd_id == each_info[0])) {
			jump = 1;
			continue;
		}
		if (eval(flag + "_sel_hdd_id") == each_info[0])
			tableHTML += '<TR class="drive_selected"><TD style="width:3%" class="no_right"><input type="radio" name="' + flag + '_sel_drive" value="' + each_info[1] + '" onclick="inner_table(\'' + flag + '\');" checked></TD>';
		else
			tableHTML += '<TR class="drive_unselected"><TD style="width:3%" class="no_right"><input type="radio" name="' + flag + '_sel_drive" value="' + each_info[1] + '" onclick="inner_table(\'' + flag + '\');"></TD>';
		tableHTML += '<TD>HDD ' + (j + 1) + '</TD>';
		tableHTML += '<TD>' + each_info[4] + '</TD>';
		tableHTML += '<TD>' + each_info[0] + '</TD>';
		tableHTML += '<TD>' + each_info[2] + '</TD>';
		tableHTML += '<TD class="no_right">' + each_info[5] + '</TD></TR>';
		j++;
	}
	tableHTML += '</TABLE>';
	
	if(usb_drive_num == 1 && jump == 1)
		tableHTML = "";
	
	eval("document.getElementById('" + flag + "_table_div').innerHTML = tableHTML");
	
	display_control();
}

function inner_table(flag) {
	if (flag == "pri")
		var chg_flag = "bak";
	else
		var chg_flag = "pri";
	
	var row_length = document.getElementById(flag + "_drive_table").rows.length - 1;
	for (i = 0; i < row_length; i++) {
		if(row_length == 1)
			var obj = "cf." + flag + "_sel_drive";
		else
			var obj = "cf." + flag + "_sel_drive[i]";
		if (eval(obj + ".checked == true")) {
			eval(flag + "_sel_hdd = " + obj + ".value");
			var sel_id = document.getElementById(flag + "_drive_table").rows[i + 1].cells[3].innerHTML;
			eval(flag + "_sel_hdd_id = '" + sel_id + "'");
			print_drive_info(chg_flag);
			break;
		}
	}
	for (i = 0; i < row_length; i++) {
		if (eval(obj + ".value == " + flag + "_sel_hdd"))
			document.getElementById(flag + "_drive_table").rows[i + 1].className = "drive_selected";
		else
			document.getElementById(flag + "_drive_table").rows[i + 1].className = "drive_unselected";
	}
}

function clear_tip(th) {
	if (entered == 0) {
		th.value = "";
		th.style.color = "black";
	}
}

function add_tip(th) {
	if (entered == 0) {
		th.value = "Enter directory name";
		th.style.color = "gray";
	}
}

function tip_or_not(th) {
	if(th.value == "")
		entered = 0;
	else
		entered = 1;
}

function set_display()
{
	for(i=1; i<arguments.length; i++)
	{
		if(document.getElementById(arguments[i]))
			eval("document.getElementById(arguments[i]).style.display = arguments[0]");
		else if(x = document.getElementsByName(arguments[i]))
		{
			for(j=0; j<x.length; j++)
				eval("x[j].style.display = arguments[0]");
		}
	}
}

function set_disabled()
{
	for(i=1; i<arguments.length; i++)
	{
		if(document.getElementById(arguments[i]))
			eval("document.getElementById(arguments[i]).disabled = arguments[0]");
		else if(x = document.getElementsByName(arguments[i]))
		{
			for(j=0; j<x.length; j++)
				eval("x[j].disabled = arguments[0]");
		}
	}
}

function usb_change_alert()
{
	var message = '<table cellpadding=0 cellspacing=2  border=0>'
	message += '<tr><td colspan=3>It seems your Primary USB drive has been modified or periodically detached from the router. What would you like to do?</td></tr>'
	message += '<tr><td colspan=3 height="20px"></td></tr>'
	message += '<tr><td colspan=3><input type="radio" value="0" name="usb_chg_act" checked><span onclick="document.forms[0].usb_chg_act[0].checked = true;">Recover data from Backup drive then delete the data on backup drive.</span></td></tr>'
	message += '<tr><td colspan=3><input type="radio" value="1" name="usb_chg_act"><span onclick="document.forms[0].usb_chg_act[1].checked = true;">Delete backup data from Backup drive</span></td></tr>'
	message += '<tr><td colspan=3><input type="radio" value="2" name="usb_chg_act"><span onclick="document.forms[0].usb_chg_act[2].checked = true;">Do nothing</span></td></tr>'
	message += '</table>';
	sAlert(message, function(){usb_change();}, "", 475, 1, "Confirm Selection");
}

function remove_msg()
{
	document.body.removeChild(document.getElementById("bgDiv"));
	document.getElementById("msgDiv").removeChild(document.getElementById("msgTitle"));
	document.forms[0].removeChild(document.getElementById("msgDiv"));
}

function click_clean()
{
	cf.submit_flag.value = "usb_backup_clean";
	cf.submit();
}

function usb_change()
{
	if(cf.usb_chg_act[0].checked == true)
		cf.hid_usb_change_flag.value = "recover";
	else if(cf.usb_chg_act[1].checked == true)
		cf.hid_usb_change_flag.value = "delete";
	else if(cf.usb_chg_act[2].checked == true)
		cf.hid_usb_change_flag.value = "nothing";
		
	cf.submit_flag.value = "usb_backup_change";
	cf.submit();
}

function open_error()
{
	window.open('USB_backup_error.htm','backup_error','resizable=0,scrollbars=yes,width=900,height=600,left=250,top=150').focus();
}
