#!/bin/sh

# This file is expected to be called by /lib/wifi/qcawifi.sh

#
# Adopt different sets of Wi-Fi boarddata files (BDF) for different wireless
# countries/regions.
#
# This function is expected to be changed frequently, so it is left here, not
# in /lib/wifi/qcawifi.sh
#
prepare_bdf_qcawifi() {
	local device="$1"

	local regulatory_domain=FCC_CE
	local qca9984_bdf_dir=/lib/firmware/QCA9984/hw.1

	config_get country "$device" country
	case "$country" in
	5000|AU)
		regulatory_domain=AU
		;;
	5001|CA)
		regulatory_domain=Canada
		;;
	156|CN)
		regulatory_domain=PR
		;;
	4015|JP)
		regulatory_domain=Japan
		;;
	esac

	if [ -d $qca9984_bdf_dir/$regulatory_domain ]; then
		/bin/cp -f $qca9984_bdf_dir/$regulatory_domain/* $qca9984_bdf_dir/
	else
		/bin/cp -f $qca9984_bdf_dir/FCC_CE/* $qca9984_bdf_dir/
	fi

	sync
}
