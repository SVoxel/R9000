#!/bin/sh

CONFIG=/bin/config
board_model_id_flag="$(/sbin/artmtd -r board_model_id | awk -F":" '{print $2}')"

#When board_model_id_flag on HW board data area is R9000
if [ "x$board_model_id_flag" != "xR8900" ]; then
	echo "R9000" > /tmp/hardware_version
	echo "R9000" > /hardware_version
	echo "R9000" > /tmp/module_name
	echo "R9000" > /module_name

	if [ "x$($CONFIG get board_region_default)" = "x1" ]; then
		/bin/config set wan_hostname="R9000"
		/bin/config set netbiosname="R9000"
		/bin/config set upnp_serverName="R9000"
		/bin/config set Device_name="R9000"

	fi

	/bin/config set bridge_netbiosname="R9000"
	/bin/config set ap_netbiosname="R9000"

	# miniupnp configure
	/bin/config set miniupnp_devupc="606449084528"
	/bin/config set miniupnp_friendlyname="NETGEAR R9000 Wireless Router"
	/bin/config set miniupnp_modelname="NETGEAR Nighthawk X10 AD7200 Smart WiFi Router"
	/bin/config set miniupnp_modelnumber="R9000"
	/bin/config set miniupnp_modelurl="http://www.netgear.com/home/products/wirelessrouters"
	/bin/config set miniupnp_modeldescription="NETGEAR R9000 RangeMax X10 AD7200 Wireless Router"
	/bin/config set miniupnp_pnpx_hwid="VEN_01f2&amp;DEV_0027&amp;REV_01 VEN_01f2&amp;DEV_8000&amp;SUBSYS_01&amp;REV_01 VEN_01f2&amp;DEV_8000&amp;REV_01 VEN_0033&amp;DEV_0008&amp;REV_01"

	#difference in net-cgi
	/bin/config set cgi_module_id="R9000"
	/bin/config set cgi_ctl_mod="r9000"
	
	#set wireless speed model
	/bin/config unset defined_mode_2
	/bin/config unset defined_mode_3

	# madwifi_scripts
	ATH_TMP=/tmp/etc/ath
	ATH_ORI=/etc/ath.orig
	[ ! -d $ATH_TMP ] && mkdir -p $ATH_TMP && cp -a $ATH_ORI/* $ATH_TMP
	sed -i 's/wsc_manufactuer=.*/wsc_manufactuer="NTGR"/g' $ATH_TMP/board.conf
	sed -i 's/wsc_model_name=.*/wsc_model_name="R9000"/g' $ATH_TMP/board.conf
	sed -i 's/wsc_model_number=.*/wsc_model_number="V1"/g' $ATH_TMP/board.conf

fi

#When board_model_id_flag on HW board data area is R8900
if [ "x$board_model_id_flag" = "xR8900" ]; then
	echo "R8900" > /tmp/hardware_version
	echo "R8900" > /hardware_version
	echo "R8900" > /tmp/module_name
	echo "R8900" > /module_name

	if [ "x$($CONFIG get board_region_default)" = "x1" ]; then
		/bin/config set netbiosname="R8900"
		/bin/config set wan_hostname="R8900"
		/bin/config set upnp_serverName="R8900"
		/bin/config set Device_name="R8900"
	fi

	/bin/config set bridge_netbiosname="R8900"
	/bin/config set ap_netbiosname="R8900"

	# miniupnp configure
	/bin/config set miniupnp_devupc="606449084528"
	/bin/config set miniupnp_friendlyname="NETGEAR R8900 Wireless Router"
	/bin/config set miniupnp_modelname="RangeMax X10 AD7000 Wireless Router"
	/bin/config set miniupnp_modelnumber="R8900"
	/bin/config set miniupnp_modelurl="http://www.netgear.com/home/products/wirelessrouters"
	/bin/config set miniupnp_modeldescription="NETGEAR R8900 RangeMax X10 AD7000 Wireless Router"
	/bin/config set miniupnp_pnpx_hwid="VEN_01f2&amp;DEV_0027&amp;REV_01 VEN_01f2&amp;DEV_8000&amp;SUBSYS_01&amp;REV_01 VEN_01f2&amp;DEV_8000&amp;REV_01 VEN_0033&amp;DEV_0008&amp;REV_01"

	#differnece in net-cgi
	/bin/config set cgi_module_id="R8900"
	/bin/config set cgi_ctl_mod="r8900"

	#set wireless speed model
	/bin/config set defined_mode_2="289"
	/bin/config set defined_mode_3="600"

	# madwifi_scripts
	ATH_TMP=/tmp/etc/ath
	ATH_ORI=/etc/ath.orig
	[ -d $ATH_TMP ] || mkdir -p $ATH_TMP && cp -a $ATH_ORI/* $ATH_TMP
	sed -i 's/wsc_manufactuer=.*/wsc_manufactuer="NTGR"/g' $ATH_TMP/board.conf
	sed -i 's/wsc_model_name=.*/wsc_model_name="R8900"/g' $ATH_TMP/board.conf
	sed -i 's/wsc_model_number=.*/wsc_model_number="V1"/g' $ATH_TMP/board.conf
fi
