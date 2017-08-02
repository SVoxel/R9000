var hour_schedule_12=["12:00 $midnight_mark."/*0*/,"12:30 a.m."/*1*/,"01:00 a.m."/*2*/,"01:30 a.m."/*3*/,"02:00 a.m."/*4*/,"02:30 a.m."/*5*/,"03:00 a.m."/*6*/,"03:30 a.m."/*7*/,"04:00 a.m."/*8*/,"04:30 a.m."/*9*/,"05:00 a.m."/*10*/,"05:30 a.m."/*11*/,"06:00 a.m."/*12*/,"06:30 a.m."/*13*/,"07:00 a.m."/*14*/,"07:30 a.m."/*15*/,"08:00 a.m."/*16*/,"08:30 a.m."/*17*/,"09:00 a.m."/*18*/,"09:30 a.m."/*19*/,"10:00 a.m."/*20*/,"10:30 a.m."/*21*/,"11:00 a.m."/*22*/,"11:30 a.m."/*23*/,"12:00 $noon_mark"/*24*/,"12:30 p.m."/*25*/,"01:00 p.m."/*26*/,"01:30 p.m."/*27*/,"02:00 p.m."/*28*/,"02:30 p.m."/*29*/,"03:00 p.m."/*30*/,"03:30 p.m."/*31*/,"04:00 p.m."/*32*/,"04:30 p.m."/*33*/,"05:00 p.m."/*34*/,"05:30 p.m."/*35*/,"06:00 p.m."/*36*/,"06:30 p.m."/*37*/,"07:00 p.m."/*38*/,"07:30 p.m."/*39*/,"08:00 p.m."/*40*/,"08:30 p.m."/*41*/,"09:00 p.m."/*42*/,"09:30 p.m."/*43*/,"10:00 p.m."/*44*/,"10:30 p.m."/*45*/,"11:00 p.m."/*46*/,"11:30 p.m."/*47*/];

var hour_schedule_24=["00:00","00:30","01:00","01:30","02:00","02:30","03:00","03:30","04:00","04:30","05:00","05:30","06:00","06:30","07:00","07:30","08:00","08:30","09:00","09:30","10:00","10:30","11:00","11:30","12:00","12:30","13:00","13:30","14:00","14:30","15:00","15:30","16:00","16:30","17:00","17:30","18:00","18:30","19:00","19:30","20:00","20:30","21:00","21:30","22:00","22:30","23:00","23:30"];

var week_days = ["Sun.", "Mon.", "Tues.", "Wed.", "Thu.", "Fri.", "Sat."];

function check_schedule_onoff()
{
	var cf = document.forms[0];
	var i = 0;
	var disable_flag, checked_flag;

	//bgn
	if(cf.enable_ap.checked==false || ntp_updated=="0")
	{
       disable_flag = true;
	   getObj('schedule_bgn_enable').color = 'gray';
	}
	else
	{
		disable_flag = false;
		getObj('schedule_bgn_enable').color = 'black';
    }
	cf.wirless_schedule.disabled = disable_flag;
    cf.add_period.disabled = disable_flag;
	cf.edit_period.disabled = disable_flag;
	cf.delete_period.disabled = disable_flag;
	

	if(wireless_schedule_num > 1)
	{
		for(i=0; i<wireless_schedule_num; i++)
		{
			cf.wireless_schedule_item[i].disabled = disable_flag;			
		}
	}
	else if(wireless_schedule_num >0)
	{
		cf.wireless_schedule_item.disabled = disable_flag;
	}
}

function enable_schedule_button()
{
	var cf = document.forms[0];
	
    if(cf.enable_ap.checked && ntp_updated=="1")
	{
	cf.wirless_schedule.disabled = false;
    cf.add_period.disabled = false;
	cf.edit_period.disabled = false;
	cf.delete_period.disabled = false;
	}	
	else
	{	
	cf.wirless_schedule.disabled = true;
    cf.add_period.disabled = true;
	cf.edit_period.disabled = true;
	cf.delete_period.disabled = true;	
	}
}

function check_wireless_schedule(chan_type, btn_flag)
{
	var cf = document.forms[0];

	if(chan_type == "bgn")
	{
		if(btn_flag == "add")
		{
			if(wireless_schedule_num >= 21)
			{
				alert("$adva_max_rules");
				return false;
			}
			else
				location.href="WLG_schedule_add.htm";//fix bug 28245 not a por-up page
		}
		else if(btn_flag == "edit")
		{
			if(wireless_schedule_num > 0)
                        {
                                var i = 0;
				cf.wladv_schedule_edit_num.value = "";
				
				if(wireless_schedule_num > 1)
				{
                                	for(i=0; i<wireless_schedule_num; i++)
                                	{
                                        	if(cf.wireless_schedule_item[i].checked)
                                                	cf.wladv_schedule_edit_num.value = cf.wireless_schedule_item[i].value;
                                	}
				}
				else
				{
					if(cf.wireless_schedule_item.checked)
						cf.wladv_schedule_edit_num.value = cf.wireless_schedule_item.value;
				}	

                                if(cf.wladv_schedule_edit_num.value == "")
                                {
                                        location.href="edit_fail.htm";
                                        return false;
                                }
                                else
                                {
                                        cf.wladv_schedule_type.value = "edit_bgn";
                                        cf.submit_flag.value = "wireless_schedule_select_to_edit";
					cf.action = "/apply.cgi?/WLG_schedule_edit.htm timestamp="+ts;
					cf.submit();
                                }
                        }
			else
                        {
                                location.href="edit_fail.htm";
                                return false;
                        }

		}
		else if(btn_flag == "delete")
                {
                        if(wireless_schedule_num > 0)
                        {
                                var i = 0;
				cf.wladv_schedule_delete_num.value = "";

				if(wireless_schedule_num > 1)
				{
                                	for(i=0; i<wireless_schedule_num; i++)
                                	{
                                        	if(cf.wireless_schedule_item[i].checked)
                                                	cf.wladv_schedule_delete_num.value = cf.wireless_schedule_item[i].value;
                                	}
				}
				else
				{
					if(cf.wireless_schedule_item.checked)
						cf.wladv_schedule_delete_num.value = cf.wireless_schedule_item.value;
				}

                                if(cf.wladv_schedule_delete_num.value == "")
                                {
                                        location.href="del_fail.htm";
                                        return false;
                                }
                                else
                                {
                                        cf.submit_flag.value = "wireless_schedule_delete";
                                        cf.action = "/apply.cgi?/WLG_adv.htm timestamp="+ts;
					cf.submit();
                                }
                        }
                        else
                        {
                                location.href="del_fail.htm";
                                return false;
                        }
                }		
	}
}

function schedule_check_pattern()
{
	var cf = document.forms[0];
	var onoff;

	if(cf.selday[0].checked)
		onoff = true;
	else if(cf.selday[1].checked)
		onoff = false;

	setDisabled(onoff, cf.sun, cf.mon, cf.tue, cf.wed, cf.thu, cf.fri, cf.sat); 
}

function check_schedule_legality(wireless_type, modify_flag, item_value)
{
	var array_name, array_num;
	if(wireless_type == "bgn")
	{
		array_name = "wireless_schedule_array";
		array_num = wireless_schedule_num;
	}

	var i;
	if(array_num > 0)
	{
		for(i=1; i<=array_num; i++)
		{
			var array_value = eval(array_name + i);
		
			if(array_value == item_value)
			{
				if(modify_flag == "add")
				{
					alert("$service_rule_dup");
					return false;
				}
				else if(modify_flag=="edit" && i!=edit_num)
				{
					alert("$service_rule_dup");
					return false;
				}
			}
		}
	}
}

function wireless_schedule_add(wireless_type, modify_flag)
{
	var cf = document.forms[0];
	var schedule_all = "", schedule_days = "";
	var i, j=0;	

	if(cf.selday[0].checked)
		schedule_days = "Daily"
	else if(cf.selday[1].checked)
	{
		var weekday;

		for(i=0; i<7; i++)
		{
			if(i == 0)
				weekday = cf.sun;
			else if(i == 1)
				weekday = cf.mon;
			else if(i == 2)
				weekday = cf.tue;
			else if(i == 3)
				weekday = cf.wed;
			else if(i == 4)
				weekday = cf.thu;
			else if(i == 5)
				weekday = cf.fri;
			else if(i == 6)
				weekday = cf.sat;
			
			if(weekday.checked)
				schedule_days = schedule_days + weekday.value + ",";
		}

		if(schedule_days.length > 0)
			schedule_days = schedule_days.substring(0, schedule_days.length-1);
		
		var selectday;/*used in for cycle,make the code easier*/
		for(i=0; i<7; i++)
		{
			if(i == 0)
				selectday = cf.sun;
			else if(i == 1)
				selectday = cf.mon;
			else if(i == 2)
				selectday = cf.tue;
			else if(i == 3)
				selectday = cf.wed;
			else if(i == 4)
				selectday = cf.thu;
			else if(i == 5)
				selectday = cf.fri;
			else if(i == 6)
				selectday = cf.sat;
		if(selectday.checked)
		 j=j+1;  /*j is the days selevted*/
		}
		
		if(j<=0)   /*estimate if there is no days selected*/
		{
		 alert("$invalid_noday");
		 return false;
		 }
	}

	if(cf.start_time.value == cf.end_time.value)
	{
		alert("$same_time");
		return false;
	}
	schedule_all = cf.start_time.value + "-" + cf.end_time.value + "-" + schedule_days;
	schedule_all = remove_space(schedule_all);

	if(check_schedule_legality(wireless_type, modify_flag, schedule_all) == false)
		return false;

	cf.schedule_info.value = schedule_all;
	return true;
}
