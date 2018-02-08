#!/bin/sh
cmd="$1";
[ -z "$cmd" ] && cmd="start"

dev_wan=brwan
qos_wan=brwan

ppp_status=`cat /tmp/ppp/ppp0-status`
if [ "$ppp_status" = "1" ] ; then
	dev_wan=ppp0
	qos_wan=ppp0
fi

qos_lan=br0
sess_num=30000
user_timeout=`expr 60 \* 60`
app_timeout=`expr 60 \* 60`

udb_param="dev_wan=$dev_wan"
udb_param="$udb_param qos_wan=$qos_wan"
udb_param="$udb_param qos_lan=$qos_lan"
udb_param="$udb_param sess_num=$sess_num"
udb_param="$udb_param user_timeout=$user_timeout"
udb_param="$udb_param app_timeout=$app_timeout"

idp_mod=tdts.ko
udb_mod=tdts_udb.ko
fw_mod=tdts_udbfw.ko
rule=rule.trf
agent=tdts_rule_agent
NTPCLIENT=ntpclient
NTPDATE=ntpdate
LIGHTTPD=lighttpd
LIC_FOLDER=tm_key
PTN_FOLDER=tm_pattern
lic_ctrl=gen_lic

dev=/dev/detector
dev_maj=190
dev_min=0

fwdev=/dev/idpfw
fwdev_maj=191
fwdev_min=0

wred_setup=wred-setup.sh
iqos_setup=iqos-setup.sh

case "$cmd" in
start)
	echo "in `pwd`"

	if [ ! -f "$rule" ]; then
		echo "Signature file $rule not found"
		exit 1
	fi

	# sync ntp                                                               
	if `command -v $NTPCLIENT >/dev/null 2>&1` ; then
		$NTPCLIENT -h time.stdtime.gov.tw -s
		echo "$NTPCLIENT -h time.stdtime.gov.tw -s";
	else
		echo "$NTPDATE time.stdtime.gov.tw" ;
		$NTPDATE time.stdtime.gov.tw
	fi

	# create dev node
	echo "Creating device nodes..."
	[ ! -c "$dev" ] && mknod $dev c $dev_maj $dev_min
	[ ! -c "$fwdev" ] && mknod $fwdev c $fwdev_maj $fwdev_min
	test -c $dev || echo "...Creat $dev failed"
	test -c $fwdev || echo "...Create $fwdev failed"

	#echo "Filter WAN bootp packets..."
	#chain=BWDPI_FILTER
	#iptables -t mangle -N $chain
	#iptables -t mangle -F $chain
	#iptables -t mangle -A $chain -i $dev_wan -p udp --sport 68 --dport 67 -j DROP
	#iptables -t mangle -A $chain -i $dev_wan -p udp --sport 67 --dport 68 -j DROP
	#iptables -t mangle -A PREROUTING -i $dev_wan -p udp -j $chain
	# insmod QoS module
	insmod cls_u32
	insmod cls_fw
	insmod sch_htb
	insmod sch_sfq
	echo "Insert IDP engine..."
	insmod ./$idp_mod || exit -1

	echo "Insert UDB ($udb_param)..."
	insmod ./$udb_mod $udb_param || exit 1

	echo "Insert forward module..."
	insmod ./$fw_mod || exit 1

	if [ -d "/${LIC_FOLDER}" ]; then
		echo "Running license control..Please Wait 25 sec...."
		cd /tm_key
		./lic-setup.sh &
		cd /TM
		sleep 25
	fi

	# start iqos
	if [ -x ./$iqos_setup ]; then
		./$iqos_setup restart
	fi

	# start dc
	if [ -x ./dc_monitor.sh ]; then
		./dc_monitor.sh &
	fi
		
	# start wrs
	if [ -x ./$wred_setup ]; then
		./$wred_setup &
		./wred_set_conf -f wred.conf
	fi
	
	# start demo gui
	if [ -d "${LIGHTTPD}" ]; then
		cd lighttpd
		./setup.sh > /dev/null 2>&1 &
		cd -
	fi

	echo "Running rule agent to setup signature file $rule..."
	if [ -d "/${PTN_FOLDER}" ]; then
		cd /tm_pattern
		./$agent -g
		cd /TM
	fi
	# clean cache
	if [ -x ./clean-cache.sh ]; then
		echo "Running clean-cache.sh..."
		./clean-cache.sh > /dev/null 2>&1 &
	fi


	;;
stop)
	# stop clean cache
	killall -9 clean-cache.sh
	killall -9 lic-setup.sh
	killall -9 gen_lic
	# stop lighttpd
	killall -9 lighttpd
	
	# stop ui_dc
	killall -9 dc_setup.sh
	killall -9 ui_data_colld
	
	# stop iqos
	if [ -x ./$iqos_setup ]; then
		./$iqos_setup stop
	fi

	# stop wrs
	killall -9 wred-setup.sh
	killall -9 wred

	# stop dc
	killall -9 dc_monitor.sh
	killall -9 data_colld

	echo "Unload engine..."
	rmmod $fw_mod > /dev/null 2>&1
	rmmod $udb_mod > /dev/null 2>&1
	rmmod $idp_mod > /dev/null 2>&1
	rmmod sch_htb
	rmmod sch_sfq
	rmmod cls_fw
	rmmod cls_u32
	echo "Remove device nodes..."
	[ -c "$dev" ] && rm -f $dev 
	[ ! -c "$dev" ] || echo "...Remove $dev failed"
	[ -c "$fwdev" ] && rm -f $fwdev
	[ ! -c "$fwdev" ] || echo "...Remove $fwdev failed"

	# Clear the conntrack entry
	#DELAY=1;
	#orig=$(cat /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_established);
	#echo "orig=$orig"
	#echo $DELAY > /proc/sys/net/ipv4/netfilter/ip_conntrack_tcp_timeout_established;
	#sleep $DELAY;
	
	;;
restart)
	$0 stop
	sleep 2
	$0 start
	;;
esac;

