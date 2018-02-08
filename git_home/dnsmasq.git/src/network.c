/* dnsmasq is Copyright (c) 2000 - 2006 Simon Kelley

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 dated June, 1991.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#include "dnsmasq.h"

int iface_check(struct daemon *daemon, int family, struct all_addr *addr, 
		struct ifreq *ifr, int *indexp)
{
  struct iname *tmp;
  int ret = 1;

  /* Note: have to check all and not bail out early, so that we set the
     "used" flags. */

  if (indexp)
    {
#if defined(__FreeBSD__) || defined(__DragonFly__)
      /* One form of bridging on FreeBSD has the property that packets
	 can be recieved on bridge interfaces which do not have an IP address.
	 We allow these to be treated as aliases of another interface which does have
	 an IP address with --dhcp-bridge=interface,alias,alias */
      struct dhcp_bridge *bridge, *alias;
      for (bridge = daemon->bridges; bridge; bridge = bridge->next)
	{
	  for (alias = bridge->alias; alias; alias = alias->next)
	    if (strncmp(ifr->ifr_name, alias->iface, IF_NAMESIZE) == 0)
	      {
		int newindex;
		
		if (!(newindex = if_nametoindex(bridge->iface)))
		  {
		    my_syslog(LOG_WARNING, _("unknown interface %s in bridge-interface"), ifr->ifr_name);
		    return 0;
		  }
		else 
		  {
		    *indexp = newindex;
		    strncpy(ifr->ifr_name,  bridge->iface, IF_NAMESIZE);
		    break;
		  }
	      }
	  if (alias)
	    break;
	}
#endif
    }
  
  if (daemon->if_names || (addr && daemon->if_addrs))
    {
      ret = 0;

      for (tmp = daemon->if_names; tmp; tmp = tmp->next)
	if (tmp->name && (strcmp(tmp->name, ifr->ifr_name) == 0))
	  ret = tmp->used = 1;
	        
      for (tmp = daemon->if_addrs; tmp; tmp = tmp->next)
	if (addr && tmp->addr.sa.sa_family == family)
	  {
	    if (family == AF_INET &&
		tmp->addr.in.sin_addr.s_addr == addr->addr.addr4.s_addr)
	      ret = tmp->used = 1;
#ifdef HAVE_IPV6
	    else if (family == AF_INET6 &&
		     IN6_ARE_ADDR_EQUAL(&tmp->addr.in6.sin6_addr, 
					&addr->addr.addr6))
	      ret = tmp->used = 1;
#endif
	  }          
    }
  
  for (tmp = daemon->if_except; tmp; tmp = tmp->next)
    if (tmp->name && (strcmp(tmp->name, ifr->ifr_name) == 0))
      ret = 0;
  
  return ret; 
}
      
static int iface_allowed(struct daemon *daemon, struct irec **irecp, int if_index, 
			 union mysockaddr *addr, struct in_addr netmask) 
{
  struct irec *iface;
  int fd;
  struct ifreq ifr;
  int dhcp_ok = 1;
  struct iname *tmp;
  
  /* check whether the interface IP has been added already 
     we call this routine multiple times. */
  for (iface = *irecp; iface; iface = iface->next) 
    if (sockaddr_isequal(&iface->addr, addr))
      return 1;
  
#ifdef HAVE_LINUX_NETWORK
  ifr.ifr_ifindex = if_index;
#endif
  
  if ((fd = socket(PF_INET, SOCK_DGRAM, 0)) == -1 ||
#ifdef HAVE_LINUX_NETWORK
      ioctl(fd, SIOCGIFNAME, &ifr) == -1 ||
#else
      !if_indextoname(if_index, ifr.ifr_name) ||
#endif
      ioctl(fd, SIOCGIFFLAGS, &ifr) == -1)
    {
      if (fd != -1)
	{
	  int errsave = errno;
	  close(fd);
	  errno = errsave;
	}
      return 0;
    }
  
  close(fd);
  
  /* If we are restricting the set of interfaces to use, make
     sure that loopback interfaces are in that set. */
  if (daemon->if_names && (ifr.ifr_flags & IFF_LOOPBACK))
    {
      struct iname *lo;
      for (lo = daemon->if_names; lo; lo = lo->next)
	if (lo->name && strcmp(lo->name, ifr.ifr_name) == 0)
	  {
	    lo->isloop = 1;
	    break;
	  }
      
      if (!lo && (lo = malloc(sizeof(struct iname))))
	{
	  lo->name = safe_malloc(strlen(ifr.ifr_name)+1);
	  strcpy(lo->name, ifr.ifr_name);
	  lo->isloop = lo->used = 1;
	  lo->next = daemon->if_names;
	  daemon->if_names = lo;
	}
    }
  
  if (addr->sa.sa_family == AF_INET &&
      !iface_check(daemon, AF_INET, (struct all_addr *)&addr->in.sin_addr, &ifr, NULL))
    return 1;
  
  for (tmp = daemon->dhcp_except; tmp; tmp = tmp->next)
    if (tmp->name && (strcmp(tmp->name, ifr.ifr_name) == 0))
      dhcp_ok = 0;
  
#ifdef HAVE_IPV6
  if (addr->sa.sa_family == AF_INET6 &&
      !iface_check(daemon, AF_INET6, (struct all_addr *)&addr->in6.sin6_addr, &ifr, NULL))
    return 1;
#endif

  /* add to list */
  if ((iface = malloc(sizeof(struct irec))))
    {
      iface->addr = *addr;
      iface->netmask = netmask;
      iface->dhcp_ok = dhcp_ok;
      iface->next = *irecp;
      *irecp = iface;
      return 1;
    }
  
  errno = ENOMEM; 
  return 0;
}

#ifdef HAVE_IPV6
static int iface_allowed_v6(struct daemon *daemon, struct in6_addr *local, 
			    int scope, int if_index, void *vparam)
{
  union mysockaddr addr;
  struct in_addr netmask; /* dummy */
  
  netmask.s_addr = 0;
  
  memset(&addr, 0, sizeof(addr));
#ifdef HAVE_SOCKADDR_SA_LEN
  addr.in6.sin6_len = sizeof(addr.in6);
#endif
  addr.in6.sin6_family = AF_INET6;
  addr.in6.sin6_addr = *local;
  addr.in6.sin6_port = htons(daemon->port);
  addr.in6.sin6_scope_id = scope;
  
  return iface_allowed(daemon, (struct irec **)vparam, if_index, &addr, netmask);
}
#endif

static int iface_allowed_v4(struct daemon *daemon, struct in_addr local, int if_index, 
			    struct in_addr netmask, struct in_addr broadcast, void *vparam)
{
  union mysockaddr addr;

  memset(&addr, 0, sizeof(addr));
#ifdef HAVE_SOCKADDR_SA_LEN
  addr.in.sin_len = sizeof(addr.in);
#endif
  addr.in.sin_family = AF_INET;
  addr.in.sin_addr = broadcast; /* warning */
  addr.in.sin_addr = local;
  addr.in.sin_port = htons(daemon->port);

  return iface_allowed(daemon, (struct irec **)vparam, if_index, &addr, netmask);
}
   

int enumerate_interfaces(struct daemon *daemon)
{
#ifdef HAVE_IPV6
  return iface_enumerate(daemon, &daemon->interfaces, iface_allowed_v4, iface_allowed_v6);
#else
  return iface_enumerate(daemon, &daemon->interfaces, iface_allowed_v4, NULL);
#endif
}

/* set NONBLOCK and CLOEXEC bits on fd: See Stevens 16.6 */
int fix_fd(int fd)
{
  int flags;

  if ((flags = fcntl(fd, F_GETFL)) == -1 ||
      fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1 ||
      (flags = fcntl(fd, F_GETFD)) == -1 ||
      fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
    return 0;

  return 1;
}

#if defined(HAVE_IPV6)
static int create_ipv6_listener(struct listener **link, int port)
{
  union mysockaddr addr;
  int tcpfd, fd;
  struct listener *l;
  int opt = 1;

  memset(&addr, 0, sizeof(addr));
  addr.in6.sin6_family = AF_INET6;
  addr.in6.sin6_addr = in6addr_any;
  addr.in6.sin6_port = htons(port);
#ifdef HAVE_SOCKADDR_SA_LEN
  addr.in6.sin6_len = sizeof(addr.in6);
#endif

  /* No error of the kernel doesn't support IPv6 */
  if ((fd = socket(AF_INET6, SOCK_DGRAM, 0)) == -1)
    return (errno == EPROTONOSUPPORT ||
	    errno == EAFNOSUPPORT ||
	    errno == EINVAL);
  
  if ((tcpfd = socket(AF_INET6, SOCK_STREAM, 0)) == -1)
    return 0;
      
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1 ||
      setsockopt(tcpfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1 ||
      setsockopt(fd, IPV6_LEVEL, IPV6_V6ONLY, &opt, sizeof(opt)) == -1 ||
      setsockopt(tcpfd, IPV6_LEVEL, IPV6_V6ONLY, &opt, sizeof(opt)) == -1 ||
      !fix_fd(fd) ||
      !fix_fd(tcpfd) ||
#ifdef IPV6_RECVPKTINFO
      setsockopt(fd, IPV6_LEVEL, IPV6_RECVPKTINFO, &opt, sizeof(opt)) == -1 ||
#else
      setsockopt(fd, IPV6_LEVEL, IPV6_PKTINFO, &opt, sizeof(opt)) == -1 ||
#endif
      bind(tcpfd, (struct sockaddr *)&addr, sa_len(&addr)) == -1 ||
      listen(tcpfd, 5) == -1 ||
      bind(fd, (struct sockaddr *)&addr, sa_len(&addr)) == -1) 
    return 0;
      
  l = safe_malloc(sizeof(struct listener));
  l->fd = fd;
  l->tcpfd = tcpfd;
  l->tftpfd = -1;
  l->family = AF_INET6;
  l->next = NULL;
  *link = l;
  
  return 1;
}
#endif

#if defined(HAVE_IPV6)
static int create_raw_ipv6_listener(struct listener **link, int port)
{
  if(config_match("ap_mode", "0"))
	  return 0;

  struct sockaddr_ll sock;
  struct listener *l;
  int tcpfd, fd, opt = 1;
  static const struct sock_filter filter_instr[] = {
		/* jump to ipv6 byte (protocol) */
		BPF_STMT(BPF_LD|BPF_B|BPF_ABS, 20),
		/* jump to L2 if it is IPPROTO_UDP, else to L4 */
		BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, IPPROTO_UDP, 0, 3),
		/* L2: jump to UDP dport */
		BPF_STMT(BPF_LD|BPF_B|BPF_ABS, 57),
		/* jump to L3 if it si 53 port, else to L4 */
		BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, 53, 0, 1),
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

  if ((fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IPV6))) == -1) {
	fprintf(stderr, "socket call failed!\n");
    return (errno == EPROTONOSUPPORT ||
	    errno == EAFNOSUPPORT ||
	    errno == EINVAL);
  }

  sock.sll_family = AF_PACKET;
  sock.sll_protocol = htons(ETH_P_IPV6);
  sock.sll_ifindex = (int)if_nametoindex("br0");

  if(bind(fd, (struct sockaddr *) &sock, sizeof(sock)) < 0){
    fprintf(stderr," bind calll failed\n");   
	return -1;
  }

  if(port == 53) {
    if (setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, &filter_prog,sizeof(filter_prog)) == -1)
	{
      fprintf(stderr," Attached filter to raw socket fd\n");   
      return -1;
    }
  }


  l = safe_malloc(sizeof(struct listener));
  l->fd = fd;
  l->tcpfd = -1;
  l->tftpfd = -1;
  l->family = AF_PACKET;
  l->next = NULL;
  *link = l;
  
  return 1;
}
#endif

struct listener *create_wildcard_listeners(int port, int have_tftp)
{
  union mysockaddr addr;
  int opt = 1;
  struct listener *l, *l6 = NULL, *ll6 = NULL;
  int tcpfd, fd, tftpfd = -1;

  memset(&addr, 0, sizeof(addr));
  addr.in.sin_family = AF_INET;
  addr.in.sin_addr.s_addr = INADDR_ANY;
  addr.in.sin_port = htons(port);
#ifdef HAVE_SOCKADDR_SA_LEN
  addr.in.sin_len = sizeof(struct sockaddr_in);
#endif

  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 ||
      (tcpfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    return NULL;
  
  if (setsockopt(tcpfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1 ||
      bind(tcpfd, (struct sockaddr *)&addr, sa_len(&addr)) == -1 ||
      listen(tcpfd, 5) == -1 ||
      !fix_fd(tcpfd) ||
#ifdef HAVE_IPV6
	 !create_ipv6_listener(&l6, port) ||
	 create_raw_ipv6_listener(&ll6, port) == -1 ||
#endif
      setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1 ||
      !fix_fd(fd) ||
#if defined(HAVE_LINUX_NETWORK) 
      setsockopt(fd, SOL_IP, IP_PKTINFO, &opt, sizeof(opt)) == -1 ||
#elif defined(IP_RECVDSTADDR) && defined(IP_RECVIF)
      setsockopt(fd, IPPROTO_IP, IP_RECVDSTADDR, &opt, sizeof(opt)) == -1 ||
      setsockopt(fd, IPPROTO_IP, IP_RECVIF, &opt, sizeof(opt)) == -1 ||
#endif 
      bind(fd, (struct sockaddr *)&addr, sa_len(&addr)) == -1)
    return NULL;

#ifdef HAVE_TFTP
  if (have_tftp)
    {
      addr.in.sin_port = htons(TFTP_PORT);
      if ((tftpfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	return NULL;
      
      if (setsockopt(tftpfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1 ||
	  !fix_fd(tftpfd) ||
#if defined(HAVE_LINUX_NETWORK) 
	  setsockopt(tftpfd, SOL_IP, IP_PKTINFO, &opt, sizeof(opt)) == -1 ||
#elif defined(IP_RECVDSTADDR) && defined(IP_RECVIF)
	  setsockopt(tftpfd, IPPROTO_IP, IP_RECVDSTADDR, &opt, sizeof(opt)) == -1 ||
	  setsockopt(tftpfd, IPPROTO_IP, IP_RECVIF, &opt, sizeof(opt)) == -1 ||
#endif 
	  bind(tftpfd, (struct sockaddr *)&addr, sa_len(&addr)) == -1)
	return NULL;
    }
#else
  (void) have_tftp;
#endif
  
  l = safe_malloc(sizeof(struct listener));
  l->family = AF_INET;
  l->fd = fd;
  l->tcpfd = tcpfd;
  l->tftpfd = tftpfd;
  l->next = l6;
  if(config_match("ap_mode", "1"))
	 l6->next = ll6;

  return l;
}

struct listener *create_bound_listeners(struct daemon *daemon)
{
  struct listener *listeners = NULL;
  struct irec *iface;
  int opt = 1;
  
  for (iface = daemon->interfaces; iface; iface = iface->next)
    {
      struct listener *new = safe_malloc(sizeof(struct listener));
      new->family = iface->addr.sa.sa_family;
      new->iface = iface;
      new->next = listeners;
      new->tftpfd = -1;

      if ((new->tcpfd = socket(iface->addr.sa.sa_family, SOCK_STREAM, 0)) == -1 ||
	  (new->fd = socket(iface->addr.sa.sa_family, SOCK_DGRAM, 0)) == -1 ||
	  setsockopt(new->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1 ||
	  setsockopt(new->tcpfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1 ||
	  !fix_fd(new->tcpfd) ||
	  !fix_fd(new->fd))
	die(_("failed to create listening socket: %s"), NULL);
      
#ifdef HAVE_IPV6
      if (iface->addr.sa.sa_family == AF_INET6)
	{
	  if (setsockopt(new->fd, IPV6_LEVEL, IPV6_V6ONLY, &opt, sizeof(opt)) == -1 ||
	      setsockopt(new->tcpfd, IPV6_LEVEL, IPV6_V6ONLY, &opt, sizeof(opt)) == -1)
	    die(_("failed to set IPV6 options on listening socket: %s"), NULL);
	}
#endif
      
      if (bind(new->tcpfd, &iface->addr.sa, sa_len(&iface->addr)) == -1 ||
	  bind(new->fd, &iface->addr.sa, sa_len(&iface->addr)) == -1)
	{
#ifdef HAVE_IPV6
	  if (iface->addr.sa.sa_family == AF_INET6 && errno == ENODEV)
	    {
	      close(new->tcpfd);
	      close(new->fd);
	      free(new);
	    }
	  else
#endif
	    {
	      prettyprint_addr(&iface->addr, daemon->namebuff);
	      die(_("failed to bind listening socket for %s: %s"), 
		  daemon->namebuff);
	    }
	}
      else
	 {
	   listeners = new;     
	   if (listen(new->tcpfd, 5) == -1)
	     die(_("failed to listen on socket: %s"), NULL);
	 }

      if ((daemon->options & OPT_TFTP) && iface->addr.sa.sa_family == AF_INET && iface->dhcp_ok)
	{
	  short save = iface->addr.in.sin_port;
	  iface->addr.in.sin_port = htons(TFTP_PORT);
	  if ((new->tftpfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1 ||
	      setsockopt(new->tftpfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1 ||
	      !fix_fd(new->tftpfd) ||
	      bind(new->tftpfd, &iface->addr.sa, sa_len(&iface->addr)) == -1)
	    die(_("failed to create TFTP socket: %s"), NULL);
	  iface->addr.in.sin_port = save;
	}
    }

  return listeners;
}

struct serverfd *allocate_sfd(union mysockaddr *addr, struct serverfd **sfds)
{
  struct serverfd *sfd;
#ifdef BIND_SRVSOCK_TO_WAN
  struct ifreq ifr;
#endif

  /* may have a suitable one already */
  for (sfd = *sfds; sfd; sfd = sfd->next )
    if (sockaddr_isequal(&sfd->source_addr, addr))
      return sfd;
  
  /* need to make a new one. */
  errno = ENOMEM; /* in case malloc fails. */
  if (!(sfd = malloc(sizeof(struct serverfd))))
    return NULL;
  
  if ((sfd->fd = socket(addr->sa.sa_family, SOCK_DGRAM, 0)) == -1)
    {
      free(sfd);
      return NULL;
    }
  
#ifdef BIND_SRVSOCK_TO_WAN
  memset(&ifr, 0x00, sizeof(ifr));
  strcpy(ifr.ifr_name, wan_ifname);
  if(setsockopt(sfd->fd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&ifr, sizeof(ifr)) == 0)
    bind_wan_success = 1;
#endif
  if (bind(sfd->fd, (struct sockaddr *)addr, sa_len(addr)) == -1 ||
      !fix_fd(sfd->fd))
    {
      int errsave = errno; /* save error from bind. */
      close(sfd->fd);
      free(sfd);
      errno = errsave;
      return NULL;
    }
  
  sfd->source_addr = *addr;
  sfd->next = *sfds;
  *sfds = sfd;
  
  return sfd;
}

void check_servers(struct daemon *daemon)
{
  struct irec *iface;
  struct server *new, *tmp, *ret = NULL;
  int port = 0;
#ifdef DNI_IPV6_FEATURE
  struct server *sresolv = NULL;
  daemon->diff_svr = 0;
#endif

  for (new = daemon->servers; new; new = tmp)
    {
      tmp = new->next;
      
      if (!(new->flags & (SERV_LITERAL_ADDRESS | SERV_NO_ADDR)))
	{
	  port = prettyprint_addr(&new->addr, daemon->namebuff);

	  /* 0.0.0.0 is nothing, the stack treats it like 127.0.0.1 */
	  if (new->addr.sa.sa_family == AF_INET &&
	      new->addr.in.sin_addr.s_addr == 0)
	    {
	      free(new);
	      continue;
	    }

	  for (iface = daemon->interfaces; iface; iface = iface->next)
	    if (sockaddr_isequal(&new->addr, &iface->addr))
	      break;
	  if (iface)
	    {
	      my_syslog(LOG_WARNING, _("ignoring nameserver %s - local interface"), daemon->namebuff);
	      free(new);
	      continue;
	    }
	  
	  /* Do we need a socket set? */
	  if (!new->sfd && !(new->sfd = allocate_sfd(&new->source_addr, &daemon->sfds)))
	    {
	      my_syslog(LOG_WARNING, 
			_("ignoring nameserver %s - cannot make/bind socket: %s"),
			daemon->namebuff, strerror(errno));
	      free(new);
	      continue;
	    }
	}
      
      /* reverse order - gets it right. */
      new->next = ret;
      ret = new;
#ifdef DNI_IPV6_FEATURE
      if (new->flags & SERV_FROM_RESOLV)
        {
	  if (NULL == sresolv)
	    sresolv = new;
	  if (new->addr.sa.sa_family != sresolv->addr.sa.sa_family)
            {
	      daemon->diff_svr = 1;
            }
	}
#endif
      
      if (new->flags & (SERV_HAS_DOMAIN | SERV_FOR_NODOTS))
	{
	  char *s1, *s2;
	  if (new->flags & SERV_HAS_DOMAIN)
	    s1 = _("domain"), s2 = new->domain;
	  else
	    s1 = _("unqualified"), s2 = _("domains");
	  
	  if (new->flags & SERV_NO_ADDR)
	    my_syslog(LOG_INFO, _("using local addresses only for %s %s"), s1, s2);
	  else if (!(new->flags & SERV_LITERAL_ADDRESS))
	    my_syslog(LOG_INFO, _("using nameserver %s#%d for %s %s"), daemon->namebuff, port, s1, s2);
	}
      else
	my_syslog(LOG_INFO, _("using nameserver %s#%d"), daemon->namebuff, port); 
    }
  
  daemon->servers = ret;
}

/* Return zero if no servers found, in that case we keep polling.
   This is a protection against an update-time/write race on resolv.conf */
int reload_servers(char *fname, struct daemon *daemon)
{
  FILE *f;
  char *line;
  struct server *old_servers = NULL;
  struct server *new_servers = NULL;
  struct server *serv;
  int gotone = 0;

  /* buff happens to be MAXDNAME long... */
  if (!(f = fopen(fname, "r")))
    {
      my_syslog(LOG_ERR, _("failed to read %s: %s"), fname, strerror(errno));
      return 0;
    }
  
  /* move old servers to free list - we can reuse the memory 
     and not risk malloc if there are the same or fewer new servers. 
     Servers which were specced on the command line go to the new list. */
  for (serv = daemon->servers; serv;)
    {
      struct server *tmp = serv->next;
      if (serv->flags & SERV_FROM_RESOLV)
	{
	  serv->next = old_servers;
	  old_servers = serv; 
	  /* forward table rules reference servers, so have to blow them away */
	  server_gone(daemon, serv);
	}
      else
	{
	  serv->next = new_servers;
	  new_servers = serv;
	}
      serv = tmp;
    }
  
  while ((line = fgets(daemon->namebuff, MAXDNAME, f)))
    {
      union mysockaddr addr, source_addr;
      char *token = strtok(line, " \t\n\r");
      
      if (!token || strcmp(token, "nameserver") != 0)
	continue;
      if (!(token = strtok(NULL, " \t\n\r")))
	continue;
      
      memset(&addr, 0, sizeof(addr));
      memset(&source_addr, 0, sizeof(source_addr));
      
      if ((addr.in.sin_addr.s_addr = inet_addr(token)) != (in_addr_t) -1)
	{
#ifdef HAVE_SOCKADDR_SA_LEN
	  source_addr.in.sin_len = addr.in.sin_len = sizeof(source_addr.in);
#endif
	  source_addr.in.sin_family = addr.in.sin_family = AF_INET;
	  addr.in.sin_port = htons(NAMESERVER_PORT);
	  source_addr.in.sin_addr.s_addr = INADDR_ANY;
	  source_addr.in.sin_port = htons(daemon->query_port);
	}
#ifdef HAVE_IPV6
      else if (inet_pton(AF_INET6, token, &addr.in6.sin6_addr) > 0)
	{
#ifdef HAVE_SOCKADDR_SA_LEN
	  source_addr.in6.sin6_len = addr.in6.sin6_len = sizeof(source_addr.in6);
#endif
	  source_addr.in6.sin6_family = addr.in6.sin6_family = AF_INET6;
	  addr.in6.sin6_port = htons(NAMESERVER_PORT);
	  source_addr.in6.sin6_addr = in6addr_any;
	  source_addr.in6.sin6_port = htons(daemon->query_port);
	}
#endif /* IPV6 */
      else
	continue;
      
      if (old_servers)
	{
	  serv = old_servers;
	  old_servers = old_servers->next;
	}
      else if (!(serv = malloc(sizeof (struct server))))
	continue;
      
      /* this list is reverse ordered: 
	 it gets reversed again in check_servers */
      serv->next = new_servers;
      new_servers = serv;
      serv->addr = addr;
      serv->source_addr = source_addr;
      serv->domain = NULL;
      serv->sfd = NULL;
      serv->flags = SERV_FROM_RESOLV;

      gotone = 1;
    }
  
  /* Free any memory not used. */
  while (old_servers)
    {
      struct server *tmp = old_servers->next;
      free(old_servers);
      old_servers = tmp;
    }

  daemon->servers = new_servers;
  fclose(f);

/*
 * If WAN/LAN IP conflict happens, and unfortunately, the new DNS nameserver is
 * the LAN's orignal IP address, if DNS-Masq doesn't update the interface lists,
 * this nameserver will be ignored:
 * ...
 * dnsmasq: ignoring nameserver 192.168.1.1 - local interface
 * ...
 * For resolving this problem, re-enumerates interface lists before checking the
 * nameservers in 'check_servers' function.
*/
#if 1
  if (gotone == 0)
      return 0;
  
  struct irec *iface;
  for (iface = daemon->interfaces; iface;) {
      struct irec *nxt = iface->next;

      /* my_syslog(LOG_WARNING, _("%s - local interface is gone!"), inet_ntoa(iface->addr.in.sin_addr)); */
      free(iface);
      iface = nxt;
  }

  daemon->interfaces = NULL;
  if (!enumerate_interfaces(daemon))
      my_syslog(LOG_WARNING, _("failed to find list of interfaces!"));

  return 1;
#endif

  return gotone;
}


/* Use an IPv4 listener socket for ioctling */
struct in_addr get_ifaddr(struct daemon* daemon, char *intr)
{
  struct listener *l;
  struct ifreq ifr;

  for (l = daemon->listeners; l && l->family != AF_INET; l = l->next);
  
  strncpy(ifr.ifr_name, intr, IF_NAMESIZE);
  ifr.ifr_addr.sa_family = AF_INET;
  
  if (!l || ioctl(l->fd, SIOCGIFADDR, &ifr) == -1)
    ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr = -1;
  
  return ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr;
}



