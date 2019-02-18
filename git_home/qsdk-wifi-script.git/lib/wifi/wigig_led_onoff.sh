#!/bin/sh

. /etc/ath/wifi.conf

init_led_mac80211() {
    local phy=""

    # "lookup_phy" should be defined in /lib/wifi/mac80211.sh
    lookup_phy "$wig_device"
    if [ -z "$phy" ]; then
        phy="phy0"
    fi

    # Configure 11ad LED to low active
    WIGIG_LED_INIT="echo 0 > /sys/kernel/debug/ieee80211/${phy}/wil6210/led_polarity"

    eval ${WIGIG_LED_INIT}
}

on_led_mac80211() {
    local radio_num=$1
    local security_type=$2
    local radio_type=$3
    local led_option=$4

    local phy=""

    # "lookup_phy" should be defined in /lib/wifi/mac80211.sh
    lookup_phy "$wig_device"
    if [ -z "$phy" ]; then
        phy="phy0"
    fi

    TRAFFIC_11AD_ON="echo 2 > /sys/kernel/debug/ieee80211/${phy}/wil6210/led_cfg" # enable 11ad led, due to wil6210 parameter is led_id=2, so echo 2 to led_cfg
    TRAFFIC_11AD_OFF="echo 0 > /sys/kernel/debug/ieee80211/${phy}/wil6210/led_cfg"
    BLINK_11AD_ON="echo 10000 0 100 100 80 80 > /sys/kernel/debug/ieee80211/${phy}/wil6210/led_blink_time"
    BLIN_11ADK_OFF="echo 10000 0 10000 0 10000 0 > /sys/kernel/debug/ieee80211/${phy}/wil6210/led_blink_time"


    if [ "$radio_type" = "11ad" ]; then
        if [ "$led_option" = "0" ]; then
            eval ${BLINK_11AD_ON}
            eval ${TRAFFIC_11AD_ON}
        elif [ "$led_option" = "1" ]; then
            eval ${BLINK_11AD_OFF}
            eval ${TRAFFIC_11AD_ON}
        elif [ "$led_option" = "2" ]; then
            eval ${BLINK_11AD_OFF}
            eval ${TRAFFIC_11AD_OFF}
        fi
    elif [ "$radio_type" = "none" ]; then
        eval ${BLINK_11AD_OFF}
        eval ${TRAFFIC_11AD_OFF}
    fi
}

off_led_mac80211() {
    local radio_num=$1
    local security_type=$2
    local radio_type=$3
    local led_option=$4

    local phy=""

    # "lookup_phy" should be defined in /lib/wifi/mac80211.sh
    lookup_phy "$wig_device"
    if [ -z "$phy" ]; then
        phy="phy0"
    fi

    TRAFFIC_11AD_ON="echo 2 > /sys/kernel/debug/ieee80211/${phy}/wil6210/led_cfg" # enable 11ad led, due to wil6210 parameter is led_id=2, so echo 2 to led_cfg
    TRAFFIC_11AD_OFF="echo 0 > /sys/kernel/debug/ieee80211/${phy}/wil6210/led_cfg"
    BLINK_11AD_ON="echo 10000 0 100 100 80 80 > /sys/kernel/debug/ieee80211/${phy}/wil6210/led_blink_time"
    BLIN_11ADK_OFF="echo 10000 0 10000 0 10000 0 > /sys/kernel/debug/ieee80211/${phy}/wil6210/led_blink_time"


    # Means that 11ad has been turned off
    if [ "$radio_type" = "none" ]; then
        eval ${BLINK_11AD_OFF}
        eval ${TRAFFIC_11AD_OFF}
    fi
}
