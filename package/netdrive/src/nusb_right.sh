#!/bin/sh

n_path=`config get usb_path`
n_folder=`config get sync_local_folder`
if [ "x$n_path" = "x" -o "x$n_folder" = "x" ]; then
    [ "x$n_path" = "x" ] && echo "your disk is not umount and not access" > /dev/console
    [ "x$n_folder" = "x" ] && echo "your folder is not select" > /dev/console
    logger -- "[Cloud Backup]Your notify disk is umount"
    exit 0 
fi
if [ "x$n_path" != "x" -a "x$n_folder" != "x" ]; then
    mount |grep $n_path |awk -F "(" '{print $2}' |awk -F "," '{print $1}' > /tmp/drive/usb_right
    usb_right=`cat /tmp/drive/usb_right`
    echo "[Cloud Backup]usb right is $usb_right"
fi
