#!/bin/sh

CPU_INFO=/tmp/debug_cpu
MEM_INFO=/tmp/debug_mem
FLASH_INFO=/tmp/debug_flash
SESSION_INFO=/tmp/debug_session

cpu_usage()
{
	USAGE1=`mpstat -P ALL | sed -n 4p | awk '{print 100-$12}'`
	echo -e CPU1: "${USAGE1}% \c" > $CPU_INFO

        USAGE2=`mpstat -P ALL | sed -n 5p | awk '{print 100-$12}'`
        echo -e CPU2: "${USAGE2}% \c" >> $CPU_INFO

        USAGE3=`mpstat -P ALL | sed -n 6p | awk '{print 100-$12}'`
        echo -e CPU3: "${USAGE3}% \c" >> $CPU_INFO

        USAGE4=`mpstat -P ALL | sed -n 7p | awk '{print 100-$12}'`
        echo -e CPU4: "${USAGE4}% \c" >> $CPU_INFO
}

mem_usage()
{
#	used_kb=`free | grep "Mem" | awk '{print $3}'`
	total_kb=`cat /proc/meminfo | head -1 | awk '{print $2}'`
	free_kb=`cat /proc/meminfo | head -2 | tail -1 | awk '{print $2}'`
	buffer_kb=`cat /proc/meminfo | head -3 | tail -1 | awk '{print $2}'`
	cached_kb=`cat /proc/meminfo | head -4 | tail -1 |awk '{print $2}'`
	used_kb=`expr ${total_kb} - ${free_kb} - ${buffer_kb} - ${cached_kb}`
	used_mb=`expr ${used_kb} / 1024`

	unused_kb=`free | grep "Mem" | awk '{print $4}'`
	unused_mb=`expr ${unused_kb} / 1024`

	total_kb=`free | grep "Mem" | awk '{print $2}'`
	total_mb=`expr ${total_kb} / 1024`

	echo "${used_mb}MB/${total_mb}MB" > $MEM_INFO
}

session_usage()
{
	used_session=`cat /proc/sys/net/ipv4/netfilter/ip_conntrack_count`
	total_session=`cat /proc/sys/net/ipv4/netfilter/ip_conntrack_max`
	
	echo "${used_session}/${total_session}" > $SESSION_INFO
}

flash_usage()
{
	echo "158MB/512MB" > $FLASH_INFO
}

dist_path=""
mnt_path="/mnt/"

check_usb_storage_folder()
{
	part_list="a b c d e f g"
	for i in $part_list; do
		[ "X$(df | grep /dev/sd"$i")" = "X" ] && continue
		#echo "sd$i"
		j=1
		while [ $j -le 20 ]; do
			tmp=`df | grep /dev/sd"$i""$j"`
			mnt_tmp=`ls $mnt_path | grep sd"$i""$j"`
			[ "X$tmp" = "X" -o "X$mnt_tmp" = "X" ] && j=$((j+1)) && continue

			dist_path="$mnt_path"sd"$i""$j"
			break;

			j=$((j+1))
		done
		[ "X$dist_path" != "X" ]  && break
	done
}

cpu_usage
mem_usage
session_usage
flash_usage
check_usb_storage_folder
if [ "X$dist_path" != "X" ]; then
	echo 1 > /tmp/debug-usb
else
	echo 0 > /tmp/debug-usb
fi
