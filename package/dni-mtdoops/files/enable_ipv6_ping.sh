#!/bin/sh

#Enable telnet
ping6_enable()
{
	ipv6_type=`/bin/config get ipv6_type`
	if [ "$1" == "pppoe" ]; then
		wan_if="ppp0"
	else
		wan_if=`/bin/config get wan_ifname`
	fi
	if [ "$1" = "start" ];then
		/usr/sbin/ip6tables -t mangle -D PREROUTING -p icmpv6 --icmpv6-type 128 -i $wan_if -j DROP
	else
		/usr/sbin/ip6tables -t mangle -D PREROUTING -p icmpv6 --icmpv6-type 128 -i $wan_if -j DROP
		/usr/sbin/ip6tables -t mangle -I PREROUTING 1 -p icmpv6 --icmpv6-type 128 -i $wan_if -j DROP
	fi
}

ping6_enable $1
