/*
 *	Handle incoming frames
 *	Linux ethernet bridge
 *
 *	Authors:
 *	Lennert Buytenhek		<buytenh@gnu.org>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/netfilter_bridge.h>
#include <linux/export.h>
#include <linux/rculist.h>
#include "br_private.h"

#ifdef CONFIG_DNI_MCAST_TO_UNICAST
#include <linux/ip.h>
#include <linux/igmp.h>
#endif

#ifdef CONFIG_DNI_DNSHIJACK_APMODE
#include <linux/ipv6.h>
#include <net/ipv6.h>
#include <linux/netfilter_ipv6/ip6_tables.h>

#if 0
#define DEBUGP printk
#else
#define DEBUGP(format, args...)
#endif

extern struct sock *dnsnl;
int sysctl_dnshijack_apmode = 0;
typedef struct dns_nl_packet_msg {
	size_t data_len;
	char saddr[6];
	unsigned char data[0];
} dns_nl_packet_msg_t;

int br_dns_netgear(struct sk_buff *skb)
{
	struct ethhdr	*eth;
	struct ipv6hdr	*ipv6h;
	struct udphdr	*udph;
	unsigned char *data, *p, dns[256], dns_buffer[256]={0};
	int	match_flag=0, i, n;
	int dataoff, dlen;
	static char *hijack_dns[] = {
		"www.routerlogin.com",
		"www.routerlogin.net",
		"routerlogin.com",
		"routerlogin.net",
		"readyshare.routerlogin.net",

		/* --- The End --- */
		NULL
	};

	if (skb == NULL){
		DEBUGP("IPv6 packet is empty, dropping\n");
		return 1;
	}
	eth = (struct ethhdr *)skb->mac_header;
	if (eth == NULL){
		DEBUGP("unable to find ipv6 eth in IPv6 packet, dropping\n");
		return 1;
	}
	
	if(eth->h_proto != htons(ETH_P_IPV6))
		return 1;

	ipv6h = ipv6_hdr(skb);
	if (ipv6h == NULL) {
		DEBUGP("unable to find ip header in IPv6 packet, dropping\n");
		return 1;
	}
		
	if (ipv6h->nexthdr != IPPROTO_UDP) 
		return 1;
	
	dataoff = sizeof(struct ipv6hdr); //ipv6 header's fixed-length is 40
	dlen = skb->len - dataoff;
	if(dlen > sizeof(dns_buffer)) return 1; /* we only need check short dns pkt, and avoid long pkt caused dns_buffer Kernel stack corrupted. */
	udph = skb_header_pointer(skb, dataoff, dlen, dns_buffer);

	if (ntohs(udph->dest) != 53)    /* DNS port: 53 */
		return 1;

	data = (void *)udph + sizeof(struct udphdr) + 12; /* Skip 12 fixed bytes header. */
	p = &dns[0];

	while ((data < skb->tail) && (n = *data++)) {
		if (n & 0xC0) {
			DEBUGP("dnshijack: Don't support compressed DNS encoding.\n");
			return 1;
		}

		if ((p - dns + n + 1) >= sizeof(dns)) {
			DEBUGP("Too long subdomain name :%d, the buffer is :%d\n", n, sizeof(dns));
			return 1;
		}
		if ((data + n) > skb->tail) {
			DEBUGP("The domain is invalid encoded!\n");
			return 1;
		}

		for (i = 0; i < n; i++)
			*p++ = *data++;
		*p++ = '.';
	}
	
	if (p != &dns[0])
		p--;
	*p = 0; /* Terminate: lose final period. */

	for (i = 0; hijack_dns[i]; i++) {
		if (strcmp((const char *)dns, hijack_dns[i]) == 0)
			match_flag = 1;
	}
	
	if(match_flag){
		DEBUGP("The hijacked DNS is : %s\n", (char *)dns);
		dns_nl_packet_msg_t *pm;
		struct nlmsghdr *nlh;
		size_t size, copy_len;
		copy_len = skb->len;
		size = NLMSG_SPACE(sizeof(*pm) + copy_len);
		struct sk_buff *skbnew = alloc_skb(4096, GFP_ATOMIC);
		if(!skbnew)
		{
			DEBUGP(KERN_ERR "Can't alloc whole buffer of size %ub!\n", 4096);
			skbnew = alloc_skb(size, GFP_ATOMIC);
			if(!skbnew)
			{
				DEBUGP(KERN_ERR "Can't alloc buffer of size %ub!\n", 4096);
				return 1;	
			}
		}
		nlh = nlmsg_put(skbnew, 0, 0, 0, (size - NLMSG_ALIGN(sizeof(*nlh))),0);
		nlh->nlmsg_len = sizeof(*pm) + skb->len;
		pm = NLMSG_DATA(nlh);
		pm->data_len = skb->len;
		memcpy(pm->saddr, eth->h_source, 6);
		skb_copy_bits(skb, 0, pm->data, skb->len);
		NETLINK_CB(skbnew).dst_group = 5;
		netlink_broadcast(dnsnl, skbnew, 0, 5, GFP_ATOMIC);
		return 0;
	}
	return 1;
nlmsg_failure:
	printk(KERN_CRIT "DNS NETLINK: error during NLMSG_PUT. This should not happen, please report to author.\n");
}

#endif

/* Hook for brouter */
br_should_route_hook_t __rcu *br_should_route_hook __read_mostly;
EXPORT_SYMBOL(br_should_route_hook);

static int br_pass_frame_up(struct sk_buff *skb)
{
	struct net_device *indev, *brdev = BR_INPUT_SKB_CB(skb)->brdev;
	struct net_bridge *br = netdev_priv(brdev);
	struct br_cpu_netstats *brstats = this_cpu_ptr(br->stats);

#ifdef CONFIG_BRIDGE_NETGEAR_ACL
	if (!br_acl_should_pass(br, skb, ACL_CHECK_SRC)) {
			br->dev->stats.rx_dropped++;
			kfree_skb(skb);
			return NET_RX_DROP;
      }
#endif

	u64_stats_update_begin(&brstats->syncp);
	brstats->rx_packets++;
	brstats->rx_bytes += skb->len;
	u64_stats_update_end(&brstats->syncp);

	/* Bridge is just like any other port.  Make sure the
	 * packet is allowed except in promisc modue when someone
	 * may be running packet capture.
	 */
	if (!(brdev->flags & IFF_PROMISC) &&
	    !br_allowed_egress(br, br_get_vlan_info(br), skb)) {
		kfree_skb(skb);
		return NET_RX_DROP;
	}

	skb = br_handle_vlan(br, br_get_vlan_info(br), skb);
	if (!skb)
		return NET_RX_DROP;

	indev = skb->dev;
	skb->dev = brdev;

	return BR_HOOK(NFPROTO_BRIDGE, NF_BR_LOCAL_IN, skb, indev, NULL,
		       netif_receive_skb);
}

/* note: already called with rcu_read_lock */
int br_handle_frame_finish(struct sk_buff *skb)
{
	const unsigned char *dest = eth_hdr(skb)->h_dest;
	struct net_bridge_port *p = br_port_get_rcu(skb->dev);
	struct net_bridge *br;
	struct net_bridge_fdb_entry *dst;
	struct net_bridge_mdb_entry *mdst;
	struct sk_buff *skb2;
	u16 vid = 0;

	if (!p || p->state == BR_STATE_DISABLED)
		goto drop;

	if (!br_allowed_ingress(p->br, nbp_get_vlan_info(p), skb, &vid))
		goto drop;

	/* insert into forwarding database after filtering to avoid spoofing */
	br = p->br;
	br_fdb_update(br, p, eth_hdr(skb)->h_source, vid);

	if (!is_broadcast_ether_addr(dest) && is_multicast_ether_addr(dest) &&
	    br_multicast_rcv(br, p, skb))
		goto drop;

	if ((p->state == BR_STATE_LEARNING) && skb->protocol != htons(ETH_P_PAE))
		goto drop;

	BR_INPUT_SKB_CB(skb)->brdev = br->dev;

	/* The packet skb2 goes to the local host (NULL to skip). */
	skb2 = NULL;

	if (br->dev->flags & IFF_PROMISC)
		skb2 = skb;

	dst = NULL;

	if (skb->protocol == htons(ETH_P_PAE)) {
		skb2 = skb;
		/* Do not forward 802.1x/EAP frames */
		skb = NULL;
	} else if (is_broadcast_ether_addr(dest))
		skb2 = skb;
	else if (is_multicast_ether_addr(dest)) {
		mdst = br_mdb_get(br, skb, vid);
		if (mdst || BR_INPUT_SKB_CB_MROUTERS_ONLY(skb)) {
			if ((mdst && mdst->mglist) ||
			    br_multicast_is_router(br))
				skb2 = skb;
			br_multicast_forward(mdst, skb, skb2);
			skb = NULL;
			if (!skb2)
				goto out;
		} else
			skb2 = skb;

		br->dev->stats.multicast++;
	} else if ((p->flags & BR_ISOLATE_MODE) ||
		   ((dst = __br_fdb_get(br, dest, vid)) && dst->is_local)) {
		skb2 = skb;
		/* Do not forward the packet since it's local. */
		skb = NULL;
	}

	if (skb) {
		if (dst) {
			dst->used = jiffies;
			br_forward(dst->dst, skb, skb2);
		} else
			br_flood_forward(br, skb, skb2);
	}

	if (skb2)
		return br_pass_frame_up(skb2);

out:
	return 0;
drop:
	kfree_skb(skb);
	goto out;
}

/* note: already called with rcu_read_lock */
static int br_handle_local_finish(struct sk_buff *skb)
{
	struct net_bridge_port *p = br_port_get_rcu(skb->dev);

	if (p->state != BR_STATE_DISABLED) {
		u16 vid = 0;

		br_vlan_get_tag(skb, &vid);
		br_fdb_update(p->br, p, eth_hdr(skb)->h_source, vid);
	}

	return 0;	 /* process further */
}

/*
 * Return NULL if skb is handled
 * note: already called with rcu_read_lock
 */
rx_handler_result_t br_handle_frame(struct sk_buff **pskb)
{
	struct net_bridge_port *p;
	struct sk_buff *skb = *pskb;
	const unsigned char *dest = eth_hdr(skb)->h_dest;
	br_should_route_hook_t *rhook;

	if (unlikely(skb->pkt_type == PACKET_LOOPBACK))
		return RX_HANDLER_PASS;

	if (!is_valid_ether_addr(eth_hdr(skb)->h_source))
		goto drop;

	skb = skb_share_check(skb, GFP_ATOMIC);
	if (!skb)
		return RX_HANDLER_CONSUMED;

	p = br_port_get_rcu(skb->dev);

	if (unlikely(is_link_local_ether_addr(dest))) {
		/*
		 * See IEEE 802.1D Table 7-10 Reserved addresses
		 *
		 * Assignment		 		Value
		 * Bridge Group Address		01-80-C2-00-00-00
		 * (MAC Control) 802.3		01-80-C2-00-00-01
		 * (Link Aggregation) 802.3	01-80-C2-00-00-02
		 * 802.1X PAE address		01-80-C2-00-00-03
		 *
		 * 802.1AB LLDP 		01-80-C2-00-00-0E
		 *
		 * Others reserved for future standardization
		 */
		switch (dest[5]) {
		case 0x00:	/* Bridge Group Address */
			/* If STP is turned off,
			   then must forward to keep loop detection */
			if (p->br->stp_enabled == BR_NO_STP)
				goto forward;
			break;

		case 0x01:	/* IEEE MAC (Pause) */
			goto drop;

		default:
			/* Allow selective forwarding for most other protocols */
			if (p->br->group_fwd_mask & (1u << dest[5]))
				goto forward;
		}

		/* Deliver packet to local host only */
		if (BR_HOOK(NFPROTO_BRIDGE, NF_BR_LOCAL_IN, skb, skb->dev,
			    NULL, br_handle_local_finish)) {
			return RX_HANDLER_CONSUMED; /* consumed by filter */
		} else {
			*pskb = skb;
			return RX_HANDLER_PASS;	/* continue processing */
		}
	}

forward:
	switch (p->state) {
	case BR_STATE_DISABLED:
		if (ether_addr_equal(p->br->dev->dev_addr, dest))
			skb->pkt_type = PACKET_HOST;

		if (BR_HOOK(NFPROTO_BRIDGE, NF_BR_PRE_ROUTING, skb, skb->dev, NULL,
			br_handle_local_finish))
			break;

		BR_INPUT_SKB_CB(skb)->brdev = p->br->dev;
		br_pass_frame_up(skb);
		break;

	case BR_STATE_FORWARDING:
#ifdef CONFIG_DNI_DNSHIJACK_APMODE
		if(sysctl_dnshijack_apmode) {
			if(br_dns_netgear(skb) == 0) {
				kfree_skb(skb);
				return RX_HANDLER_CONSUMED;
			}
		}
#endif
		rhook = rcu_dereference(br_should_route_hook);
		if (rhook) {
			if ((*rhook)(skb)) {
				*pskb = skb;
				return RX_HANDLER_PASS;
			}
			dest = eth_hdr(skb)->h_dest;
		}
		/* fall through */
	case BR_STATE_LEARNING:
		if (ether_addr_equal(p->br->dev->dev_addr, dest))
			skb->pkt_type = PACKET_HOST;

		BR_HOOK(NFPROTO_BRIDGE, NF_BR_PRE_ROUTING, skb, skb->dev, NULL,
			br_handle_frame_finish);
		break;
	default:
drop:
		kfree_skb(skb);
	}
	return RX_HANDLER_CONSUMED;
}
