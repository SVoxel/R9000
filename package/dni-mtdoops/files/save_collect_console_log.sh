#!/bin/sh

find_mtd_part() {
	local PART="$(grep "\"$1\"" /proc/mtd | awk -F: '{print $1}')"
	local PREFIX=/dev/mtd

	PART="${PART##mtd}"
	[ -d /dev/mtd ] && PREFIX=/dev/mtd/
	echo "${PART:+$PREFIX$PART}"
}

echo "Save the collect log into debug-log.zip and upload to user"

#Disblae wireless debug log
iwpriv ath0 dbgLVL 0x100
iwpriv ath1 dbgLVL 0x100

module_name=`cat /module_name`

# Save the router config file
/bin/config backup /tmp/NETGEAR_$module_name.cfg

mtd_oops="$(find_mtd_part 'crashdump')"

/sbin/debug_save_panic_log $mtd_oops

#disable dynamic debug for 11ad
#[ -f /proc/sys/kernel/printk ] && echo '7' > /proc/sys/kernel/printk
#[ -f /sys/kernel/debug/dynamic_debug/control ] && echo 'module wil6210 -p' > /sys/kernel/debug/dynamic_debug/control

cd /tmp

# System will zipped all debug files into 1 zip file and save to client browser
# So a debug-log.zip file will includes
# (1) Console log
# (2) Basic debug information
# (3) router config file
# (4) LAN/WAN packet capture

#Disable the capture
killall tcpdump
killall tcpdump
killall basic_log.sh 
killall console_log.sh 
killall wireless_log_detail.sh  
killall 11ad_fw_log_capture.sh; killall wigig_logcollector
killall debug_here_log.sh
killall thermal_log.sh

echo close > /sys/devices/platform/serial8250/console

dd if=/dev/mtd_crashdump of=/tmp/panic_log.txt bs=131072 count=2
[ -f /tmp/panic_log.txt ] && unix2dos /tmp/panic_log.txt
#[ -f /tmp/Panic-log.txt ] && unix2dos /tmp/Panic-log.txt
[ -f /tmp/Console-log1.txt ] && unix2dos /tmp/Console-log1.txt
[ -f /tmp/Console-log2.txt ] && unix2dos /tmp/Console-log2.txt 
[ -f /tmp/basic_debug_log.txt ] && unix2dos /tmp/basic_debug_log.txt
[ -f /tmp/wirless_log1.txt ] && unix2dos /tmp/wireless_log1.txt
[ -f /tmp/wirless_log2.txt ] && unix2dos /tmp/wireless_log2.txt
[ -f /tmp/thermal-log1.txt ] && unix2dos /tmp/thermal-log1.txt
[ -f /tmp/thermal-log2.txt ] && unix2dos /tmp/thermal-log2.txt
[ -e /tmp/radardetect.log ] && RADARLOG=radardetect.log

[ -f /tmp/debug_here_log_1.txt ] && unix2dos /tmp/debug_here_log_1.txt
[ -f /tmp/debug_here_log_2.txt ] && unix2dos /tmp/debug_here_log_2.txt

collect_log=`cat /tmp/collect_debug`

if [ "x$collect_log" = "x1" ];then
	zip debug-log.zip  panic_log.txt Console-log1.txt Console-log2.txt basic_debug_log.txt lan.pcap wan.pcap wireless_log1.txt wireless_log2.txt debug_here_log_1.txt debug_here_log_2.txt thermal-log1.txt thermal-log2.txt $RADARLOG
else
	zip debug-log.zip NETGEAR_$module_name.cfg  panic_log.txt  Console-log1.txt Console-log2.txt basic_debug_log.txt lan.pcap wan.pcap wireless_log1.txt wireless_log2.txt debug_here_log_1.txt debug_here_log_2.txt thermal-log1.txt thermal-log2.txt $RADARLOG
fi

[ -f /tmp/hostapd.log ] && {
	for hostapd_log in `cat /tmp/hostapd.log`
	do
		unix2dos $hostapd_log
		zip debug-log.zip $hostapd_log
		[ $? -eq "0" ] && rm -f $hostapd_log
	done
	# Recover back to normal wireless
	wlan updateconf
	wlan down
	wlan up
}

logfile_11ad_num=$(ls /tmp/logFile_* | wc -l)
if [ "x$11ad_logfile_num" != "x0" ]; then
    logfiles_11ad=$(ls /tmp/logFile_* | xargs)
    zip debug-log.zip $logfiles_11ad /tmp/hostapd-phy0.log
    rm -rf /tmp/logFile_*
fi
#restore 11ad setting
/bin/config unset enable_11ad_hostapd_debug
#start up 11ad watchdog
/sbin/11ad_linkloss_wd.sh &
#disable 11ad dynamic debug
#echo "module wil6210" -p > /sys/kernel/debug/dynamic_debug/control # too many logs leads console to abnormal
#echo 7 > /proc/sys/kernel/printk
#restart 11ad
wigig updateconf
wigig down
wigig up

rm -f /tmp/hostapd.log
rm -f /tmp/.radardetect_lock

cd /tmp
rm -rf debug-usb debug_cpu debug_wlan debug_flash debug_mem debug_mirror_on debug_session NETGEAR_$module_name.cfg panic_log.txt Console-log1.txt Console-log2.txt basic_debug_log.txt lan.pcap wan.pcap wireless_log1.txt wireless_log2.txt debug_here_log_1.txt debug_here_log_2.txt thermal-log1.txt thermal-log2.txt $RADARLOG

echo 0 > /tmp/collect_debug
