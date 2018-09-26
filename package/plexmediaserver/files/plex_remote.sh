#!/bin/sh

PATH=/bin:/sbin:/usr/bin:/usr/sbin

old_port=""
new_port=""

plex_remote_start()
{
while true 
do
    [ "x`ps -w | grep "Plex Media Server" | grep -v grep`" = "x" ] && exit
    xml_path=`config get plex_file_path`
    cat $xml_path/"Library/Application Support/Plex Media Server"/Preferences.xml > /tmp/xml.txt
    content=`cat /tmp/xml.txt`
    new_port=`echo ${content#*ManualPortMappingPort=} |awk -F" " {'print $1'} |grep -o '[0-9]\+'`
    if [ "$new_port" != "$old_port" ];then
        content=`cat /tmp/xml.txt`
        ip=`config get lan_ipaddr`
        old_port=$new_port
        echo "YES;TCP;32400;$new_port;$ip;Plex Media Server; @#$&*!" > /tmp/plex_rule
        net-wall restart
    else
        sleep 2
    fi
    sleep 1
done
}

case "$1" in
    start)
        ps -w | egrep "plex_remote.sh start" | egrep -v "egrep" | awk '{print $1}' | grep -v $$ | xargs kill -9 2>/dev/null
        plex_remote_start
        ;;
esac
