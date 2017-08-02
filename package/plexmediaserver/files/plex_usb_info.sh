#!/bin/sh

rm -rf /tmp/plex_curUSB_info
rm -rf /tmp/usb_check
ls -l /sys/block |grep sd |awk '{print $9}' > /tmp/usb_par

plex_drive(){
	while read line
	do
		i=1
		par_num=`cat /proc/partitions |grep $line |awk '{print $4}' |egrep sd[a-z] |wc -l`
		while [ $i -le $par_num ]
		do
			number=`ls /sys/block/$line/device/scsi_disk |awk -F":" '{print $1}'`
			sd=`cat /proc/partitions |grep $line|awk '{print $4}'|egrep sd[a-z] |sed -n "$i"p`
			id=`vol_id /dev/"$sd" 2>/dev/null |grep ID_FS_UUID|cut -d= -f2`
			path_tmp=`mount |grep $sd |sed -n '1p' |grep "/tmp/mnt" |awk -F" " '{print $3}'`
			[ "x$path_tmp" = "x" -o "x`vol_id /dev/$sd 2>/dev/null |head -1`" = "x" ] && i=`expr $i + 1` && continue
			[ "x`parted -s /dev/$line print | grep \"Partition Table\" | awk '{print $3}'`" != "xloop" -a "x`parted -s /dev/$line print noshare | grep $sd`" != "x" ] && i=`expr $i + 1` && continue
			path=`mount |grep $sd |sed -n '1p' |grep "/tmp/mnt" |grep rw |awk -F" " '{print $3}'`
			if [ "x$path" = "x" -a "x`config get plex_file_path`" = "x$path_tmp" ];then
				echo "Your USB Drive is read-only, please choose a new Drive for Plex Library."
				i=`expr $i + 1` 
				continue
			fi
			mybook=`vol_id /dev/$sd 2>/dev/null |grep ID_FS_LABEL |cut -d= -f2 |head -1`
			model=`cat /sys/block/$line/device/model` 
			size=`df -h | grep $line |grep "$sd" | grep "/tmp/mnt/sd" | awk '{print $2}'`
			free_size=`df -h | grep $line |grep "$sd" | grep "/tmp/mnt/sd" | awk '{print $4}'`
			free_size_tmp=`df -m | grep $line |grep "$sd" | grep "/tmp/mnt/sd" | awk '{print $4}'`
			plex_library_path=`config get plex_file_path`
			[ "x$plex_library_path" != "x" ] && Transcode_size=`du -sm $plex_library_path/Library/Application\ Support/Plex\ Media\ Server/Cache/Transcode 2>/dev/null |awk '{print $1}'`
		 	[ "x$Transcode_size" = "x" ] && Transcode_size=0
			flag_size=`expr $free_size_tmp + $Transcode_size`
			if [ $flag_size -ge 1024 ];then		    
				flag_size=`echo "scale=1;$flag_size/1024"|bc` 
				if [ `echo $flag_size |cut -d. -f2` -gt 5 ];then
					flag_size_tmp=`echo $flag_size |cut -d. -f1`
					flag_size_tmp=`expr $flag_size_tmp + 1`
				else
					flag_size_tmp=`echo $flag_size |cut -d. -f1`
				fi
				if [ $flag_size_tmp -ge 1024 ];then
					flag_size=`echo "sclae=1;$flag_size_tmp/1024"|bc`T
				else
					flag_size="$flag_size"G
				fi   
			else
				flag_size="$flag_size"M
			fi
			sernum=`cat /proc/scsi/usb-storage/$number |grep Serial |awk -F" " '{print $3}'`
			if [ "x`vol_id /dev/$sd 2>/dev/null`" != "x" -a "x$id" = "x" ];then
				if [ "x`echo $sd | cut -b 4-`" != "x" ];then
					id="`echo $sd | cut -b 4-`-`cat /sys/block/$line/$sd/size`-`mount |grep $sd |sed -n '1p' |awk '{print $5}'`"
				else
					id="$sernum-`cat /sys/block/$line/size`-`mount |grep $sd |sed -n '1p' |awk '{print $5}'`"
				fi
			fi
			i=`expr $i + 1`
			if [ "x$path" != "x" -a "x$id" != "x" -a "x`vol_id /dev/$sd 2>/dev/null`" != "x" ];then
				echo $id$sernum,$path,$model,$size,$free_size,$flag_size,$mybook >> /tmp/plex_curUSB_info
			fi
		done
	done < /tmp/usb_par
}

plex_library(){
	if [ "x$(config get plex_select_usb)" = "x" ]; then
		if [ `config get plex_library_change` -eq 1 ];then
			echo "Please select a new plex library drive."
			logger -- "[PLEX]You USB Drive for plex libary has removed,"
			if [ ! -f /tmp/plex_curUSB_info ];then
				config set plex_library_change=0
				exit 0
			fi
		else
			[ -f /tmp/plex_curUSB_info ] || exit 0
			usb=`cat /tmp/plex_curUSB_info |head -1|awk -F "," '{print $1}'`
			path=`cat /tmp/plex_curUSB_info |head -1|awk -F "," '{print $2}'`
			config set plex_select_usb="$usb,$path"
		fi
	else
		if [ ! -f /tmp/plex_curUSB_info ];then
			config set plex_library_change=0
			exit 0
		fi	
		idsum=`config get plex_select_usb`
		uuid=`echo $idsum |awk -F"," '{print $1}'`
		path_old=`echo $idsum |awk -F"," '{print $2}'`
		media_err=`cat /tmp/plex_curUSB_info |grep $uuid `
		path_new=`echo $media_err |awk -F"," '{print $2}'`
		#echo $media_err
		if [ "x$media_err" != "x" ];then
			[ $path_old != $path_new ] && config set plex_select_usb="$uuid,$path_new"
			[ "x`/bin/ps -w|grep cmdplexmediaserver |grep stop |grep -v grep 2>/dev/null`" = "x" ] && echo "Plex library drive plugged !!"
			config set plex_library_change=0
		else
			config set plex_library_change=1
			config set plex_select_usb=
		fi
	fi
}

plex_drive
case "$1" in
	fresh)
	plex_library
	;;
esac
