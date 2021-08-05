#!/bin/sh

# Collect basi debug information

out_fqn=/tmp/basic_debug_log.txt

display_command() { 
	echo "[ $1 ]"  >> $out_fqn
	eval "$1"      >> $out_fqn 2>> $out_fqn
	echo -e "\n"   >> $out_fqn
}

while [ 1 ]
do
	
while read LINE; do
    display_command "$LINE"
done << EOF
ifconfig
cat /proc/meminfo
ps ww
free 
cat /etc/resolv.conf
cat /proc/net/arp
top -b | head -n 20
mpstat -P ALL
EOF

	sleep 300

	filesize=`ls -l $out_fqn | awk '{print $5}'`
	if [ $filesize -ge 1048576  ]; then
		echo "filesize if over, rm basic_debug_log.txt"
		rm -rf $out_fqn
	fi
done

