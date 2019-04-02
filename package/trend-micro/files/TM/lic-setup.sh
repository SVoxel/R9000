#!/bin/sh

PID_FILE=gen_lic.pid
MON_INTL=5

run_lic()
{
  cmd="$1"
  [ -z "$cmd" ] && cmd="start"

  case "$cmd" in
    start)
        echo "Start license control daemon..."
        LD_LIBRARY_PATH=. ./gen_lic
        ;;
    stop)
        echo "Stop license control daemon..."
        killall -INT gen_lic
        ;;
  esac
}

# program monitor # 
while [ true ];
do
  if [ ! -e $PID_FILE -o ! -e /proc/`cat $PID_FILE` ]; then 
    run_lic
  fi

  sleep $MON_INTL;
done
