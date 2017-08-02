#!/bin/sh

drives_info_path="/tmp/usb_test_disk"

drives_info(){
	rm -rf /tmp/usb_device_test
	rm -rf /tmp/usb_test_disk
	ls -l /sys/block | grep sd |awk '{print $9}' > /tmp/usb_device_test

	while read line
	do
		i=1
		par_num=`cat /proc/partitions | grep $line | awk '{print $4}'| egrep sd[a-z] |grep $line |wc -l`
		while [ $i -le $par_num ]
		do
			number=`ls /sys/block/$line/device/scsi_disk |awk -F":" '{print $1}'`
			sd=`cat /proc/partitions |grep $line|awk '{print $4}'|egrep sd[a-z] |sed -n "$i"p`
			id=`vol_id /dev/"$sd" 2>/dev/null |grep ID_FS_UUID|cut -d= -f2`
			path=`mount |grep $sd |grep "/tmp/mnt/sd"|grep rw|awk -F" " '{print $3}'`
			[ "x$path" = "x" -o "x`vol_id /dev/$sd 2>/dev/null`" = "x" ] && i=`expr $i + 1` && continue
			mybook=`vol_id /dev/$sd 2>/dev/null |grep ID_FS_LABEL |cut -d= -f2 |head -1`
			[ "x$mybook" = "x" ] && mybook=$sd
			model=`cat /sys/block/$line/device/model |sed s/[[:space:]]//g`
			size=`parted -s /dev/$line print |grep "Disk /dev" | awk -F: '{print $2}' |sed s/[[:space:]]//g`
			sernum=`cat /proc/scsi/usb-storage/$number |grep Serial |awk -F" " '{print $3}'`
			i=`expr $i + 1`
			if [ "x$path" != "x" -a "x`vol_id /dev/$sd 2>/dev/null`" != "x" ];then
				echo $id$sernum,$path,$size,$mybook >> /tmp/usb_test_disk
			fi
		done
	done < /tmp/usb_device_test
}

start_action(){
	[ ! -f $drives_info_path -o "x`cat $drives_info_path |head -1`" = "x" ] && echo "No usb drive plug in!" && return
	[ ! -d /tmp/usb_test ] && mkdir -p /tmp/usb_test
	[ "x`ls /tmp/usb_test 2>/dev/null`" = "x" ] && touch /tmp/usb_test/test1 && touch /tmp/usb_test/test2
	while read line
	do
		drive_path=`echo $line |awk -F "," '{print $2}'`
		if [ ! -d $drive_path ];then
			echo "Drive is not mount!"
			return
		fi
		[ "x`ls $drive_path/ 2>/dev/null`" != "x" ] && rm -rf $drive_path/*
		echo ""
		echo "Start sync Router(/tmp/usb_test) file to Drive($drive_path)"
		echo ""
		echo "Router -------->> Drive"
		echo ""
		cp -rf /tmp/usb_test/* $drive_path/
		file=`ls /tmp/usb_test/ 2>/dev/null`
		echo "Sync file list: $file"
		echo ""
		echo "Sync Drive file successful!"
		echo ""
		echo "Delete Drive file"
		echo ""
		rm -rf $drive_path/*
		echo "Delete successful!"
		echo ""
		echo "Start Restore file to Router"
		echo ""
		echo "Drive -------->> Router"
		echo ""
		cp -rf /tmp/usb_test/* $drive_path/
		file=`ls $drive_path/ 2>/dev/null`
		echo "Sync file list: $file"
		echo ""
		echo "Restore Drive file successful!"
	done <$drives_info_path
	rm -rf /tmp/usb_test
	echo ""
	return 
}

case "$1" in
	start)
		drives_info
		start_action
	;;
	usb_info)
		drives_info
	;;
esac

