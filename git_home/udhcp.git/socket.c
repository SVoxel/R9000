/*
 * socket.c -- DHCP server client/server socket creation
 *
 * udhcp client/server
 * Copyright (C) 1999 Matthew Ramsay <matthewr@moreton.com.au>
 *			Chris Trew <ctrew@moreton.com.au>
 *
 * Rewrite by Russ Dill <Russ.Dill@asu.edu> July 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <errno.h>
#include <features.h>
#include <linux/filter.h>
#if __GLIBC__ >=2 && __GLIBC_MINOR >= 1
#include <netpacket/packet.h>
#include <net/ethernet.h>
#else
#include <asm/types.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#endif

#include "debug.h"

int read_interface(char *interface, int *ifindex, u_int32_t *addr, unsigned char *arp)
{
	int fd;
	struct ifreq ifr;
	struct sockaddr_in *our_ip;

	memset(&ifr, 0, sizeof(struct ifreq));
	if((fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) >= 0) {
		ifr.ifr_addr.sa_family = AF_INET;
		strcpy(ifr.ifr_name, interface);

		if (addr) { 
			if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
				our_ip = (struct sockaddr_in *) &ifr.ifr_addr;
				*addr = our_ip->sin_addr.s_addr;
				DEBUG(LOG_INFO, "%s (our ip) = %s", ifr.ifr_name, inet_ntoa(our_ip->sin_addr));
			} else {
				LOG(LOG_ERR, "SIOCGIFADDR failed, is the interface up and configured?: %s", 
						strerror(errno));
				return -1;
			}
		}
		
		if (ioctl(fd, SIOCGIFINDEX, &ifr) == 0) {
			DEBUG(LOG_INFO, "adapter index %d", ifr.ifr_ifindex);
			*ifindex = ifr.ifr_ifindex;
		} else {
			LOG(LOG_ERR, "SIOCGIFINDEX failed!: %s", strerror(errno));
			return -1;
		}
		if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
			memcpy(arp, ifr.ifr_hwaddr.sa_data, 6);
			DEBUG(LOG_INFO, "adapter hardware address %02x:%02x:%02x:%02x:%02x:%02x",
				arp[0], arp[1], arp[2], arp[3], arp[4], arp[5]);
		} else {
			LOG(LOG_ERR, "SIOCGIFHWADDR failed!: %s", strerror(errno));
			return -1;
		}
	} else {
		LOG(LOG_ERR, "socket failed!: %s", strerror(errno));
		return -1;
	}
	close(fd);
	return 0;
}


int listen_socket(unsigned int ip, int port, char *inf)
{
	struct ifreq interface;
	int fd;
	struct sockaddr_in addr;
	int n = 1;

	DEBUG(LOG_INFO, "Opening listen socket on 0x%08x:%d %s\n", ip, port, inf);
	if ((fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		DEBUG(LOG_ERR, "socket call failed: %s", strerror(errno));
		return -1;
	}
	
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = ip;

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &n, sizeof(n)) == -1) {
		close(fd);
		return -1;
	}
	if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (char *) &n, sizeof(n)) == -1) {
		close(fd);
		return -1;
	}

	strncpy(interface.ifr_ifrn.ifrn_name, inf, IFNAMSIZ);
	if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE,(char *)&interface, sizeof(interface)) < 0) {
		close(fd);
		return -1;
	}

	if (bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1) {
		close(fd);
		return -1;
	}
	
	return fd;
}


int raw_socket(int ifindex, int port)
{
	int fd;
	struct sockaddr_ll sock;
	static const struct sock_filter filter_instr[] = {
		/* load 9th byte (protocol) */
		BPF_STMT(BPF_LD|BPF_B|BPF_ABS, 9),
		/* jump to L1 if it is IPPROTO_UDP, else to L4 */
		BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, IPPROTO_UDP, 0, 6),
		/* L1: load halfword from offset 6 (flags and frag offset) */
		BPF_STMT(BPF_LD|BPF_H|BPF_ABS, 6),
		/* jump to L4 if any bits in frag offset field are set, else to L2 */
		BPF_JUMP(BPF_JMP|BPF_JSET|BPF_K, 0x1fff, 4, 0),
		/* L2: skip IP header (load index reg with header len) */
		BPF_STMT(BPF_LDX|BPF_B|BPF_MSH, 0),
		/* load udp destination port from halfword[header_len + 2] */
		BPF_STMT(BPF_LD|BPF_H|BPF_IND, 2),
		/* jump to L3 if udp dport is CLIENT_PORT, else to L4 */
		BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, 68, 0, 1),
		/* L3: accept packet */
		BPF_STMT(BPF_RET|BPF_K, 0xffffffff),
		/* L4: discard packet */
		BPF_STMT(BPF_RET|BPF_K, 0),
	};
	static const struct sock_fprog filter_prog = {
		.len = sizeof(filter_instr) / sizeof(filter_instr[0]),
		/* casting const away: */
		.filter = (struct sock_filter *) filter_instr,
	};


	DEBUG(LOG_INFO, "Opening raw socket on ifindex %d\n", ifindex);
	if ((fd = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_IP))) < 0) {
		DEBUG(LOG_ERR, "socket call failed: %s", strerror(errno));
		return -1;
	}
	
	sock.sll_family = AF_PACKET;
	sock.sll_protocol = htons(ETH_P_IP);
	sock.sll_ifindex = ifindex;
	if (bind(fd, (struct sockaddr *) &sock, sizeof(sock)) < 0) {
		DEBUG(LOG_ERR, "bind call failed: %s", strerror(errno));
		close(fd);
		return -1;
	}

	if(port == 68) {
		if (setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, &filter_prog,
			sizeof(filter_prog)) >= 0)
			DEBUG(LOG_INFO, "Attached filter to raw socket fd %d", fd);
	}

	return fd;
}

