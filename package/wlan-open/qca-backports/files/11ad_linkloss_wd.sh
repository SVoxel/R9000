#!/bin/sh

PATH=/bin:/sbin:/usr/bin:/usr/sbin
CONFIG=/bin/config
WIG_ENABLED=$($CONFIG get endis_wig_radio)

# Grab disconnect reason code
reason1="NA"
reason2="NA"
reason3="NA"
reason4="NA"
reason5="NA"

alive_msg_interval=60

alive_msg_counter=0

recover_r9000_11ad() {
	echo "11ad watchdog start recovering 11ad..."

	for pid in `pidof hostapd`; do
		grep -E "phy0" /proc/$pid/cmdline >/dev/null && \
			kill $pid
	done
	ifconfig wlan0 down
	sleep 5
	ifconfig wlan0 up
	sleep 5
	hostapd -P /var/run/wifi-phy0.pid -B /var/run/hostapd-phy0.conf
}

# Run only if AD band enabled
if [ "$WIG_ENABLED" = "1" ]; then
	while [ TRUE ]
	do
		reason1=$(dmesg | grep "proto 1 wmi 4");
		reason2=$(dmesg | grep "proto 1 wmi 6");
		reason3=$(dmesg | grep "Unhandled event 0x180e");
		reason4=$(dmesg | grep "wlan0: Firmware error detected");
		reason5=$(dmesg -c | grep "proto 0 wmi 4");


		if [ "$reason1" != "" ]
		then
			echo "$reason1"
			recover_r9000_11ad
		elif [ "$reason2" != "" ]
		then
			echo "$reason2"
			recover_r9000_11ad
		elif [ "$reason3" != "" ]
		then
			echo "$reason3"
			recover_r9000_11ad
		elif [ "$reason4" != "" ]
		then
			echo "$reason4"
			sleep 10
			recover_r9000_11ad
		elif [ "$reason5" != "" ]
		then
			echo "$reason5"
			recover_r9000_11ad
		fi
		sleep 1

		alive_msg_counter=$((alive_msg_counter + 1))
		if [ $alive_msg_counter -eq $alive_msg_interval ]
		then
			echo "11ad link loss watch dog is alive"
			alive_msg_counter=0
		fi
	done
fi
