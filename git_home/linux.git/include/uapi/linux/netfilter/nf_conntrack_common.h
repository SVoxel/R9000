#ifndef _UAPI_NF_CONNTRACK_COMMON_H
#define _UAPI_NF_CONNTRACK_COMMON_H
/* Connection state tracking for netfilter.  This is separated from,
   but required by, the NAT layer; it can also be used by an iptables
   extension. */
enum ip_conntrack_info {
	/* Part of an established connection (either direction). */
	IP_CT_ESTABLISHED,

	/* Like NEW, but related to an existing connection, or ICMP error
	   (in either direction). */
	IP_CT_RELATED,

	/* Started a new connection to track (only
           IP_CT_DIR_ORIGINAL); may be a retransmission. */
	IP_CT_NEW,

	/* >= this indicates reply direction */
	IP_CT_IS_REPLY,

	IP_CT_ESTABLISHED_REPLY = IP_CT_ESTABLISHED + IP_CT_IS_REPLY,
	IP_CT_RELATED_REPLY = IP_CT_RELATED + IP_CT_IS_REPLY,
	IP_CT_NEW_REPLY = IP_CT_NEW + IP_CT_IS_REPLY,	
	/* Number of distinct IP_CT types (no NEW in reply dirn). */
	IP_CT_NUMBER = IP_CT_IS_REPLY * 2 - 1
};

/* Bitset representing status of connection. */
enum ip_conntrack_status {
	/* It's an expected connection: bit 0 set.  This bit never changed */
	IPS_EXPECTED_BIT = 0,
	IPS_EXPECTED = (1 << IPS_EXPECTED_BIT),

	/* We've seen packets both ways: bit 1 set.  Can be set, not unset. */
	IPS_SEEN_REPLY_BIT = 1,
	IPS_SEEN_REPLY = (1 << IPS_SEEN_REPLY_BIT),

	/* Conntrack should never be early-expired. */
	IPS_ASSURED_BIT = 2,
	IPS_ASSURED = (1 << IPS_ASSURED_BIT),

	/* Connection is confirmed: originating packet has left box */
	IPS_CONFIRMED_BIT = 3,
	IPS_CONFIRMED = (1 << IPS_CONFIRMED_BIT),

	/* Connection needs src nat in orig dir.  This bit never changed. */
	IPS_SRC_NAT_BIT = 4,
	IPS_SRC_NAT = (1 << IPS_SRC_NAT_BIT),

	/* Connection needs dst nat in orig dir.  This bit never changed. */
	IPS_DST_NAT_BIT = 5,
	IPS_DST_NAT = (1 << IPS_DST_NAT_BIT),

	/* Both together. */
	IPS_NAT_MASK = (IPS_DST_NAT | IPS_SRC_NAT),

	/* Connection needs TCP sequence adjusted. */
	IPS_SEQ_ADJUST_BIT = 6,
	IPS_SEQ_ADJUST = (1 << IPS_SEQ_ADJUST_BIT),

	/* NAT initialization bits. */
	IPS_SRC_NAT_DONE_BIT = 7,
	IPS_SRC_NAT_DONE = (1 << IPS_SRC_NAT_DONE_BIT),

	IPS_DST_NAT_DONE_BIT = 8,
	IPS_DST_NAT_DONE = (1 << IPS_DST_NAT_DONE_BIT),

	/* Both together */
	IPS_NAT_DONE_MASK = (IPS_DST_NAT_DONE | IPS_SRC_NAT_DONE),

	/* Connection is dying (removed from lists), can not be unset. */
	IPS_DYING_BIT = 9,
	IPS_DYING = (1 << IPS_DYING_BIT),

	/* Connection has fixed timeout. */
	IPS_FIXED_TIMEOUT_BIT = 10,
	IPS_FIXED_TIMEOUT = (1 << IPS_FIXED_TIMEOUT_BIT),

	/* Conntrack is a template */
	IPS_TEMPLATE_BIT = 11,
	IPS_TEMPLATE = (1 << IPS_TEMPLATE_BIT),

	/* Conntrack is a fake untracked entry */
	IPS_UNTRACKED_BIT = 12,
	IPS_UNTRACKED = (1 << IPS_UNTRACKED_BIT),

	/* Conntrack got a helper explicitly attached via CT target. */
	IPS_HELPER_BIT = 13,
	IPS_HELPER = (1 << IPS_HELPER_BIT),

	IPS_SNATP2P_SRC_BIT = 14,
	IPS_SNATP2P_SRC = (1 << IPS_SNATP2P_SRC_BIT),

	IPS_SNATP2P_DST_BIT = 15,
	IPS_SNATP2P_DST = (1 << IPS_SNATP2P_DST_BIT),

	/* Both together. */
	IPS_SNATP2P_MASK = (IPS_SNATP2P_DST | IPS_SNATP2P_SRC),

	IPS_SNATP2P_SRC_DONE_BIT = 16,
	IPS_SNATP2P_SRC_DONE = (1 << IPS_SNATP2P_SRC_DONE_BIT),

	IPS_SNATP2P_DST_DONE_BIT = 17,
	IPS_SNATP2P_DST_DONE = (1 << IPS_SNATP2P_DST_DONE_BIT),

	/* Both together. */
	IPS_SNATP2P_DONE_MASK = (IPS_SNATP2P_DST_DONE | IPS_SNATP2P_SRC_DONE),

	/* Refresh the idle time of ALG data session's master conntrack */
	IPS_ALG_REFRESH_BIT = 18,
	IPS_ALG_REFRESH = (1 << IPS_ALG_REFRESH_BIT),

	IPS_CONENAT_BIT = 19,
	IPS_CONENAT = (1<< IPS_CONENAT_BIT),

	IPS_TRIGGER_BIT = 20,
	IPS_TRIGGER = (1 << IPS_TRIGGER_BIT),

	IPS_SPI_DoS_BIT = 21,
	IPS_SPI_DoS = (1 << IPS_SPI_DoS_BIT),

	/*
	 * In Netgear's unofficial "Home Wireless Router IPv6 Spec"
	 * (it will be merged into Home Wireless Router Spec V1.10
	 * according to Netgear), IPv6 SPI Firewall does not have NAT on IPv6,
	 * and there are two routing filtering modes:
	 * Secured Mode (default) and Open Mode.
	 */
	IPS_IPV6_ROUTING_FILTERING_BIT = 22,
	IPS_IPV6_ROUTING_FILTERING = (1 << IPS_IPV6_ROUTING_FILTERING_BIT),

	IPS_RANGE_PORT_FULL_BIT = 23,
	IPS_RANGE_PORT_FULL = (1 << IPS_RANGE_PORT_FULL_BIT),

	/* [NETGEAR SPEC 2.0] 1.10 NAT Session Management */
	IPS_NAT_STATIC_HIGH_PRIORITY_BIT = 24,
	IPS_NAT_STATIC_HIGH_PRIORITY = (1 << IPS_NAT_STATIC_HIGH_PRIORITY_BIT),
};

/* Connection tracking event types */
enum ip_conntrack_events {
	IPCT_NEW,		/* new conntrack */
	IPCT_RELATED,		/* related conntrack */
	IPCT_DESTROY,		/* destroyed conntrack */
	IPCT_REPLY,		/* connection has seen two-way traffic */
	IPCT_ASSURED,		/* connection status has changed to assured */
	IPCT_PROTOINFO,		/* protocol information has changed */
	IPCT_HELPER,		/* new helper has been set */
	IPCT_MARK,		/* new mark has been set */
	IPCT_NATSEQADJ,		/* NAT is doing sequence adjustment */
	IPCT_SECMARK,		/* new security mark has been set */
	IPCT_LABEL,		/* new connlabel has been set */
};

enum ip_conntrack_expect_events {
	IPEXP_NEW,		/* new expectation */
	IPEXP_DESTROY,		/* destroyed expectation */
};

/* expectation flags */
#define NF_CT_EXPECT_PERMANENT		0x1
#define NF_CT_EXPECT_INACTIVE		0x2
#define NF_CT_EXPECT_USERSPACE		0x4


#endif /* _UAPI_NF_CONNTRACK_COMMON_H */
