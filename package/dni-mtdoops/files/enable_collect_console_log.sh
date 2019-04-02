#!/bin/sh

echo enable > /sys/devices/platform/serial8250/console

/sbin/basic_log.sh &
/sbin/console_log.sh &
/sbin/wireless_log_detail.sh &
/usr/sbin/11ad_fw_log_capture.sh &
/sbin/capture_packet.sh 
/sbin/debug_here_log.sh &
/sbin/thermal_log.sh &

touch /tmp/.radardetect_lock
