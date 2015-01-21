/*--------------------------------------------------------------------*
 *
 * Developed by;
 *	Neal Horman - http://www.wanlink.com
 *	Copyright (c) 2004 Neal Horman. All Rights Reserved
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
 *	CVSID:  $Id: iflookup.c,v 1.7 2011/07/29 21:23:17 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		iflookup.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: iflookup.c,v 1.7 2011/07/29 21:23:17 neal Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include "ifi.h"
#include "dns.h"

int test(long ip)
{       struct ifi_info *ifihead,*ifi;
        int     match = 0;
	int	count = 0;
	struct in_addr inip;
	struct in_addr inmsk;

	inip.s_addr = htonl(ip);

	printf("%s\n",inet_ntoa(inip));
#ifdef SIOCGIFNETMASK
	for(ifi = ifihead = get_ifi_info(AF_INET,1); ifi != NULL; ifi = ifi->ifi_next)
	{       long    ifiip   = ntohl(((struct sockaddr_in *) ifi->ifi_addr)->sin_addr.s_addr);
		long    ifimsk  = (ifi->ifi_netmask != NULL ? ntohl(((struct sockaddr_in *) ifi->ifi_netmask)->sin_addr.s_addr) : 0);

		if(ifi->ifi_flags&IFF_UP)
		{	char *pip,*pmsk;

			inip.s_addr = htonl(ifiip);
			inmsk.s_addr = htonl(ifimsk);
			pip = strdup(inet_ntoa(inip));
			pmsk = strdup(inet_ntoa(inmsk));

			count += (match = ((ifiip&ifimsk) == (ip&ifimsk)));
			printf("\t%s - %s/%s%s\n",ifi->ifi_name,
				pip,pmsk,
				match ? " - match" : "");
		}
	}
#else
#error no netmask to do anything with
#endif

	return(count);
}

void usage()
{
	printf("usage: ipaddress ...\n"
		);

	exit(1);
}

int main(int argc, char **argv)
{
	if(argc>1)
	{
		argv++;
		argc--;
		while(argc > 0)
		{
			if(!test(ntohl(inet_addr(*argv))))
				printf("\tNo match\n");

			argc--;
			argv++;
		}
	}
	else
		usage();

	return(0);
}
