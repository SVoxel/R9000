/* dnsmasq is Copyright (c) 2000-2006 Simon Kelley

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 dated June, 1991.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#include "dnsmasq.h"

static char *compile_opts = 
#ifndef HAVE_IPV6
"no-"
#endif
"IPv6 "
#ifndef HAVE_GETOPT_LONG
"no-"
#endif
"GNU-getopt "
#ifdef HAVE_BROKEN_RTC
"no-RTC "
#endif
#ifdef NO_FORK
"no-MMU "
#endif
#ifndef HAVE_ISC_READER
"no-"
#endif
"ISC-leasefile "
#ifndef HAVE_DBUS
"no-"
#endif
"DBus "
#ifdef NO_GETTEXT
"no-"
#endif
"I18N "
#ifndef HAVE_TFTP
"no-"
#endif
"TFTP";

static pid_t pid;
static int pipewrite;

#ifdef DNI_IPV6_FEATURE
extern void check_timeout_forward(struct daemon *daemon, time_t now);
#endif

static int set_dns_listeners(struct daemon *daemon, time_t now, fd_set *set, int *maxfdp);
static void check_dns_listeners(struct daemon *daemon, fd_set *set, time_t now);
static void sig_handler(int sig);

int in_hijack;
#ifdef DNI_PARENTAL_CTL
int parentalcontrol_enable = 0;
char *pc_table_file = "/tmp/parentalcontrol.conf";
#endif
#ifdef BIND_SRVSOCK_TO_WAN
char *wan_ifname = ""; /* interface name of WAN */
int bind_wan_success = 0; /* To fix bug 22747, indicate whether server socket bind to WAN interface successfully */
#endif

//ifdef SUP_MUL_PPPOE
/**** support for multi pppoe function ***************/
struct dname_record *dname_list = NULL;
char *dname_check_file = "/etc/ppp/pppoe2-domain.conf";
time_t dname_save_time = 0;
//endif

int main (int argc, char **argv)
{
  struct daemon *daemon;
  int bind_fallback = 0;
  int bad_capabilities = 0;
  time_t now, last = 0;
  struct sigaction sigact;
  struct iname *if_tmp;
  int piperead, pipefd[2], log_fd;
  unsigned char sig;
  
#ifndef NO_GETTEXT
  setlocale(LC_ALL, "");
  bindtextdomain("dnsmasq", LOCALEDIR); 
  textdomain("dnsmasq");
#endif

  pid = 0;
 
  sigact.sa_handler = sig_handler;
  sigact.sa_flags = 0;
  sigemptyset(&sigact.sa_mask);
  sigaction(SIGUSR1, &sigact, NULL);
  sigaction(SIGUSR2, &sigact, NULL);
  sigaction(SIGHUP, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);
  sigaction(SIGALRM, &sigact, NULL);
  sigaction(SIGCHLD, &sigact, NULL);

  /* ignore SIGPIPE */
  sigact.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &sigact, NULL);

  daemon = read_opts(argc, argv, compile_opts);
  log_fd = log_start(daemon); 
  
  if (daemon->edns_pktsz < PACKETSZ)
    daemon->edns_pktsz = PACKETSZ;
  daemon->packet_buff_sz = daemon->edns_pktsz > DNSMASQ_PACKETSZ ? 
    daemon->edns_pktsz : DNSMASQ_PACKETSZ;
  daemon->packet = safe_malloc(daemon->packet_buff_sz);
  
  if (!daemon->lease_file)
    {
      if (daemon->dhcp)
	daemon->lease_file = LEASEFILE;
    }
#ifndef HAVE_ISC_READER
  else if (!daemon->dhcp)
    die(_("ISC dhcpd integration not available: set HAVE_ISC_READER in src/config.h"), NULL);
#endif
  
#ifdef HAVE_LINUX_NETWORK
  netlink_init(daemon);
#elif !(defined(IP_RECVDSTADDR) && \
	defined(IP_RECVIF) && \
	defined(IP_SENDSRCADDR))
  if (!(daemon->options & OPT_NOWILD))
    {
      bind_fallback = 1;
      daemon->options |= OPT_NOWILD;
    }
#endif

#ifndef HAVE_TFTP
  if (daemon->options & OPT_TFTP)
    die(_("TFTP server not available: set HAVE_TFTP in src/config.h"), NULL);
#endif

  daemon->interfaces = NULL;
  if (!enumerate_interfaces(daemon))
    die(_("failed to find list of interfaces: %s"), NULL);
    
  if (daemon->options & OPT_NOWILD) 
    {
      daemon->listeners = create_bound_listeners(daemon);

      for (if_tmp = daemon->if_names; if_tmp; if_tmp = if_tmp->next)
	if (if_tmp->name && !if_tmp->used)
	  die(_("unknown interface %s"), if_tmp->name);
  
      for (if_tmp = daemon->if_addrs; if_tmp; if_tmp = if_tmp->next)
	if (!if_tmp->used)
	  {
	    prettyprint_addr(&if_tmp->addr, daemon->namebuff);
	    die(_("no interface with address %s"), daemon->namebuff);
	  }
    }
  else if (!(daemon->listeners = create_wildcard_listeners(daemon->port, daemon->options & OPT_TFTP)))
    die(_("failed to create listening socket: %s"), NULL);
  
  cache_init(daemon->cachesize, daemon->options & OPT_LOG);

  now = dnsmasq_time();
  
  if (daemon->dhcp)
    {
#if !defined(HAVE_LINUX_NETWORK) && !defined(IP_RECVIF)
      int c;
      struct iname *tmp;
      for (c = 0, tmp = daemon->if_names; tmp; tmp = tmp->next)
	if (!tmp->isloop)
	  c++;
      if (c != 1)
	die(_("must set exactly one interface on broken systems without IP_RECVIF"), NULL);
#endif
      dhcp_init(daemon);
      lease_init(daemon, now);
    }

  if (daemon->options & OPT_DBUS)
#ifdef HAVE_DBUS
    {
      char *err;
      daemon->dbus = NULL;
      daemon->watches = NULL;
      if ((err = dbus_init(daemon)))
	die(_("DBus error: %s"), err);
    }
#else
  die(_("DBus not available: set HAVE_DBUS in src/config.h"), NULL);
#endif
  
  /* If query_port is set then create a socket now, before dumping root
     for use to access nameservers without more specific source addresses.
     This allows query_port to be a low port */
  if (daemon->query_port)
    {
      union  mysockaddr addr;
      memset(&addr, 0, sizeof(addr));
      addr.in.sin_family = AF_INET;
      addr.in.sin_addr.s_addr = INADDR_ANY;
      addr.in.sin_port = htons(daemon->query_port);
#ifdef HAVE_SOCKADDR_SA_LEN
      addr.in.sin_len = sizeof(struct sockaddr_in);
#endif
      allocate_sfd(&addr, &daemon->sfds);
#ifdef HAVE_IPV6
      memset(&addr, 0, sizeof(addr));
      addr.in6.sin6_family = AF_INET6;
      addr.in6.sin6_addr = in6addr_any;
      addr.in6.sin6_port = htons(daemon->query_port);
#ifdef HAVE_SOCKADDR_SA_LEN
      addr.in6.sin6_len = sizeof(struct sockaddr_in6);
#endif
      allocate_sfd(&addr, &daemon->sfds);
#endif
    }
  
  /* Use a pipe to carry signals back to the event loop in a race-free manner */
  if (pipe(pipefd) == -1 || !fix_fd(pipefd[0]) || !fix_fd(pipefd[1])) 
    die(_("cannot create pipe: %s"), NULL);
  
  piperead = pipefd[0];
  pipewrite = pipefd[1];
  /* prime the pipe to load stuff first time. */
  sig = SIGHUP; 
  write(pipewrite, &sig, 1);
  
  if (!(daemon->options & OPT_DEBUG))   
    {
      FILE *pidfile;
      fd_set test_set;
      int maxfd = -1, i; 
      int nullfd = open("/dev/null", O_RDWR);

      /* The following code "daemonizes" the process. 
	 See Stevens section 12.4 */

#ifndef NO_FORK      
      if (!(daemon->options & OPT_NO_FORK))
	{
	  if (fork() != 0 )
	    _exit(0);
	  
	  setsid();
	  
	  if (fork() != 0)
	    _exit(0);
	}
#endif
      
      chdir("/");
      umask(022); /* make pidfile 0644 */
      
      /* write pidfile _after_ forking ! */
      if (daemon->runfile && (pidfile = fopen(daemon->runfile, "w")))
      	{
	  fprintf(pidfile, "%d\n", (int) getpid());
	  fclose(pidfile);
	}
      
      umask(0);
      
      FD_ZERO(&test_set);
      set_dns_listeners(daemon, now, &test_set, &maxfd);
#ifdef HAVE_DBUS
      set_dbus_listeners(daemon, &maxfd, &test_set, &test_set, &test_set);
#endif
      for (i=0; i<64; i++)
	{
	  if (i == piperead || i == pipewrite || i == log_fd)
	    continue;

#ifdef HAVE_LINUX_NETWORK
	  if (i == daemon->netlinkfd)
	    continue;
#endif
	  
	  if (daemon->dhcp && 
	      ((daemon->lease_stream && i == fileno(daemon->lease_stream)) || 
#ifndef HAVE_LINUX_NETWORK
	       i == daemon->dhcp_raw_fd ||
	       i == daemon->dhcp_icmp_fd ||
#endif
	       i == daemon->dhcpfd))
	    continue;

	  if (i <= maxfd && FD_ISSET(i, &test_set))
	    continue;

	  /* open  stdout etc to /dev/null */
	  if (i == STDOUT_FILENO || i == STDERR_FILENO || i == STDIN_FILENO)
	    dup2(nullfd, i);
	  else
	    close(i);
	}
    }
  
  /* if we are to run scripts, we need to fork a helper before dropping root. */
  daemon->helperfd = create_helper(daemon, log_fd);
   
  if (!(daemon->options & OPT_DEBUG))   
    {
      /* UID changing, etc */
      struct passwd *ent_pw = daemon->username ? getpwnam(daemon->username) : NULL;    
      
      if (daemon->groupname || ent_pw)
	{
	  gid_t dummy;
	  struct group *gp;
	  
	  /* change group for /etc/ppp/resolv.conf otherwise get the group for "nobody" */
	  if ((daemon->groupname && (gp = getgrnam(daemon->groupname))) || 
	      (ent_pw && (gp = getgrgid(ent_pw->pw_gid))))
	    {
	      /* remove all supplimentary groups */
	      setgroups(0, &dummy);
	      setgid(gp->gr_gid);
	    } 
	}
      
      if (ent_pw && ent_pw->pw_uid != 0)
	{     
#ifdef HAVE_LINUX_NETWORK
	  /* On linux, we keep CAP_NETADMIN (for ARP-injection) and
	     CAP_NET_RAW (for icmp) if we're doing dhcp */
	  cap_user_header_t hdr = safe_malloc(sizeof(*hdr));
	  cap_user_data_t data = safe_malloc(sizeof(*data));
	  hdr->version = _LINUX_CAPABILITY_VERSION;
	  hdr->pid = 0; /* this process */
	  data->effective = data->permitted = data->inheritable =
	    (1 << CAP_NET_ADMIN) | (1 << CAP_NET_RAW) |
	    (1 << CAP_SETGID) | (1 << CAP_SETUID);
	  
	  /* Tell kernel to not clear capabilities when dropping root */
	  if (capset(hdr, data) == -1 || prctl(PR_SET_KEEPCAPS, 1) == -1)
	    bad_capabilities = errno;
	  else
#endif
	    {
	      /* finally drop root */
	      setuid(ent_pw->pw_uid);
	      
#ifdef HAVE_LINUX_NETWORK
	      data->effective = data->permitted = 
		(1 << CAP_NET_ADMIN) | (1 << CAP_NET_RAW);
	      data->inheritable = 0;
	      
	      /* lose the setuid and setgid capbilities */
	      capset(hdr, data);
#endif
	    }
	}
    }
  
#ifdef HAVE_LINUX_NETWORK
  if (daemon->options & OPT_DEBUG) 
    prctl(PR_SET_DUMPABLE, 1);
#endif

  if (daemon->cachesize != 0)
    my_syslog(LOG_INFO, _("started, version %s cachesize %d"), VERSION, daemon->cachesize);
  else
    my_syslog(LOG_INFO, _("started, version %s cache disabled"), VERSION);
  
  my_syslog(LOG_INFO, _("compile time options: %s"), compile_opts);
  
#ifdef HAVE_DBUS
  if (daemon->options & OPT_DBUS)
    {
      if (daemon->dbus)
	my_syslog(LOG_INFO, _("DBus support enabled: connected to system bus"));
      else
	my_syslog(LOG_INFO, _("DBus support enabled: bus connection pending"));
    }
#endif

  if (bind_fallback)
    my_syslog(LOG_WARNING, _("setting --bind-interfaces option because of OS limitations"));
  
  if (!(daemon->options & OPT_NOWILD)) 
    for (if_tmp = daemon->if_names; if_tmp; if_tmp = if_tmp->next)
      if (if_tmp->name && !if_tmp->used)
	my_syslog(LOG_WARNING, _("warning: interface %s does not currently exist"), if_tmp->name);
   
  if (daemon->options & OPT_NO_RESOLV)
    {
      if (daemon->resolv_files && !daemon->resolv_files->is_default)
	my_syslog(LOG_WARNING, _("warning: ignoring resolv-file flag because no-resolv is set"));
      daemon->resolv_files = NULL;
      if (!daemon->servers)
	my_syslog(LOG_WARNING, _("warning: no upstream servers configured"));
    } 

  if (daemon->max_logs != 0)
    my_syslog(LOG_INFO, _("asynchronous logging enabled, queue limit is %d messages"), daemon->max_logs);

  if (daemon->dhcp)
    {
      struct dhcp_context *dhcp_tmp;
      
      for (dhcp_tmp = daemon->dhcp; dhcp_tmp; dhcp_tmp = dhcp_tmp->next)
	{
	  prettyprint_time(daemon->dhcp_buff2, dhcp_tmp->lease_time);
	  strcpy(daemon->dhcp_buff, inet_ntoa(dhcp_tmp->start));
	  my_syslog(LOG_INFO, 
		    (dhcp_tmp->flags & CONTEXT_STATIC) ? 
		    _("DHCP, static leases only on %.0s%s, lease time %s") :
		    _("DHCP, IP range %s -- %s, lease time %s"),
		    daemon->dhcp_buff, inet_ntoa(dhcp_tmp->end), daemon->dhcp_buff2);
	}
    }

#ifdef HAVE_TFTP
  if (daemon->options & OPT_TFTP)
    {
      long max_fd = sysconf(_SC_OPEN_MAX);

#ifdef FD_SETSIZE
      if (FD_SETSIZE < max_fd)
	max_fd = FD_SETSIZE;
#endif

      my_syslog(LOG_INFO, "TFTP %s%s %s", 
		daemon->tftp_prefix ? _("root is ") : _("enabled"),
		daemon->tftp_prefix ? daemon->tftp_prefix: "",
		daemon->options & OPT_TFTP_SECURE ? _("secure mode") : "");
      
      /* This is a guess, it assumes that for small limits, 
	 disjoint files might be served, but for large limits, 
	 a single file will be sent to may clients (the file only needs
	 one fd). */

      max_fd -= 30; /* use other than TFTP */
      
      if (max_fd < 0)
	max_fd = 5;
      else if (max_fd < 100)
	max_fd = max_fd/2;
      else
	max_fd = max_fd - 20;

      if (daemon->tftp_max > max_fd)
	{
	  daemon->tftp_max = max_fd;
	  my_syslog(LOG_WARNING, 
		    _("restricting maximum simultaneous TFTP transfers to %d"), 
		    daemon->tftp_max);
	}
    }
#endif

  if (!(daemon->options & OPT_DEBUG) && (getuid() == 0 || geteuid() == 0))
    {
      if (bad_capabilities)
	my_syslog(LOG_WARNING, _("warning: setting capabilities failed: %s"), strerror(bad_capabilities));

      my_syslog(LOG_WARNING, _("running as root"));
    }
  
  check_servers(daemon);

#ifdef SUP_STATIC_PPTP
  if (1 == daemon->static_pptp_enable) {
    load_static_pptp_server(daemon);
    alarm(1);
  }
#endif

  pid = getpid();
  
  while (1)
    {
      int maxfd = -1;
      struct timeval t, *tp = NULL;
      fd_set rset, wset, eset;
      
      FD_ZERO(&rset);
      FD_ZERO(&wset);
      FD_ZERO(&eset);
      
      /* if we are out of resources, find how long we have to wait
	 for some to come free, we'll loop around then and restart
	 listening for queries */
      if ((t.tv_sec = set_dns_listeners(daemon, now, &rset, &maxfd)) != 0)
	{
	  t.tv_usec = 0;
	  tp = &t;
	}

      /* Whilst polling for the dbus, or doing a tftp transfer, wake every quarter second */
      if (daemon->tftp_trans ||
	  ((daemon->options & OPT_DBUS) && !daemon->dbus))
	{
	  t.tv_sec = 0;
	  t.tv_usec = 250000;
	  tp = &t;
	}

#ifdef HAVE_DBUS
      set_dbus_listeners(daemon, &maxfd, &rset, &wset, &eset);
#endif	
  
      if (daemon->dhcp)
	{
	  FD_SET(daemon->dhcpfd, &rset);
	  bump_maxfd(daemon->dhcpfd, &maxfd);
	}

#if 0 //def HAVE_LINUX_NETWORK
      FD_SET(daemon->netlinkfd, &rset);
      bump_maxfd(daemon->netlinkfd, &maxfd);
#endif
      
      FD_SET(piperead, &rset);
      bump_maxfd(piperead, &maxfd);

#if 0     
      while (helper_buf_empty() && do_script_run(daemon));

      if (!helper_buf_empty())
	{
	  FD_SET(daemon->helperfd, &wset);
	  bump_maxfd(daemon->helperfd, &maxfd);
	}
#endif
      
      /* must do this just before select(), when we know no
	 more calls to my_syslog() can occur */
      set_log_writer(&wset, &maxfd);
      
      /* set `select` timeout value to 2 seconds, so we can check 
	 the resolv files as soon as it is modified once per second 
	 max.. Well, it maybe influence the performance, and it may 
	 be a NOT GOOD idea, but it works ... */
      tp = &t;

#ifndef DNI_IPV6_FEATURE
      tp->tv_sec = 2;
#else
     /* as requests of DNS in IPv6 spec, maybe one query have to re-sent
        if previous query timeout, and if one query was sent and no
        receive response in 3 seconds, we consider it is timeout.
        if use 2 seconds in `select`, after 4 seconds passed since one
        query was sent and no receive response, it could re-sent query
        again. so change this value to 1 second. */
      tp->tv_sec = 1;
#endif
      tp->tv_usec = 0; 
      if (select(maxfd+1, &rset, &wset, &eset, tp) < 0)
	{
	  /* otherwise undefined after error */
	  FD_ZERO(&rset); FD_ZERO(&wset); FD_ZERO(&eset);
	}

      now = dnsmasq_time();

      check_log_writer(&wset);

      /* Check for changes to resolv files once per second max. */
      /* Don't go silent for long periods if the clock goes backwards. */
      if (last == 0 || difftime(now, last) > 1.0 || difftime(now, last) < -1.0)
	{
	  last = now;

#ifdef HAVE_ISC_READER
	  if (daemon->lease_file && !daemon->dhcp)
	    load_dhcp(daemon, now);
#endif

	  if (!(daemon->options & OPT_NO_POLL))
	    {
	      struct resolvc *res, *latest;
	      struct stat statbuf;
	      time_t last_change = 0;
	      /* There may be more than one possible file. 
		 Go through and find the one which changed _last_.
		 Warn of any which can't be read. */
	      for (latest = NULL, res = daemon->resolv_files; res; res = res->next)
		if (stat(res->name, &statbuf) == -1)
		  {
		    if (!res->logged)
		      my_syslog(LOG_WARNING, _("failed to access %s: %s"), res->name, strerror(errno));
		    res->logged = 1;
		  }
		else
		  {
		    res->logged = 0;
		    if (statbuf.st_mtime != res->mtime || statbuf.st_size != res->size)
		      {
			res->mtime = statbuf.st_mtime;
			res->size = statbuf.st_size;
			/* For we have only one `/tmp/resolv.conf` file, so we check the file whether modified
			   or not by comparing the `mtime`( != ), NOT after(>) or before(<); Because the system 
			   time is adjusted when NTP updating. */
			#if 1
			latest = res;
			#else
			if (difftime(statbuf.st_mtime, last_change) > 0.0)
			  {
			    last_change = statbuf.st_mtime;
			    latest = res;
			  }
			#endif
		      }
		  }
	      
	      if (latest)
		{
		  static int warned = 0;
		  if (reload_servers(latest->name, daemon))
		    {
		      my_syslog(LOG_INFO, _("reading %s"), latest->name);
		      warned = 0;
		      check_servers(daemon);
		      if (daemon->options & OPT_RELOAD)
			cache_reload(daemon->options, daemon->namebuff, daemon->domain_suffix, daemon->addn_hosts);
		    }
		  else 
		    {
		      latest->mtime = 0;
		      if (!warned)
			{
			  my_syslog(LOG_WARNING, _("no servers found in %s, will retry"), latest->name);
			  warned = 1;
			}
		    }
		}
#ifdef SUP_STATIC_PPTP
	      if (1 == daemon->static_pptp_enable) {
	        struct stat statbuf;
		if (NULL != daemon->pptp_resolv_file && -1 != stat(daemon->pptp_resolv_file->name, &statbuf)) {
		  if (statbuf.st_mtime != daemon->pptp_resolv_file->mtime) {
		    if (0 == load_static_pptp_server(daemon))
		      daemon->pptp_resolv_file->mtime = statbuf.st_mtime;
		  }
		}
	      }
#endif
	    }
	}

      if (FD_ISSET(piperead, &rset))
	{
	  pid_t p;

	  if (read(piperead, &sig, 1) == 1)
	    switch (sig)
	      {
	      case SIGHUP:
		clear_cache_and_reload(daemon, now);
		if (daemon->resolv_files && (daemon->options & OPT_NO_POLL))
		  {
		    reload_servers(daemon->resolv_files->name, daemon);
		    check_servers(daemon);
		  }
#ifdef SUP_STATIC_PPTP
		if (1 == daemon->static_pptp_enable) {
		  load_static_pptp_server(daemon);
		}
#endif
		break;
		
	      case SIGUSR1:
		#if 1
		in_hijack = 1;
		my_syslog(LOG_INFO, _("In DNS Hijack mode!!!"));
		#else
		dump_cache(daemon, now);
		#endif
		break;
	
	      case SIGUSR2:
		in_hijack = 0;
		my_syslog(LOG_INFO, _("NOT DNS Hijack mode!!!"));
		break;

	      case SIGALRM:
		if (daemon->dhcp)
		  {
		    lease_prune(NULL, now);
		    lease_update_file(daemon, now);
		  }
#ifdef SUP_STATIC_PPTP
		if (1 == daemon->static_pptp_enable) {
		  timer_check_static_pptp_query(daemon);
		  alarm(1);
		}
#endif
		break;
		
	      case SIGTERM:
		{
		  int i;
		  /* Knock all our children on the head. */
		  for (i = 0; i < MAX_PROCS; i++)
		    if (daemon->tcp_pids[i] != 0)
		      kill(daemon->tcp_pids[i], SIGALRM);
		  
		  /* handle pending lease transitions */
		  if (daemon->helperfd != -1)
		    {
		      /* block in writes until all done */
		      if ((i = fcntl(daemon->helperfd, F_GETFL)) != -1)
			fcntl(daemon->helperfd, F_SETFL, i & ~O_NONBLOCK); 
		      do {
			helper_write(daemon);
		      } while (!helper_buf_empty() || do_script_run(daemon));
		      close(daemon->helperfd);
		    }
		     
		  if (daemon->lease_stream)
		    fclose(daemon->lease_stream);

		  my_syslog(LOG_INFO, _("exiting on receipt of SIGTERM"));
		  exit(0);
		}

	      case SIGCHLD:
		/* See Stevens 5.10 */
		/* Note that if a script process forks and then exits
		   without waiting for its child, we will reap that child.
		   It is not therefore safe to assume that any dieing children
		   whose pid != script_pid are TCP server threads. */ 
		while ((p = waitpid(-1, NULL, WNOHANG)) > 0)
		  {
		    int i;
		    for (i = 0 ; i < MAX_PROCS; i++)
		      if (daemon->tcp_pids[i] == p)
			{
			  daemon->tcp_pids[i] = 0;
			  break;
			}
		  }
		break;
	      }
	}
      
#if 0 //def HAVE_LINUX_NETWORK
      if (FD_ISSET(daemon->netlinkfd, &rset))
	netlink_multicast(daemon);
#endif
      
#ifdef HAVE_DBUS
      /* if we didn't create a DBus connection, retry now. */ 
     if ((daemon->options & OPT_DBUS) && !daemon->dbus)
	{
	  char *err;
	  if ((err = dbus_init(daemon)))
	    my_syslog(LOG_WARNING, _("DBus error: %s"), err);
	  if (daemon->dbus)
	    my_syslog(LOG_INFO, _("connected to system DBus"));
	}
      check_dbus_listeners(daemon, &rset, &wset, &eset);
#endif

      check_dns_listeners(daemon, &rset, now);

#ifdef HAVE_TFTP
      check_tftp_listeners(daemon, &rset, now);
#endif      

      if (daemon->dhcp && FD_ISSET(daemon->dhcpfd, &rset))
	dhcp_packet(daemon, now);

#if 0     
      if (daemon->helperfd != -1 && FD_ISSET(daemon->helperfd, &wset))
	helper_write(daemon);
#endif
    }
}

static void sig_handler(int sig)
{
  if (pid == 0)
    {
      /* ignore anything other than TERM during startup
	 and in helper proc. (helper ignore TERM too) */
      if (sig == SIGTERM)
	exit(0);
    }
  else if (pid == getpid())
    {
      /* master process */
      unsigned char sigchr = sig;
      int errsave = errno;
      write(pipewrite, &sigchr, 1);
      errno = errsave;
    }
  else
    {
      /* alarm is used to kill TCP children after a fixed time. */
      if (sig == SIGALRM)
	_exit(0);
    }
}


void clear_cache_and_reload(struct daemon *daemon, time_t now)
{
  cache_reload(daemon->options, daemon->namebuff, daemon->domain_suffix, daemon->addn_hosts);
  if (daemon->dhcp)
    {
      if (daemon->options & OPT_ETHERS)
	dhcp_read_ethers(daemon);
      dhcp_update_configs(daemon->dhcp_conf);
      lease_update_from_configs(daemon); 
      lease_update_file(daemon, now); 
      lease_update_dns(daemon);
    }
}

static int set_dns_listeners(struct daemon *daemon, time_t now, fd_set *set, int *maxfdp)
{
  struct serverfd *serverfdp;
  struct listener *listener;
  int wait, i;
  
#ifdef HAVE_TFTP
  int  tftp = 0;
  struct tftp_transfer *transfer;
  for (transfer = daemon->tftp_trans; transfer; transfer = transfer->next)
    {
      tftp++;
      FD_SET(transfer->sockfd, set);
      bump_maxfd(transfer->sockfd, maxfdp);
    }
#endif
  
  /* will we be able to get memory? */
  get_new_frec(daemon, now, &wait);
  
  for (serverfdp = daemon->sfds; serverfdp; serverfdp = serverfdp->next)
    {
      FD_SET(serverfdp->fd, set);
      bump_maxfd(serverfdp->fd, maxfdp);
    }
	  
  for (listener = daemon->listeners; listener; listener = listener->next)
    {
      /* only listen for queries if we have resources */
      if (wait == 0)
	{
	  FD_SET(listener->fd, set);
	  bump_maxfd(listener->fd, maxfdp);
	}

      /* death of a child goes through the select loop, so
	 we don't need to explicitly arrange to wake up here */
      for (i = 0; i < MAX_PROCS; i++)
	if (daemon->tcp_pids[i] == 0)
	  {
	    FD_SET(listener->tcpfd, set);
	    bump_maxfd(listener->tcpfd, maxfdp);
	    break;
	  }

#ifdef HAVE_TFTP
      if (tftp <= daemon->tftp_max && listener->tftpfd != -1)
	{
	  FD_SET(listener->tftpfd, set);
	  bump_maxfd(listener->tftpfd, maxfdp);
	}
#endif

    }
  
  return wait;
}

static void check_dns_listeners(struct daemon *daemon, fd_set *set, time_t now)
{
  struct serverfd *serverfdp;
  struct listener *listener;	  
  
  for (serverfdp = daemon->sfds; serverfdp; serverfdp = serverfdp->next)
    if (FD_ISSET(serverfdp->fd, set))
      reply_query(serverfdp, daemon, now);
  
  for (listener = daemon->listeners; listener; listener = listener->next)
    {
      if (FD_ISSET(listener->fd, set))
	receive_query(listener, daemon, now); 
 
#ifdef HAVE_TFTP     
      if (listener->tftpfd != -1 && FD_ISSET(listener->tftpfd, set))
	tftp_request(listener, daemon, now);
#endif

      if (FD_ISSET(listener->tcpfd, set))
	{
	  int confd;
	  struct irec *iface = NULL;
	  pid_t p;
	  
	  while((confd = accept(listener->tcpfd, NULL, NULL)) == -1 && errno == EINTR);
	  
	  if (confd == -1)
	    continue;
	  
	  if (daemon->options & OPT_NOWILD)
	    iface = listener->iface;
	  else
	    {
	      union mysockaddr tcp_addr;
	      socklen_t tcp_len = sizeof(union mysockaddr);
	      /* Check for allowed interfaces when binding the wildcard address:
		 we do this by looking for an interface with the same address as 
		 the local address of the TCP connection, then looking to see if that's
		 an allowed interface. As a side effect, we get the netmask of the
		 interface too, for localisation. */
	      
	      /* interface may be new since startup */
	      if (enumerate_interfaces(daemon) &&
		  getsockname(confd, (struct sockaddr *)&tcp_addr, &tcp_len) != -1)
		for (iface = daemon->interfaces; iface; iface = iface->next)
		  if (sockaddr_isequal(&iface->addr, &tcp_addr))
		    break;
	    }
	  
	  if (!iface)
	    {
	      shutdown(confd, SHUT_RDWR);
	      close(confd);
	    }
#ifndef NO_FORK
	  else if (!(daemon->options & OPT_DEBUG) && (p = fork()) != 0)
	    {
	      if (p != -1)
		{
		  int i;
		  for (i = 0; i < MAX_PROCS; i++)
		    if (daemon->tcp_pids[i] == 0)
		      {
			daemon->tcp_pids[i] = p;
			break;
		      }
		}
	      close(confd);
	    }
#endif
	  else
	    {
	      unsigned char *buff;
	      struct server *s; 
	      int flags;
	      struct in_addr dst_addr_4;
	      
	      dst_addr_4.s_addr = 0;
	      
	       /* Arrange for SIGALARM after CHILD_LIFETIME seconds to
		  terminate the process. */
	      if (!(daemon->options & OPT_DEBUG))
		alarm(CHILD_LIFETIME);
	      
	      /* start with no upstream connections. */
	      for (s = daemon->servers; s; s = s->next)
		 s->tcpfd = -1; 
	      
	      /* The connected socket inherits non-blocking
		 attribute from the listening socket. 
		 Reset that here. */
	      if ((flags = fcntl(confd, F_GETFL, 0)) != -1)
		fcntl(confd, F_SETFL, flags & ~O_NONBLOCK);
	      
	      if (listener->family == AF_INET)
		dst_addr_4 = iface->addr.in.sin_addr;
	      
	      buff = tcp_request(daemon, confd, now, dst_addr_4, iface->netmask);
	       
	      shutdown(confd, SHUT_RDWR);
	      close(confd);
	      
	      if (buff)
		free(buff);
	      
	      for (s = daemon->servers; s; s = s->next)
		if (s->tcpfd != -1)
		  {
		    shutdown(s->tcpfd, SHUT_RDWR);
		    close(s->tcpfd);
		  }
#ifndef NO_FORK		   
	      if (!(daemon->options & OPT_DEBUG))
		_exit(0);
#endif
	    }
	}
    }
#ifdef DNI_IPV6_FEATURE
  check_timeout_forward(daemon, now);
#endif
}


int make_icmp_sock(void)
{
  int fd;
  int zeroopt = 0;

  if ((fd = socket (AF_INET, SOCK_RAW, IPPROTO_ICMP)) != -1)
    {
      if (!fix_fd(fd) ||
	  setsockopt(fd, SOL_SOCKET, SO_DONTROUTE, &zeroopt, sizeof(zeroopt)) == -1)
	{
	  close(fd);
	  fd = -1;
	}
    }

  return fd;
}

int icmp_ping(struct daemon *daemon, struct in_addr addr)
{
  /* Try and get an ICMP echo from a machine. */

  /* Note that whilst in the three second wait, we check for 
     (and service) events on the DNS and TFTP  sockets, (so doing that
     better not use any resources our caller has in use...)
     but we remain deaf to signals or further DHCP packets. */

  int fd;
  struct sockaddr_in saddr;
  struct { 
    struct ip ip;
    struct icmp icmp;
  } packet;
  unsigned short id = rand16();
  unsigned int i, j;
  int gotreply = 0;
  time_t start, now;

#ifdef HAVE_LINUX_NETWORK
  if ((fd = make_icmp_sock()) == -1)
    return 0;
#else
  int opt = 2000;
  fd = daemon->dhcp_icmp_fd;
  setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt));
#endif

  saddr.sin_family = AF_INET;
  saddr.sin_port = 0;
  saddr.sin_addr = addr;
#ifdef HAVE_SOCKADDR_SA_LEN
  saddr.sin_len = sizeof(struct sockaddr_in);
#endif
  
  memset(&packet.icmp, 0, sizeof(packet.icmp));
  packet.icmp.icmp_type = ICMP_ECHO;
  packet.icmp.icmp_id = id;
  for (j = 0, i = 0; i < sizeof(struct icmp) / 2; i++)
    j += ((u16 *)&packet.icmp)[i];
  while (j>>16)
    j = (j & 0xffff) + (j >> 16);  
  packet.icmp.icmp_cksum = (j == 0xffff) ? j : ~j;
  
  while (sendto(fd, (char *)&packet.icmp, sizeof(struct icmp), 0, 
		(struct sockaddr *)&saddr, sizeof(saddr)) == -1 &&
	 retry_send());
  
  for (now = start = dnsmasq_time(); 
       difftime(now, start) < (float)PING_WAIT;)
    {
      struct timeval tv;
      fd_set rset, wset;
      struct sockaddr_in faddr;
      int maxfd = fd; 
      socklen_t len = sizeof(faddr);
      
      tv.tv_usec = 250000;
      tv.tv_sec = 0; 
      
      FD_ZERO(&rset);
      FD_ZERO(&wset);
      FD_SET(fd, &rset);
      set_dns_listeners(daemon, now, &rset, &maxfd);
      set_log_writer(&wset, &maxfd);

      if (select(maxfd+1, &rset, &wset, NULL, &tv) < 0)
	{
	  FD_ZERO(&rset);
	  FD_ZERO(&wset);
	}

      now = dnsmasq_time();

      check_log_writer(&wset);
      check_dns_listeners(daemon, &rset, now);

#ifdef HAVE_TFTP
      check_tftp_listeners(daemon, &rset, now);
#endif

      if (FD_ISSET(fd, &rset) &&
	  recvfrom(fd, &packet, sizeof(packet), 0,
		   (struct sockaddr *)&faddr, &len) == sizeof(packet) &&
	  saddr.sin_addr.s_addr == faddr.sin_addr.s_addr &&
	  packet.icmp.icmp_type == ICMP_ECHOREPLY &&
	  packet.icmp.icmp_seq == 0 &&
	  packet.icmp.icmp_id == id)
	{
	  gotreply = 1;
	  break;
	}
    }
  
#ifdef HAVE_LINUX_NETWORK
  close(fd);
#else
  opt = 1;
  setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt));
#endif

  return gotreply;
}

 
