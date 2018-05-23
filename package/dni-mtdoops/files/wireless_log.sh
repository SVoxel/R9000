#!/bin/sh

# Collect basi debug information

#Enable hostapd debug log
enable_hostapd_debug=`/bin/config get enable_hostapd_debug`
[ "$enable_hostapd_debug" == "1" ] && {
	uci set wireless.wlg.hostapd_debug_level=2
	uci set wireless.wla.hostapd_debug_level=2
	wlan down; wlan up
	sleep 30
}

#Connection issue debug
iwpriv ath0 dbgLVL 0x11C00180
iwpriv ath1 dbgLVL 0x11C00180
iwpriv ath01 dbgLVL 0x11C00180
iwpriv ath11 dbgLVL 0x11C00180

#Scan to get acs report, need to wait acs finished.
iwpriv ath1 acsreport 1
iwpriv ath0 acsreport 1
sleep 30
wifitool ath1 acsreport
wifitool ath0 acsreport

while [ 1 ]
do
	wlanconfig ath0 list
	wlanconfig ath1 list
	wlanconfig ath01 list
	wlanconfig ath11 list
	sleep 1
	athstats
	iwpriv ath0 txrx_fw_stats 3
	iwpriv ath0 txrx_fw_stats 6
	sleep 1
	iwpriv ath1 txrx_fw_stats 3
	iwpriv ath1 txrx_fw_stats 6
	sleep 1
	iwpriv ath01 txrx_fw_stats 3
	iwpriv ath01 txrx_fw_stats 6
	sleep 1
	iwpriv ath11 txrx_fw_stats 3
	iwpriv ath11 txrx_fw_stats 6
	sleep 1
	iwconfig
	sleep 1
	ps | grep hostap
	sleep 60
		
done

