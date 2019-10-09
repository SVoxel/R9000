#!/bin/sh

while true
do
	genlic=`ps -w | grep gen_lic | grep -v gen_lic`
	lickey=`ls /tm_key/ | grep license.key`
	licbak=`ls /tm_key/ | grep lic_bak.key`
	iqos_status=`cat /proc/bw_dpi_conf |grep Available| cut -d : -f 2`
	if [ "$iqos_status" = " 00000083" ]; then
		lickey=`ls /tm_key/ | grep license.key`
		licbak=`ls /tm_key/ | grep lic_bak.key`
		if [ "x$lickey" != "x" ] && [ "x$licbak" != "x" ]; then
			md5sum /tm_key/license.key > /tm_pattern/keymd5
			md5sum /tm_key/lic_bak.key >> /tm_pattern/keymd5
			cp -r /tm_pattern/keymd5 /etc/config/
			cp -r /tm_key/license.key /etc/config/
			cp -r /tm_key/lic_bak.key /etc/config/
			exit
		fi
	else
		echo "iqos generate license fail ,wait retry !" >/dev/console
	fi

	sleep 300
done
