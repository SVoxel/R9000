#ifndef __CONFIG_H__
#define __CONFIG_H__

#define NET_IPC_IFNAME  "brwan"  /* The Router's WAN side Ethernet Interface name. */
#define NET_PPP_IFNAME  "ppp0"  /* The Router's WAN side P-2-P Interface name. */
#define NET_LOC_IFNAME  "br0"   /* The Router's LAN side Interface name. */
#define NET_LAN_ETH     "ethlan"  /* The Router's Ethernet LAN device name */

/*********************** detcable.c *******************************/
#define PLUG_OFF 0
#define PLUG_IN 1

#define ENET_UNIT_LAN   1
#define ENET_UNIT_WAN   0
#define PHY_SPEC_STATUS 0x11

#define SPEED(val)          (((val) >> 14) & 0x3)
#define DUPLEX(val)         (((val) >> 13) & 0x1)
#define PAGE_RECEIVED(val)      (((val) >> 12) & 0x1)
#define SPEED_DUPLEX_RESOLVED(val)  (((val) >> 11) & 0x1)
#define LINK(val)           (((val) >> 10) & 0x1)
#define MDI_CROSSOVER_STATUS(val)   (((val) >> 6) & 0x1)
#define SMARTSPEED_DOWNGRADE(val)   (((val) >> 5) & 0x1)
#define ENERGY_DETECT_STATUS(val)   (((val) >> 4) & 0x1)
#define TRANSMIT_PAUSE_ENABLED(val) (((val) >> 3) & 0x1)
#define RECEIV_PAUSE_ENABLED(val)   (((val) >> 2) & 0x1)
#define POLARITY(val)           (((val) >> 1) & 0x1)
#define JABBER(val)         (((val) >> 0) & 0x1)

#define DETWANV6
#define DNI_WAN_DNS3_SUPPORT 1
#define RFC3442_249_SUPPORT 1
#define WLAN_COMMON_SUUPPORT 1

#endif

