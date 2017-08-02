#!/bin/sh

plex_monitor_current_pid=$$
plex_monitor_current_name=`echo $0 | awk -F "/" '{print $NF}'`
plex_montior_operation=""
irq_change_value="8"
irq_backup_file="/tmp/irq_backup_file"

irq_backup()
{
    rm -rf $irq_backup_file
    for i in `ls /proc/irq/`
    do
        if [ -d /proc/irq/$i ];then
            value=`cat /proc/irq/$i/smp_affinity`
            echo "$i $value" >> $irq_backup_file
        fi
    done
}

irq_change()
{
    [ "x`config get plex_monitor`" = "x1" ] && return 
    if [ -f $irq_backup_file ];then
        num=`wc -l $irq_backup_file | awk '{print $1}'`
        i=0
        while [ $i -lt $num ]
        do
            i=`expr $i + 1`
            irq=`cat $irq_backup_file | awk '{print $1}' | sed -n "${i}p"`
            value="$irq_change_value"
            echo $value > /proc/irq/$irq/smp_affinity
        done
    fi
    config set plex_monitor=1
}

irq_restore()
{
    [ "x`config get plex_monitor`" != "x1" ] && return 
    if [ -f $irq_backup_file ];then
        num=`wc -l $irq_backup_file | awk '{print $1}'`
        i=0
        while [ $i -lt $num ]
        do
            i=`expr $i + 1`
            irq=`cat $irq_backup_file | awk '{print $1}' | sed -n "${i}p"`
            value=`cat $irq_backup_file | awk '{print $2}' | sed -n "${i}p"`
            echo $value > /proc/irq/$irq/smp_affinity
        done
    fi
    config set plex_monitor=0
}

plex_monitoring()
{
    if [ "x`/bin/ps -w | egrep "Plex Media Scan|Plex Transcoder|Plex New Transc" | egrep -v egrep`" = "x" ];then
			plex_montior_operation="stop"
	else
		double=1
		while [ $double -le 2 ]
		do
			cpu_utilization=`mpstat -P ALL |sed '1,4d' |head -3 |awk '{print $12}'`
			cpu_utilization_t=`echo $cpu_utilization |awk '{print $1+$2+$3}'`
			cpu_utilization=${cpu_utilization_t%.*}
			cpu_utilization=`expr 300 - $cpu_utilization`
			if [ $cpu_utilization -gt 210 ];then
				[ "x$double" = "x1" ] && sleep 5 && double=`expr $double + 1` && continue
				[ "x$double" = "x2" ] && double=3 && break
			else
				break	
			fi
		done
		if [ "x$double" = "x3" ];then
			plex_montior_operation="start"
		else
			plex_montior_operation="stop"
		fi
	fi
    #Set different value to plex_process_run,plex gui will warning different information.
    #value=0 means plex start failed,value=1 means plex start successful,value=2 means cannot get ntp time,value=3 means plex process crash.
    [ "x`/bin/ps -w | grep "Plex Media Server" | grep -v grep`" = "x" -a "x`config get plexmediaserver_enable`" = "x1" ] && config set plex_process_run=3 && plex_montior_operation="" && logger -- "[PLEX]You Plex Media Server is crash for unknow issue, please re-enable it,"
    [ "x$plex_montior_operation" = "x" ] && irq_restore && exit
    [ "$plex_montior_operation" = "start" ] && irq_change
    [ "$plex_montior_operation" = "stop" ] && irq_restore
}

plex_monitor_start()
{
    while true
    do
        plex_monitoring
        sleep 5
    done
}

plex_monitor_stop()
{
    /bin/ps -w | egrep "$plex_monitor_current_name start|$plex_monitor_current_name stop" | egrep -v "egrep" | awk '{print $1}' | grep -v "$plex_monitor_current_pid" | xargs kill -9 2>/dev/null
    plex_monitoring
}

case "$1" in
    start)
        /bin/ps -w | egrep "$plex_monitor_current_name start" | egrep -v "egrep" | awk '{print $1}' | grep -v "$plex_monitor_current_pid" | xargs kill -9 2>/dev/null
        plex_monitor_start
    ;;
    stop)
        plex_monitor_stop
    ;;
    backup)
        irq_backup
    ;;
esac

