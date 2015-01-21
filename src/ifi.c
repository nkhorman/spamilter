/*--------------------------------------------------------------------*
 *
 * Developed by;
 *	W. Richard Stevens - http://www.kohala.com
 *	Copyright (c) 1998 W. Richard Stevens. All Rights Reserved
 *
 *	This code is published at
 *	ftp://ftp.kohala.com/pub/rstevens/unpv12e.tar.gz as a result
 * 	of Richard's exelent work documenting the BSD TCP/IP network stack.
 *	One of which is;
 *	"Unix Network Programming - Network APIs: Sockets and XTI Volume 1"
 *
 *	I took this portion of the library and modified it. Richard does
 *	not appear to have placed the source under a specific license,
 *	although he does disclaim damages;
 *
 *	"        LIMITS OF LIABILITY AND DISCLAIMER OF WARRANTY
 *	
 *	The author and publisher of the book "UNIX Network Programming" have
 *	used their best efforts in preparing this software.  These efforts
 *	include the development, research, and testing of the theories and
 *	programs to determine their effectiveness.  The author and publisher
 *	make no warranty of any kind, express or implied, with regard to
 *	these programs or the documentation contained in the book. The author
 *	and publisher shall not be liable in any event for incidental or
 *	consequential damages in connection with, or arising out of, the
 *	furnishing, performance, or use of these programs."
 *
 *	Sadly, we will see no more books published by him.
 *
 *	
 * Modified by;
 *	Neal Horman - http://www.wanlink.com
 *	Portions Copyright (c) 2003 Neal Horman. All Rights Reserved
 *
 *	Redistribution and use in source and binary forms, with or without
 *	modification, are permitted provided that the following conditions
 *	are met:
 *	1. Redistributions of source code must retain the above copyright
 *	   notice, this list of conditions and the following disclaimer.
 *	2. Redistributions in binary form must reproduce the above copyright
 *	   notice, this list of conditions and the following disclaimer in the
 *	   documentation and/or other materials provided with the distribution.
 *	3. All advertising materials mentioning features or use of this software
 *	   must display the following acknowledgement:
 *	This product includes software developed by Neal Horman.
 *	4. Neither the name Neal Horman nor the names of any contributors
 *	   may be used to endorse or promote products derived from this software
 *	   without specific prior written permission.
 *	
 *	THIS SOFTWARE IS PROVIDED BY NEAL HORMAN AND ANY CONTRIBUTORS ``AS IS'' AND
 *	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *	ARE DISCLAIMED.  IN NO EVENT SHALL NEAL HORMAN OR ANY CONTRIBUTORS BE LIABLE
 *	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *	OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *	OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *	SUCH DAMAGE.
 *
 *	CVSID:  $Id: ifi.c,v 1.14 2011/07/29 21:23:17 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		ifi.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: ifi.c,v 1.14 2011/07/29 21:23:17 neal Exp $";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>

#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>

#ifdef OS_SunOS
#include <sys/sockio.h>
#endif

#include "ifi.h"

#ifdef __FreeBSD__
#define HAVE_SOCKADDR_SA_LEN 1
#endif

#ifndef max
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif

struct ifi_info * get_ifi_info(int family, int doaliases)
{	struct ifi_info		*ifi, *ifihead = NULL, **ifipnext;
	int			sockfd, len, lastlen, flags, myflags;
	char			*ptr, *buf = NULL, lastname[IFNAMSIZ], *cptr;
	struct ifconf		ifc;
	struct ifreq		*ifr, ifrcopy;
	struct sockaddr_in	*sinptr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd != -1)
	{
		lastlen = 0;
		len = 100 * sizeof(struct ifreq);	/* initial buffer size guess */
		for ( ; ; )
		{
			buf = calloc(1,len);
			ifc.ifc_len = len;
			ifc.ifc_buf = buf;
			if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0)
			{
				if (errno != EINVAL)
					printf("ioctl error ");
				if (lastlen != 0)
					printf("ioctl lastlen ");
			}
			else
			{
				if (ifc.ifc_len == lastlen)
					break;		/* success, len has not changed */
				lastlen = ifc.ifc_len;
			}
			len += 10 * sizeof(struct ifreq);	/* increment */
			free(buf);
		}
		ifihead = NULL;
		ifipnext = &ifihead;
		lastname[0] = 0;

		for (ptr = buf; ptr < buf + ifc.ifc_len; )
		{
			ifr = (struct ifreq *) ptr;

#ifdef	HAVE_SOCKADDR_SA_LEN
			len = max(sizeof(struct sockaddr), ifr->ifr_addr.sa_len);
#else
			switch (ifr->ifr_addr.sa_family)
			{
#ifdef	IPV6
				case AF_INET6:	
					len = sizeof(struct sockaddr_in6);
					break;
#endif
				case AF_INET:	
				default:	
					len = sizeof(struct sockaddr);
					break;
			}
#endif	/* HAVE_SOCKADDR_SA_LEN */
			ptr += sizeof(ifr->ifr_name) + len;	/* for next one in buffer */

			if (ifr->ifr_addr.sa_family != family)
				continue;	/* ignore if not desired address family */

			myflags = 0;
			if ( (cptr = strchr(ifr->ifr_name, ':')) != NULL)
				*cptr = 0;		/* replace colon will null */
			if (strncmp(lastname, ifr->ifr_name, IFNAMSIZ) == 0)
			{
				if (doaliases == 0)
					continue;	/* already processed this interface */
				myflags = IFI_ALIAS;
			}
			memcpy(lastname, ifr->ifr_name, IFNAMSIZ);

			ifrcopy = *ifr;
			ioctl(sockfd, SIOCGIFFLAGS, &ifrcopy);
			flags = ifrcopy.ifr_flags;
			if ((flags & IFF_UP) == 0)
				continue;	/* ignore if interface not up */

			ifi = calloc(1, sizeof(struct ifi_info));
			*ifipnext = ifi;			/* prev points to this new one */
			ifipnext = &ifi->ifi_next;	/* pointer to next one goes here */

			ifi->ifi_flags = flags;		/* IFF_xxx values */
			ifi->ifi_myflags = myflags;	/* IFI_xxx values */
			memcpy(ifi->ifi_name, ifr->ifr_name, IFI_NAME);
			ifi->ifi_name[IFI_NAME-1] = '\0';

			switch (ifr->ifr_addr.sa_family)
			{
				case AF_INET:
					sinptr = (struct sockaddr_in *) &ifr->ifr_addr;
					if (ifi->ifi_addr == NULL)
					{
						ifi->ifi_addr = calloc(1, sizeof(struct sockaddr_in));
						memcpy(ifi->ifi_addr, sinptr, sizeof(struct sockaddr_in));

#ifdef	SIOCGIFBRDADDR
						if (flags & IFF_BROADCAST)
						{
							ioctl(sockfd, SIOCGIFBRDADDR, &ifrcopy);
							sinptr = (struct sockaddr_in *) &ifrcopy.ifr_broadaddr;
							ifi->ifi_brdaddr = calloc(1, sizeof(struct sockaddr_in));
							memcpy(ifi->ifi_brdaddr, sinptr, sizeof(struct sockaddr_in));
						}
#endif

#ifdef	SIOCGIFDSTADDR
						if (flags & IFF_POINTOPOINT)
						{
							ioctl(sockfd, SIOCGIFDSTADDR, &ifrcopy);
							sinptr = (struct sockaddr_in *) &ifrcopy.ifr_dstaddr;
							ifi->ifi_dstaddr = calloc(1, sizeof(struct sockaddr_in));
							memcpy(ifi->ifi_dstaddr, sinptr, sizeof(struct sockaddr_in));
						}
#endif
#ifdef SIOCGIFNETMASK
						ioctl(sockfd, SIOCGIFNETMASK, &ifrcopy);
						sinptr = (struct sockaddr_in *) &ifrcopy.ifr_addr;
						ifi->ifi_netmask = calloc(1, sizeof(struct sockaddr_in));
						memcpy(ifi->ifi_netmask, sinptr, sizeof(struct sockaddr_in));
#endif
					}
					break;

				default:
					break;
			}
		}
	}

	if(buf != NULL)
		free(buf);
	if(sockfd != -1)
		close(sockfd);

	return(ifihead);	/* pointer to first structure in linked list */
}

void free_ifi_info(struct ifi_info *ifihead)
{
	struct ifi_info	*ifi, *ifinext;

	for (ifi = ifihead; ifi != NULL; ifi = ifinext)
	{
		if (ifi->ifi_addr != NULL)
			free(ifi->ifi_addr);
		if (ifi->ifi_brdaddr != NULL)
			free(ifi->ifi_brdaddr);
		if (ifi->ifi_dstaddr != NULL)
			free(ifi->ifi_dstaddr);
#ifdef SIOCGIFNETMASK
		if(ifi->ifi_netmask != NULL)
			free(ifi->ifi_netmask);
#endif
		ifinext = ifi->ifi_next;	/* can't fetch ifi_next after free() */
		free(ifi);			/* the ifi_info{} itself */
	}
}

int ifi_islocalip(long ip)
{	struct ifi_info	*ifihead = NULL;
	struct ifi_info	*ifi = NULL;
        int		match = 0;

	for(ifi = ifihead = get_ifi_info(AF_INET,1); ifi != NULL && !match; ifi = ifi->ifi_next)
	{
		if(ifi->ifi_flags&IFF_UP)
			match = (ip == ntohl(((struct sockaddr_in *) ifi->ifi_addr)->sin_addr.s_addr));
	}

	free_ifi_info(ifihead);

	return(match);
}

int ifi_islocalnet(long ip)
{	struct ifi_info	*ifihead = NULL;
	struct ifi_info	*ifi = NULL;
        int		match = 0;

#ifdef SIOCGIFNETMASK
	for(ifi = ifihead = get_ifi_info(AF_INET,1); ifi != NULL && !match; ifi = ifi->ifi_next)
	{       long    ifiip   = ntohl(((struct sockaddr_in *) ifi->ifi_addr)->sin_addr.s_addr);
		long    ifimsk  = ntohl(((struct sockaddr_in *) ifi->ifi_netmask)->sin_addr.s_addr);

		if(ifi->ifi_flags&IFF_UP)
		{
			match = ((ifiip&ifimsk) == (ip&ifimsk));
/*
#ifdef _UNIT_TEST
			printf("ifi_islocalnet: interface ip %u.%u.%u.%u / %u.%u.%u.%u match %u\n"
				,(ifiip&0xff000000)>>24,(ifiip&0x00ff0000)>>16,(ifiip&0x0000ff00)>>8,(ifiip&0x000000ff)
				,(ifimsk&0xff000000)>>24,(ifimsk&0x00ff0000)>>16,(ifimsk&0x0000ff00)>>8,(ifimsk&0x000000ff)
				,match
				);
#endif
*/
		}
	}

	free_ifi_info(ifihead);
#else
#error ifi_islocalnet will not work correctly without SIOCGIFNETMASK discovery, see the comment following this error!
#endif
/*
	ifi_islocalnet will not work correctly without SIOCGIFNETMASK discovery.
	this means that spamilter WILL do filter checks against ALL INTERNAL as well as external hosts.
	You should consider using the popauth code, and manually populating the db with the ip address
	of all your internal hosts, or modifying this code to do ip network/mask checking.

	You should add your trusted hosts' network entries to the lnh array below
 */

#define MAKEIP(a,b,c,d) ((unsigned long)((a<<24)|(b<<16)|(c<<8)|(d)))
 	{	int	i,q;
		struct _localnethosts
		{
			long ip;
			long mask;
		} lnh[] =
		{
			// always allow localhost
			{0x7f000001l,0xff000000l},	// ip = 127.0.0.0/8
#include "ifilocal.inc"

/* if this host is internal to a firewall, then you may also want to use one or more of these */
#ifdef ALLOW_LOCALNET_HOSTS
			/* rfc1918 networks */
			{0x0a000000l,0xff000000l},	/* ip = 10.0.0.0/8 */
			{0xac100000l,0xfff00000l},	/* ip = 172.16.0.0/12 */
			{0xc0a80000l,0xffff0000l},	/* ip = 192.168.0.0/16 */

			/* draft-manning-dsua-03.txt (1 May 2000) nets, specifically DHCP auto-configuration */
			{0xa9fe0000l,0xffff0000l},	/* ip = 169.254.0.0/16 */
#endif
		};

		for(i=0,q=sizeof(lnh)/sizeof(lnh[0]); i<q && !match; i++)
		{
			match |= ((lnh[i].ip & lnh[i].mask) == (ip & lnh[i].mask));
/*
#ifdef _UNIT_TEST
			printf("ifi_islocalnet: LocalNetHostsTable ip %u.%u.%u.%u / %u.%u.%u.%u match %u\n"
				,(lnh[i].ip&0xff000000)>>24,(lnh[i].ip&0x00ff0000)>>16,(lnh[i].ip&0x0000ff00)>>8,(lnh[i].ip&0x000000ff)
				,(lnh[i].mask&0xff000000)>>24,(lnh[i].mask&0x00ff0000)>>16,(lnh[i].mask&0x0000ff00)>>8,(lnh[i].mask&0x000000ff)
				,match
				);
#endif
*/
		}
	}

	return(match);
}

#ifdef _UNIT_TEST
int main(int argc, char **argv)
{	int bUsage = 0;

	if(argc>1)
	{
		argv++;
		argc--;
		while(argc > 1)
		{
			if(strcasecmp(*argv,"--islocalnet") == 0)
			{	struct hostent *phostent = gethostbyname(*(argv+1));
				unsigned long ip = (phostent != NULL ? ntohl(*(long *)phostent->h_addr) : 0);

				if(ip)
					printf("%s is localnet %u\n",*(argv+1),ifi_islocalnet(ip));
			}
			else
			{
				bUsage = 1;
				printf("unhandled args '%s %s'\n",*argv,*(argv+1));
			}
			argc-=2;
			argv+=2;
		}

		if(argc > 0)
		{
			bUsage = 1;
			printf("unhandled arg '%s\n",*argv);
		}
	}
	else
		bUsage = 1;

	if(bUsage)
		printf("usage: [--islocalnet IpAddress / hostname]\n");

	return 0;
}
#endif
