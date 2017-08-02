/* dnsmasq is Copyright (c) 2000-2007 Simon Kelley

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 dated June, 1991.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#include "dnsmasq.h"

/* Implement logging to /dev/log asynchronously. If syslogd is 
   making DNS lookups through dnsmasq, and dnsmasq blocks awaiting
   syslogd, then the two daemons can deadlock. We get around this
   by not blocking when talking to syslog, instead we queue up to 
   MAX_LOGS messages. If more are queued, they will be dropped,
   and the drop event itself logged. */

/* The "wire" protocol for logging is defined in RFC 3164 */

/* From RFC 3164 */
#define MAX_MESSAGE 1024

/* defaults in case we die() before we log_start() */
static int log_fac = LOG_DAEMON;
static int log_stderr = 0; 
static int log_fd = -1;
static int log_to_file = 0;
static int entries_alloced = 0;
static int entries_lost = 0;
static int connection_good = 1;
static int max_logs = 0;

struct log_entry {
  int offset, length;
  struct log_entry *next;
  char payload[MAX_MESSAGE];
};

static struct log_entry *entries = NULL;
static struct log_entry *free_entries = NULL;


int log_start(struct daemon *daemon)
{
  int flags;

  log_stderr = !!(daemon->options & OPT_DEBUG);

  if (daemon->log_fac != -1)
    log_fac = daemon->log_fac;
#ifdef LOG_LOCAL0
  else if (daemon->options & OPT_DEBUG)
    log_fac = LOG_LOCAL0;
#endif

  if (daemon->log_file)
    {
      log_fd = open(daemon->log_file, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR|S_IRGRP); 
      log_to_file = 1;
      daemon->max_logs = 0;
    }
  else
    log_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
  
  if (log_fd == -1)
    die(_("cannot open %s: %s"), daemon->log_file ? daemon->log_file : "log");
  
  /* if queuing is inhibited, make sure we allocate
     the one required buffer now. */
  if ((max_logs = daemon->max_logs) == 0)
    {  
      free_entries = safe_malloc(sizeof(struct log_entry));
      free_entries->next = NULL;
      entries_alloced = 1;
    }

  if ((flags = fcntl(log_fd, F_GETFD)) != -1)
    fcntl(log_fd, F_SETFD, flags | FD_CLOEXEC);

  /* if max_log is zero, leave the socket blocking */
  if (max_logs != 0 && (flags = fcntl(log_fd, F_GETFL)) != -1)
    fcntl(log_fd, F_SETFL, flags | O_NONBLOCK);
  
  return log_fd;
}
  
static void log_write(void)
{
  ssize_t rc;
  int tried_stream = 0;
  
  while (entries)
    {
      connection_good = 1;

      if ((rc = write(log_fd, entries->payload + entries->offset, entries->length)) != -1)
	{
	  entries->length -= rc;
	  entries->offset += rc;
	  if (entries->length == 0)
	    {
	      struct log_entry *tmp = entries;
	      entries = tmp->next;
	      tmp->next = free_entries;
	      free_entries = tmp;
	      
	      if (entries_lost != 0)
		{
		  int e = entries_lost;
		  entries_lost = 0; /* avoid wild recursion */
		  my_syslog(LOG_WARNING, _("overflow: %d log entries lost"), e);
		}	  
	    }
	  continue;
	}
      
      if (errno == EINTR)
	continue;

      if (errno == EAGAIN)
	return; /* syslogd busy, go again when select() or poll() says so */
      
      if (errno == ENOBUFS)
	{
	  connection_good = 0;
	  return;
	}
      
      /* Once a stream socket hits EPIPE, we have to close and re-open */
      if (errno == EPIPE)
	goto reopen_stream;
      
      if (!log_to_file &&
	  (errno == ECONNREFUSED || 
	   errno == ENOTCONN || 
	   errno == EDESTADDRREQ || 
	   errno == ECONNRESET))
	{
	  /* socket went (syslogd down?), try and reconnect. If we fail,
	     stop trying until the next call to my_syslog() 
	     ECONNREFUSED -> connection went down
	     ENOTCONN -> nobody listening
	     (ECONNRESET, EDESTADDRREQ are *BSD equivalents)
	     EPIPE comes from broken stream socket (we ignore SIGPIPE) */
	  
	  struct sockaddr_un logaddr;
	  
#ifdef HAVE_SOCKADDR_SA_LEN
	  logaddr.sun_len = sizeof(logaddr) - sizeof(logaddr.sun_path) + strlen(_PATH_LOG) + 1; 
#endif
	  logaddr.sun_family = AF_LOCAL;
	  strncpy(logaddr.sun_path, _PATH_LOG, sizeof(logaddr.sun_path));

	  /* Got connection back? try again. */
	  if (connect(log_fd, (struct sockaddr *)&logaddr, sizeof(logaddr)) != -1)
	    continue;
	  
	  /* errors from connect which mean we should keep trying */
	  if (errno == ENOENT || 
	      errno == EALREADY || 
	      errno == ECONNREFUSED ||
	      errno == EISCONN || 
	      errno == EINTR ||
	      errno == EAGAIN)
	    {
	      /* try again on next syslog() call */
	      connection_good = 0;
	      return;
	    }

	  /* we start with a SOCK_DGRAM socket, but syslog may want SOCK_STREAM */
	  if (!tried_stream && errno == EPROTOTYPE)
	    {
	    reopen_stream:
	      tried_stream = 1;
	      close(log_fd);
	      if ((log_fd = socket(AF_UNIX, SOCK_STREAM, 0)) != -1)
		{
		  int flags;

		  if ((flags = fcntl(log_fd, F_GETFD)) != -1)
		    fcntl(log_fd, F_SETFD, flags | FD_CLOEXEC);
		  
		  /* if max_log is zero, leave the socket blocking */
		  if (max_logs != 0 && (flags = fcntl(log_fd, F_GETFL)) != -1)
		    fcntl(log_fd, F_SETFL, flags | O_NONBLOCK);
		 
		  continue;
		}
	    }
	}
      
      /* give up - fall back to syslog() - this handles out-of-space
	 when logging to a file, for instance. */
      log_fd = -1;
      my_syslog(LOG_CRIT, _("log failed: %s"), strerror(errno));
      return;
    }
}

void my_syslog(int priority, const char *format, ...)
{
  va_list ap;
  struct log_entry *entry;
  time_t time_now;
  char *p;
  size_t len;
  
  va_start(ap, format); 
  
  if (log_stderr) 
    {
      fprintf(stderr, "dnsmasq: ");
      vfprintf(stderr, format, ap);
      fputc('\n', stderr);
    }
  
  if (log_fd == -1)
    {
      /* fall-back to syslog if we die during startup or fail during running. */
      static int isopen = 0;
      if (!isopen)
	{
	  openlog("dnsmasq", LOG_PID, log_fac);
	  isopen = 1;
	}
      vsyslog(priority, format, ap);
      va_end(ap);
      return;
    }
  
  if ((entry = free_entries))
    free_entries = entry->next;
  else if (entries_alloced < max_logs && (entry = malloc(sizeof(struct log_entry))))
    entries_alloced++;
  
  if (!entry)
    entries_lost++;
  else
    {
      /* add to end of list, consumed from the start */
      entry->next = NULL;
      if (!entries)
	entries = entry;
      else
	{
	  struct log_entry *tmp;
	  for (tmp = entries; tmp->next; tmp = tmp->next);
	  tmp->next = entry;
	}
      
      time(&time_now);
      p = entry->payload;
      if (!log_to_file)
	p += sprintf(p, "<%d>", priority | log_fac);
      
      p += sprintf(p, "%.15s dnsmasq[%d]: ", ctime(&time_now) + 4, getpid());
      len = p - entry->payload;
      len += vsnprintf(p, MAX_MESSAGE - len, format, ap) + 1; /* include zero-terminator */
      entry->length = len > MAX_MESSAGE ? MAX_MESSAGE : len;
      entry->offset = 0;

      /* replace terminator with \n */
      if (log_to_file)
	entry->payload[entry->length - 1] = '\n';
    }
  
  /* almost always, logging won't block, so try and write this now,
     to save collecting too many log messages during a select loop. */
  log_write();
  
  /* Since we're doing things asynchronously, a cache-dump, for instance,
     can now generate log lines very fast. With a small buffer (desirable),
     that means it can overflow the log-buffer very quickly,
     so that the cache dump becomes mainly a count of how many lines 
     overflowed. To avoid this, we delay here, the delay is controlled 
     by queue-occupancy, and grows exponentially. The delay is limited to (2^8)ms.
     The scaling stuff ensures that when the queue is bigger than 8, the delay
     only occurs for the last 8 entries. Once the queue is full, we stop delaying
     to preserve performance.
  */

  if (entries && max_logs != 0)
    {
      int d;
      
      for (d = 0,entry = entries; entry; entry = entry->next, d++);
      
      if (d == max_logs)
	d = 0;
      else if (max_logs > 8)
	d -= max_logs - 8;

      if (d > 0)
	{
	  struct timespec waiter;
	  waiter.tv_sec = 0;
	  waiter.tv_nsec = 1000000 << (d - 1); /* 1 ms */
	  nanosleep(&waiter, NULL);
      
	  /* Have another go now */
	  log_write();
	}
    } 
 
  va_end(ap);
}

void set_log_writer(fd_set *set, int *maxfdp)
{
  if (entries && log_fd != -1 && connection_good)
    {
      FD_SET(log_fd, set);
      bump_maxfd(log_fd, maxfdp);
    }
}

void check_log_writer(fd_set *set)
{
  if (log_fd != -1 && FD_ISSET(log_fd, set))
    log_write();
}

void die(char *message, char *arg1)
{
  char *errmess = strerror(errno);
  
  if (!arg1)
    arg1 = errmess;

  log_stderr = 1; /* print as well as log when we die.... */
  my_syslog(LOG_CRIT, message, arg1, errmess);
  
  log_stderr = 0;
  my_syslog(LOG_CRIT, _("FAILED to start up"));
  
  exit(1);
}
