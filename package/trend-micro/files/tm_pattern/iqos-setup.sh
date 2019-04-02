#!/bin/sh

cmd=$1
iqos_conf=qos.conf

cd $(dirname $0)
sample_bin=$(pwd)/sample.bin
cd -
iqos_setup=$(basename $0)

if [ -z "$sample_bin" -o  -z "$iqos_setup" ]; then
	echo "wrong cmd path"
	exit 1
fi

if [ -z "$cmd" ]; then
	echo "$0 start|stop|restart"
	exit 1
fi

case "$cmd" in 
start)
	echo "Start iQoS..."
	if [ -x ./tcd ]; then
		./tcd &
	fi
	sleep 3
	$sample_bin -a set_qos_on
	;;
stop)
    echo "Stop iQoS..."
	$sample_bin -a set_qos_off
	sleep 3
	killall -9 tcd
	;;
restart)
	$0 stop
	$sample_bin -a set_qos_conf -R $iqos_conf
	$0 start
	;;
*)
	echo "$0 start|stop|restart"
	;;
esac
