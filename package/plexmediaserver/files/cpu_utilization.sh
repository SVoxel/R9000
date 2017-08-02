#!/bin/sh

cpu_usage_file="/tmp/cpu_usage_file"
top_usage_file="/tmp/top_usage_file"
top_usage_tmp_file="/tmp/top_usage_tmp_file"
top_min_width="  PID USER     STATUS   NI   RSS  PPID %CPU %MEM COMMAND"

cpu_usage()
{
    rm -rf $cpu_usage_file
    echo "CPU %usage" > $cpu_usage_file
    mpstat -P ALL | sed '1,3d' | awk '{print $2,100-$12}' >> $cpu_usage_file
    echo "" >> $cpu_usage_file
    free | sed -n '1p' | awk '{print $1,$2,$3}' >> $cpu_usage_file
    free | sed -n '4p' | awk '{print $2,$3,$4}' >> $cpu_usage_file
}

top_usage()
{
    rm -rf $top_usage_file
    top_cut_off_pid=`echo "$top_min_width" | awk -F " USER" '{print $1}'| wc -c`
    top_cut_off_ni_1=`echo "$top_min_width" | awk -F "  NI" '{print $1}'| wc -c`
    top_cut_off_ni_2=`echo "$top_min_width" | awk -F "   RSS" '{print $1}'| wc -c`
    top_cut_off_ppid=`echo "$top_min_width" | awk -F "%CPU" '{print $1}'| wc -c`
    top -bn 1 | sed '1,2d' | cut -b 1-$top_cut_off_pid,$top_cut_off_ni_1-$top_cut_off_ni_2,$top_cut_off_ppid- > $top_usage_tmp_file
    cat $top_usage_tmp_file | head -1 > $top_usage_file
    if [ "x$1" = "x" ];then
        cat $top_usage_tmp_file | sed '1d' | sed '$d' | sort -k3nr >> $top_usage_file
    else
        cat $top_usage_tmp_file | sed '1d' | sed '$d' | sort -k3nr | head -$1 >> $top_usage_file
    fi
    rm -rf $top_usage_tmp_file
}

case "$1" in
    cpu)
        cpu_usage
    ;;
    top)
        top_usage $2
    ;;
esac

