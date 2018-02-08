#!/bin/sh

usage(){
    echo "wlandebug"
    echo " option:"
    echo "  -b [0|1] set/clear basic system information"
    echo "  -a [0|1] set/clear advanced system information"
    echo "  -w [0|1] set/clear basic wireless information"
    echo "  -h [0|1] set/clear hostapd debug flag"
}

print_section_hd(){
    printf "########### [ %-20s ] #############\n" "$1"
}

basic_sys_info=1
advanced_sys_info=1
basic_wlan_info=1
qca_debug_tool=1
qtn_section_debug=0


while getopts "b:a:w:q:" opt;do
    case "$opt" in
        b)
            basic_sys_info=$OPTARG;;
        a)
            advanced_sys_info=$OPTARG;;
        w)
            basic_wlan_info=$OPTARG;;
        q)
            qtn_debug=$OPTARG;;
        *)
            echo "Unknown option $OPTARG"; usage; exit 1;;
    esac
done

if [ "x$sctipt_debug" = "x1" ]; then
        echo "basic_sys_info=$basic_sys_info"
        echo "advanced_sys_info=$advanced_sys_info"
        echo "basic_wlan_info=$basic_wlan_info"
        echo "hostapd_debug=$hostapd_debug"
fi

basic_sys_info_func(){
        if [ $1 -eq 1 ]; then
                [ -f /bin/config ] && {
                print_section_hd 'Configuration file' >> $2
                /bin/config show >> $2
                }

                print_section_hd "uptime" >> $2 
                cat /proc/uptime >> $2

                print_section_hd "meminfo" >> $2
                cat /proc/meminfo >> $2

                print_section_hd "vmstat" >> $2
                cat /proc/vmstat >> $2

                [ -f /proc/net/arp ] && {
                print_section_hd "arp" >> $2
                cat /proc/net/arp >> $2
                }

                [ -f /proc/bus/pci/devices ] && {
                print_section_hd "/proc/bus/pci/devices" >> $2
                cat /proc/bus/pci/devices >> $2
                }

                print_section_hd "ifconfig" >> $2
                ifconfig >> $2

                print_section_hd "lspci" >> $2
                lspci >> $2

                print_section_hd "Process List" >> $2
                ps >> $2

                print_section_hd "Bridge Information" >> $2
                brctl show >> $2

                print_section_hd "interrupts (1)" >> $2
                cat /proc/interrupts >> $2

                sleep 3
                print_section_hd "interrupts (2)" >> $2
                cat /proc/interrupts >> $2
        fi
}

advanced_sys_info_func(){
        if [ $1 -eq 1 ]; then
            print_section_hd "modules" >> $2
            cat /proc/modules >> $2

            # print_section_hd "kallsyms"
            # cat /proc/kallsyms

            [ -f /proc/softirqs ] && {
                print_section_hd "softirqs" >> $2
                cat /proc/softirqs >> $2
            }
        fi
}

basic_wlan_info_func(){
        vif_ap_list=`iwconfig 2>/dev/null | grep -e Mode:Master -B1 | grep -e IEEE | awk '{print $1}'`
        wifi_list=`ifconfig | grep "^wifi" | awk '{print $1}'`
            
        print_section_hd 'Show Wireless Settings' >> $2
        [ -f /etc/config/wireless ] && cat /etc/config/wireless >> $2
        [ -f /sbin/dni_apup ] && cat /sbin/dni_apup >> $2

        if [ $1 -eq 1 ]; then
            [ -f /proc/athversion ] && { 
                print_section_hd "Wireless Driver version" >> $2
                cat /proc/athversion >> $2
            }
            
            print_section_hd "iwconfig" >> $2
            iwconfig 2>/dev/null >> $2

            print_section_hd "Associated station information" >> $2
            for vap in $vif_ap_list; do
                echo "[VAP: $vap]" >> $2
                wlanconfig $vap list sta >> $2
                iwpriv $vap txrx_fw_stats 3 # cannot redirect to file
                iwpriv $vap txrx_fw_stats 6 # cannot redirect to file
                echo ""
            done

        fi
}

qca_debug_tool_func(){
        if [ $1 -eq 1 ]; then
            print_section_hd "apstats" >> $2
            apstats -R >> $2

            print_section_hd "80211stats" >> $2
            for vap in $vif_ap_list; do
                echo "[VAP: $vap]" >> $2
                80211stats -i $vap >> $2
                echo "" >> $2
            done

            print_section_hd "athstats" >> $2
            for w in $wifi_list; do
                echo "[Radio: $w]" >> $2
                athstats -i $w >> $2
                echo "" >> $2
            done
        fi
}

# Special part, only for QTN.
qtn_debug_func(){
        if [ $1 -eq 1 ]; then
                print_section_hd "Status of QTN EP side" >> $2
                ping 1.1.1.2 -c 3 2>&1 >> $2

                print_section_hd "Status of configuring QTN module" >> $2
                cat /tmp/qt-wireless.log >> $2
        fi
}

file_num=1

#enable dynamic debug for 11ad
#[ -f /proc/sys/kernel/printk ] && echo '8' > /proc/sys/kernel/printk 
#if [ -f /sys/kernel/debug/dynamic_debug/control ]; then
#        echo "module wil6210" +p > /sys/kernel/debug/dynamic_debug/control
#        echo "module wil6210 format 'DBG[ IRQ]'" -p > /sys/kernel/debug/dynamic_debug/control
#        echo "module wil6210 format 'DBG[TXRX]'" -p > /sys/kernel/debug/dynamic_debug/control
#fi

while true
do
        print_section_hd "$(date -R) start " /tmp/wireless_log${file_num}.txt
        basic_sys_info_func $basic_sys_info /tmp/wireless_log${file_num}.txt
        advanced_sys_info_fun $advanced_sys_info /tmp/wireless_log${file_num}.txt
        basic_wlan_info_func $basic_wlan_info /tmp/wireless_log${file_num}.txt
        qtn_debug_func $qtn_debug /tmp/wireless_log${file_num}.txt
        print_section_hd "$(date -R) end " /tmp/wireless_log${file_num}.txt
        filesize=`ls -l /tmp/wireless_log${file_num}.txt | awk '{print $5}'`
	if [ $filesize -ge 5242880 ]; then
		echo "filesize if over, change to another Console-log file"
		if [ $file_num -eq 1 ]; then
			file_num=2;
		else
			file_num=1;
		fi
		# Once 1 file has reached the maximum(5MB), start write to another file
		[ -f /tmp/wireless_log${file_num}.txt ] && rm -rf /tmp/wireless_log${file_num}.txt
	fi
        sleep 60
done


# dbgLVL, getdbgLVL, dbgLVL_high, getdbgLVL_high
# enum {
#     /* IEEE80211_PARAM_DBG_LVL */
#     IEEE80211_MSG_TDLS      = 0,  0x00000001   /* TDLS */
#     IEEE80211_MSG_ACS,            0x00000002   /* auto channel selection */
#     IEEE80211_MSG_SCAN_SM,        0x00000004   /* scan state machine */
#     IEEE80211_MSG_SCANENTRY,      0x00000008   /* scan entry */
#     IEEE80211_MSG_WDS,            0x00000010   /* WDS handling */
#     IEEE80211_MSG_ACTION,         0x00000020   /* action management frames */
#     IEEE80211_MSG_ROAM,           0x00000040   /* sta-mode roaming */
#     IEEE80211_MSG_INACT,          0x00000080   /* inactivity handling */
#     IEEE80211_MSG_DOTH      = 8,  0x00000100   /* 11.h */
#     IEEE80211_MSG_IQUE,           0x00000200   /* IQUE features */
#     IEEE80211_MSG_WME,            0x00000400   /* WME protocol */
#     IEEE80211_MSG_ACL,            0x00000800   /* ACL handling */
#     IEEE80211_MSG_WPA,            0x00001000   /* WPA/RSN protocol */
#     IEEE80211_MSG_RADKEYS,        0x00002000   /* dump 802.1x keys */
#     IEEE80211_MSG_RADDUMP,        0x00004000   /* dump 802.1x radius packets */
#     IEEE80211_MSG_RADIUS,         0x00008000   /* 802.1x radius client */
#     IEEE80211_MSG_DOT1XSM   = 16, 0x00010000   /* 802.1x state machine */
#     IEEE80211_MSG_DOT1X,          0x00020000   /* 802.1x authenticator */
#     IEEE80211_MSG_POWER,          0x00040000   /* power save handling */
#     IEEE80211_MSG_STATE,          0x00080000   /* state machine */
#     IEEE80211_MSG_OUTPUT,         0x00100000   /* output handling */
#     IEEE80211_MSG_SCAN,           0x00200000   /* scanning */
#     IEEE80211_MSG_AUTH,           0x00400000   /* authentication handling */
#     IEEE80211_MSG_ASSOC,          0x00800000   /* association handling */
#     IEEE80211_MSG_NODE      = 24, 0x01000000   /* node handling */
#     IEEE80211_MSG_ELEMID,         0x02000000   /* element id parsing */
#     IEEE80211_MSG_XRATE,          0x04000000   /* rate set handling */
#     IEEE80211_MSG_INPUT,          0x08000000   /* input handling */
#     IEEE80211_MSG_CRYPTO,         0x10000000   /* crypto work */
#     IEEE80211_MSG_DUMPPKTS,       0x20000000   /* IFF_LINK2 equivalant */
#     IEEE80211_MSG_DEBUG,          0x40000000   /* IFF_DEBUG equivalent */
#     IEEE80211_MSG_MLME,           0x80000000   /* MLME */
#     /* IEEE80211_PARAM_DBG_LVL_HIGH */
#     IEEE80211_MSG_RRM       = 32,           /* Radio resource measurement */
#     IEEE80211_MSG_WNM,                      /* Wireless Network Management */
#     IEEE80211_MSG_P2P_PROT,                 /* P2P Protocol driver */
#     IEEE80211_MSG_PROXYARP,                 /* 11v Proxy ARP */
#     IEEE80211_MSG_L2TIF,                    /* Hotspot 2.0 L2 TIF */
#     IEEE80211_MSG_WIFIPOS,                  /* WifiPositioning Feature */
#     IEEE80211_MSG_WRAP,                     /* WRAP or Wireless ProxySTA */
#     IEEE80211_MSG_DFS,                      /* DFS debug mesg */

#     IEEE80211_MSG_NUM_CATEGORIES,           /* total ieee80211 messages */
#     IEEE80211_MSG_UNMASKABLE = IEEE80211_MSG_MAX,  /* anything */
#     IEEE80211_MSG_ANY = IEEE80211_MSG_MAX,  /* anything */
# };


# ATHDebug, getATHDebug

# enum {
#     ATH_DEBUG_XMIT = 0,    /* basic xmit operation */
#     ATH_DEBUG_XMIT_DESC,   /* xmit descriptors */
#     ATH_DEBUG_RECV,        /* basic recv operation */
#     ATH_DEBUG_RECV_DESC,   /* recv descriptors */
#     ATH_DEBUG_RATE,        /* rate control */
#     ATH_DEBUG_RESET,       /* reset processing */
#     ATH_DEBUG_MAT,         /* MAT for s/w proxysta */
#     ATH_DEBUG_BEACON,      /* beacon handling */
#     ATH_DEBUG_WATCHDOG,    /* watchdog timeout */
#     ATH_DEBUG_SCAN,        /* scan debug prints */
#     ATH_DEBUG_GREEN_AP,    /* GreenAP debug prints */
#     ATH_DEBUG_HTC_WMI,     /* htc/wmi debug prints */
#     ATH_DEBUG_INTR,        /* ISR */
#     ATH_DEBUG_TX_PROC,     /* tx ISR proc */
#     ATH_DEBUG_RX_PROC,     /* rx ISR proc */
#     ATH_DEBUG_BEACON_PROC, /* beacon ISR proc */
#     ATH_DEBUG_CALIBRATE,   /* periodic calibration */
#     ATH_DEBUG_KEYCACHE,    /* key cache management */
#     ATH_DEBUG_STATE,       /* 802.11 state transitions */
#     ATH_DEBUG_NODE,        /* node management */
#     ATH_DEBUG_LED,         /* led management */
#     ATH_DEBUG_TX99,        /* tx99 function */
#     ATH_DEBUG_DCS,         /* dynamic channel switch */
#     ATH_DEBUG_UAPSD,       /* uapsd */
#     ATH_DEBUG_DOTH,        /* 11.h */
#     ATH_DEBUG_CWM,         /* channel width managment */
#     ATH_DEBUG_PPM,         /* Force PPM management */
#     ATH_DEBUG_PWR_SAVE,    /* PS Poll and PS save */
#     ATH_DEBUG_SWR,         /* SwRetry mechanism */
#     ATH_DEBUG_AGGR_MEM,
#     ATH_DEBUG_BTCOEX,      /* BT coexistence */
#     ATH_DEBUG_FATAL,       /* fatal errors */
#     ATH_DEBUG_WNM_FMS,
#     /*
#      * First fill in the UNUSED values above, then
#      * add new values here.
#      */

#     ATH_DEBUG_NUM_CATEGORIES,
#     ATH_DEBUG_ANY,
#     ATH_DEBUG_UNMASKABLE = ATH_DEBUG_MAX_MASK,
