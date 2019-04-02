#!/bin/sh

# Collect basi debug information
filepath=/tmp
filename=/tmp/debug_here_log_1.txt
LBD="telnet 127.0.0.1 7787"

while [ 1 ]
do

echo "======================Debug level update======================" >> $filename    
echo "-------------(echo "dbg level stamon dump")| $LBD-------------" >> $filename
	(echo "dbg level stamon dump"; sleep 2)| $LBD >> $filename 
echo "-------------(echo "dbg level bandmon dump")| $LBD-------------" >> $filename
	(echo "dbg level bandmon dump"; sleep 2)| $LBD >> $filename 
    
echo "======================Run time check======================" >> $filename
date=`date`
echo "-----------------------$date----------------------" >> $filename
echo "-----------------------dbg here----------------------" >> $filename
echo "-------------(echo "dbg here"; sleep 300)| $LBD-------------" >> $filename
	(echo "dbg here"; sleep 300)| $LBD >> $filename 

echo "-----------------------stadb s----------------------" >> $filename
	(echo "stadb s")| $LBD >> $filename 
echo "========================next loop==================================" >> $filename


	backup_number=2
	current_file_size=`ls -l $filename | awk '{print $5}'`
	current_total_size=`ls -l $filepath | awk '/debug_here/ {SUM += $5} END {print SUM}'`
	single_size_limit=5000000
	total_size_limit=`awk "BEGIN {print $single_size_limit*$backup_number}"`


	if [ $current_file_size -ge $single_size_limit ]; then
	    echo "filesize is over, redirect basic_debug_log_bak.txt"
	    filename=/tmp/debug_here_log_2.txt
	    [ -f $filename  ] && [ $current_total_size -ge $total_size_limit ] && {
		rm $filename
	    }
	    if [ $current_total_size -ge $total_size_limit ]; then
		echo "backup filesize is over, redirect original file,and rm previous file"
		filename=/tmp/debug_here_log_1.txt
		rm $filename
	    fi
	fi
	
done

