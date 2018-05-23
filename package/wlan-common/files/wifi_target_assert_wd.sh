#!/bin/sh

collect_dump()
{
#2.4G
athdiag --read --address=0x00400000 --length=0x00060000 --file=03.dram.dump --device=/sys/devices/soc.0/fd800000.pcie-external0/pci0001:00/0001:00:00.0/0001:01:00.0/0001:02:03.0/0001:03:00.0
athdiag --read --address=0x00980000 --length=0x00050000 --file=03.iram.dump --device=/sys/devices/soc.0/fd800000.pcie-external0/pci0001:00/0001:00:00.0/0001:01:00.0/0001:02:03.0/0001:03:00.0
athdiag --read --address=0x0004A000 --length=0x00005000 --file=03.ce.dump --device=/sys/devices/soc.0/fd800000.pcie-external0/pci0001:00/0001:00:00.0/0001:01:00.0/0001:02:03.0/0001:03:00.0
athdiag --read --address=0x00030000 --length=0x00007000 --file=03.apb_reg.dump --device=/sys/devices/soc.0/fd800000.pcie-external0/pci0001:00/0001:00:00.0/0001:01:00.0/0001:02:03.0/0001:03:00.0
athdiag --read --address=0x0003f000 --length=0x00003000 --file=03.apb_reg2.dump --device=/sys/devices/soc.0/fd800000.pcie-external0/pci0001:00/0001:00:00.0/0001:01:00.0/0001:02:03.0/0001:03:00.0
athdiag --read --address=0x00043000 --length=0x00003000 --file=03.wifi_reg.dump --device=/sys/devices/soc.0/fd800000.pcie-external0/pci0001:00/0001:00:00.0/0001:01:00.0/0001:02:03.0/0001:03:00.0
athdiag --read --address=0x00080000 --length=0x00006000 --file=03.soc_reg.dump --device=/sys/devices/soc.0/fd800000.pcie-external0/pci0001:00/0001:00:00.0/0001:01:00.0/0001:02:03.0/0001:03:00.0

#5G
athdiag --read --address=0x00400000 --length=0x00060000 --file=04.dram.dump --device=/sys/devices/soc.0/fd800000.pcie-external0/pci0001:00/0001:00:00.0/0001:01:00.0/0001:02:07.0/0001:04:00.0
athdiag --read --address=0x00980000 --length=0x00050000 --file=04.iram.dump --device=/sys/devices/soc.0/fd800000.pcie-external0/pci0001:00/0001:00:00.0/0001:01:00.0/0001:02:07.0/0001:04:00.0
athdiag --read --address=0x0004A000 --length=0x00005000 --file=04.ce.dump --device=/sys/devices/soc.0/fd800000.pcie-external0/pci0001:00/0001:00:00.0/0001:01:00.0/0001:02:07.0/0001:04:00.0
athdiag --read --address=0x00030000 --length=0x00007000 --file=04.apb_reg.dump --device=/sys/devices/soc.0/fd800000.pcie-external0/pci0001:00/0001:00:00.0/0001:01:00.0/0001:02:07.0/0001:04:00.0
athdiag --read --address=0x0003f000 --length=0x00003000 --file=04.apb_reg2.dump --device=/sys/devices/soc.0/fd800000.pcie-external0/pci0001:00/0001:00:00.0/0001:01:00.0/0001:02:07.0/0001:04:00.0
athdiag --read --address=0x00043000 --length=0x00003000 --file=04.wifi_reg.dump --device=/sys/devices/soc.0/fd800000.pcie-external0/pci0001:00/0001:00:00.0/0001:01:00.0/0001:02:07.0/0001:04:00.0
athdiag --read --address=0x00080000 --length=0x00006000 --file=04.soc_reg.dump --device=/sys/devices/soc.0/fd800000.pcie-external0/pci0001:00/0001:00:00.0/0001:01:00.0/0001:02:07.0/0001:04:00.0

sleep 3
zip dump.zip *.dump
}

check_wifi_target_assert()
{
	local module_reload=`cat /proc/athversion`
	[ "$module_reload" = "9" ] && wifi_reload=1 && echo "wifi target assert, reload wifi driver"
}

check_wifi_interrupts()
{
	### wifi0 use CPU4 only ###
	local wifi0_int_tmp=`cat /proc/interrupts | grep wifi0 | awk '{print $5}'`

	### wifi1 will use CPU3 and CPU4 ### 
	local wifi1_int1_tmp=`cat /proc/interrupts | grep wifi1 | awk '{print $4}'`
	local wifi1_int2_tmp=`cat /proc/interrupts | grep wifi1 | awk '{print $5}'`
	local wifi1_int_tmp=`expr $wifi1_int1_tmp + $wifi1_int2_tmp`

	if [ "x$wifi0_int_tmp" = "x$wifi0_int_counter" ]; then
		echo "wifi0 interrupt counter stop, reload wifi driver"
		echo "previous: $wifi0_int_tmp, current: $wifi0_int_counter"
		wifi_reload=1	
	fi

	if [ "x$wifi1_int_tmp" = "x$wifi1_int_counter" ]; then
		echo "wifi1 interrupt counter stop, reload wifi driver"
		echo "previous: $wifi1_int_tmp, current: $wifi1_int_counter"
		wifi_reload=1	
	fi
	
	wifi0_int_counter=$wifi0_int_tmp
	wifi1_int_counter=$wifi1_int_tmp
	
}

check_hostapd_ath()
{
        local wifidev=$1
        local athdev=$2
	local driver_ssid=0
	local dni_ssid=0

	if [ -f "/var/run/hostapd-${wifidev}" ] && [ -f "/var/run/wifi-${athdev}.pid" ]; then
		driver_ssid=`hostapd_cli -p /var/run/hostapd-${wifidev} -i ${athdev} GET ssid`
		dni_ssid=`cat /var/run/hostapd-${athdev}.conf | grep ssid | awk -F '=' '{print $2}' | head -1`
		if [ "$driver_ssid" != "$dni_ssid" ]; then
			wifi_reload=1
			echo "${athdev} hostapd get wrong ssid, driver ssid: $driver_ssid, config ssid: $dni_ssid"
		fi
	fi
}

check_hostapd(){
        local vaps=$1
        for t_vap in $vaps
        do
                if [ "$t_vap" = "ath0" -o "$t_vap" = "ath01" ]; then
                        check_hostapd_ath $t_vap wifi0
                else
                        check_hostapd_ath $t_vap wifi1
                fi
        done
}

check_hostapd_connect()
{
	local athdev=$1
	local wifidev=$2
	
	hostapd_cli -i "$athdev" -p /var/run/hostapd-${wifidev} all_sta > /tmp/${wifidev}_${athdev}_all_sta
	hostapdWPAPTKState=`cat /tmp/${wifidev}_${athdev}_all_sta |grep hostapdWPAPTKState|awk -F '=' '{print $2}'`
	[ "$hostapdWPAPTKState" = "11" ] && hostapdWPAPTKState_flag=1
}

check_hostapd_EAP_common()
{
	local athdev=$1
	local wifidev=$2
	local hostap_is_run=
	
	hostap_is_run=`ps -www|grep hostapd |grep $athdev.conf`
	
	if [ "x$hostap_is_run" = "x" ]; then
		echo "start hostapd for ${wifidev}-${athdev} ......."
		hostapd -P /var/run/wifi-${athdev}.pid -B /var/run/hostapd-${athdev}.conf -e /var/run/entropy-${athdev}.bin
                sleep 3
                hostapd_cli -i ${athdev} -P /var/run/hostapd_cli-${athdev}.pid -a /lib/wifi/wps-hostapd-update-uci -p /var/run/hostapd-${wifidev} -B
		### when restart hostap, also init the old static data.
		dot11RSNA4WayHandshakeFailures_${athdev}_old=0
		dot11RSNAEapReceived_${athdev}_old=0
		dot11RSNAEapFailedToSend_${athdev}_old=0
		### hostap disappear and restart, NO need to check EAP M1 this time
		return 0
	fi
	

	hostapd_cli -i "$athdev" -p /var/run/hostapd-${wifidev} mib > /tmp/${wifidev}_${athdev}_mibinfo 
	
	dot11RSNA4WayHandshakeFailures=`cat /tmp/${wifidev}_${athdev}_mibinfo |grep dot11RSNA4WayHandshakeFailures|awk -F '=' '{print $2}'`
	dot11RSNAEapReceived=`cat /tmp/${wifidev}_${athdev}_mibinfo |grep dot11RSNAEapReceived|awk -F '=' '{print $2}'`			
	dot11RSNAEapFailedToSend=`cat /tmp/${wifidev}_${athdev}_mibinfo |grep dot11RSNAEapFailedToSend|awk -F '=' '{print $2}'`			
	eval dot11RSNA4WayHandshakeFailures_old=\${dot11RSNA4WayHandshakeFailures_${athdev}_old}
	eval dot11RSNAEapReceived_old=\${dot11RSNAEapReceived_${athdev}_old}
	eval dot11RSNAEapFailedToSend_old=\${dot11RSNAEapFailedToSend_${athdev}_old}
			
	EapFailedToSendNum=`expr $dot11RSNAEapFailedToSend - $dot11RSNAEapFailedToSend_old`
	## In 2 minutes, failed to send eap m1 more than twice, need restart hostapd for ath device
	if [ $EapFailedToSendNum -ge 2 ]; then
		echo "EapFailedToSendNum > 2: As EAP-M1 failed to send, restart all hostapd"
		hostapd_reload=1
		dot11RSNAEapFailedToSend=0
	fi
	
	if [ $dot11RSNA4WayHandshakeFailures -gt $dot11RSNA4WayHandshakeFailures_old -a "x$dot11RSNAEapReceived" = "x$dot11RSNAEapReceived_old" ]; then
		echo "dot11RSNA4WayHandshakeFailures ++ : As EAP-M1 failed to send, restart all hostapd"
		hostapd_reload=1
		dot11RSNA4WayHandshakeFailures=0
		dot11RSNAEapReceived=0
	fi
	
	## update the data for next compare
	eval dot11RSNA4WayHandshakeFailures_${athdev}_old=$dot11RSNA4WayHandshakeFailures
	eval dot11RSNAEapReceived_${athdev}_old=$dot11RSNAEapReceived
	eval dot11RSNAEapFailedToSend_${athdev}_old=$dot11RSNAEapFailedToSend
}

check_hostapd_EAP()
{
        local vaps=$1
	### check time for 2 minus.
	hostapd_EAP_InterVal_cnt=$(($hostapd_EAP_InterVal_cnt+1))
	[ $hostapd_EAP_InterVal_cnt -lt $hostapd_EAP_InterVal ] && return 0

	hostapd_EAP_InterVal_cnt=0
	bridge_mode=`config get bridge_mode`
	[ "x$bridge_mode" = "x1" ] && return 0

	[ -z "$vaps" ] && return 0

	### check if any clients connected, if yes, skip EAP checking ###
	hostapdWPAPTKState_flag=0
	for t_vap in $vaps
	do
                if [ "$t_vap" = "ath0" -o "$t_vap" = "ath01" ]; then
	                check_hostapd_connect $t_vap wifi0
                else
	                check_hostapd_connect $t_vap wifi1
                fi
	done
	# If any vap has connected device, just return
	[ "$hostapdWPAPTKState_flag" = "1" ] && return 0

        for t_vap in $vaps
        do
                if [ "$t_vap" = "ath0" -o "$t_vap" = "ath01" ]; then
	                check_hostapd_EAP_common $t_vap wifi0
                else
	                check_hostapd_EAP_common $t_vap wifi1
                fi
        done
}

down_up_vap()
{
    ifname=$1
    no_assoc=`iwconfig $ifname | grep Not-Associated`
    if [ "$no_assoc" != "" ]; then
        ifconfig $ifname down
        ifconfig $ifname up
        echo "wifi_target_assert_wd: $ifname Not-Associated, up it"
    fi
}

### check vap not up
check_vap_not_up()
{
        local vaps=$1

        bridge_mode=`config get bridge_mode`
        [ "x$bridge_mode" = "x1" ] && return 0

        for t_vap in $vaps
        do
                down_up_vap $t_vap
        done
}

check_samba_daemon()
{
	local usb_num=`cat /tmp/usbdisknum`
	local nmbd=`ps -w|grep nmbd | grep -v grep 2>&-`
	local smbd=`ps -w|grep smbd | grep -v grep 2>&-`
	[ "x$usb_num" != "x0" -a "x`config get usb_enableNet`" = "x0" ] && [ "x$nmbd" = "x" -o "x$smbd" = "x" ]  && update_smb
}

check_qos_conf()
{
	local qosfile=`cat /TM/qos.conf`
	if [ -e /TM/qos.conf ]; then
		if [ "x$qosfile" = "x" ]; then
			echo "qos is empty, copy rom file to it"
			cp /rom/iQoS/R9000/TM/qos.conf /TM/qos.conf
		fi
	else
		echo "qos is not exist, copy rom file to it"
		cp /rom/iQoS/R9000/TM/qos.conf /TM/qos.conf
	fi
}

# Generate WPA/WPA2 vap list
check_eap_vap(){
	local vap_list=$1
	local m_vap=""

	for athdev in $vap_list
	do
		case $athdev in
			ath0) ath_pre="wla" ;;
			ath1) ath_pre="wl" ;;
			ath01) ath_pre="wla1" ;;
			ath11) ath_pre="wlg1" ;;
		esac

		### Get the encrypt from nvram ###
		sectype=`config get ${ath_pre}_sectype`
		[ "x$sectype" != "x3" -a "x$sectype" != "x4" -a "x$sectype" != "x5" -a "x$sectype" != "x6" ] && continue
		[ -f "/var/run/hostapd-${athdev}.conf" ] && encrypt_mode=`cat /var/run/hostapd-${athdev}.conf |grep wpa_key_mgmt|grep -e PSK -e EAP`
		if [ "x$encrypt_mode" != "x" ]; then		
			m_vap="$m_vap $athdev"	
		fi
	done
	eap_monitor_vap=$m_vap
}

check_user_operate_wifi(){
        local m_vap=""

        wifi1_enable=`config get endis_wl_radio`
        wifi0_enable=`config get endis_wla_radio`
        ath11_enable=`config get wlg1_endis_guestNet`
        ath01_enable=`config get wla1_endis_guestNet`
        hw_btn_state=$(config get wl_hw_btn_state)

        [ "$wifi0_enable" = "1" -a "$hw_btn_state" = "on" ] && m_vap="$m_vap ath0"
        [ "$wifi0_enable" = "1" -a "$ath01_enable" = "1" -a "$hw_btn_state" = "on" ] && m_vap="$m_vap ath01"
        [ "$wifi1_enable" = "1" -a "$hw_btn_state" = "on" ] && m_vap="$m_vap ath1"
        [ "$wifi1_enable" = "1" -a "$ath11_enable" = "1" -a "$hw_btn_state" = "on" ] && m_vap="$m_vap ath11"
        
        echo $m_vap
}

#should add check ipv6 type in the future
check_dhcp6c_process()
{
	ps w | grep dhcp6c | grep -v grep > /dev/null
	if [ $? -ne 0  ]; then
		let "recovery_num+=1"
		echo "dhcp6c process exit and try to recovery $recovery_num times ..."
		echo "ppp2-status value is `cat /tmp/ppp/ppp2-status`"
		/etc/net6conf/net6conf restart
	fi

}



wifi_reload=0
hostapd_reload=0
wifi0_int_counter=0
wifi1_int_counter=0
hostapd_EAP_InterVal_cnt=0
hostapd_EAP_InterVal=6 ### check if hostapd is running every 2 minute. ###

### define hostapdWPAPTKState_flag ###
hostapdWPAPTKState_flag=0

### define dot11RSNA4WayHandshakeFailures/dot11RSNAEapReceived for every athdev 
dot11RSNA4WayHandshakeFailures_ath0_old=0
dot11RSNA4WayHandshakeFailures_ath01_old=0
dot11RSNA4WayHandshakeFailures_ath1_old=0
dot11RSNA4WayHandshakeFailures_ath11_old=0
dot11RSNAEapReceived_ath0_old=0
dot11RSNAEapReceived_ath01_old=0
dot11RSNAEapReceived_ath1_old=0
dot11RSNAEapReceived_ath11_old=0
dot11RSNAEapFailedToSend_ath0_old=0
dot11RSNAEapFailedToSend_ath01_old=0
dot11RSNAEapFailedToSend_ath1_old=0
dot11RSNAEapFailedToSend_ath11_old=0
monitor_vap=""
eap_monitor_vap=""

#recovery dhcp6c
recovery_num=0
ipv6_type="dhcp"

sleep 30

while [ TRUE ]
do
        sleep 20 

        #check samba
        check_samba_daemon

        #check qos conf
        check_qos_conf

        monitor_vap=$(check_user_operate_wifi) 
	# Generate vap list for hostapd EAP M1 wtd
	check_eap_vap "$monitor_vap"

        if [ "$(echo $monitor_vap | sed 's/ //g')" = "" ]; then
                continue
        fi

        echo 0 > /tmp/wifi_down

        ### check wifi interrupts ###
        check_wifi_interrupts

        ### check wifi target assert ###
        check_wifi_target_assert

        ### check hostapd issue ###
        check_hostapd "$monitor_vap"

        ### check for no eap M1 send ###
        check_hostapd_EAP "$eap_monitor_vap"

        if [ "$wifi_reload" = "1" -o "$hostapd_reload" = "1" ]; then
		[ "$wifi_reload" = "1" ] && collect_dump
                wifi_reload=0
		hostapd_reload=0
                uci set wireless.qcawifi.module_reload=1	
                echo 1 > /tmp/wifi_down #1 means this wifi reload is triggered by wifi watchdog, not GUI
                iwpriv wifi0 pdev_reset 5
                sleep 3
                iwpriv wifi1 pdev_reset 5
                sleep 3
                wlan down
                wlan up
                echo 0 > /tmp/wifi_down
        fi

        ### check vap not up ###
        #check_vap_not_up "$monitor_vap"

	#check dhcp6c process
	if [ "x`config get ipv6_type`" = "xpppoe"  ]; then
		if [ "x`config get ipv6_sameinfo`" = "x1" ]; then
			ipaddr=`ifconfig ppp0 | grep "inet addr" | cut -f2 -d: | cut -f1 -d' '`
		else
			ipaddr=`ifconfig ppp1 | grep "inet addr" | cut -f2 -d: | cut -f1 -d' '`
		fi 
		if [ "x$ipaddr" != "x" ]; then
			check_dhcp6c_process  
		fi
	fi 

done

