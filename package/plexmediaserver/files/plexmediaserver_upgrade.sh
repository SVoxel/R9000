#!/bin/sh

oc () { "$@" >> /dev/console 2>&1 ; } # stdout & stderr to /dev/console

#plex_download_url="https://plex.tv/downloads/details/1?channel=0&build=linux-openwrt-armv7&distro=netgear"
plex_download_url="ftp://updates1.netgear.com/sw-apps/plex/r9000/"
plex_download_url_https="https://updates1.netgear.com/sw-apps/plex/r9000/"
plex_version_name="*.*.*.*"
plex_filter_name="plexmediaserver-r9k-"$plex_version_name"-armv7.tgz"
plex_volume_name="plexmediaserver"
plex_partition_mount_point="/tmp/plexmediaserver"
plex_init_script_path="/etc/plexmediaserver/cmdplexmediaserver"
plex_check_result="/tmp/plex_check_result"
plex_download_result="/tmp/plex_download_result"
plex_binary_untar_info="/tmp/plex_binary_untar_info"
plex_upgrade_result="/tmp/plex_upgrade_result"
plex_check_tmp="/tmp/plex_check_tmp"
plex_check_tmp2="/tmp/plex_check_tmp2"
plex_latest_version="/tmp/plex_latest_version"
plex_current_version="/tmp/plex_current_version"

newer_version() # current version $1: x.x.x.x, latest version $2: x.x.x.x,
{
	local v1 v2 n=1

	while [ 1 ]; do
		v1=$(echo $1 |awk -F. '{print $'$n'}')
		v2=$(echo $2 |awk -F. '{print $'$n'}')
		[ "$v1" -gt "$v2" ] && return 1
		[ "$v1" -lt "$v2" ] && return 0
		[ "$n" = "4"  ] && [ "$v1" -eq "$v2" ] && return 1
		n=$(($n + 1))
	done
}

#Check whether there have a new plex version in netgear server 
plex_check()
{
    [ `/bin/ps -w | grep "plexmediaserver_upgrade.sh" | grep "check" | grep -v grep | wc -l` -gt 2 ] && return
	[ "x`config get plex_update_run`" = "x3" -o "x`config get plex_update_run`" = "x4" ] && return
	[ "x`config get plex_update_run`" = "x2" -a "x$1" = "xauto" ] && return
    [ "x$1" != "xauto" ] && config set plex_update_run=2 && config commit
	if [ "x`ls /tmp/plex_current_version 2>/dev/null`" = "x" ];then
		if [ "x`ls $plex_partition_mount_point/plex_current_version 2>/dev/null`" != "x" ];then
			cp  $plex_partition_mount_point/plex_current_version /tmp/
		fi
	fi
    rm -rf $plex_check_result $plex_download_result $plex_upgrade_result 2>/dev/null
    rm -rf $plex_check_tmp $plex_check_tmp2 2>/dev/null
    curl --insecure --connect-timeout 30 --keepalive-time 30 --retry 1 $plex_download_url_https"verify_binary.txt" -o $plex_check_tmp2 2>/dev/null 
    plex_echo=`echo $?`
    if [ $plex_echo -ne 0 ];then
	curl --insecure --connect-timeout 30 --keepalive-time 30 $plex_download_url"verify_binary.txt" -o $plex_check_tmp2 2>/dev/null 
	plex_echo=`echo $?`
    else
	plex_download_url=$plex_download_url_https
    fi
    cat $plex_check_tmp2 | grep "$plex_filter_name" |awk -F "fileHash=\"" '{print $2}'> $plex_check_tmp
    if [ $plex_echo -eq 0 ];then
		oc echo "plex check successful." 
		echo "0" > $plex_check_result
    elif [ $plex_echo -ne 0 ];then
		oc echo "plex check failed!" 
		echo "1" > $plex_check_result
		config set plex_have_new=0 
		update_run=`config get plex_update_run`
		[ "x$update_run" != "x0" -a "x$1" != "xauto" ] && config set plex_update_run=0 
		config commit 
		return 0
    fi
	line=`cat $plex_check_tmp`
	plex_name=`echo $line |awk -F "fileName=\"" '{print $2}' |awk -F "\"" '{print $1}'`
	plex_hash=`echo $line |awk -F "\"" '{print $1}'`
	plex_url=`echo $line |awk -F "url=\"" '{print $2}' |awk -F "\"/>" '{print $1}'`
	plex_version=`echo $plex_name |awk -F "-" '{print $3"."$4}'`
	plex_date=`cat $plex_check_tmp2 |grep "createdAt=" |awk -F "createdAt=\"" '{print $2}' |awk '{print $1}'`

	rm -rf $plex_latest_version 2>/dev/null
	echo "$plex_name" "$plex_date" "$plex_hash" "$plex_url">$plex_latest_version
	latest_version=$plex_version
	if [ -f $plex_partition_mount_point/plex_current_version ];then
		current_version=`cat $plex_partition_mount_point/plex_current_version |awk '{print $1}' |awk -F "-" '{print $3"."$4}'`
	else 
		current_version=""
	fi
	[ "x`config get plex_have_new`" != "x0" ] && config set plex_have_new=0
	if [ "x$current_version" = "x" ];then
		[ "x`config get plexmediaserver_enable`" = "x1" ] && config set plex_have_new=1 
	else
		if newer_version $current_version $latest_version;then
			config set plex_have_new=1
		fi
	fi
	config commit
}

plex_check_size()
{
    plex_partition_size=`df -h | grep "$plex_volume_name" | awk '{print $2}'`
    [ "x$plex_partition_size" = "x" ] && return 0
    case $plex_partition_size in
        *k)
            plex_partition_size=`echo $plex_partition_size | awk -F "k" '{print $1}' | awk -F "." '{print $1}'`
            plex_partition_size=`expr $plex_partition_size \* 1024`
        ;;
        *M)
            plex_partition_size=`echo $plex_partition_size | awk -F "M" '{print $1}' | awk -F "." '{print $1}'`
            plex_partition_size=`expr $plex_partition_size \* 1024 \* 1024`
        ;;
    esac

    rm -rf $plex_binary_untar_info
    tar -tvf $1 | awk '{print $3}' > $plex_binary_untar_info

    plex_binary_untar_size=0
    while read LINE
    do
        plex_binary_untar_size=`expr $plex_binary_untar_size + $LINE`
    done < $plex_binary_untar_info

    if [ $plex_partition_size -gt $plex_binary_untar_size ];then
        return 1
    else
        return 0
    fi
}

plex_download()
{
    [ "x$1" = "x" ] && oc echo "plex download: no url for download binary!" && echo "1" > $plex_download_result && config set plex_update_run=0 && config commit && return 0
    [ ! -d $plex_partition_mount_point ] && oc echo "plex download: no plex partition!" && echo "4" > $plex_download_result && config set plex_update_run=0 && config commit && return 0
    config set plex_update_run=3
	config commit
    binary_name=`echo $1 |awk -F "/" '{print $6}'`
    ls /tmp/$plex_filter_name 2>/dev/null | grep -v "$binary_name" | xargs rm -rf
    if [ "x`ls /tmp/$binary_name 2>/dev/null`" = "x/tmp/$binary_name" ];then
        oc echo "plex download: plex binary have been downloaded, no need to download again."
        config set plex_download_percent=100
        if plex_check_size /tmp/$binary_name;then
            oc echo "plex download: plex check size failed, should not be upgraded!"
            config set plex_download_percent=0
            echo "3" > $plex_download_result && config set plex_update_run=0 && config commit && return 0
        else
            oc echo "plex download: plex binary download and check size OK."
            oldsum=`cat /tmp/plex_latest_version |grep $binary_name |awk '{print $3}'`
            newsum=`sha1sum /tmp/$binary_name |awk '{print $1}'`
            if [ "x$oldsum" = "x$newsum" ];then 
                echo "plex download: plex checksum successful."
                config set plex_download_percent=0
                echo "0" > $plex_download_result 
                return 0
            else
                echo "plex download: plex checksum failed, binary will be re downloaded."
                config set plex_download_percent=0
                rm -rf /tmp/$binary_name
            fi
        fi
    fi

    /etc/plexmediaserver/plexmediaserver_upgrade.sh download_percent $1 &
    curl --insecure --connect-timeout 60 --keepalive-time 180 $1 -o /tmp/$binary_name 2>/dev/nul
    #curl --insecure $plex_download_url$1 -o /tmp/$1 2>/dev/null
    [ `echo $?` -ne 0 ] && oc echo "plex download: download failed!" && echo "2" > $plex_download_result && rm -rf /tmp/$binary_name && config set plex_update_run=0 && config commit && return 0
    
    oldsum=`cat /tmp/plex_latest_version |grep $binary_name |awk '{print $3}'`
    newsum=`sha1sum /tmp/$binary_name |awk '{print $1}'`
    if [ "x$oldsum" = "x$newsum" ];then 
        echo "plex download: plex checksum successful."
    else
        echo "plex download: plex checksum failed, should not be upgraded."
        echo "5" > $plex_download_result && config set plex_update_run=0 && config commit && return 0
        rm -rf /tmp/$binary_name
        return 0
    fi
    if plex_check_size /tmp/$binary_name;then
        oc echo "plex download: plex check size failed, should not be upgraded!"
        echo "3" > $plex_download_result && config set plex_update_run=0 && config commit && return 0
    else
        oc echo "plex download: plex binary download and check size OK."
        echo "0" > $plex_download_result 
    fi
}
plex_upgrade()
{
    [ "x$1" = "x" ] && oc echo "plex upgrade: no binary name for upgrade!" && echo "2" > $plex_upgrade_result && config set plex_update_run=0 && config commit && return 0
    config set plex_update_run=4
    config commit
    [ "x`ls /tmp/$1 2>/dev/null`" != "x/tmp/$1" ] && oc echo "plex upgrade: binary not exist!" && echo "3" > $plex_upgrade_result && config set plex_update_run=0 && config commit && return 0
    plex_upgrade_version=`echo $1 | awk -F "-" '{print $3"."$4}'`
    if [ "x$plex_upgrade_version" != "x" ];then
        if [ "x`find $plex_partition_mount_point/ -type d -name "*$plex_upgrade_version*"`" != "x" ];then
            oc echo "plex upgrade: the same version, no need to upgrade again"
            echo "1" > $plex_upgrade_result
			config set plex_update_run=0;
			config commit
            return 0
        fi
    fi
    $plex_init_script_path stop 1
    [ "x`/bin/ps -w | grep Plex | grep -v grep`" != "x" ] && oc echo "plex upgrade: stop Plex failed!" && echo "4" > $plex_upgrade_result && config set plex_update_run=0 && config commit && return 0
    rm -rf $plex_partition_mount_point/*
    sync
    tar -zxf /tmp/$1 -C $plex_partition_mount_point/ 2>/dev/null
        targetdir=`ls /tmp/$1 | awk -F "-" '{print $3"-"$4}'`
	cp -p -f /etc/plexmediaserver/libpthread-0.9.33.2.so $plex_partition_mount_point/$targetdir
	ln -sf libpthread-0.9.33.2.so $plex_partition_mount_point/$targetdir/libpthread.so.0
	version=`ls /tmp/$1 | awk -F "-" '{print $3"."$4}' |grep -v "debug" 2>/dev/null`
	cat $plex_latest_version |grep $version  |grep -v "debug" >$plex_current_version
	if [ "x`cat $plex_current_version |awk '{print $1}' 2>/dev/null`" = "x" ];then
		name=$1
		date=`date "+%Y-%m-%d"`
		binary_hash="000000000000000"
		url=$plex_download_url
		echo "$name" "$date" "$binary_hash" "$url" >$plex_current_version
	fi
	cp $plex_current_version $plex_partition_mount_point/
	#echo "plex-uclibc-armv7-"$version".tgz" >$plex_current_version_bk
	plexmediaserver_script=`ls $plex_partition_mount_point/*.*.*.*/plexmediaserver.sh 2>/dev/null`
	[ -f $plexmediaserver_script ] && cp $plexmediaserver_script $plex_partition_mount_point/
    sync
    [ `echo $?` -ne 0 ] && oc echo "plex upgrade: untar plex packet failed!" && echo "5" > $plex_upgrade_result && rm -rf $plex_partition_mount_point/* && config set plex_update_run=0 && config commit && return 0
    [ "x`config get plexmediaserver_enable`" = "x1" ] && $plex_init_script_path start
    [ "x`config get plexmediaserver_enable`" = "x1" -a "x`/bin/ps -w | grep Plex | grep -v grep`" = "x" ] && oc echo "plex upgrade: start Plex failed!"
    oc echo "plex upgrade:  plex upgrade successful" && echo "0" > $plex_upgrade_result && config set plex_update_run=1 && config set plex_have_new=0 && config commit && return 0
}

plex_auto_update()
{
    [ `/bin/ps -w | grep "plexmediaserver_upgrade.sh" | grep "auto_update" | grep -v grep | wc -l` -gt 2 ] && return
    if [ "x`ls /tmp/plex_current_version 2>/dev/null`" = "x" ];then
        if [ "x`ls $plex_partition_mount_point/plex_current_version 2>/dev/null`" != "x" ];then
            cp  $plex_partition_mount_point/plex_current_version /tmp/
        fi
    fi
    while true
    do
        pid=`/bin/ps -w | grep "Plex Media Server" | grep -v grep`
        if [ "x$pid" != "x" ];then
            break
        else
            [ "x`ls $plex_partition_mount_point/*.*.* 2>/dev/null`" != "x" ] && sleep 30 && continue
            if [ "x`ls /tmp/$plex_filter_name 2>/dev/null`" != "x" ]
            then
                binary_backup=`ls /tmp/$plex_filter_name 2>/dev/null | awk -F "tmp/" '{print $2}'` 
                plex_upgrade $binary_backup
                continue
            fi
            plex_check 
            check_result=`cat $plex_check_result`
            [ $check_result -ne 0 ] && sleep 30 && continue 
            latest_version=`cat $plex_latest_version | head -1 |awk '{print $1}'`
            plex_download $latest_version
            download_result=`cat $plex_download_result`
            [ $download_result -ne 0 ] && sleep 30 && continue 
            plex_upgrade $latest_version
            upgrade_result=`cat $plex_upgrade_result`
            [ $upgrade_result -ne 0 ] && sleep 30 && continue 
        fi
    done
}

plex_update()
{
    [ `/bin/ps -w | grep "plexmediaserver_upgrade.sh" | egrep "init_update|manual_update|auto_sched_update" | grep -v grep | wc -l` -gt 3 ] && return
    [ "x$1" = "x1" -a "x`ls $plex_partition_mount_point/plex_current_version 2>/dev/null`" != "x" ] && return
	[ "x$1" = "x1" -a "x`config get plexmediaserver_enable`" != "x1" ] && config set plex_retry_flag=0 && config commit && return
	sleep 5
    plex_check 
    check_result=`cat $plex_check_result`
    if [ $check_result -ne 0 ];then
		[ "x`config get plex_update_run`" = "x2" ] && config set plex_update_run=-1 && return
		[ $check_result -eq 3 ] && return
		if [ "x$1" = "x1" ];then
			/etc/plexmediaserver/plexmediaserver_upgrade.sh retry_update 1 &
		else
			/etc/plexmediaserver/plexmediaserver_upgrade.sh retry_update &
		fi
		return
	fi
    have_new=`config get plex_have_new`
    [ "x$have_new" != "x1" ] && config set plex_retry_flag=0 && config set plex_update_run=-1 && return
    latest_version=`cat $plex_latest_version | head -1 |awk '{print $1}'`
    latest_url=`cat $plex_latest_version | head -1 |awk '{print $4}'`
    plex_download $latest_url
    download_result=`cat $plex_download_result`
    if [ $download_result -ne 0 ];then
		if [ "x$1" = "x1" ];then
			/etc/plexmediaserver/plexmediaserver_upgrade.sh retry_update 1 &
		else
			/etc/plexmediaserver/plexmediaserver_upgrade.sh retry_update &
		fi
		return
	fi	
    plex_upgrade $latest_version
    upgrade_result=`cat $plex_upgrade_result`
    if [ $upgrade_result -ne 0 ];then
		if [ "x$1" = "x1" ];then
			/etc/plexmediaserver/plexmediaserver_upgrade.sh retry_update 1 &
		else
			/etc/plexmediaserver/plexmediaserver_upgrade.sh retry_update &
		fi
		echo "plex update fail!"
		logger -- "[PLEX]Plex new binary upgrade fail,"
	else
		[ "x`config get plex_retry_flag`" != "x0" ] && config set plex_retry_flag=0 && config commit
		logger -- "[PLEX]Plex new binary upgrade success,"
	fi
    [ $upgrade_result -eq 0 -a "x$1" = "x1" ] && config set plex_auto_upgrade_enable=1 && config commit
}

#For plex update retry 3 times

dynamic_sleep_time(){
	sleep_t=10
	i=1
	while [ $i -lt $1 ]
	do
		sleep_t=`expr $sleep_t \* 2`
		i=`expr $i + 1`
	done
	echo $sleep_t
}

plex_update_retry(){
	[ `/bin/ps -w | grep "plexmediaserver_upgrade.sh" | grep "retry_update" | grep -v grep | wc -l` -gt 2 ] && return
	retry_times=`config get plex_retry_flag`
	if [ "x$retry_times" = "x0" -o "x$retry_times" = "x" ];then
		retry_times=1
		config set plex_retry_flag=$retry_times
	else
		retry_times=`expr $retry_times + 1`
		config set plex_retry_flag=$retry_times
	fi
	flag=1
	while true
	do
		if plex_check_network 1;then
			[ "x$flag" = "x1" ] && echo "Network disconnect,waiting for network connection..." && flag=
			sleep 20
			continue
		fi
		dynamic_sleep=$(dynamic_sleep_time $retry_times)
		[ $dynamic_sleep -gt 5120 ] && config set plex_retry_flag=10 && dynamic_sleep=5120
		echo "Network connect, sleep $dynamic_sleep second for plex retry upgrade"
		sleep $dynamic_sleep
		echo "Retry to update plex binary."
		if [ "x$1" = "x1" ];then
			/etc/plexmediaserver/plexmediaserver_upgrade.sh init_update &
			break
		else
			/etc/plexmediaserver/plexmediaserver_upgrade.sh manual_update &
			break
		fi
	done
}

plex_check_network(){
	retry=$1
	while true
	do
		net=`ping -c 3 www.netgear.com 2>/dev/null`
		sleep 3
		if [ "x$net" != "x" ];then
			break
		else
			sleep 5
		fi
		retry=`expr $retry + 1`
		[ $retry -gt 10 ] && return 0
	done
	return 1
}

plex_check_update(){
	[ `/bin/ps -w | grep "plexmediaserver_upgrade.sh" | grep "check_update" | grep -v grep | wc -l` -gt 2 ] && return
	update_state=`config get plex_update_run`
	if [ "x$update_state" = "x" -o "x$update_state" = "x-1" -o "x$update_state" = "x0" -o "x$update_state" = "x1" ];then
		if [ "x`config get plex_retry_flag`" != "x" -a "x`config get plex_retry_flag`" != "x0" ];then
			if [ "x`ls $plex_partition_mount_point/plex_current_version 2>/dev/null`" = "x" ];then
				/etc/plexmediaserver/plexmediaserver_upgrade.sh retry_update 1 &
			else
				/etc/plexmediaserver/plexmediaserver_upgrade.sh retry_update &
			fi
		fi
		return
	fi

	if [ "x$update_state" = "x2" -o "x$update_state" = "x3" ];then
		if [ "x`ls $plex_partition_mount_point/plex_current_version 2>/dev/null`" != "x" ];then
			retry=1
			if plex_check_network $retry;then
				echo "plex update failed."
				config set plex_update_run=0
				config commit
				return 1
			fi
			if [ "x`config get plex_have_new`" = "x1" ];then
				[ "x`/bin/ps -w | grep "plexmediaserver_upgrade.sh" | egrep "manual_update|auto_sched_update" | grep -v grep`" != "x" ] && echo "plex will be resum updated." && return 1
				echo "plex will be re updated."
				config set plex_update_run=-1
				config set plex_download_percent=0
				/etc/plexmediaserver/plexmediaserver_upgrade.sh manual_update &
			else
				config set plex_update_run=-1
				config commit
			fi
		else
			retry=1
			if plex_check_network $retry;then
				echo "plex update failed."
				config set plex_update_run=0
				config commit
				return 1
			fi
			if [ "x`config get plexmediaserver_enable`" = "x1" ];then
				[ "x`/bin/ps -w | grep "plexmediaserver_upgrade.sh" | grep "init_update" | grep -v grep`" != "x" ] && echo "plex will be resum updated." && return 1
				echo "plex will be re updated."
				config set plex_update_run=-1
				config set plex_download_percent=0
				/etc/plexmediaserver/plexmediaserver_upgrade.sh init_update &
			else
				config set plex_update_run=-1
				config commit
			fi
		fi
	elif [ "x$update_state" = "x4" ];then
		if [ "x`ls $plex_partition_mount_point/plex_current_version 2>/dev/null`" = "x" ];then
			rm -rf $plex_partition_mount_point/*
			retry=1
			if plex_check_network $retry;then
				echo "plex update failed."
				config set plex_update_run=0
				config commit
				return 1
			fi
			if [ "x`config get plexmediaserver_enable`" = "x1" ];then 
				[ "x`/bin/ps -w | grep "plexmediaserver_upgrade.sh" | egrep "init_update|manual_update|auto_sched_update" | grep -v grep`" != "x" ] && echo "plex will be resum updated." && return 1
				echo "plex will be re updated."
				config set plex_update_run=-1
				config set plex_download_percent=0
				/etc/plexmediaserver/plexmediaserver_upgrade.sh init_update &
			else
				config set plex_update_run=-1
				config commit
			fi
		else
			config set plex_update_run=1
			config commit
		fi
	else
		config set plex_update_run=-1
		config commit
	fi
}

plex_download_percent()
{
	[ `/bin/ps -w | grep "plexmediaserver_upgrade.sh" | grep "download_percent" | grep -v grep | wc -l` -gt 2 ] && return
	plex_download_state=`config get plex_update_run`
	[ "x$plex_download_state" = "x" -o "x$plex_download_state" = "x-1" ] && return
	[ "x$plex_download_state" = "x0" -o "x$plex_download_state" = "x1" ] && return
	total_size=`curl --insecure --connect-timeout 20 --keepalive-time 30 -I $1 2>/dev/null | grep "Content-Length" |awk '{print $2}'`
	[ "x$total_size" = "x" ] && total_size=94371840
	total_size=`expr $total_size \/ 1024 \/ 1024`
	while true
	do
		plex_download_state=`config get plex_update_run`
		[ "x$plex_download_state" = "x" -o "x$plex_download_state" = "x-1" ] && config set plex_download_percent=0 && break
		[ "x$plex_download_state" = "x0" -o "x$plex_download_state" = "x1" ] && config set plex_download_percent=0 && break
		[ "x$plex_download_state" = "x2" ] && config set plex_download_percent=0 && break
		if [ "x$plex_download_state" = "x4" ];then
			config set plex_download_percent=100
			sleep 3
			break
		elif [ "x$plex_download_state" = "x3" ];then
			cur_size=`du -sm /tmp/$plex_filter_name 2> /dev/null| awk '{print $1}'`
			[ "x$cur_size" = "x" ] && cur_size=0
			[ $cur_size -lt 0 ] && cur_size=0
			[ $cur_size -gt $total_size ] && cur_size=$total_size
			percent=`expr $cur_size \* 100 \/ $total_size`
			[ $percent -lt 0 ] && percent=0
			[ $percent -gt 100 ] && percent=100
			config set plex_download_percent=$percent
			[ "x$plex_download_percent" = "x" ] && plex_download_percent=0
			sleep 5
			continue
		fi
	done
	config set plex_download_percent=0
	config commit
}

#Through rate, it is possible for your main Plex Media Server database become corrupted.
#In such solution, you can delete the bad database Library.
plex_library_reset()
{
	if [ "x`config get plexmediaserver_enable`" = "x1" ];then
	    #Stop Plex Media Server before reset library
	    echo "Stop Plex Media Server before reset library."
	    $plex_init_script_path stop 1
	    [ "x`/bin/ps -w | grep "Plex" | grep -v grep`" != "x" ] && {
	        echo "Stop Plex Media Server fail, will force kill Plex."
	        /bin/ps -w | grep "Plex" | grep -v plexmediaserver | grep -v grep | awk '{print $1}' | xargs kill -9 2>/dev/null
	    }
	fi
	#Delete Plex Library in selected USB Drive
	rm -rf /tmp/plex_reset_result
	local plex_library_path=`config get plex_select_usb |awk -F"," '{print $2}'`
	[ "x$plex_library_path" = "x" ] && echo "No selected Drive for plex,delete Library fail." && echo 1 > /tmp/plex_reset_result && return 1 
	sd=`mount |grep "$plex_library_path" |grep "/dev/sd" |awk '{print $1}' |awk -F"/" '{print $3}'`
	[ ! -d $plex_library_path -o "x`vol_id -u /dev/$sd 2>/dev/null`" = "x" ] && echo "Selected Drive not exist or USB Drive broken" && echo 2 > /tmp/plex_reset_result && return 1 
	rm -rf $plex_library_path/Library/Application\ Support/Plex\ Media\ Server/Plug-in\ Support/Databases/ 2>/dev/null
	if [ `echo $?` -eq 0 ];then
	    echo "Remove Library Database successful."
		echo 0 > /tmp/plex_reset_result
	else
	    echo "Remove Library Database failed"
	    #test filesystem can write ?
	    right=`mount | grep "$plex_library_path" | awk -F"(" '{print $2}' |awk -F"," '{print $1}' 2>/dev/null`
	    if [ "x$right" = "xro" ];then
	        echo "Filesystem can't write..." > /dev/console
	        echo 3 > /tmp/plex_reset_result
	    else
	        echo "Your library drive have some unknow error, please replace a new drive and try again..." > /dev/console
	        echo 4 > /tmp/plex_reset_result
	    fi
	fi
	[ "x`config get plexmediaserver_enable`" = "x1" ] && $plex_init_script_path start
}

case "$1" in
    check)
        plex_check $2
    ;;
    download)
        plex_download $2
    ;;
    upgrade)
        plex_upgrade $2
    ;;
    auto_update)
        plex_auto_update
    ;;
    init_update)
        plex_update 1 
    ;;
    auto_sched_update)
        plex_update
    ;;
    manual_update)
        plex_update
    ;;
    check_update)
        plex_check_update
    ;;
    download_percent)
        plex_download_percent $2
    ;;
    retry_update)
        plex_update_retry $2
    ;;
    plex_reset)
        plex_library_reset
    ;;
esac

