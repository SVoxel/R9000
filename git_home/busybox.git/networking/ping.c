/* vi: set sw=4 ts=4: */
/*
 * $Id: ping.c,v 1.56 2004/03/15 08:28:48 andersen Exp $
 * Mini ping implementation for busybox
 *
 * Copyright (C) 1999 by Randolph Chung <tausq@debian.org>
 *
 * Adapted from the ping in netkit-base 0.10:
 * Copyright (c) 1989 The Regents of the University of California.
 * Derived from software contributed to Berkeley by Mike Muuss.
 *
 * Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
 */

#include <netinet/ip_icmp.h>
#include "busybox.h"

enum {
	DEFDATALEN = 56,
	MAXIPLEN = 60,
	MAXICMPLEN = 76,
	MAXPACKET = 65468,
	MAX_DUP_CHK = (8 * 128),
	MAXWAIT = 10,
	PINGINTERVAL = 1		/* second */
};

static void ping(const char *host);

/* common routines */

static int in_cksum(unsigned short *buf, int sz)
{
	int nleft = sz;
	int sum = 0;
	unsigned short *w = buf;
	unsigned short ans = 0;

	while (nleft > 1) {
		sum += *w++;
		nleft -= 2;
	}

	if (nleft == 1) {
		*(unsigned char *) (&ans) = *(unsigned char *) w;
		sum += ans;
	}

	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	ans = ~sum;
	return ans;
}

#ifndef CONFIG_FEATURE_FANCY_PING

/* simple version */

static char *hostname;

static void noresp(int ign)
{
	printf("No response from %s\n", hostname);
	exit(EXIT_FAILURE);
}

static void ping(const char *host)
{
	struct hostent *h;
	struct sockaddr_in pingaddr;
	struct icmp *pkt;
	int pingsock, c;
	char packet[datalen + ICMP_MINLEN + MAXIPLEN + MAXICMPLEN];

	pingsock = create_icmp_socket();

	memset(&pingaddr, 0, sizeof(struct sockaddr_in));

	pingaddr.sin_family = AF_INET;
	h = xgethostbyname(host);
	memcpy(&pingaddr.sin_addr, h->h_addr, sizeof(pingaddr.sin_addr));
	hostname = h->h_name;

	pkt = (struct icmp *) packet;
	memset(pkt, 0, sizeof(packet));
	pkt->icmp_type = ICMP_ECHO;
	pkt->icmp_cksum = in_cksum((unsigned short *) pkt, sizeof(packet));

	c = sendto(pingsock, packet, datalen + ICMP_MINLEN, 0,
			   (struct sockaddr *) &pingaddr, sizeof(struct sockaddr_in));

	if (c < 0) {
		if (ENABLE_FEATURE_CLEAN_UP) close(pingsock);
		bb_perror_msg_and_die("sendto");
	}

	signal(SIGALRM, noresp);
	alarm(5);					/* give the host 5000ms to respond */
	/* listen for replies */
	while (1) {
		struct sockaddr_in from;
		socklen_t fromlen = sizeof(from);

		c = recvfrom(pingsock, packet, sizeof(packet), 0,
				(struct sockaddr *) &from, &fromlen);
		if (c < 0) {
			if (errno != EINTR)
				bb_perror_msg("recvfrom");
			continue;
		}
		if (c >= 76) {			/* ip + icmp */
			struct iphdr *iphdr = (struct iphdr *) packet;

			pkt = (struct icmp *) (packet + (iphdr->ihl << 2));	/* skip ip hdr */
			if (pkt->icmp_type == ICMP_ECHOREPLY)
				break;
		}
	}
	if (ENABLE_FEATURE_CLEAN_UP) close(pingsock);
	printf("%s is alive!\n", hostname);
}

int ping_main(int argc, char **argv)
{
	argc--;
	argv++;
	if (argc < 1)
		bb_show_usage();
	ping(*argv);
	return EXIT_SUCCESS;
}

#else /* ! CONFIG_FEATURE_FANCY_PING */

/* full(er) version */

#define OPT_STRING "qc:s:I:"
enum {
	OPT_QUIET = 1 << 0,
};

static struct sockaddr_in pingaddr;
static struct sockaddr_in sourceaddr;
static int pingsock = -1;
static unsigned datalen; /* intentionally uninitialized to work around gcc bug */

static unsigned long ntransmitted, nreceived, nrepeats, pingcount;
static int myid;
static unsigned long tmin = ULONG_MAX, tmax, tsum;
static char rcvd_tbl[MAX_DUP_CHK / 8];

static struct hostent *hostent;

static void sendping(int);
static void pingstats(int);
static void unpack(char *, int, struct sockaddr_in *);

#define	A(bit)		rcvd_tbl[(bit)>>3]	/* identify byte in array */
#define	B(bit)		(1 << ((bit) & 0x07))	/* identify bit in byte */
#define	SET(bit)	(A(bit) |= B(bit))
#define	CLR(bit)	(A(bit) &= (~B(bit)))
#define	TST(bit)	(A(bit) & B(bit))

/**************************************************************************/

static void pingstats(int junk)
{
	int status;

	signal(SIGINT, SIG_IGN);

	printf("\n--- %s ping statistics ---\n", hostent->h_name);
	printf("%lu packets transmitted, ", ntransmitted);
	printf("%lu packets received, ", nreceived);
	if (nrepeats)
		printf("%lu duplicates, ", nrepeats);
	if (ntransmitted)
		printf("%lu%% packet loss\n",
			   (ntransmitted - nreceived) * 100 / ntransmitted);
	if (nreceived)
		printf("round-trip min/avg/max = %lu.%lu/%lu.%lu/%lu.%lu ms\n",
			   tmin / 10, tmin % 10,
			   (tsum / (nreceived + nrepeats)) / 10,
			   (tsum / (nreceived + nrepeats)) % 10, tmax / 10, tmax % 10);
	if (nreceived != 0)
		status = EXIT_SUCCESS;
	else
		status = EXIT_FAILURE;
	exit(status);
}

static void sendping(int junk)
{
	struct icmp *pkt;
	int i;
	char packet[datalen + ICMP_MINLEN];

	pkt = (struct icmp *) packet;

	pkt->icmp_type = ICMP_ECHO;
	pkt->icmp_code = 0;
	pkt->icmp_cksum = 0;
	pkt->icmp_seq = htons(ntransmitted); /* don't ++ here, it can be a macro */
	pkt->icmp_id = myid;
	CLR(ntohs(pkt->icmp_seq) % MAX_DUP_CHK);
	ntransmitted++;

	gettimeofday((struct timeval *) &pkt->icmp_dun, NULL);
	pkt->icmp_cksum = in_cksum((unsigned short *) pkt, sizeof(packet));

	i = sendto(pingsock, packet, sizeof(packet), 0,
			   (struct sockaddr *) &pingaddr, sizeof(struct sockaddr_in));

	if (i < 0)
		bb_perror_msg_and_die("sendto");
	if ((size_t)i != sizeof(packet))
		bb_error_msg_and_die("ping wrote %d chars; %d expected", i,
			   (int)sizeof(packet));

	signal(SIGALRM, sendping);
	if (pingcount == 0 || ntransmitted < pingcount) {	/* schedule next in 1s */
		alarm(PINGINTERVAL);
	} else {					/* done, wait for the last ping to come back */
		/* todo, don't necessarily need to wait so long... */
		signal(SIGALRM, pingstats);
		alarm(MAXWAIT);
	}
}

static char *icmp_type_name(int id)
{
	switch (id) {
	case ICMP_ECHOREPLY:		return "Echo Reply";
	case ICMP_DEST_UNREACH:		return "Destination Unreachable";
	case ICMP_SOURCE_QUENCH:	return "Source Quench";
	case ICMP_REDIRECT:			return "Redirect (change route)";
	case ICMP_ECHO:				return "Echo Request";
	case ICMP_TIME_EXCEEDED:	return "Time Exceeded";
	case ICMP_PARAMETERPROB:	return "Parameter Problem";
	case ICMP_TIMESTAMP:		return "Timestamp Request";
	case ICMP_TIMESTAMPREPLY:	return "Timestamp Reply";
	case ICMP_INFO_REQUEST:		return "Information Request";
	case ICMP_INFO_REPLY:		return "Information Reply";
	case ICMP_ADDRESS:			return "Address Mask Request";
	case ICMP_ADDRESSREPLY:		return "Address Mask Reply";
	default:					return "unknown ICMP type";
	}
}

static void unpack(char *buf, int sz, struct sockaddr_in *from)
{
	struct icmp *icmppkt;
	struct iphdr *iphdr;
	struct timeval tv, *tp;
	int hlen, dupflag;
	unsigned long triptime;

	gettimeofday(&tv, NULL);

	/* discard if too short */
	if (sz < (datalen + ICMP_MINLEN))
		return;

	/* check IP header */
	iphdr = (struct iphdr *) buf;
	hlen = iphdr->ihl << 2;
	sz -= hlen;
	icmppkt = (struct icmp *) (buf + hlen);
	if (icmppkt->icmp_id != myid)
		return;				/* not our ping */

	if (icmppkt->icmp_type == ICMP_ECHOREPLY) {
		u_int16_t recv_seq = ntohs(icmppkt->icmp_seq);
		++nreceived;
		tp = (struct timeval *) icmppkt->icmp_data;

		/* If packet is too short, results will be truncated */
		if (sz < (ICMP_MINLEN + sizeof(tv.tv_sec) + sizeof(tv.tv_usec)))
			return;

		if ((tv.tv_usec -= tp->tv_usec) < 0) {
			--tv.tv_sec;
			tv.tv_usec += 1000000;
		}
		tv.tv_sec -= tp->tv_sec;

		triptime = tv.tv_sec * 10000 + (tv.tv_usec / 100);
		tsum += triptime;
		if (triptime < tmin)
			tmin = triptime;
		if (triptime > tmax)
			tmax = triptime;

		if (TST(recv_seq % MAX_DUP_CHK)) {
			++nrepeats;
			--nreceived;
			dupflag = 1;
		} else {
			SET(recv_seq % MAX_DUP_CHK);
			dupflag = 0;
		}

		if (option_mask32 & OPT_QUIET)
			return;

		printf("%d bytes from %s: icmp_seq=%u", sz,
			   inet_ntoa(*(struct in_addr *) &from->sin_addr.s_addr),
			   recv_seq);
		printf(" ttl=%d", iphdr->ttl);
		printf(" time=%lu.%lu ms", triptime / 10, triptime % 10);
		if (dupflag)
			printf(" (DUP!)");
		puts("");
	} else
		if (icmppkt->icmp_type != ICMP_ECHO)
			bb_error_msg("warning: got ICMP %d (%s)",
					icmppkt->icmp_type,
					icmp_type_name(icmppkt->icmp_type));
	fflush(stdout);
}

static void ping(const char *host)
{
	char packet[datalen + MAXIPLEN + MAXICMPLEN];
	int sockopt;

	pingsock = create_icmp_socket();

	if (sourceaddr.sin_addr.s_addr) {
		xbind(pingsock, (struct sockaddr*)&sourceaddr, sizeof(sourceaddr));
	}

	memset(&pingaddr, 0, sizeof(struct sockaddr_in));

	pingaddr.sin_family = AF_INET;
	hostent = xgethostbyname(host);
	if (hostent->h_addrtype != AF_INET)
		bb_error_msg_and_die("unknown address type; only AF_INET is currently supported");

	memcpy(&pingaddr.sin_addr, hostent->h_addr, sizeof(pingaddr.sin_addr));

	/* enable broadcast pings */
	setsockopt_broadcast(pingsock);

	/* set recv buf for broadcast pings */
	sockopt = 48 * 1024;
	setsockopt(pingsock, SOL_SOCKET, SO_RCVBUF, (char *) &sockopt,
			   sizeof(sockopt));

	printf("PING %s (%s)",
			hostent->h_name,
			inet_ntoa(*(struct in_addr *) &pingaddr.sin_addr.s_addr));
	if (sourceaddr.sin_addr.s_addr) {
		printf(" from %s",
			inet_ntoa(*(struct in_addr *) &sourceaddr.sin_addr.s_addr));
	}
	printf(": %d data bytes\n", datalen);

	signal(SIGINT, pingstats);

	/* start the ping's going ... */
	sendping(0);

	/* listen for replies */
	while (1) {
		struct sockaddr_in from;
		socklen_t fromlen = (socklen_t) sizeof(from);
		int c;

		c = recvfrom(pingsock, packet, sizeof(packet), 0,
				(struct sockaddr *) &from, &fromlen);
		if (c < 0) {
			if (errno != EINTR)
				bb_perror_msg("recvfrom");
			continue;
		}
		unpack(packet, c, &from);
		if (pingcount > 0 && nreceived >= pingcount)
			break;
	}
	pingstats(0);
}

/* TODO: consolidate ether-wake.c, dnsd.c, ifupdown.c, nslookup.c
 * versions of below thing. BTW we have far too many "%u.%u.%u.%u" too...
*/
static int parse_nipquad(const char *str, struct sockaddr_in* addr)
{
	char dummy;
	unsigned i1, i2, i3, i4;
	if (sscanf(str, "%u.%u.%u.%u%c",
			   &i1, &i2, &i3, &i4, &dummy) == 4
	&& ( (i1|i2|i3|i4) <= 0xff )
	) {
		uint8_t* ptr = (uint8_t*)&addr->sin_addr;
		ptr[0] = i1;
		ptr[1] = i2;
		ptr[2] = i3;
		ptr[3] = i4;
		return 0;
	}
	return 1; /* error */
}

int ping_main(int argc, char **argv)
{
	char *opt_c, *opt_s, *opt_I;

	datalen = DEFDATALEN; /* initialized here rather than in global scope to work around gcc bug */

	/* exactly one argument needed */
	opt_complementary = "=1";
	getopt32(argc, argv, OPT_STRING, &opt_c, &opt_s, &opt_I);
	if (option_mask32 & 2) pingcount = xatoul(opt_c); // -c
	if (option_mask32 & 4) datalen = xatou16(opt_s); // -s
	if (option_mask32 & 8) { // -I
/* TODO: ping6 accepts iface too:
		if_index = if_nametoindex(*argv);
		if (!if_index) ...
make it true for ping. */
		if (parse_nipquad(opt_I, &sourceaddr))
			bb_show_usage();
	}

	myid = (int16_t) getpid();
	ping(argv[optind]);
	return EXIT_SUCCESS;
}
#endif /* ! CONFIG_FEATURE_FANCY_PING */
