#!/bin/sh

# 2018-12-20
# Add this script for RAX120 to collect crash log by "ramoops"
# - This script will append new crash log collected by "ramoops"
#   to mtd partition "mtdoops"

#crash_mtd=$(/usr/sbin/part_dev crashinfo)
crash_mtd=/dev/mtd_crashdump
ramoops_dir="/tmp/ramoops"
pstore_dir="$ramoops_dir/pstore"
crash_log="$ramoops_dir/crash_log"
dump_file="$ramoops_dir/mtd_dump"
dump_file_tmp="${dump_file}_tmp"

# Crash Log Partition: /dev/mtd29 - crashinfo
# size: 512KB, 4 blocks
# - 2 blocks reserved for bad block
# - 2 blocks used for storing crash log
#   -- crash log size 32K, can sotre 256/32 = 8 times crash log
partition_size=262144 # 256 Kbytes
crash_log_size=32768  # 32 Kbytes

print_log() {
	echo "$@" > /dev/console
}

# check crash log size to aviod it exceed crash_log_size
check_crash_log() {
	log_size=$(wc -c $crash_log | awk '{print $1}')
	if [ $log_size -gt $crash_log_size ]; then
		crash_log_bak="${crash_log}_bak"
		cp $crash_log $crash_log_bak

		dd if=$crash_log_bak of=$crash_log bs=$crash_log_size count=1
		rm $crash_log_bak
	fi
}

# append new crash log to flash
sync_crash_log_to_flash() {
	print_log "[dni_ramoops_init] === sync_crash_log_to_flash start ==="

	offset=0
	target_offset=0
	max_count=$(($partition_size/$crash_log_size - 1))

	nanddump -q -l $partition_size -f $dump_file $crash_mtd

	for i in $(seq 0 $max_count)
	do
		offset=$(($i * $crash_log_size))
		head_two_bytes=$(hexdump -s $offset -n 2 $dump_file | sed -n '1p' | awk '{print $2}')
		if [ "$head_two_bytes" = "ffff" ]; then
			target_offset=$offset
			break
		fi
	done

	# append new crash log to dump file (old crash log)
	if [ $target_offset -eq $offset ]; then
		dd if=$crash_log of=$dump_file bs=1 seek=$target_offset conv=notrunc
	else
		# If "$target_offset" is not equal "$offset",
		# it means old crash log already reaches the MAX count.
		# Need to append new crash log in the end and cut the first crash log.

		target_offset=$(($partition_size - $crash_log_size))
		dd if=/dev/zero bs=$partition_size count=1 | tr "\000" "\377" > $dump_file_tmp

		# cut first crash log
		dd if=$dump_file of=$dump_file_tmp bs=1 skip=$crash_log_size conv=notrunc
		# append new crash log in the end
		dd if=$crash_log of=$dump_file_tmp bs=1 seek=$target_offset conv=notrunc

		mv $dump_file_tmp $dump_file
	fi

	print_log "[dni_ramoops_init] === erase $crash_mtd ==="
	flash_erase $crash_mtd 0 0

	print_log "[dni_ramoops_init] === write crash log to $crash_mtd ==="
	# write dump file (with new crash log) back to mtd device
	nandwrite -m -p -q $crash_mtd $dump_file

	rm $dump_file
	print_log "[dni_ramoops_init] === sync_crash_log_to_flash done ==="
}

ramoops_init() {
	# mount pstore to get ramoops logs
	mkdir -p $ramoops_dir
	mkdir -p $pstore_dir
	mount -t pstore pstore $pstore_dir

	msg_file=$(ls $pstore_dir)
	if [ -n "$msg_file" ]; then
		cat $pstore_dir/dmesg-ramoops-1 | sed '1,2d' >> $crash_log
		rm $pstore_dir/*

		[ ! -f $crash_log ] && return

		print_log "[dni_ramoops_init] === crash log detected in ramoops ==="
		check_crash_log
		sync_crash_log_to_flash
	fi
}

ramoops_init
