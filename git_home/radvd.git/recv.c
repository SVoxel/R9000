/*
 *   $Id$
 *
 *   Authors:
 *    Pedro Roque		<roque@di.fc.ul.pt>
 *    Lars Fenneberg		<lf@elemental.net>	 
 *
 *   This software is Copyright 1996,1997 by the above mentioned author(s), 
 *   All Rights Reserved.
 *
 *   The license which is distributed with this software in the file COPYRIGHT
 *   applies to this software. If your distribution is missing this file, you
 *   may request it from <pekkas@netcore.fi>.
 *
 */

#include <config.h>
#include <includes.h>
#include <radvd.h>

#if defined(__UCLIBC__) && (__UCLIBC_MAJOR__ == 0) && (__UCLIBC_MINOR__ == 9) && (__UCLIBC_SUBLEVEL__ < 29)
struct cmsghdr *
cmsg_nxthdr (struct msghdr *mhdr, struct cmsghdr *cmsg)
{
  if ((size_t) cmsg->cmsg_len < sizeof (struct cmsghdr))
    /* The kernel header does this so there may be a reason.  */
    return NULL;

  cmsg = (struct cmsghdr *) ((unsigned char *) cmsg
			     + CMSG_ALIGN (cmsg->cmsg_len));
  if ((unsigned char *) (cmsg + 1) > ((unsigned char *) mhdr->msg_control
				      + mhdr->msg_controllen)
      || ((unsigned char *) cmsg + CMSG_ALIGN (cmsg->cmsg_len)
	  > ((unsigned char *) mhdr->msg_control + mhdr->msg_controllen)))
    /* No more entries.  */
    return NULL;
  return cmsg;
}
#endif

int
recv_rs_ra(int sock, unsigned char *msg, struct sockaddr_in6 *addr,
                 struct in6_pktinfo **pkt_info, int *hoplimit)
{                 
	struct msghdr mhdr;
	struct cmsghdr *cmsg;
	struct iovec iov;
	static unsigned char *chdr = NULL;
	static unsigned int chdrlen = 0;
	int len;
	fd_set rfds;

	if( ! chdr )
	{
		chdrlen = CMSG_SPACE(sizeof(struct in6_pktinfo)) +
				CMSG_SPACE(sizeof(int));
		if ((chdr = malloc(chdrlen)) == NULL) {
			flog(LOG_ERR, "recv_rs_ra: malloc: %s", strerror(errno));
			return -1;
		}
	}

	FD_ZERO( &rfds );
	FD_SET( sock, &rfds );

	if( select( sock+1, &rfds, NULL, NULL, NULL ) < 0 )
	{
		if (errno != EINTR)
			flog(LOG_ERR, "select: %s", strerror(errno));
			
		return -1;
	}

	iov.iov_len = MSG_SIZE;
	iov.iov_base = (caddr_t) msg;

	memset(&mhdr, 0, sizeof(mhdr));
	mhdr.msg_name = (caddr_t)addr;
	mhdr.msg_namelen = sizeof(*addr);
	mhdr.msg_iov = &iov;
	mhdr.msg_iovlen = 1;
	mhdr.msg_control = (void *)chdr;
	mhdr.msg_controllen = chdrlen;

	len = recvmsg(sock, &mhdr, 0);

	if (len < 0)
	{
		if (errno != EINTR)
			flog(LOG_ERR, "recvmsg: %s", strerror(errno));
			
		return len;
	}

	*hoplimit = 255;

#if defined(__UCLIBC__) && (__UCLIBC_MAJOR__ == 0) && (__UCLIBC_MINOR__ == 9) && (__UCLIBC_SUBLEVEL__ < 29)
        for (cmsg = CMSG_FIRSTHDR(&mhdr); cmsg != NULL; cmsg = cmsg_nxthdr(&mhdr, cmsg))
#else
        for (cmsg = CMSG_FIRSTHDR(&mhdr); cmsg != NULL; cmsg = CMSG_NXTHDR(&mhdr, cmsg))
#endif
	{
          if (cmsg->cmsg_level != IPPROTO_IPV6)
          	continue;
          
          switch(cmsg->cmsg_type)
          {
#ifdef IPV6_HOPLIMIT
              case IPV6_HOPLIMIT:
                if ((cmsg->cmsg_len == CMSG_LEN(sizeof(int))) && 
                    (*(int *)CMSG_DATA(cmsg) >= 0) && 
                    (*(int *)CMSG_DATA(cmsg) < 256))
                {
                  *hoplimit = *(int *)CMSG_DATA(cmsg);
                }
                else
                {
                  flog(LOG_ERR, "received a bogus IPV6_HOPLIMIT from the kernel! len=%d, data=%d",
                  	cmsg->cmsg_len, *(int *)CMSG_DATA(cmsg));
                  return (-1);	
                }  
                break;
#endif /* IPV6_HOPLIMIT */
              case IPV6_PKTINFO:
                if ((cmsg->cmsg_len == CMSG_LEN(sizeof(struct in6_pktinfo))) &&
                    ((struct in6_pktinfo *)CMSG_DATA(cmsg))->ipi6_ifindex)
                {
                  *pkt_info = (struct in6_pktinfo *)CMSG_DATA(cmsg);
                }
                else
                {
                  flog(LOG_ERR, "received a bogus IPV6_PKTINFO from the kernel! len=%d, index=%d", 
                  	cmsg->cmsg_len, ((struct in6_pktinfo *)CMSG_DATA(cmsg))->ipi6_ifindex);
                  return (-1);
                } 
                break;
          }
	}
	
	dlog(LOG_DEBUG, 4, "recvmsg len=%d", len);

	return len;
}
