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

	if [ -f "var/run/hostapd-wifi0" ] && [ -f "/var/run/wifi-ath0.pid" ]; then
		driver_ssid=`hostapd_cli -p /var/run/hostapd-wifi0 -i ath0 GET ssid`
		dni_ssid=`cat /var/run/hostapd-ath0.conf | grep ssid | awk -F '=' '{print $2}'`
		if [ "$driver_ssid" != "$dni_ssid" ]; then
			wifi_reload=1
			echo "ath0 hostapd get wrong ssid, driver ssid: $driver_ssid, config ssid: $dni_ssid"
		fi
	fi

	if [ -f "var/run/hostapd-wifi1" ] && [ -f "/var/run/wifi-ath1.pid" ]; then
		driver_ssid=`hostapd_cli -p /var/run/hostapd-wifi1 -i ath1 GET ssid`
		dni_ssid=`cat /var/run/hostapd-ath1.conf | grep ssid | awk -F '=' '{print $2}'`
		if [ "$driver_ssid" != "$dni_ssid" ]; then
			wifi_reload=1
			echo "ath1 hostapd get wrong ssid, driver ssid: $driver_ssid, config ssid: $dni_ssid"
		fi
	fi

	if [ -f "var/run/hostapd-wifi0" ] && [ -f "/var/run/wifi-ath01.pid" ]; then
		driver_ssid=`hostapd_cli -p /var/run/hostapd-wifi0 -i ath01 GET ssid`
		dni_ssid=`cat /var/run/hostapd-ath01.conf | grep ssid | awk -F '=' '{print $2}'`
		if [ "$driver_ssid" != "$dni_ssid" ]; then
			wifi_reload=1
			echo "ath01 hostapd get wrong ssid, driver ssid: $driver_ssid, config ssid: $dni_ssid"
		fi
	fi

	if [ -f "var/run/hostapd-wifi1" ] && [ -f "/var/run/wifi-ath11.pid" ]; then
		driver_ssid=`hostapd_cli -p /var/run/hostapd-wifi1 -i ath11 GET ssid`
		dni_ssid=`cat /var/run/hostapd-ath11.conf | grep ssid | awk -F '=' '{print $2}'`
		if [ "$driver_ssid" != "$dni_ssid" ]; then
			wifi_reload=1
			echo "ath11 hostapd get wrong ssid, driver ssid: $driver_ssid, config ssid: $dni_ssid"
		fi
	fi
}

check_samba_daemon()
{
	local usb_num=`cat /tmp/usbdisknum`
	local nmbd=`ps -w|grep nmbd | grep -v grep 2>&-`
	local smbd=`ps -w|grep smbd | grep -v grep 2>&-`
	[ "x$usb_num" != "x0" -a "x`config get usb_enableNet`" = "x0" ] && [ "x$nmbd" = "x" -o "x$smbd" = "x" ]  && update_smb
}

wifi_reload=0
wifi0_int_counter=0
wifi1_int_counter=0

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

	if [ "$wifi_reload" = "1" ]; then
		wifi_reload=0
		collect_dump
		uci set wireless.qcawifi.module_reload=1	
		echo 1 > /tmp/wifi_down #1 means this wifi reload is triggered by wifi watchdog, not GUI
		wlan down
		wlan up
		echo 0 > /tmp/wifi_down
	fi

	#check samba
	check_samba_daemon
done

