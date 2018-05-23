#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "dniconfig.h"


extern char *optarg;
/* libconfig.so */
extern char *config_get(char *name);
extern int config_match(char *name, char *match);
extern int config_invmatch(char *name, char *match);

#ifdef linux
#include <sys/utsname.h>
#include <sys/time.h>
typedef u_int32_t __u32;
#include <sys/timex.h>
#else
#define main ntpclient
extern struct hostent *gethostbyname(const char *name);
extern int h_errno;
#define herror(hostname) \
	fprintf(stderr,"Error %d looking up hostname %s\n", h_errno,hostname)
typedef uint32_t __u32;
#endif

#define JAN_1970        0x83aa7e80      /* 2208988800 1970 - 1900 in seconds */
#define NTP_PORT (123)
#define DAY_TIME 86400
#define NETGEAR_PERIOD 20

/* How to multiply by 4294.967296 quickly (and not quite exactly)
 * without using floating point or greater than 32-bit integers.
 * If you want to fix the last 12 microseconds of error, add in
 * (2911*(x))>>28)
 */
#define NTPFRAC(x) ( 4294*(x) + ( (1981*(x))>>11 ) )

/* The reverse of the above, needed if we want to set our microsecond
 * clock (via settimeofday) based on the incoming time in NTP format.
 * Basically exact.
 */
#define USEC(x) ( ( (x) >> 12 ) - 759 * ( ( ( (x) >> 10 ) + 32768 ) >> 16 ) )

/* Converts NTP delay and dispersion, apparently in seconds scaled
 * by 65536, to microseconds.  RFC1305 states this time is in seconds,
 * doesn't mention the scaling.
 * Should somehow be the same as 1000000 * x / 65536
 */
#define sec2u(x) ( (x) * 15.2587890625 )

struct ntptime {
	unsigned int coarse;
	unsigned int fine;
};

void send_packet(int usd, struct ntptime *udp_send_ntp);
int rfc1305print(char *data, struct ntptime *arrival, struct ntptime *udp_send_ntp);
int udp_handle(int usd, char *data, int data_len, struct sockaddr *sa_source, int sa_len, struct ntptime *udp_send_ntp);

/* global variables (I know, bad form, but this is a short program) */
char incoming[1500];
struct timeval time_of_send;
int set_clock=0;   /* non-zero presumably needs root privs */


int contemplate_data(unsigned int absolute, double skew, double errorbar, int freq);

int get_current_freq()
{
	/* OS dependent routine to get the current value of clock frequency.
	 */
#ifdef linux
	struct timex txc;
	txc.modes = 0;
	if (__adjtimex(&txc) < 0) {
		perror("adjtimex"); exit(1);
	}
	return txc.freq;
#else
	return 0;
#endif
}

void send_packet(int usd, struct ntptime *udp_send_ntp)
{
	__u32 data[12];
	struct timeval now;
#define LI 0
#define VN 3
#define MODE 3
#define STRATUM 0
#define POLL 4 
#define PREC -6

	if (sizeof(data) != 48) {
		fprintf(stderr,"size error\n");
		return;
	}
	bzero((char*)data,sizeof(data));
	data[0] = htonl (
		( LI << 30 ) | ( VN << 27 ) | ( MODE << 24 ) |
		( STRATUM << 16) | ( POLL << 8 ) | ( PREC & 0xff ) );
	data[1] = htonl(1<<16);  /* Root Delay (seconds) */
	data[2] = htonl(1<<16);  /* Root Dispersion (seconds) */
	gettimeofday(&now,NULL);
	data[10] = htonl(now.tv_sec + JAN_1970); /* Transmit Timestamp coarse */
	data[11] = htonl(NTPFRAC(now.tv_usec));  /* Transmit Timestamp fine   */
	
	udp_send_ntp->coarse = now.tv_sec + JAN_1970;
	udp_send_ntp->fine   = NTPFRAC(now.tv_usec);
	
	send(usd, data, 48, 0);
	time_of_send = now;
}

int udp_handle(int usd, char *data, int data_len, 
				  struct sockaddr *sa_source, int sa_len, struct ntptime *udp_send_ntp)
{
	struct timeval udp_arrival;
	struct ntptime udp_arrival_ntp;

#ifdef _PRECISION_SIOCGSTAMP
	if ( ioctl(usd, SIOCGSTAMP, &udp_arrival) < 0 ) {
		perror("ioctl-SIOCGSTAMP");
		gettimeofday(&udp_arrival, NULL);
	}
#else
	gettimeofday(&udp_arrival, NULL);
#endif
	udp_arrival_ntp.coarse = udp_arrival.tv_sec + JAN_1970;
	udp_arrival_ntp.fine = NTPFRAC(udp_arrival.tv_usec);
	
	if ((udp_arrival_ntp.coarse > (udp_send_ntp->coarse + 5)) 
		||((udp_arrival_ntp.coarse == (udp_send_ntp->coarse + 5))&&(udp_arrival_ntp.fine > udp_send_ntp->fine)))
		return -1;

	return(rfc1305print(data, &udp_arrival_ntp, udp_send_ntp));
}

double ntpdiff( struct ntptime *start, struct ntptime *stop)
{
	int a;
	unsigned int b;
	a = stop->coarse - start->coarse;
	if (stop->fine >= start->fine) {
		b = stop->fine - start->fine;
	} else {
		b = start->fine - stop->fine;
		b = ~b;
		a -= 1;
	}
	
	return a*1.e6 + b * (1.e6/4294967296.0);
}


int rfc1305print(char *data, struct ntptime *arrival, struct ntptime *udp_send_ntp)
{
/* straight out of RFC-1305 Appendix A */
	int li, vn, mode, stratum, poll, prec;
	int delay, disp, refid;
	struct ntptime reftime, orgtime, rectime, xmttime;
	double etime,stime,skew1,skew2;
	int freq;
	FILE *fp;
	char cmd[128];
	char *pid_file = "/tmp/run/syslogd.pid";

#define Data(i) ntohl(((unsigned int *)data)[i])
	li      = Data(0) >> 30 & 0x03;
	vn      = Data(0) >> 27 & 0x07;
	mode    = Data(0) >> 24 & 0x07;
	stratum = Data(0) >> 16 & 0xff;
	poll    = Data(0) >>  8 & 0xff;
	prec    = Data(0)       & 0xff;
	if (prec & 0x80) prec|=0xffffff00;
	delay   = Data(1);
	disp    = Data(2);
	refid   = Data(3);
	reftime.coarse = Data(4);
	reftime.fine   = Data(5);
	orgtime.coarse = Data(6);
	orgtime.fine   = Data(7);
	rectime.coarse = Data(8);
	rectime.fine   = Data(9);
	xmttime.coarse = Data(10);
	xmttime.fine   = Data(11);
#undef Data

	if ((orgtime.coarse != udp_send_ntp->coarse)
		|| ((orgtime.coarse == udp_send_ntp->coarse) && (orgtime.fine != udp_send_ntp->fine))) {
		return -1;
	}
	if ((xmttime.coarse == 0) && (xmttime.fine == 0)) {
		return -1;
	}
	if (mode != 4) {
		return -1;
	}

	if (set_clock) {   /* you'd better be root, or ntpclient will crash! */
		struct timeval tv_set;
		/* it would be even better to subtract half the slop */
		tv_set.tv_sec  = xmttime.coarse - JAN_1970;
		/* divide xmttime.fine by 4294.967296 */
		tv_set.tv_usec = USEC(xmttime.fine);
		
		if (settimeofday(&tv_set,NULL) < 0) {
			perror("settimeofday");
			exit(1);
		}
	}

	etime = ntpdiff(&orgtime, arrival);
	stime = ntpdiff(&rectime, &xmttime);
	skew1 = ntpdiff(&orgtime, &rectime);
	skew2 = ntpdiff(&xmttime, arrival);
	freq = get_current_freq();
	
	
	/* log the first entry */
	system("[ -f /tmp/ntp_updated ] || { "
			"touch /tmp/ntp_updated; "
			"/usr/sbin/ntpst set; "
			"}");


	if (access(pid_file,F_OK) == 0){
		sprintf(cmd,"kill -USR1 $(cat %s)",pid_file);
		system(cmd);
		sleep(1);
	}
	syslog(LOG_WARNING, "[Time synchronized with NTP server]");
	fflush(stdout);

	return 1;
}

int check_valid_ipaddr(char *ip, unsigned int *ip_addr)
{
	*ip_addr = inet_addr(ip);

	if ((INADDR_NONE == *ip_addr) || (*ip_addr == 0))
		return 0;

	return 1;
}

int stuff_net_addr(struct in_addr *p, char *hostname)
{
	struct hostent *ntpserver;

	ntpserver = gethostbyname(hostname);
	if (ntpserver == NULL) {
		/* avoid printing: "time-h.netgear.com: Unknown host" */
		/* herror(hostname); */
		return 0;
	}
	if (ntpserver->h_length != 4) {
		fprintf(stderr, "oops %d\n", ntpserver->h_length);
		return 0;
	}
	
	memcpy(&(p->s_addr), ntpserver->h_addr_list[0], 4);
	return 1;
}

int setup_receive(int usd, unsigned int interface, unsigned short port)
{
	struct sockaddr_in sa_rcvr;

	bzero((char *) &sa_rcvr, sizeof(sa_rcvr));
	sa_rcvr.sin_family = AF_INET;
	sa_rcvr.sin_addr.s_addr = htonl(interface);
	sa_rcvr.sin_port = htons(port);

	if (bind(usd, (struct sockaddr *) &sa_rcvr, sizeof(sa_rcvr)) == -1) {
		fprintf(stderr, "Could not bind to udp port %d\n", port);
		perror("bind");
		return 0;
	}

	listen(usd, 3);

	/* Make "usd" close on child process when call system(),
	 * so that the child process will not inherit the parent resource */
	fcntl(usd, F_SETFD, FD_CLOEXEC);

	return 1;
}

int setup_transmit(int usd, char *host, unsigned short port)
{
	struct sockaddr_in sa_dest;
	unsigned int ip_addr=0;
	
	bzero((char *)&sa_dest, sizeof(sa_dest));
	sa_dest.sin_family = AF_INET;
	
	/*Spec NTP V2 the input field of the "Set your preferred NTP server" option shall support both entering*/
	/*an IP address or DNS address*/
	if (check_valid_ipaddr(host, &ip_addr)) 
	{
	    sa_dest.sin_addr.s_addr = inet_addr(host);
	} 
	else 
	{
            if (!stuff_net_addr(&(sa_dest.sin_addr), host))
			return 0;
	}
	sa_dest.sin_port = htons(port);
	
	if (connect(usd, (struct sockaddr *)&sa_dest, sizeof(sa_dest)) == -1) {
		perror("connect");
		return 0;
	}

	return 1;
}

void primary_loop(int usd, int num_probes, int cycle_time)
{
	fd_set fds;
	struct sockaddr sa_xmit;
	int i, pack_len, sa_xmit_len, probes_sent;
	struct timeval to;
	struct ntptime udp_send_ntp;
	int steady_state = 0;

	probes_sent = 0;
	sa_xmit_len = sizeof(sa_xmit);
	to.tv_sec = 0;
	to.tv_usec = 0;
	
	for (;;) {
		FD_ZERO(&fds);
		FD_SET(usd, &fds);
		i = select(usd+1, &fds, NULL, NULL, &to);  /* Wait on read or error */
		if ((i != 1) || (!FD_ISSET(usd,&fds))) {
			if (i == EINTR) continue;
			if (i < 0) perror("select");
			
			if ((to.tv_sec == 0) || (to.tv_sec == cycle_time) 
					|| (to.tv_sec == DAY_TIME)) {
				if (steady_state != 1 
					&& probes_sent >= num_probes && num_probes != 0) {
					break;
				}
				steady_state = 0;
				send_packet(usd, &udp_send_ntp);
				++probes_sent;
				to.tv_sec = cycle_time;
				to.tv_usec = 0;
			}
			continue;
		}
		
		pack_len = recvfrom(usd, incoming, sizeof(incoming), 0, &sa_xmit, &sa_xmit_len);
		
		if (pack_len < 0) {
			perror("recvfrom");
		    /* A query receives no successful response, the retry algorithm must 
			*  wait that random delay period before initiating the first retry query.
			*/
			select(1, NULL, NULL, NULL, &to);
		} else if (pack_len > 0 && pack_len < sizeof(incoming)) {
			steady_state = udp_handle(usd, incoming, pack_len, 
						&sa_xmit, sa_xmit_len, &udp_send_ntp);
		} else {
			printf("Ooops.  pack_len=%d\n", pack_len);
			fflush(stdout);
		}

		if (steady_state == 1) {
			//to.tv_sec = DAY_TIME;
			//to.tv_usec = 0;
			exit(0);
		} else if (probes_sent >= num_probes && num_probes != 0) {
			break;
		}
	}
	/*when program is out of primary loop,the NTP server is fail,so delete the file.*/
	system("rm -f /tmp/ntp_updated");
	return;
}

/****************************************************************************
		 Before sending NTP packet, the WAN connection must be connected. 
  ****************************************************************************/
#define PPP_STATUS	"/etc/ppp/ppp0-status"
#define PPP1_STATUS      "/etc/ppp/pppoe1-status"
#define BPA_STATUS	"/tmp/bpa_info"
#define CABLE_FILE	"/tmp/port_status"

enum {
	NET_PPP = 1,
	NET_OTHER	// BPA & DHCP & StaticIP
};

struct in_addr get_ipaddr(char *ifname)
{
	int fd;
	struct ifreq ifr;
	struct in_addr pa;

	pa.s_addr = 0;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0)
		return pa;

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_addr.sa_family = AF_INET;
	strcpy(ifr.ifr_name, ifname);
	if (ioctl(fd, SIOCGIFADDR, &ifr) == 0)
		pa = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr;
	close(fd);

	return pa;
}

/* '\0' means read failed */
static char readc(char *file)
{
	int fd;
	char value;

	fd = open(file, O_RDONLY, 0666);
	if (fd < 0)
		return 0;
	if (read(fd, &value, 1) != 1)
		value = 0;
	close(fd);

	return value;
}

/* DHCP / StaticIP ... */
static inline int eth_up(void)
{
	return readc(CABLE_FILE) == '1';
}

/* It is ONLY used for PPPoE/PPTP mode. */
static inline int ppp_up(void)
{
	return readc(PPP_STATUS) == '1';
}

/* It is ONLY used for mulpppoe mode. */
static inline int ppp_up_mul(void)
{
        return readc(PPP1_STATUS) == '1';
}

/* 1). `up time: %lu`; 2). `down time: %lu` */
static inline int bpa_up(void)
{
	return readc(BPA_STATUS) == 'u';
}

static int net_verified(int *modep)
{
	char *p;
	int mode, alive;

	p = config_get("wan_proto");
	if (!strcmp(p, "pppoe") ||!strcmp(p, "pptp")) {
		mode = NET_PPP;
		alive = ppp_up();
	} else if (!strcmp(p, "bigpond")) {
		mode = NET_OTHER;
		alive = bpa_up();
	} else if (!strcmp(p, "mulpppoe1")) {
		mode = NET_PPP;
		alive = ppp_up_mul();
	} else {
		mode = NET_OTHER;
		alive = eth_up();
	}
	*modep = mode;

	return alive;
}

static int wan_conn_up(void)
{
	int mode, alive;
	struct in_addr ip;

	alive = net_verified(&mode);
	if (alive == 0)
		ip.s_addr = 0;
	else if (mode == NET_PPP)
		ip = get_ipaddr(PPP_IFNAME);
	else
		ip = get_ipaddr(NET_IFNAME);

	return ip.s_addr != 0;
}


int main(int argc, char *argv[]) {
	int usd;  /* socket */
	int c;
	/* These parameters are settable from the command line
	   the initializations here provide default behavior */
	unsigned short udp_local_port = 0;   /* default of 0 means kernel chooses */
	int probe_count = 1;            /* default of 0 means loop forever */
	int cycle_time = 15;          /* request timeout in seconds */
	int min_interval = 0;
	int max_interval = 0;
	char *hostname = NULL;          /* must be set */
	char *sec_host = "0.0.0.0";
	char *ntps = "0.0.0.0";
	struct timeval to;
	FILE *fp = NULL;

	unsigned long seed = 0;

	seed = time(0);
	srand(seed);

	/* ntpclient -h time.stdtime.gov.tw -s */
	for (;;) {
		c = getopt(argc, argv, "c:h:s");
		if (c == EOF) break;
		switch (c) {
			case 'c':
 				probe_count = atoi(optarg);
				break;
			case 'h':
				hostname = optarg;
				break;
			case 's':
				set_clock = 1;
				probe_count = 1;
				break;
			default:
				exit(1);
		}
	}

	if (hostname == NULL) {
		exit(1);
	}

	if (strcmp(sec_host, "0.0.0.0") == 0)
		sec_host = hostname;

	if (min_interval > max_interval || min_interval < 0 || max_interval < 0) 
	{
		exit(1);
	} 
	else if (max_interval == 0) 
	{
		max_interval = cycle_time;
		min_interval = cycle_time;
	} 
	else
	{
		cycle_time = min_interval + rand()%(max_interval-min_interval+1);
	}
	daemon(1, 1);


	while(1) {
		ntps = (strcmp(ntps, hostname) == 0) ? sec_host : hostname;

		/* Startup sequence */
		if ((usd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
			perror ("socket");
			goto cont;
		}

		if (!wan_conn_up() && config_match("ap_mode", "0") && config_match("bridge_mode", "0")) {
			/* printf("The WAN connection is NOT up!\n"); */
			close(usd);
			goto cont;
		}

		if (!setup_receive(usd, INADDR_ANY, udp_local_port)
		    || !setup_transmit(usd, ntps, NTP_PORT)) 
		{
			close(usd);
			to.tv_sec = cycle_time;
			to.tv_usec = 0;
			select(1, NULL, NULL, NULL, &to);
			goto loop;
		}

		primary_loop(usd, probe_count, 5);
		close(usd);
	loop:
		/* [NETGEAR Spec 8.6]:Subsequent queries will double the preceding query interval 
		 * until the interval has exceeded the steady state query interval, at which point 
		 * and new random interval between 15.00 and 60.00 seconds is selected and the 
		 * process repeats.
		 */

		if ((cycle_time * 2) > DAY_TIME)
			cycle_time = min_interval + rand()%(max_interval-min_interval+1);
		else
			cycle_time = cycle_time * 2;
		continue;

	cont:	
		/* [NETGEAR Spec 8.6]: we will wait randomly calculated period of 0 to 240 seconds 
		 * before issuing the first NTP query upon subsequent power-ons or resets. 
		 */
		
		to.tv_sec = rand() % (NETGEAR_PERIOD + 1);
		to.tv_usec = 0;
		select(1, NULL, NULL, NULL, &to);

	}
	
	return 0;
}
