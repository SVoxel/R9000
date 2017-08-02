#!/bin/sh

drive_dir=/tmp/drive
USB_MAP_TABLE=/tmp/plexmediaserver/.usb_map_table
newpath=""

login() 
{
	#create folders to save http responses
	[ ! -d "$dirve_dir" ] && mkdir -p $drive_dir

	netdrive --login	

	[ "x$(config get amazon_login_status)" != "x200" ] && exit 0

	netdrive --endpoint && netdrive --rootid &
}

logout() 
{
	killall netdrive
	rm $drive_dir -rf
	config set last_drive_account=$(config get drive_account)
    config set usb_path=
    config set sync_local_folder=
    config set syncup_folder_id=
    config set amazon_sync=
    config set drive_folder_invalid=0
    config set drive_rootID=
    config set sync_auto=1
    config set sync_manul=0
}

saveDir() #1: 0:initial save 1:save and delete 2:save but not delete
{
	local bk_pid=$(ps |grep "netdrive --backup" |grep -v "grep" |awk -F " " '{print $1}')
	if [ ! -z $bk_pid ]; then
		kill $bk_pid	
	fi

	local usbPath=$(config get usb_path)

	if [ -z $(echo $usbPath |grep tmp) ]; then
		usbPath="/tmp$usbPath"
		config set usb_path=$usbPath	
	fi
	
	#get usb serial number
	local sd=`echo $usbPath |awk -F "/" '{print $NF}' |awk -F "" '{print $1$2$3}'`	
	local dev_info=`sed -n "/,$sd,sd[a-o]/p" $USB_MAP_TABLE` 
	local serial=`echo $dev_info |awk -F"," '{print $1}'`
	config set usb_serial=$serial

	[ -s $usbPath/.cloudBackup ] && rm $usbPath/.cloudBackup && rm $usbPath/.amazon_Sync_errorMsg

	config set amazon_sync=" "

    config set syncup_folder_id=
    if [ "x$(config get last_sync_folder)" != "x" -a "x$(config get last_sync_usb_serial)" != "x" -a "x$(config get last_sync_usb_path)" != "x" ];then 
        checkUSBandFolder "$(config get last_sync_folder)" "$(config get last_sync_usb_serial)" "$(config get last_sync_usb_path)"
        local ret=$?
        if [ $ret = "0" ]; then
            config set drive_show_prompt=0
            cmdsched
        else
            config set drive_show_prompt=1
        fi
    else
        config set drive_show_prompt=0
    fi
}

checkUSBandFolder() # $1:folder name $2:usb serial number $3:usb path
{
	echo "TESTA $1 $2 $3"
	[ -z $2 ] && return 0
	for sdx in $(ls /tmp/mnt)
	do
		echo "TESTB $sdx"
		local sd=`echo $sdx |cut -b '1-3'`
		local n=`echo $sdx |cut -b '4'`
		local org_n=`echo $3 |awk -F "/" '{print $NF}' |cut -b '4'`	
		echo "TESTC $sd $n $org_n"
		[ "x$n" != "x$org_n" ] && continue

		local dev_info=`sed -n "/,$sd,sd[a-o]/p" $USB_MAP_TABLE` 
		local dev=`echo $dev_info |awk -F"," '{print $3}'`
		local num=`ls /sys/block/$dev/device/scsi_device |awk -F ":" '{print $1}'`
		local serial=`cat /proc/scsi/usb-storage/$num |grep "Serial" |awk '{print $NF}'`
		echo "TESTE $serial $2"
		[ "x$serial" != "x$2" ] && continue
		
		newpath="/tmp/mnt/$sdx"
		echo "TESTH $newpath"
		if [ -d "$newpath/$1" ]; then
			echo "TESTH return0"
			return 0 #both and folder valid
		else
			echo "TESTH return2"
			return 2 #usb valid but folder invalid
		fi
	done
	echo "TESTH return1"
	return 1 #usb invalid 
}

checkname()
{
	killall netdrive
	config set amazon_sync=" "
	echo "BEGIN" > /tmp/drive/backupSize
    echo > /tmp/ambackup_log

	echo "[Cloud Backup]Check folder name. Please wait..."
	echo "[Cloud Backup]Check folder name. Please wait..." >>/tmp/ambackup_log
	local folder_name=$(config get sync_local_folder)
	if [ "x$folder_name" = "x/" ]; then
		folder_name=$(config get sync_root_name | awk -F"(" '{print $2}' | awk -F")" '{print $1}')
	fi
	netdrive --checkname --name "$folder_name"
}

checkcfl() #1: option
{
	[ $1 = "1" ] && config set backup_cfl=1	|| config set backup_cfl=2	
}

detCloud()
{
	[ "x$(ps |grep "netdrive --backup" |grep -v grep |grep -v netdrive.sh)" != "x" ] && echo "[Cloud Backup]Last Backup has not Finished!" && exit 0	
	[ "x$(ps |grep "netdrive --notify" |grep -v grep |grep -v netdrive.sh)" != "x" ] && echo "[Cloud Backup]Notify Backup has not Finished!" && exit 0	
	
	[ $(config get drive_folder_invalid) = "1" ] && echo "[Cloud Backup]No USB or Folder invalid" && exit 0
	
	local usbPath=$(config get usb_path)
	local folderName=$(config get sync_local_folder)
	local id=$(config get syncup_folder_id)
	[ -z $usbPath ] || [ -z $folderName ] || [ -z $id ] && exit 0

	local pathname="$usbPath/$folderName"
	[ ! -d $pathname ] && echo "[Cloud Backup]Backup folder invalid" && exit 0
	echo "Detect $pathname"
	echo "Detect $pathname,don't have backup process and notify process" >> /tmp/ambackup_log
    rm /tmp/even_w
	netdrive --detCloud --id $id --pathname "$pathname"
	netdrive --backup &	
}

check()
{
	local usbPath=$(config get usb_path)
	local usbSerial=$(config get usb_serial)
	local syncFolder=$(config get sync_local_folder)
	
	[ -z $usbPath ] || [ -z $syncFolder ] && exit 0
	
	checkUSBandFolder "$syncFolder" "$usbSerial" "$usbPath"
	local ret=$?
	if [ $ret = "0" ]; then
		config set drive_folder_invalid=0
		config set usb_path=$newpath
		if [ $(config get amazon_sync) = "-3" ]; then
			config set backup_error_other="Unknown error"
			config set amazon_sync=" "
		fi
	else
		config set drive_folder_invalid=1
		config set backup_error_other="[Cloud Backup]Your selected folder is invalid now"
		config set amazon_sync=-3
	fi
}

opt() #$1: 1,2,3
{
	local usbPath=$(config get usb_path)
	config set drive_show_prompt=0
	
	if [ $1 -eq 2 ]; then #recover
		echo "$[Cloud Backup]recover"
		local id=$(config get last_sync_folder_id)
		local name=$(config get last_sync_folder)
		local pathname="$usbPath/$name"
		while [ -d "$usbPath/$name" ]
		do
			name="$name-CloudBackupRecover"
			pathname="$usbPath/$name"
            chmod 777 $pathname
		done
		netdrive --recover --name "$name" --pathname "$pathname" --id $id & 
	elif [ $1 -eq 3 ]; then #delete 
		echo "[Cloud Backup]Delete"
		local id=$(config get last_sync_folder_id)
		netdrive --id $id --delete &
	fi
}

nusb_mount() #1:sd**
{
	local usb_serial=$(config get usb_serial)
	[ -z $usb_serial ] && exit 0

	local usbPath=$(config get usb_path)
	
	local sd_num=`echo $1 |cut -b 4`
	local sd=`echo $1 |cut -b '1-3'`
	local dev_info=`sed -n "/,$sd,sd[a-o]/p" $USB_MAP_TABLE` 
	local dev=`echo $dev_info |awk -F"," '{print $3}'`
	local num=`ls /sys/block/$sd/device/scsi_device |awk -F ":" '{print $1}'`
	local serial=`cat /proc/scsi/usb-storage/$num |grep "Serial" |awk '{print $NF}'`
	#if [ $serial = $usb_serial ] && [ "$sd_num" = "`echo $usbPath |cut -b 13`" ]; then
    if [ $serial = $usb_serial ]; then
        config set usb_path="/tmp/mnt/$1"
        if [ "$(config get sync_manul)" = "1" ]; then
            sch_notify
            if [ -n $(config get amazon_sync) -a "$(config get amazon_sync)" != "0" ]; then
                echo "[Cloud Backup]Continue last Backup"
                echo "[Cloud Backup]Continue last Backup" >> /tmp/ambackup_log
				config set drive_folder_invalid=0
                netdrive --backup > /dev/console &	
            fi
            cmdsched
        fi
        if [ "$(config get sync_auto)" = "1" ];then
            notify_bk >/dev/console &
        fi
        exit 0
    else
        exit 0
    fi
}

nusb_umount() #1:sd**
{
#	local sd=`echo $1 |cut -b '1-3'`
#	local num=`ls /sys/block/$sd/device/scsi_device |awk -F ":" '{print $1}'`
#	local serial=`cat /proc/scsi/usb-storage/$num |grep "Serial" |awk '{print $NF}'`
#	[ "x$serial" != "x$(config get usb_serial)" ] && exit 0
    usb_serial=`config get usb_serial`
    [ "x$usb_serial" = "x" ] && exit 0
    usb_path=`config get usb_path | awk -F"/" '{print $NF}'`
    usb_ex=`grep -rn $usb_serial /proc/scsi/usb-storage`
    if [ "x$usb_ex" = "x" -a "x$usb_path" = "x$1" ]; then  
        echo "[Cloud Backup]Your hard disk has removed" > /dev/console
        echo "[Cloud Backup]Your hard disk has remove and selected folder is invalid now" >> /tmp/ambackup_log
        /etc/email/send_email_alert amazon_cloud_backup drive_usb_removed &
        logger -- "[Cloud Backup]Your USB HDD had been removed,"
	
        [ "$(config get sync_manul)" = "1" ] && config set sync_manul=0 && cmdsched && config set sync_manul=1
        notify_stop
    else
        echo "[Cloud Backup]move other usb " > /dev/console
        echo "[Cloud Backup]move other usb " >> /tmp/ambackup_log
    fi
	config set amazon_sync=-2
	config set backup_error_folder=invalid_folder
}

notify_bk()
{
    check
    auto_enable=`config get sync_auto`
    nusb_path=`config get usb_path`
    nfolder=`config get sync_local_folder`
    echo -n 0 > /tmp/drive/backupNum
    echo -n 0 > /tmp/drive/backupfNum
    [ -f /tmp/drive/backupFail ] && rm /tmp/drive/backupFail
    config set amazon_fcount=0
    config set amazon_dcount=0
    if [ "$auto_enable" = "1" -a "x$nusb_path" != "x" -a "x$nfolder" != "x" ]; then
        ndir="$nusb_path/$nfolder"
        echo "notify listen  $ndir ........."
        killall inotifywait
        rm /tmp/even_w
        /usr/sbin/inotifywait --exclude '.ReadyDLNA|.cloudBackup|.amazon_Sync_errorMsg' -rqm --format '%w|%e|%f' -e modify -e moved_from -e moved_to -e create -e delete "$ndir" -o /tmp/even_w &
        killall am_listen
        ps -w |grep "netdrive --backup" |grep -v "grep" | awk '{print $1}' |xargs kill -9 2>/dev/null
        ps -w |grep "netdrive --notify" |grep -v "grep" | awk '{print $1}' |xargs kill -9 2>/dev/null
        /usr/sbin/am_listen &
        echo 0 > /tmp/bk_status
        /usr/sbin/netdrive --backup > /dev/console &
        while [ 1 ]; do
            [ -f /tmp/bk_status ] && bk_done=`cat /tmp/bk_status`
            if [ "$bk_done" = "1" ]; then #finish backup
                while [ 1 ]; do
                    echo -n 0 > /tmp/drive/backupNum
                    echo -n 0 > /tmp/drive/backupfNum
                    echo 0 > /tmp/notify_status
                    if [ -f /tmp/drive/backupFail ];then
                        echo "backup stage have fail file" > /dev/console
                        echo "backup stage have fail file" >> /tmp/ambackup_log
                        cat /tmp/drive/backupFail >> /tmp/even_w
                        rm /tmp/drive/backupFail
                    fi
                    if [ -s /tmp/even_w ]; then
                        config set create_done=0
                        /usr/sbin/netdrive --notify > /dev/console &
                        while [ 1 ]; do
                            [ -f /tmp/notify_status ] && notify_done=`cat /tmp/notify_status`
                            if [ "$notify_done" = "1" ]; then #finish backup
                                echo "notify this file/folder is done" > /dev/console
                                break
                            else
                                sleep 5
                            fi
                        done
                    else
                        config set notify_name=
                        config set notify_kind=
                        config set notify_path=
                        config set notify_action=
                    fi
                        sleep 5
                done
            else
                sleep 5
            fi
        done
    fi
}

manul()
{
	local det_pid=$(ps |grep "netdrive --detCloud" |grep -v "grep" |awk -F " " '{print $1}')
	while [ ! -z $det_pid ]
	do
		echo "wait"
		sleep 2	
		det_pid=$(ps |grep "netdrive --detCloud" |grep -v "grep" |awk -F " " '{print $1}')
	done
	
	[ "x$(ps |grep "netdrive --backup" |grep -v grep |grep -v netdrive.sh)" != "x" ] && echo "[Cloud Backup]Last Backup has not Finished!" && exit 0	
	[ "x$(config get drive_contentUrl)" = "x" ] || [ "x$(config get drive_metadataUrl)" = "x" ] && netdrive --endpoint
	if [ "x$(config get drive_contentUrl)" = "x" ] || [ "x$(config get drive_metadataUrl)" = "x" ]; then
		config set amazon_sync=-2
		config set backup_error_other="[Cloud Backup]Backup failure due to unknown error"
		/etc/email/send_email_alert amazon_cloud_backup unknown_error 
        logger -- "[Cloud Backup]Backup failure due to unknown error,"
		echo "[Cloud Backup Error]Fail to get endpoint" && exit 0
	fi

	[ ! -d "/tmp/drive" ] && mkdir -p "/tmp/drive"
	config set amazon_sync=1
	config set drive_folder_invalid=0
    echo -n 0 > /tmp/drive/backupNum
    echo -n 0 > /tmp/drive/backupfNum
    config set amazon_fcount=0
    config set amazon_dcount=0
	echo "BEGIN" > /tmp/drive/backupSize
    config set sch_done=0
    config set snow_flag=0
    echo 0 > /tmp/bk_status
    if [ -s /tmp/ambk_even ]; then
        config set snow_flag=1
        /usr/sbin/am_listen 
        sleep 8
        bkf=`config get amazon_fcount`
        bkd=`config get amazon_dcount`
        echo "[Cloud Backup]file num=$bkf,folder num=$bkd" > /dev/console
        echo  -n > /tmp/ambk_even
    fi
    netdrive --backup > /dev/console &
    sch_failagain &
}

sch_notify()
{
    musb_path=`config get usb_path`
    mfolder=`config get sync_local_folder`
    mdir="$musb_path/$mfolder"
    if [ "x$musb_path" != "x" -a "x$mfolder" != "x" ]; then
        echo "schedule listen $mdir" > /dev/console
        rm /tmp/ambk_even
        /usr/sbin/inotifywait -rqm --format '%w|%e|%f'  -e moved_from -e moved_to "$mdir"  -o /tmp/ambk_even &
    fi
}

sch_failagain()
{
    while [ 1 ]; do
        [ -f /tmp/bk_status ] && sch_fin=`cat /tmp/bk_status`
        if [ "$sch_fin" = "1" ]; then #finish backup
            if [ -s /tmp/drive/backupFail ]; then
                echo -n > /tmp/drive/backupFail
                ps -w |grep "netdrive --backup" |grep -v "grep" | awk '{print $1}' |xargs kill -9 2>/dev/null
                echo 0 > /tmp/bk_status
                config set sch_done=0
                config set snow_flag=0
                echo "schedule mode have fail files and snow flag=$(config get snow_flag)" > /dev/console
                echo "schedule mode have fail files and snow flag=$(config get snow_flag)" >> /tmp/ambackup_log
                netdrive --backup > /dev/console &
            else
                echo "schedule mode have no fail files" > /dev/console
                break
            fi
        else
            sleep 5
        fi
    done
}

download_log()
{
    if [ -s /tmp/drive/backupFail ]; then
        zip amazon_log.zip /tmp/ambackup_log /tmp/drive/backupFail
    else
        zip amazon_log.zip /tmp/ambackup_log
    fi
}
notify_stop()
{
	echo "[Cloud Backup]stop notify process ..." > /dev/console
    rm /tmp/even_w
    rm /tmp/now_line
    rm /tmp/ambk_even
    echo > /tmp/ambackup_log
    ps -w |grep notify_bk |grep -v "grep" | awk '{print $1}' |xargs kill -9 2>/dev/null
    ps -w |grep inotifywait |grep -v "grep" | awk '{print $1}' |xargs kill -9 2>/dev/null
    ps -w |grep nusb_mount |grep -v "grep" | awk '{print $1}' |xargs kill -9 2>/dev/null
    ps -w |grep am_listen |grep -v "grep" | awk '{print $1}' |xargs kill -9 2>/dev/null
    ps -w |grep "netdrive --backup" |grep -v "grep" | awk '{print $1}' |xargs kill -9 2>/dev/null
    ps -w |grep "netdrive --notify" |grep -v "grep" | awk '{print $1}' |xargs kill -9 2>/dev/null
}

case "$1" in
	login)
	login
	;;
	logout)
	logout
	;;
	notify_bk)
	notify_bk 
	;;
	sch_notify)
	sch_notify 
	;;
	manul)
	manul
	;;
	save)
	saveDir $2
	;;
	check)
	check
	;;
	opt)
	opt $2
	;;
	nusb_mount)
	nusb_mount $2
	;;
	nusb_umount)
	nusb_umount $2
	;;
	checkname)
	checkname
	;;
	cfl)
	checkcfl $2
	;;
	detCloud)
	detCloud
	;;
    am_log)
    download_log
	;;
	notify_stop)
	notify_stop
	;;
	*)
	echo "Usage: $0 {login|logout|notify_bk|sch_notify|manul|save|check|opt|nusb_mount|nusb_umount|checkname|cfl|detCloud|am_log|notify_stop}"
	exit 1
esac

exit 0
