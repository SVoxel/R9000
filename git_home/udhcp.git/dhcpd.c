/* dhcpd.c
 *
 * udhcp Server
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

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>

#include "debug.h"
#include "dhcpd.h"
#include "arpping.h"
#include "socket.h"
#include "options.h"
#include "files.h"
#include "leases.h"
#include "packet.h"
#include "serverpacket.h"
#include "pidfile.h"


/* globals */
struct dhcpOfferedAddr *leases;
struct server_config_t server_config;
static int signal_pipe[2];
extern unsigned char g_src_addr[6];
#ifdef DHCPD_HAVE_NAS 
int deviceA_flag=0;
u_int8_t deviceA_mac[16];
u_int8_t nas_mac[16];
u_int32_t nas_ip,deviceA_ip;
int hashA;
int have_nas=0;
#endif

/* Exit and cleanup */
static void exit_server(int retval)
{
	pidfile_delete(server_config.pidfile);
	CLOSE_LOG();
	exit(retval);
}


/* Signal handler */
static void signal_handler(int sig)
{
	if (send(signal_pipe[1], &sig, sizeof(sig), MSG_DONTWAIT) < 0) {
		LOG(LOG_ERR, "Could not send signal: %s", 
			strerror(errno));
	}
}

#ifdef DHCPD_SHOW_HOSTNAME

#define NIPQUAD(addr)	\
	((unsigned char *)&addr)[0],	\
	((unsigned char *)&addr)[1],	\
	((unsigned char *)&addr)[2],	\
	((unsigned char *)&addr)[3]

void show_clients_hostname(void)
{
	FILE *fp;
	unsigned int i;
	struct dhcpOfferedAddr *lease;

	if ((fp = fopen(HOSTNAME_SHOWFILE, "w")) == NULL)
		return;

	for (i = 0; i < server_config.max_leases; i++) {
		lease = &(leases[i]);
		/**
		 * For beta issue: TD-23
		 * If use Ixia with some virtual DHCP clients to test "Attached Device" feature,
		 * Ixia could not send arp packet actively, we need request all IPs that DHCP server
		 * assigned while user refresh "Attached Device" table.
		 * So we should record all IPs that DHCP server assigned even not get host name.
		 */
		if (lease->yiaddr == 0)
			continue;

		fprintf(fp, "%u.%u.%u.%u %s\n", NIPQUAD(lease->yiaddr),
				lease->hostname);
	}

	fclose(fp);
}

#endif

#ifdef DHCPD_HAVE_NAS 
char *cat_file(char *name)
{
	int i;
	FILE *fp;
	static char buf[512];

	buf[0] = '\0';

	fp = fopen(name, "r");
	if (fp == NULL)
		return buf;
	fgets(buf, sizeof(buf), fp);
	fclose(fp);

	i = 0;
	while (buf[i] != '\0' && buf[i] != '\r' && buf[i] != '\n')
		i++;
	buf[i] = '\0';

	return buf;
}

//Algorithm:
//hash=(nas mac XOR dev mac)XOR(nas ip XOR dev ip)
//hash=hash XOR (hash >> 16) XOR (hash >> 8)
//hash=hash %2
int nas_ly23(u_int32_t nas_ip, u_int8_t *nas_mac,u_int32_t dev_ip,u_int8_t *dev_mac)
{
    int hashd;
    hashd=((nas_mac[5]^dev_mac[5])^(ntohl(nas_ip^dev_ip)));
    hashd=(hashd^(hashd >> 16));
    hashd=(hashd^(hashd >> 8));
    hashd=hashd %2;
    return hashd; 
}

// if return 1, it will send DHCP ACK to finishi ip assign
// if return 0, it will send NAK and then start another dhcp process
int nas_dhcp(u_int8_t *mac,u_int32_t ip,char *name)
{
    u_int32_t deviceB_ip;
    char *gt_nas=NULL;
    char nas_status[16];
    u_int8_t deviceB_mac[16];
    int hashB;
    if(strcmp(name,"nas")==0)//if dev is nas dev, save nas  ip and mac  
    {   nas_ip=ntohl(ip);
        memcpy(nas_mac,mac,6);
        return 1;
    }
    //if it not nas dev but the second join in dev except nas
    //we define it device A
    if((strcmp(name,"nas") !=0) && (deviceA_flag==0)) 
    {   deviceA_ip=ntohl(ip);
        memcpy(deviceA_mac,mac,6);
        deviceA_flag=1; 
        return 1;
    }
//if not nas or not device A, we define it devce B
    if((strcmp(name,"nas")!=0) && (deviceA_flag !=0))
    {   deviceB_ip=ntohl(ip);
        memcpy(deviceB_mac,mac,6);
//bond-show 2 get nas status to check nas lacp is up or not,
//if active, mean up , else mean not up
        system("bond-show 2 > /tmp/nas_stat");
        gt_nas=cat_file("/tmp/nas_stat");
        strncpy(nas_status,gt_nas,sizeof(nas_status));
        if(strcmp(nas_status,"active")==0)
        {
            //use nas_ly23 to get device A with nas hashA
            hashA=nas_ly23(nas_ip,nas_mac,deviceA_ip,deviceA_mac);
            //use nas_ly23 to get device B with nas hashB
            hashB=nas_ly23(nas_ip,nas_mac,deviceB_ip,deviceB_mac);
            //deviceA_flag=0;
            //if hashA=hashB, it return 0 to send NAK and start another dhcp to get another ip 
            if(hashA == hashB)
            {
                FILE *fp=fopen("/tmp/nas_st","w");
                int tmp_nas=0;
                fputc(tmp_nas,fp);
                fclose(fp);  
                return 0;
            }
            else
                return 1; //if hashA != hashB, return 1 to send ACK to get ip
        }
        else 
            return 1; // if bond-show 2 is not active, it mean nas not up , so return 1 to get ip directly
    }
}
#endif

#ifdef COMBINED_BINARY	
int udhcpd_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{	
	fd_set rfds;
	struct timeval tv;
	int server_socket = -1;
	int bytes, retval;
	struct dhcpMessage packet;
	unsigned char *state;
	unsigned char *server_id, *requested;
	u_int32_t server_id_align, requested_align;
	unsigned long timeout_end;
	struct option_set *option;
	struct dhcpOfferedAddr *lease;
	int pid_fd;
	int max_sock;
	int sig;
	u_int32_t client_ip_align;
	u_int8_t client_hw_addr[6];
	int raw_server_socket = -1;
	int raw_socket_read_succ = 0;
	struct dhcpMessage raw_packet;
	suseconds_t raw_read_timeout;

#ifdef DHCPD_STATIC_LEASE
        uint32_t static_lease_ip;
        struct dhcpOfferedAddr static_lease;
#endif

	OPEN_LOG("udhcpd");
	LOG(LOG_INFO, "udhcp server (v%s) started", VERSION);

	memset(&server_config, 0, sizeof(struct server_config_t));
	
	if (argc < 2)
		read_config(DHCPD_CONF_FILE);
	else read_config(argv[1]);

	pid_fd = pidfile_acquire(server_config.pidfile);
	pidfile_write_release(pid_fd);

	if ((option = find_option(server_config.options, DHCP_LEASE_TIME))) {
		memcpy(&server_config.lease, option->data + 2, 4);
		server_config.lease = ntohl(server_config.lease);
	}
	else server_config.lease = LEASE_TIME;
	
	leases = malloc(sizeof(struct dhcpOfferedAddr) * server_config.max_leases);
	memset(leases, 0, sizeof(struct dhcpOfferedAddr) * server_config.max_leases);
#ifndef DHCPD_STATIC_LEASE /* Don't use DHCP Lease cache */
	read_leases(server_config.lease_file);
#endif

#ifdef DHCPD_SHOW_HOSTNAME
	show_clients_hostname();
#endif

	if (read_interface(server_config.interface, &server_config.ifindex,
			   &server_config.server, server_config.arp) < 0)
		exit_server(1);

#ifndef DEBUGGING
	pid_fd = pidfile_acquire(server_config.pidfile); /* hold lock during fork. */
	if (daemon(0, 0) == -1) {
		perror("fork");
		exit_server(1);
	}
	pidfile_write_release(pid_fd);
#endif

	socketpair(AF_UNIX, SOCK_STREAM, 0, signal_pipe);
	signal(SIGUSR1, signal_handler);
	signal(SIGTERM, signal_handler);
#ifdef DHCPD_HAVE_NAS
    int nas_result;
    char h_name[16];
    char *g_nas=NULL;
    char nas_sta[16];
    char *g_na=NULL;
    char nas_s[16];
	unsigned char *t_name;
    if(access("/tmp/nas_stat",0)==0)
    unlink("/tmp/nas_stat");
#endif

	timeout_end = time(0) + server_config.auto_time;
	while(1) { /* loop until universe collapses */
#ifdef DHCPD_HAVE_NAS
        system("bond-show 2 > /tmp/nas_stat");
        g_na=cat_file("/tmp/nas_stat");
        strncpy(nas_s,g_na,sizeof(nas_s));
        //after plug off nas or nas is not up,  if have nas_up file,need to delete it
        if(strcmp(nas_s,"active")!=0)
        {
            if(access("/tmp/nas_up",0)==0)
                unlink("/tmp/nas_up");
        }
#endif
		if (server_socket < 0) {
 			if ((server_socket = listen_socket(INADDR_ANY, SERVER_PORT, server_config.interface)) < 0) {
				LOG(LOG_ERR, "FATAL: couldn't create server socket, %s", strerror(errno));
				exit_server(0);
			}			
		}

		if (raw_server_socket < 0) {
			/*
			 * Bug 22814 [NETGARE]
			 * create raw socket with PF_PACKET protocol family to obtain layer2 source mac address
			 */
			if ((raw_server_socket = raw_socket(server_config.ifindex, 0)) < 0) {
				LOG(LOG_ERR, "FATAL: couldn't create raw server socket, %s", strerror(errno));
				exit_server(0);
			}			
			struct timeval raw_rcv_timeout;
			raw_rcv_timeout.tv_sec = 1;
			if (setsockopt(raw_server_socket, SOL_SOCKET, SO_RCVTIMEO, (char *) &raw_rcv_timeout, sizeof(raw_rcv_timeout)) == -1) {
				 LOG(LOG_ERR, "FATAL: couldn't set SO_RCVTIMEO for raw server socket, %s", strerror(errno));
			}
		}

		FD_ZERO(&rfds);
		FD_SET(server_socket, &rfds);
		FD_SET(signal_pipe[0], &rfds);
		if (server_config.auto_time) {
			tv.tv_sec = timeout_end - time(0);
			tv.tv_usec = 0;
		}
		if (!server_config.auto_time || tv.tv_sec > 0) {
			max_sock = server_socket > signal_pipe[0] ? server_socket : signal_pipe[0];
			retval = select(max_sock + 1, &rfds, NULL, NULL, 
					server_config.auto_time ? &tv : NULL);
		} else retval = 0; /* If we already timed out, fall through */

		if (retval == 0) {
#ifndef DHCPD_STATIC_LEASE			
			write_leases();
#endif
			timeout_end = time(0) + server_config.auto_time;
			continue;
		} else if (retval < 0 && errno != EINTR) {
			DEBUG(LOG_INFO, "error on select");
			continue;
		}
		
		if (FD_ISSET(signal_pipe[0], &rfds)) {
			if (read(signal_pipe[0], &sig, sizeof(sig)) < 0)
				continue; /* probably just EINTR */
			switch (sig) {
			case SIGUSR1:
				LOG(LOG_INFO, "Received a SIGUSR1");
#ifndef DHCPD_STATIC_LEASE				
				write_leases();
#endif
				/* why not just reset the timeout, eh */
				timeout_end = time(0) + server_config.auto_time;
				continue;
			case SIGTERM:
				LOG(LOG_INFO, "Received a SIGTERM");
				exit_server(0);
			}
		}

		if ((bytes = get_packet(&packet, server_socket)) < 0) { /* this waits for a packet - idle */
			if (bytes == -1 && errno != EINTR) {
				DEBUG(LOG_INFO, "error on read, %s, reopening socket", strerror(errno));
				close(server_socket);
				server_socket = -1;
			}
			continue;
		} else {
			DEBUG(LOG_INFO, "Receive DHCP packet(ID: 0x%08x) from client %02x:%02x:%02x:%02x:%02x:%02x", packet.xid,
				packet.chaddr[0], packet.chaddr[1], packet.chaddr[2], packet.chaddr[3], packet.chaddr[4], packet.chaddr[5]);
		}

		/*
		 * Bug 22814[NETGARE]
		 * use get_raw_packet() to get datalink layer info
		 */
		gettimeofday(&tv, NULL);
		raw_read_timeout = ((tv.tv_sec * 1000000) + tv.tv_usec) + 1000000;
		raw_socket_read_succ = 0;
		do {
			if (get_raw_packet(&raw_packet, raw_server_socket) >= 0) {
				raw_socket_read_succ = 1;
				DEBUG(LOG_INFO, "Source MAC of DHCP packet(ID 0x%08x) is %02x:%02x:%02x:%02x:%02x:%02x", raw_packet.xid,
					g_src_addr[0], g_src_addr[1], g_src_addr[2], g_src_addr[3], g_src_addr[4], g_src_addr[5]);
				break;
			}
			gettimeofday(&tv, NULL);
		} while ((tv.tv_sec * 1000000) + tv.tv_usec < raw_read_timeout);

		if (raw_socket_read_succ == 0 || packet.xid != raw_packet.xid) {
			DEBUG(LOG_INFO, "Can not get the source MAC address of DHCP packet, will send response to the chaddr of this packet");
			memcpy(g_src_addr, packet.chaddr, 6);
		}

		if ((state = get_option(&packet, DHCP_MESSAGE_TYPE)) == NULL) {
			DEBUG(LOG_ERR, "couldn't get option from packet, ignoring");
			continue;
		}

#ifdef DHCPD_STATIC_LEASE
		/* Look for a static lease */
                static_lease_ip = get_ip_by_mac(&packet.chaddr);
#endif

		/* ADDME: look for a static lease */
#ifdef DHCPD_STATIC_LEASE
		if (static_lease_ip) {
                        memcpy(&static_lease.chaddr, &packet.chaddr, 16);
                        static_lease.yiaddr = static_lease_ip;
                        static_lease.expires = 0;

                        lease = &static_lease;
                } else
#endif
                lease = find_lease_by_chaddr(packet.chaddr);

		switch (state[0]) {
		case DHCPDISCOVER:
			DEBUG(LOG_INFO,"received DISCOVER");
			
			if (sendOffer(&packet) < 0) {
				LOG(LOG_ERR, "send OFFER failed");
			}
			break;			
 		case DHCPREQUEST:
			DEBUG(LOG_INFO, "received REQUEST");

			requested = get_option(&packet, DHCP_REQUESTED_IP);
			server_id = get_option(&packet, DHCP_SERVER_ID);
#ifdef DHCPD_HAVE_NAS
            //use hostname of dhcp request pakcet to distinguish nas dev
            t_name = get_option(&packet, DHCP_HOST_NAME);
			if( t_name != NULL ){
				  strncpy(h_name,t_name,3);
				  if(strcmp(h_name,"nas")==0) 
					  system("echo 1 > /tmp/nas_up");
		          g_nas=cat_file("/tmp/nas_up");
				  strncpy(nas_sta,g_nas,sizeof(nas_sta));
		          if(strcmp(nas_sta,"1")==0)
				      have_nas=1;
			      else have_nas=0;
			}
#endif

			if (requested) memcpy(&requested_align, requested, 4);
			if (server_id) memcpy(&server_id_align, server_id, 4);

#ifdef DHCPD_STATIC_LEASE
			if (requested && ip_reserved(requested_align) && static_lease_ip == 0) {
				sendNAK(&packet);
				break;
			}
#endif

			if (lease) { /*ADDME: or static lease */
				if (server_id) {
					/* SELECTING State */
					DEBUG(LOG_INFO, "server_id = %08x", ntohl(server_id_align));
					if (server_id_align == server_config.server && requested && 
					    requested_align == lease->yiaddr) {
#ifdef DHCPD_HAVE_NAS
//if nas_result is 0 means hashA=hashB, so need send NAk 
//and then will start another dhcp process to get another ip
                        if(have_nas==1)
                        {
                            nas_result=nas_dhcp(packet.chaddr,lease->yiaddr,h_name);
                            if(nas_result==0) 
                                sendNAK(&packet);
                            else
                                sendACK(&packet, lease->yiaddr);
                        }
                        else
#endif
						sendACK(&packet, lease->yiaddr);
					}
				} else {
					if (requested) {
						/* INIT-REBOOT State */
                        if (lease->yiaddr == requested_align)
                        {
#ifdef DHCPD_HAVE_NAS
                            if(have_nas==1)
                            {
                                nas_result=nas_dhcp(packet.chaddr,requested_align,h_name);
                                if(nas_result==0) 
                                    sendNAK(&packet);
                                else
                                    sendACK(&packet, lease->yiaddr);
                            }
                            else
#endif
                                sendACK(&packet, lease->yiaddr);
                        }
						else sendNAK(&packet);
					} else {
						/* RENEWING or REBINDING State */
						if (lease->yiaddr == packet.ciaddr)
                        {
#ifdef DHCPD_HAVE_NAS
                            if(have_nas==1)
                            {
                                nas_result=nas_dhcp(packet.chaddr,packet.ciaddr,h_name);
                                if(nas_result==0) 
                                    sendNAK(&packet);
                                else
                                    sendACK(&packet, lease->yiaddr);
                            }
                            else
#endif
                                sendACK(&packet, lease->yiaddr);
                        }
						else {
							/* don't know what to do!!!! */
							sendNAK(&packet);
						}
					}						
				}
			} else {
				if (server_id && server_id_align != server_config.server)
					break;

				if (requested)
					client_ip_align = requested_align;
				else if (packet.ciaddr)
					client_ip_align = packet.ciaddr;
				else
					break;

				if ((lease = find_lease_by_yiaddr(client_ip_align))) {
					if (lease_expired(lease)) {
						/* probably best if we drop this lease */
						memset(lease->chaddr, 0, 16);
					/* make some contention for this address */
					} else
						sendNAK(&packet);
				} else if (ntohl(client_ip_align) < ntohl(server_config.start) ||
					   ntohl(client_ip_align) > ntohl(server_config.end)) {
					sendNAK(&packet);
				} else {
					memset(client_hw_addr, 0, 6);
					if (arpping(client_ip_align, server_config.server, server_config.arp,
						server_config.interface, client_hw_addr) == 0) {
						if (bcmp(client_hw_addr, packet.chaddr, 6) == 0 || bcmp(client_hw_addr, g_src_addr, 6) == 0)
                        {
#ifdef DHCPD_HAVE_NAS
                            if(have_nas==1)
                            {
                                nas_result=nas_dhcp(packet.chaddr,client_ip_align,h_name);
                                if(nas_result==0)
                                    sendNAK(&packet);
                                else
                                    sendACK(&packet, client_ip_align);
                            }
                            else
#endif
                                sendACK(&packet, client_ip_align);
                        }
						else {
							sendNAK(&packet);
							add_lease(client_hw_addr, client_ip_align, server_config.conflict_time);
						}
					} else
                    {
#ifdef DHCPD_HAVE_NAS
                        if(have_nas==1)
                        {
                            nas_result=nas_dhcp(packet.chaddr,client_ip_align,h_name);
                                if(nas_result==0)
                                    sendNAK(&packet);
                                else
                                    sendACK(&packet, client_ip_align);
                        }
                        else
#endif
                            sendACK(&packet, client_ip_align);
                    }
				}
			}
			break;
		case DHCPDECLINE:
			DEBUG(LOG_INFO,"received DECLINE");
			if (lease) {
				memset(lease->chaddr, 0, 16);
				lease->expires = time(0) + server_config.decline_time;
			}			
			break;
		case DHCPRELEASE:
			DEBUG(LOG_INFO,"received RELEASE");
			if (lease) lease->expires = time(0);
			break;
		case DHCPINFORM:
			DEBUG(LOG_INFO,"received INFORM");
			send_inform(&packet);
			break;	
		default:
			LOG(LOG_WARNING, "unsupported DHCP message (%02x) -- ignoring", state[0]);
		}
	}

	return 0;
}

