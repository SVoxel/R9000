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

check_11ad_wd()
{
	local ad_script=`ps -www|grep 11ad_linkloss_wd.sh |grep -v grep 2>&-`
	if [ "x$ad_script" = "x" ]; then
		/sbin/11ad_linkloss_wd.sh &
	fi	
}

check_hostapd()
{
	local driver_ssid=0
	local dni_ssid=0

	if [ -f "/var/run/hostapd-wifi0" ] && [ -f "/var/run/wifi-ath0.pid" ]; then
		driver_ssid=`hostapd_cli -p /var/run/hostapd-wifi0 -i ath0 GET ssid`
		dni_ssid=`cat /var/run/hostapd-ath0.conf | grep ssid | awk -F '=' '{print $2}' | head -1`
		if [ "$driver_ssid" != "$dni_ssid" ]; then
			wifi_reload=1
			echo "ath0 hostapd get wrong ssid, driver ssid: $driver_ssid, config ssid: $dni_ssid"
		fi
	fi

	if [ -f "/var/run/hostapd-wifi1" ] && [ -f "/var/run/wifi-ath1.pid" ]; then
		driver_ssid=`hostapd_cli -p /var/run/hostapd-wifi1 -i ath1 GET ssid`
		dni_ssid=`cat /var/run/hostapd-ath1.conf | grep ssid | awk -F '=' '{print $2}' | head -1`
		if [ "$driver_ssid" != "$dni_ssid" ]; then
			wifi_reload=1
			echo "ath1 hostapd get wrong ssid, driver ssid: $driver_ssid, config ssid: $dni_ssid"
		fi
	fi

	if [ -f "var/run/hostapd-wifi0" ] && [ -f "/var/run/wifi-ath01.pid" ]; then
		driver_ssid=`hostapd_cli -p /var/run/hostapd-wifi0 -i ath01 GET ssid`
		dni_ssid=`cat /var/run/hostapd-ath01.conf | grep ssid | awk -F '=' '{print $2}' | head -1`
		if [ "$driver_ssid" != "$dni_ssid" ]; then
			wifi_reload=1
			echo "ath01 hostapd get wrong ssid, driver ssid: $driver_ssid, config ssid: $dni_ssid"
		fi
	fi

	if [ -f "/var/run/hostapd-wifi1" ] && [ -f "/var/run/wifi-ath11.pid" ]; then
		driver_ssid=`hostapd_cli -p /var/run/hostapd-wifi1 -i ath11 GET ssid`
		dni_ssid=`cat /var/run/hostapd-ath11.conf | grep ssid | awk -F '=' '{print $2}' | head -1`
		if [ "$driver_ssid" != "$dni_ssid" ]; then
			wifi_reload=1
			echo "ath11 hostapd get wrong ssid, driver ssid: $driver_ssid, config ssid: $dni_ssid"
		fi
	fi
}

check_hostapd_EAP_common()
{
	local athdev=$1
	local wifidev=$2
	local wifi_enable=0
	local wl_guest_enable=0
	local is_guest_net=0
	local hostap_is_run=
	local enable=1
	
	[ "x$athdev" = "x" -o "x$wifidev" = "x" ] && return 0
	
	if [ $athdev = "ath0" -o $athdev = "ath01" ]; then
		ath_pre="wla"
		wifi_enable=`config get endis_wla_radio`
	elif [ $athdev = "ath1" -o $athdev = "ath11" ]; then
		ath_pre="wl"
		wifi_enable=`config get endis_wl_radio`
	fi
	
	### wifi disable, NO check.	
	[ "x$wifi_enable" != "x$enable" ] && return 0
	
	if [ $athdev = "ath11" ]; then
		ath_pre="wlg1"
		is_guest_net=1
		wl_guest_enable=`config get wlg1_endis_guestNet`		
	elif [ $athdev = "ath01" ]; then
		ath_pre="wla1"
		is_guest_net=1
		wl_guest_enable=`config get wla1_endis_guestNet`		
	fi
	[ $is_guest_net = $enable -a $wl_guest_enable != $enable ] && return 0

	### Get the encrypt from nvram ###
	sectype=`config get ${ath_pre}_sectype`
	[ "x$sectype" != "x3" -a "x$sectype" != "x4" -a "x$sectype" != "x5" -a "x$sectype" != "x6" ] && return 0
	[ -f "/var/run/hostapd-${athdev}.conf" ] && encrypt_mode=`cat /var/run/hostapd-${athdev}.conf |grep wpa_key_mgmt|grep -e PSK -e EAP`
	if [ "x$encrypt_mode" != "x" ]; then		
		hostap_is_run=`ps -www|grep hostapd |grep $athdev.conf`
		
		if [ "x$hostap_is_run" = "x" ]; then
			echo "start hostapd for ${wifidev}-${athdev} ......."
			hostapd -P /var/run/wifi-${athdev}.pid -B /var/run/hostapd-${athdev}.conf -e /var/run/entropy-${athdev}.bin
                        sleep 3
                        hostapd_cli -i ${athdev} -P /var/run/hostapd_cli-${athdev}.pid -a /lib/wifi/wps-hostapd-update-uci -p /var/run/hostapd-${wifidev} -B
			### when restart hostap, also init the old static data.
			dot11RSNA4WayHandshakeFailures_${athdev}_old=0
			dot11RSNAEapReceived_${athdev}_old=0
			### hostap disappear and restart, NO need to check EAP M1 this time
			return 0
		fi
		
		hostapd_cli -i "$athdev" -p /var/run/hostapd-${wifidev} mib > /tmp/${wifidev}_${athdev}_mibinfo 
		
		dot11RSNA4WayHandshakeFailures=`cat /tmp/${wifidev}_${athdev}_mibinfo |grep dot11RSNA4WayHandshakeFailures|awk -F '=' '{print $2}'`
		dot11RSNAEapReceived=`cat /tmp/${wifidev}_${athdev}_mibinfo |grep dot11RSNAEapReceived|awk -F '=' '{print $2}'`			
		eval dot11RSNA4WayHandshakeFailures_old=\${dot11RSNA4WayHandshakeFailures_${athdev}_old}
		eval dot11RSNAEapReceived_old=\${dot11RSNAEapReceived_${athdev}_old}
				
		## No EAP M1 send out, need restart hostapd for ath device
		if [ $dot11RSNA4WayHandshakeFailures -gt 0 -a "x$dot11RSNAEapReceived" = "x$dot11RSNAEapReceived_old" ]; then
			echo "As EAP-M1 no send, Restart hostapd for ${wifidev}-${athdev} ......."
			#### restart hostapd for this vap.
			for pid in `pidof hostapd`; do               
			grep -E "${athdev}" /proc/$pid/cmdline >/dev/null && \
				kill $pid                                    
			done
            		wlan vap "$wifidev" "$athdev"
			dot11RSNA4WayHandshakeFailures=0
			dot11RSNAEapReceived=0
		fi
		
		## update the data for next compare
		eval dot11RSNA4WayHandshakeFailures_${athdev}_old=$dot11RSNA4WayHandshakeFailures
		eval dot11RSNAEapReceived_${athdev}_old=$dot11RSNAEapReceived
	fi
}

check_hostapd_EAP()
{
	### check time for 2 minus.
	hostapd_EAP_InterVal_cnt=$(($hostapd_EAP_InterVal_cnt+1))
	[ $hostapd_EAP_InterVal_cnt -lt $hostapd_EAP_InterVal ] && return 0

	hostapd_EAP_InterVal_cnt=0
	bridge_mode=`config get bridge_mode`
	[ "x$bridge_mode" = "x1" ] && return 0

	check_hostapd_EAP_common ath0 wifi0
	check_hostapd_EAP_common ath01 wifi0
	check_hostapd_EAP_common ath1 wifi1
	check_hostapd_EAP_common ath11 wifi1
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

check_vap_not_up()
{
    ### check vap not up
	bridge_mode=`config get bridge_mode`
	[ "x$bridge_mode" = "x1" ] && return 0
 
    wifi1_enable=`config get endis_wl_radio`
    wifi0_enable=`config get endis_wla_radio`
    ath11_enable=`config get wlg1_endis_guestNet`
    ath01_enable=`config get wla1_endis_guestNet`
    if [ "$wifi1_enable" = "1" ]; then
        down_up_vap ath1
        [ "$ath11_enable" = "1" ] && down_up_vap ath11
    fi
    if [ "$wifi0_enable" = "1" ]; then
        down_up_vap ath0
        [ "$ath01_enable" = "1" ] && down_up_vap ath01
    fi
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

wifi_reload=0
wifi0_int_counter=0
wifi1_int_counter=0
hostapd_EAP_InterVal_cnt=0
hostapd_EAP_InterVal=6 ### check if hostapd is running every 2 minute. ###

### define dot11RSNA4WayHandshakeFailures/dot11RSNAEapReceived for every athdev 
dot11RSNA4WayHandshakeFailures_ath0_old=0
dot11RSNA4WayHandshakeFailures_ath01_old=0
dot11RSNA4WayHandshakeFailures_ath1_old=0
dot11RSNA4WayHandshakeFailures_ath11_old=0
dot11RSNAEapReceived_ath0_old=0
dot11RSNAEapReceived_ath01_old=0
dot11RSNAEapReceived_ath1_old=0
dot11RSNAEapReceived_ath11_old=0

while [ TRUE ]
do
	sleep 20 

	echo 0 > /tmp/wifi_down

	### check wifi interrupts ###
	check_wifi_interrupts

	### check 11ad watchdog ###
#	check_11ad_wd
	
	### check wifi target assert ###
	check_wifi_target_assert
	
	### check hostapd issue ###
	check_hostapd

	### check for no eap M1 send ###
	check_hostapd_EAP

	if [ "$wifi_reload" = "1" ]; then
		wifi_reload=0
		collect_dump
		uci set wireless.qcawifi.module_reload=1	
		echo 1 > /tmp/wifi_down #1 means this wifi reload is triggered by wifi watchdog, not GUI
		wlan down
		wlan up
		echo 0 > /tmp/wifi_down
	fi

    ### check vap not up ###
    check_vap_not_up

	#check samba
	check_samba_daemon

	#check qos conf
	check_qos_conf
done

