function select_lable(num)
{
        if(num==0)
                document.getElementById("streamboost_qos").className="label_click";
        else
                document.getElementById("streamboost_qos").className="label_unclick";
        if(num==1)
                document.getElementById("wmm").className="label_click";
        else
                document.getElementById("wmm").className="label_unclick";

        select_num=num;
	if(select_num==0){
		if(top.have_advanced_qos == "1")
			goto_formframe("QOS_wait.htm");
		else
			goto_formframe("QOS_dynamic.htm");
	}
	else// if(select_num==1)
		this.location.href="QOS_wmm.htm";
}

function wmmMain()
{
	location.href="QOS_wmm.htm";
}

function qos_advanced()
{
	if($$("#speedtest").get(0).disabled == false) {
		if(top.have_advanced_qos == "1")
			this.location.href = "QOS_basic_dynamic.htm";
		else
			this.location.href = "QOS_dynamic.htm";
		return;
	}

	var cf = document.forms[0];
	cf.hid_cancel_speedtest.value = "1";
	if(first_flag == "1"){
		cf.hid_trend_micro_enable.value = 0;
		cf.hid_first_flag.value = "0";
		parent.ookla_speedtest_flag = 0;
	}
	else if(parent.ookla_speedtest_flag == 1) {
		parent.ookla_speedtest_flag = 0;
	}
	if(top.have_advanced_qos == "1")
		check_confirm(cf, "QOS_basic_dynamic.htm", ts);
	else
		check_confirm(cf, "QOS_dynamic.htm", ts);
}

function qos_basic()
{
	location.href="QOS_basic.htm";
}

function format_version(vir)
{
	var head = vir.substring(0, 4);
	var middle = vir.substring(4, 8);
	var tail = vir.substring(15);
	return parseInt(head) % 2013 + "." + middle + "." + tail;
}

function format_time(time)
{
	var year = time.substring(0, 4);
	var mouth = time.substring(4, 6);
	var day = time.substring(6);
	var mon_eng = new Array("January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December");
	return mon_eng[parseInt(mouth)-1]+" "+day+", "+year;
}

function confirm_dialox()
{
	var cf=document.forms[0];
	//if(cf.help_improve.checked == true && cf.streamboostEnable.checked == true) {
		window.open('QOS_improve_service.htm','newwindow','resizable=no,scrollbars=no,toolbar=no,menubar=no,status=no,location=no,alwaysRaised=yes,z-look=yes,width=800,height=600,left=200,top=100').focus();
	//}
}

function setSpeed(num)
{
	if(num == "2") {
		sAlert("$set_bandwidth_warning");
		document.getElementById("define_radio1").style.display = "";
		document.getElementById("define_radio2").style.display = "";
		document.getElementById("speedtest_content").style.display = "none";
	} else {
		document.getElementById("define_radio1").style.display = "none";
		document.getElementById("define_radio2").style.display = "none";
		document.getElementById("speedtest_content").style.display = ""
	}
}

function clearNoNum(obj)
{
	obj.value = obj.value.replace(/[^\d.]/g,"");
	obj.value = obj.value.replace(/^\./g,"");
	obj.value = obj.value.replace(/\.{2,}/g,".");
	obj.value = obj.value.replace(".","$#$").replace(/\./g,"").replace("$#$",".");
	obj.value = obj.value.replace(/^(\-)*(\d+)\.(\d\d)(\d).*$/,'$$1$$2.$$3');
}

function check_qos_apply(cf)
{
	var cf=document.forms[0];

	if(top.have_advanced_qos == 1)
		cf.hid_trend_micro_enable.value=1;
	else 
	{
		if(cf.dynamic_qos_enable.checked == true)
			cf.hid_trend_micro_enable.value=1;
		else
			cf.hid_trend_micro_enable.value=0;
		
	}

	if(cf.AutoUpdateEnable.checked == true)
		cf.hid_detect_database.value=1;
	else{
		cf.hid_detect_database.value=0;
		cf.hid_update_agreement.value="1";
	}

	if(cf.help_improve.checked == true)
		cf.hid_improve_service.value=1;
	else
		cf.hid_improve_service.value=0;

	if(cf.qosSetting[0].checked == true)
		cf.hid_bandwidth_type.value=0;
	else
		cf.hid_bandwidth_type.value=1;

	if(cf.AutoUpdateEnable.checked == true && (first_flag != "0" ||(first_flag == "0" && cf.qosSetting[0].checked == false)) && update_agreement == "1" && cf.hid_trend_micro_enable.value == "1")
	{
		sAlert("$share_mac_warn",function(){
			var cf=document.forms[0];
			cf.hid_update_agreement.value = "0";
                        check_qos_apply2();
		},
		function(){
			var cf=document.forms[0];
			cf.AutoUpdateEnable.checked = false;
			cf.hid_detect_database.value=0;
			cf.hid_update_agreement.value = "1";
			check_qos_apply2();

		});
	}
	else
		check_qos_apply2();
}

function check_qos_apply2()
{
	var cf=document.forms[0];

        if(cf.qosSetting[0].checked == true && cf.hid_trend_micro_enable.value == "1")
        {
                if(first_flag == "0") {
                        sAlert("$warning_bandwidth", function(){
				var cf=document.forms[0];
                                if(internet_status == "0"){
                                        sAlert("$internet_down");
                                        return false;
                                }
                                cf.hid_first_flag.value="1";
                                parent.ookla_speedtest_flag == 1;
				check_qos_apply3();
                        },
			function(){return false;});

                } else {
                        cf.hid_first_flag.value="2";
			check_qos_apply3();
                }
	}
	else
		check_qos_apply3();
}

function check_qos_apply3()
{
	var cf=document.forms[0];
	var trend_micro_uplink=parseFloat(cf.uplink_value.value).toFixed(2);
	var trend_micro_downlink=parseFloat(cf.downlink_value.value).toFixed(2);

	var invalid = 0;
	if(isNaN(trend_micro_downlink) || isNaN(trend_micro_uplink)) {
		invalid = 1;
	}
	else {
		cf.downlink_value.value = trend_micro_downlink;
		cf.uplink_value.value = trend_micro_uplink;
	}

	if(cf.qosSetting[1].checked == true) {
		if(internet_status == "0" && cf.hid_trend_micro_enable.value == "1"){
			sAlert("$internet_down");
			return false;
		}
		if(cf.uplink_value.value == "" || cf.downlink_value.value == "")
		{
			sAlert("$max_required");
			return false;
		}
		else if(trend_micro_uplink<0.10 || trend_micro_uplink>1000.00 || trend_micro_downlink<0.10 || trend_micro_downlink>1000.00 || invalid == 1)
		{
			sAlert("$range_error");
			return false;
		}
		else
		{
			cf.hid_trend_micro_uplink.value=parseInt((cf.uplink_value.value)*1000000/8);
			cf.hid_trend_micro_downlink.value=parseInt((cf.downlink_value.value)*1000000/8);
		}
		if(first_flag == "0")
			cf.hid_first_flag.value="0";
		else
			cf.hid_first_flag.value="2";
	}
	
	cf.submit();
}	

function check_wmm_apply(cf)
{
	if(cf.wmm_enable_2g.checked == true)
		cf.qos_endis_wmm.value=1;
	else
		cf.qos_endis_wmm.value=0;
	if(cf.wmm_enable_5g.checked == true)
		cf.qos_endis_wmm_a.value=1;
	else
		cf.qos_endis_wmm_a.value=0;
	cf.submit();
}

function check_confirm(cf, url)
{
	cf.hid_bandwidth_type.value=0;
	if(cf.uplink_value.value == "")
		cf.hid_trend_micro_uplink.value="";
	else
		cf.hid_trend_micro_uplink.value=parseInt((cf.uplink_value.value)*1000000/8);
	if(cf.downlink_value.value == "")
		cf.hid_trend_micro_downlink.value="";
	else
		cf.hid_trend_micro_downlink.value=parseInt((cf.downlink_value.value)*1000000/8);
	if(cf.AutoUpdateEnable.checked == true)
		cf.hid_detect_database.value=1;
	else
		cf.hid_detect_database.value=0;
	if(cf.help_improve.checked == true)
		cf.hid_improve_service.value=1;
	else
		cf.hid_improve_service.value=0;
	cf.submit_flag.value="apply_trend_micro";
	cf.action="/apply.cgi?/" + url + " timestamp=" + ts;
	cf.submit();
}

function check_basic_ookla_speedtest(form)
{
	if(internet_status == "0"){
		sAlert("$internet_down");
		return false;
	}
	parent.ookla_speedtest_flag = 1;
	form.submit_flag.value="ookla_speedtest";
	form.action="/func.cgi?/QOS_basic.htm timestamp="+ts;
	form.submit();
	return true;
}

function check_ookla_speedtest(form)
{
	if(internet_status == "0" && location.href.indexOf("QOS_dynamic.htm") > 0){
		getObj("indicate").innerHTML="$speedtest_connect";
		getObj("indicate").style.color="red";
		return false;
	}
	if(location.href.indexOf("QOS_dynamic.htm") > 0){
		form.submit_flag.value="ookla_speedtest";
		form.action="/func.cgi?/QOS_dynamic.htm timestamp="+ts;
		parent.ookla_speedtest_flag = 1;
	}
	else if(location.href.indexOf("QOS_basic_setting.htm") > 0){
		if(internet_status == "0"){
			sAlert("$internet_down");
			return false;
		}
		form.submit_flag.value="basic_qos_ookla_speedtest";
		form.action="/func.cgi?/QOS_basic_setting.htm timestamp="+ts;
		parent.basic_qos_ookla_speedtest_flag = 1;
	}
	form.submit();
	return true;
}

function check_basic_manual_update(form)
{
	if(internet_status == "0"){
		sAlert("$internet_down");
		return false;
	}
	form.submit_flag.value="detect_update";
	form.action="/apply.cgi?/QOS_basic.htm timestamp="+ts;
	form.submit();
	return true;
}

function check_manual_update(form)
{
	if(internet_status == "0"){
		sAlert("$internet_down");
		return false;
	}
	form.submit_flag.value="detect_update";
	form.action="/apply.cgi?/QOS_dynamic.htm timestamp="+ts;
	form.submit();
	return true;
}

function device_icon(type_name)
{
	if(type_name < 1 || type_name > 51)
		type_name = 47;
	return "<img src=image/streamboost/"+type_name+".jpg width=66px height=44px id=icon_img />";
}

function edit_select_device(mac,ip,name,priority,devtype,contype,onOff)
{
	var cf = document.forms[0];

	if( mac ) parent.qos_edit_mac=mac;
	if( ip ) parent.qos_edit_ip=ip;
	if( name ) parent.qos_edit_name=name;
	if( priority ) parent.qos_edit_priority=priority;
	if( devtype )parent.qos_edit_devtype=devtype;
	if( contype )parent.qos_edit_contype=contype;
	if( onOff )parent.qos_edit_onOff=onOff;
	
	location.href = "QOS_edit_devices.htm";
}

function select_device(mac,ip,name,priority, devtype, contype)
{
	parent.qos_edit_mac=mac;
	parent.qos_edit_ip=ip;
	parent.qos_edit_name=name;
	parent.qos_edit_priority=priority;
	parent.qos_edit_devtype=devtype;
	parent.qos_edit_contype=contype;

	if(top.is_ru_version == 1)
		document.getElementsByName("edit")[0].className="common_bt";
	else
		document.getElementsByName("edit")[0].className="short_common_bt";
	document.getElementsByName("edit")[0].disabled=false;
}

function show_bora(type)
{

	var device_bora="";
	if(enable_block_device == "1")
	{
		if(type=="Allowed")
			device_bora="<b style='color:#5bb6e5'>$acc_allow</b>";
		else if(type=="Blocked")
			device_bora="<b style='color:#ff1300'>$acc_block</b>";
		else
			device_bora="<b style='color:#ff1300'>$acc_block</b>";
	}
	return device_bora;
}

function show_icon_name(num)
{
	var device_icon_name="$qos_device47";
	if(num=="1")
		device_icon_name="$qos_device1";
	else if(num=="2")
		device_icon_name="$qos_device2";
	else if(num=="3")
		device_icon_name="$qos_device3";
	else if(num=="4")
		device_icon_name="$qos_device4";
	else if(num=="5")
		device_icon_name="$qos_device5";
	else if(num=="6")
		device_icon_name="$qos_device6";
	else if(num=="7")
		device_icon_name="$qos_device7";
	else if(num=="8")
		device_icon_name="$qos_device8";
	else if(num=="9")
		device_icon_name="$qos_device9";
	else if(num=="10")
		device_icon_name="$qos_device10";
	else if(num=="11")
		device_icon_name="$qos_device11";
	else if(num=="12")
		device_icon_name="$qos_device12";
	else if(num=="13")
		device_icon_name="$qos_device13";
	else if(num=="14")
		device_icon_name="$qos_device14";
	else if(num=="15")
		device_icon_name="$qos_device15";
	else if(num=="16")
		device_icon_name="$qos_device16";
	else if(num=="17")
		device_icon_name="$qos_device17";
	else if(num=="18")
		device_icon_name="$qos_device18";
	else if(num=="19")
		device_icon_name="$qos_device19";
	else if(num=="20")
		device_icon_name="$qos_device20";
	else if(num=="21")
		device_icon_name="$qos_device21";
	else if(num=="22")
		device_icon_name="$qos_device22";
	else if(num=="23")
		device_icon_name="$qos_device23";
	else if(num=="24")
		device_icon_name="$qos_device24";
	else if(num=="25")
		device_icon_name="$qos_device25";
	else if(num=="26")
		device_icon_name="$qos_device26";
	else if(num=="27")
		device_icon_name="$qos_device27";
	else if(num=="28")
		device_icon_name="$qos_device28";
	else if(num=="29")
		device_icon_name="$qos_device29";
	else if(num=="30")
		device_icon_name="$qos_device30";
	else if(num=="31")
		device_icon_name="$qos_device31";
	else if(num=="32")
		device_icon_name="$qos_device32";
	else if(num=="33")
		device_icon_name="$qos_device33";
	else if(num=="34")
		device_icon_name="$qos_device34";
	else if(num=="35")
		device_icon_name="$qos_device35";
	else if(num=="36")
		device_icon_name="$qos_device36";
	else if(num=="37")
		device_icon_name="$qos_device37";
	else if(num=="38")
		device_icon_name="$qos_device38";
	else if(num=="39")
		device_icon_name="$qos_device39";
	else if(num=="40")
		device_icon_name="$qos_device40";
	else if(num=="41")
		device_icon_name="$qos_device41";
	else if(num=="42")
		device_icon_name="$qos_device42";
	else if(num=="43")
		device_icon_name="$qos_device43";
	else if(num=="44")
		device_icon_name="$qos_device44";
	else if(num=="45")
		device_icon_name="$qos_device45";
	else if(num=="46")
		device_icon_name="$qos_device46";
	else if(num=="47")
		device_icon_name="$qos_device47";
	else if(num=="48")
		device_icon_name="$qos_device48";
	else if(num=="49")
		device_icon_name="$qos_device49";
	else if(num=="50")
                device_icon_name="$qos_device50";
	else if(num=="51")
                device_icon_name="$qos_device51";
	else
		device_icon_name="$qos_device47";

	return device_icon_name;
}

function show_type_name(name)
{
	var device_type="";
	if(name=="wired")
		device_type="$acc_wired";
	else if(name=="primary")
		device_type="2.4G $wireless";
	else if(name=="guest")
		device_type="2.4G $guest_wireless";
	else if(name=="gre")
		device_type="2.4G $guest_wireless";
	else if(name=="primary_an")
		device_type="5G $wireless";
	else if(name=="guest_an")
		device_type="5G $guest_wireless";
	else if(name=="gre_an")
		device_type="5G $guest_wireless";
	else if(name=="vpn")
		device_type="$qos_vpn";
	else if(name=="primary_ad")
		device_type="60G $wireless";
	else
		device_type="$acc_wired";

	return device_type;
}

function sort_bandwidth(a, b)
{
	return (a.down_rate < b.down_rate)? 1 : -1;
}

function sort_alp(a, b)
{
	return (a.name.toLowerCase() > b.name.toLowerCase()) ? 1: -1;
}

function sort_con(a, b)
{
	return (a.contype < b.contype) ? 1: -1;
}

function sort_on_off(a, b)
{
	return (a.isOnOff < b.isOnOff)? 1 : -1;
}

function show_type2(name)
{
	var device_type="";
	if(name=="wired")
		device_type="<img src=image/eth.gif />";
	else if(name=="primary")
		device_type="<img src=image/wifi.png /><b class='short'>2.4G</b>";
	else if(name=="guest")
		device_type="<img src=image/wifi.png /><b class='long'>2.4G Guest</b>";
	else if(name=="gre")
		device_type="<img src=image/wifi.png /><b class='long'>2.4G Guest</b>";
	else if(name=="primary_an")
		device_type="<img src=image/wifi.png /><b class='short'>5G</b>";
	else if(name=="guest_an")
		device_type="<img src=image/wifi.png /><b class='long'>5G Guest</b>";
	else if(name=="gre_an")
		device_type="<img src=image/wifi.png /><b class='long'>5G Guest</b>";
	else if(name=="vpn")
		device_type="<img src=image/vpn.gif />";
	else if(name=="primary_ad")
		device_type="<img src=image/wifi.png /><b class='short'>60G</b>";
	else
		device_type="<img src=image/eth.gif />";

	return device_type;
}

function show_type(name)
{
	var device_type="";
	if(name=="wired")
		device_type="<div class='eth'></div>";
	else if(name=="primary")
		device_type="<div class='wifi'>2.4G</div>";
	else if(name=="guest")
		device_type="<div class='wifi'>2.4G Guest</div>";
	else if(name=="gre")
		device_type="<div class='wifi'>2.4G Guest</div>";
	else if(name=="primary_an")
		device_type="<div class='wifi'>5G</div>";
	else if(name=="guest_an")
		device_type="<div class='wifi'>5G Guest</div>";
	else if(name=="gre_an")
		device_type="<div class='wifi'>5G Guest</div>";
	else if(name=="vpn")
		device_type="<div class='contype'></div>";
	else if(name=="primary_ad")
		device_type="<div class='wifi'>60G</div>";
	else
		device_type="<div class='eth'></div>";

	return device_type;
}
function show_priority(pri)
{
	var device_priority="";
	if(pri=="HIGHEST")
		device_priority="$qos_highest";
	else if(pri=="HIGH")
		device_priority="$qos_high";
	else if(pri=="MEDIUM")
		device_priority="$medium_mark";
	else if(pri=="LOW")
		device_priority="$qos_low";
	else
		device_priority="$medium_mark";

	return device_priority;
}

function show_pri_num(pri)
{
	var device_num="2";
	if(pri=="HIGHEST")
		device_num="4";
	else if(pri=="HIGH")
		device_num="3";
	else if(pri=="MEDIUM")
		device_num="2";
	else if(pri=="LOW")
		device_num="1";
	else
		device_num="2";

	return device_num;
}

function TSorter(num){
        var table = Object;
        var trs = Array;
        var ths = Array;
        var curSortCol = Object;
        var prevSortCol = num;
        var sortType = Object;

        function get(){}

        function getCell(index){
                return trs[index].cells[curSortCol]
        }

        this.init = function(tableName)
        {
                table = document.getElementById(tableName);
                ths = table.getElementsByTagName("th");
                for(var i = 1; i < ths.length ; i++)
                {
                        ths[i].onclick = function()
                        {
                                sort(this);
                        }
                }
                return true;
        };

	this.def_sort = function(tableName, defNum)
	{
		table = document.getElementById(tableName);
		ths = table.getElementsByTagName("th");
		sort(ths[defNum]);
		return true;
	};

        function sort(oTH)
        {
                curSortCol = oTH.cellIndex;
                sortType = oTH.abbr;
                trs = table.tBodies[0].getElementsByTagName("tr");

                setGet(sortType)

                for(var j=0; j<trs.length; j++)
                {
                        if(trs[j].className == 'detail_row')
                        {
                                closeDetails(j+2);
                        }
                }

                if(prevSortCol == curSortCol)
                {
                        oTH.className = (oTH.className != 'descend' ? 'descend' : 'ascend' );
			reverseTable();
                }
                else
                {
                        oTH.className = 'descend';
                        if(ths[prevSortCol].className != 'exc_cell'){ths[prevSortCol].className = '';}
                        quicksort(0, trs.length);
                }
                prevSortCol = curSortCol;
        }

        function setGet(sortType)
        {
                switch(sortType)
                {
                        case "float_text":
                                get = function(index){
                                        return parseFloat(getCell(index).firstChild.value);
                                };
                                break;
                        case "str_text":
                                get = function(index){
                                        return getCell(index).firstChild.value;
                                };
                                break;
                        case "ip_text":
                                get = function(index) {
                                        var value = getCell(index).firstChild.nodeValue;
                                        var each_info = value.split(".");
                                        split_part = parseInt(each_info[0]) + parseInt(each_info[1]) + parseInt(each_info[2]) + parseInt(each_info[3], 10);
                                        return parseInt(split_part);
                                }
                                break;
                        default:
                                get = function(index){  return getCell(index).firstChild.nodeValue;};
                                break;
                };
        }

	function exchange(i, j)
        {
                if(i == j+1) {
                        table.tBodies[0].insertBefore(trs[i], trs[j]);
                } else if(j == i+1) {
                        table.tBodies[0].insertBefore(trs[j], trs[i]);
                } else {
                        var tmpNode = table.tBodies[0].replaceChild(trs[i], trs[j]);
                        if(typeof(trs[i]) == "undefined") {
                                table.appendChild(tmpNode);
                        } else {
                                table.tBodies[0].insertBefore(tmpNode, trs[i]);
                        }
                }
        }

        function reverseTable()
        {
                for(var i = 1; i<trs.length; i++)
                {
                        table.tBodies[0].insertBefore(trs[i], trs[0]);
                }
        }

	function quicksort(lo, hi)
        {
                if(hi <= lo+1) return;

                if((hi - lo) == 2) {
                        if(get(hi-1) > get(lo)) exchange(hi-1, lo);
                        return;
                }

                var i = lo + 1;
                var j = hi - 1;

                if(get(lo) > get(i)) exchange(i, lo);
                if(get(j) > get(lo)) exchange(lo, j);
                if(get(lo) > get(i)) exchange(i, lo);

                var pivot = get(lo);

                while(true) {
                        j--;
                        while(pivot > get(j)) j--;
                        i++;
                        while(get(i) > pivot) i++;
                        if(j <= i) break;
                        exchange(i, j);
                }
                exchange(lo, j);

                if((j-lo) < (hi-j)) {
                        quicksort(lo, j);
                        quicksort(j+1, hi);
                } else {
                        quicksort(j+1, hi);
                        quicksort(lo, j);
                }
        }
}

function show_or_hid_refresh(cf, tag)
{
	if(tag == 0)
	{
		if(cf.enable_auto_refresh.checked)
			cf.hid_dev_auto_refresh.value = "1";
		else	
			cf.hid_dev_auto_refresh.value="0";
			
		cf.action="/apply.cgi?/QOS_show_device.htm timestamp="+ts;
		cf.submit_flag.value="auto_refresh_value";
		cf.submit();
	}
	else
	{
		if(cf.enable_auto_refresh.checked)
			cf.hid_ap_auto_refresh.value = "1";
		else
			cf.hid_ap_auto_refresh.value="0";

		cf.submit();
	}
}

var type_list = {"1": ["Amazon Kindle", "20", "1"], "2": ["Android Device", "20", "2"], "3": ["Android Phone", "30", "3"], "4": ["Android Tablet", "20", "4"], "5": ["Apple Airport Express", "20", "5"], "6": ["Blu-ray player", "10", "6"], "7": ["Bridge", "20", "7"], "8": ["Cable STB", "10", "8"], "9": ["Camera", "30", "9"], "10": ["Router", "20", "10"], "11": ["DVR", "10", "11"], "12": ["Gaming Console", "10", "12"], "13": ["iMac", "20", "13"], "14": ["iPad", "20", "14"], "15": ["iPad mini", "20", "15"], "16": ["iPhone 5/5S/5C", "30 ", "16"], "17": ["iPhone", "30", "17"], "18": ["iPod Touch", "30", "18"], "19": ["Linux PC", "20", "19"], "20": ["Mac mini", "20", "20"], "21": ["Mac Pro", "20", "21"], "22": ["Mac Book", "20", "22"], "23": ["Media Device", "10", "23"], "24": ["Network Device", "30", "24"], "25": ["Other STB", "10", "25"], "26": ["Powerline", "20", "26"], "27": ["Printer", "30", "27"], "28": ["Repeater", "20", "28"], "29": ["Satellite STB", "10", "29"], "30": ["scanner", "30", "30"], "31": ["Sling box", "10", "31"], "32": ["Smart phone", "30", "32"], "33": ["Storage (NAS)", "40", "33"], "34": ["Switch", "20", "34"], "35": ["TV", "10", "35"], "36": ["Tablet", "20", "36"], "37": ["Unix PC", "20", "37"], "38": ["Windows PC", "20", "38"], "39": ["Surface", "20", "39"], "40": ["Wifi Extender", "20", "40"], "41": ["Apple TV", "10", "41"], "42": ["AV Receiver", "10", "42"], "43": ["Chromcast", "10", "43"], "44": ["Google Nexus 5", "30 ", "44"], "45": ["Google Nexus 7", "30", "45"], "46": ["Google Nexus 10", "20", "46"], "47": ["Other", "30", "47"], "48": ["WN1000RP", "20", "48"], "49": ["WN2500RP", "20", "49"], "50": ["VoIP", "10", "50"], "51": ["Iphone6", "30", "51"], "52": ["Arlo", "20", "52"], "53": ["Amazon Fire TV", "20", "53"], "54": ["Smart Watch", "20", "54"]};
