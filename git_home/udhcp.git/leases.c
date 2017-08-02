/* 
 * leases.c -- tools to manage DHCP leases 
 * Russ Dill <Russ.Dill@asu.edu> July 2001
 */

#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "debug.h"
#include "dhcpd.h"
#include "files.h"
#include "options.h"
#include "leases.h"
#include "arpping.h"

unsigned char blank_chaddr[] = {[0 ... 15] = 0};
#ifdef DHCPD_HAVE_NAS    
extern u_int32_t latest_addr;
extern int nas_g;
#endif

/* clear every lease out that chaddr OR yiaddr matches and is nonzero */
void clear_lease(u_int8_t *chaddr, u_int32_t yiaddr)
{
	unsigned int i, j;
	
	for (j = 0; j < 16 && !chaddr[j]; j++);
	
	for (i = 0; i < server_config.max_leases; i++)
		if ((j != 16 && !memcmp(leases[i].chaddr, chaddr, 16)) ||
		    (yiaddr && leases[i].yiaddr == yiaddr)) {
			memset(&(leases[i]), 0, sizeof(struct dhcpOfferedAddr));
		}
}


/* add a lease into the table, clearing out any old ones */
struct dhcpOfferedAddr *add_lease(u_int8_t *chaddr, u_int32_t yiaddr, unsigned long lease)
{
	struct dhcpOfferedAddr *oldest;
	
	/* clean out any old ones */
	clear_lease(chaddr, yiaddr);
		
	oldest = oldest_expired_lease();
	
	if (oldest) {
		memcpy(oldest->chaddr, chaddr, 16);
		oldest->yiaddr = yiaddr;
		oldest->expires = time(0) + lease;
	}
	
	return oldest;
}


/* true if a lease has expired */
int lease_expired(struct dhcpOfferedAddr *lease)
{
	return (lease->expires < (unsigned long) time(0));
}	


/* Find the oldest expired lease, NULL if there are no expired leases */
struct dhcpOfferedAddr *oldest_expired_lease(void)
{
	struct dhcpOfferedAddr *oldest = NULL;
	unsigned long oldest_lease = time(0);
	unsigned int i;

	
	for (i = 0; i < server_config.max_leases; i++)
		if (oldest_lease > leases[i].expires) {
			oldest_lease = leases[i].expires;
			oldest = &(leases[i]);
		}
	return oldest;
		
}


/* Find the first lease that matches chaddr, NULL if no match */
struct dhcpOfferedAddr *find_lease_by_chaddr(u_int8_t *chaddr)
{
	unsigned int i;

	for (i = 0; i < server_config.max_leases; i++)
		if (!memcmp(leases[i].chaddr, chaddr, 16)) return &(leases[i]);
	
	return NULL;
}


/* Find the first lease that matches yiaddr, NULL is no match */
struct dhcpOfferedAddr *find_lease_by_yiaddr(u_int32_t yiaddr)
{
	unsigned int i;

	for (i = 0; i < server_config.max_leases; i++)
		if (leases[i].yiaddr == yiaddr) return &(leases[i]);
	
	return NULL;
}


/* find an assignable address, it check_expired is true, we check all the expired leases as well.
 * Maybe this should try expired leases by age... */
u_int32_t find_address(int check_expired) 
{
	u_int32_t addr, ret;
	struct dhcpOfferedAddr *lease = NULL;

#ifdef DHCPD_HAVE_NAS
//if hashA=hashB,use last ip as the start ip to get a free ip to assign to deviceB    
    if(nas_g==1)
    {
        addr = ntohl(latest_addr);
        for (;addr <= ntohl(server_config.end); addr++) {

            if (!(addr & 0xFF)) continue;
            if ((addr & 0xFF) == 0xFF) continue;
            ret = htonl(addr);
            if ((!(lease = find_lease_by_yiaddr(ret)) ||
                        (check_expired  && lease_expired(lease))) &&
#ifdef DHCPD_STATIC_LEASE
                    !ip_reserved(ret) &&
#endif
                    !check_ip(ret, NULL)) {
                return ret;
                break;
            }
        }
    }
    else
    {
#endif
	addr = ntohl(server_config.start); /* addr is in host order here */
	for (;addr <= ntohl(server_config.end); addr++) {

		/* ie, 192.168.55.0 */
		if (!(addr & 0xFF)) continue;

		/* ie, 192.168.55.255 */
		if ((addr & 0xFF) == 0xFF) continue;

		/* lease is not taken */
		ret = htonl(addr);
		if ((!(lease = find_lease_by_yiaddr(ret)) ||

		     /* or it expired and we are checking for expired leases */
		     (check_expired  && lease_expired(lease))) &&

#ifdef DHCPD_STATIC_LEASE
		     /* check the ip is not a reserved ip */
		     !ip_reserved(ret) &&
#endif
		     /* and it isn't on the network */
	    	     !check_ip(ret, NULL)) {
			return ret;
			break;
		}
	}
#ifdef DHCPD_HAVE_NAS    
    }
#endif
	return 0;
}


/* check is an IP is taken, if it is, add it to the lease table */
int check_ip(u_int32_t addr, u_int8_t *chaddr)
{
	struct in_addr temp;

#ifdef DHCPD_CHECK_SERVER_IP
	if (addr == server_config.server) {
		server_config.conflict_time = ~0;
		add_lease(blank_chaddr, addr, server_config.conflict_time);
		return 1;
	}
#endif

	if (arpping(addr, server_config.server, server_config.arp, server_config.interface, chaddr) == 0) {
		temp.s_addr = addr;
	 	LOG(LOG_INFO, "%s belongs to someone, reserving it for %ld seconds", 
	 		inet_ntoa(temp), server_config.conflict_time);
		add_lease(blank_chaddr, addr, server_config.conflict_time);
		return 1;
	} else return 0;
}
#ifdef DHCPD_STATIC_LEASE
/***************************************************************
 *             Static Lease
 ***************************************************************/

/* Check to see if a mac has an associated static lease */
uint32_t get_ip_by_mac(void *arg)
{
       uint8_t *mac = arg;
       struct static_lease *cur = server_config.static_leases;

       while (cur != NULL) {
               if (memcmp(cur->mac, mac, 6) == 0)
                       return cur->ip;

               cur = cur->next;
       }

       return 0;
}

/* Check to see if an ip is reserved as a static ip */
int ip_reserved(uint32_t ip)
{
       struct static_lease *cur = server_config.static_leases;

       while (cur != NULL) {
               if (cur->ip == ip)
                       return 1;

               cur = cur->next;
       }

       return 0;
}
#endif
