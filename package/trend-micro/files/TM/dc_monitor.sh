#!/bin/sh

COLL_INTL=1800
CFG_POLL_INTL=43200

PID_FILE=data_colld.pid
MON_INTL=5

run_dc()
{
  cmd="$1"
  [ -z "$cmd" ] && cmd="start"

  case "$cmd" in
    start)
        echo "Start data collection daemon..."
        LD_LIBRARY_PATH=. ./data_colld -i $COLL_INTL -p $CFG_POLL_INTL -b # -v 
        ;;
    stop)
        echo "Stop data collection daemon..."
        killall -9 data_colld
        ;;
  esac
}

# program monitor # 
while [ true ];
do
  if [ ! -e $PID_FILE ]; then
    run_dc start
  elif [ ! -e /proc/`cat $PID_FILE`/status ]; then 
    run_dc start
  fi

  sleep $MON_INTL;
done

