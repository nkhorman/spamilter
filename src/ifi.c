/*--------------------------------------------------------------------*
 *
 * Developed by;
 *	Neal Horman - http://www.wanlink.com
 *	Portions Copyright (c) 2003-2015 Neal Horman. All Rights Reserved
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
#include <netdb.h>
#include <errno.h>

#ifdef OS_SunOS
#include <sys/sockio.h>
#endif

#include "ifi.h"
#include "ifidb.h"
#include "list.h"

#ifndef max
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif

#ifndef s6_addr32
#define s6_addr32 __u6_addr.__u6_addr32
#endif

#include <ifaddrs.h>

#ifdef _UNIT_TEST_IFI

#include <arpa/inet.h>

char *gDbpath = "/var/db/spamilter";

char *inet_ntopAf(int afType, const char *in)
{	char *pstr = NULL;

	if(in != NULL)
	{	size_t s = 0;

		switch(afType)
		{
			case AF_INET: s = INET_ADDRSTRLEN; break;
			case AF_INET6: s = INET6_ADDRSTRLEN; break;
		}

		if(s > 0 && (pstr = calloc(1,s)) != NULL)
			pstr = (char *)inet_ntop(afType, in, pstr, s);
	}

	return pstr;
}
#endif

enum
{
	MATCH_NONE,
	MATCH_HOST,
	MATCH_NETWORK,
};

typedef struct _iil_t
{
	int afType;
	union
	{
		struct in_addr ipv4;
		struct in6_addr ipv6;
	};
	int match;
	int matchType; // MATCH_HOST, MATCH_NETWORK
	int matchAllow;
	list_t *pIfiDb;
}iil_t; // Ifi Is Local Type

static int ifi_callbackIsLocal(int afType, void *pAddr, void *pMask, void *pCtx)
{	iil_t *pIil = (iil_t *)pCtx;

	if(afType == pIil->afType)
	{	int isLocal = 0;

#ifdef _UNIT_TEST_IFI
		char *pStrAddr = inet_ntopAf(afType, pAddr);
		char *pStrMask = inet_ntopAf(afType, pMask);
		char *pStr = inet_ntopAf(afType, (char *)&pIil->ipv4);
		printf("%s %s %s\n", pStrAddr, pStrMask, pStr);
		free(pStrAddr);
		free(pStrMask);
		free(pStr);
#endif

		switch(pIil->afType)
		{
			case AF_INET:
				switch(pIil->matchType)
				{
					case MATCH_HOST:
						pIil->match = (pIil->ipv4.s_addr == ((struct in_addr *)pAddr)->s_addr);
						pIil->matchAllow = 1;
						break;
					case MATCH_NETWORK:
						isLocal = (
							(pIil->ipv4.s_addr & ((struct in_addr *)pMask)->s_addr) ==
							(((struct in_addr *)pAddr)->s_addr & ((struct in_addr *)pMask)->s_addr)
							);

						if(pIil->pIfiDb != NULL)
							pIil->match = ifiDb_CheckAllow(pIil->afType, (char *)&pIil->ipv4, pIil->pIfiDb, &pIil->matchAllow, isLocal);
						else
						{
							pIil->match = isLocal;
							pIil->matchAllow = 1;
						}
						break;
				}
				break;

			case AF_INET6:
				switch(pIil->matchType)
				{
					case MATCH_HOST:
						pIil->match = IN6_ARE_ADDR_EQUAL(&pIil->ipv6, (struct in6_addr *)pAddr);
						pIil->matchAllow = 1;
						break;
					case MATCH_NETWORK:
						{	struct in6_addr l = pIil->ipv6;
							struct in6_addr r = *(struct in6_addr *)pAddr;
							struct in6_addr m = *(struct in6_addr *)pMask;;

							isLocal = (
								(l.s6_addr32[0] & m.s6_addr32[0]) ==  (r.s6_addr32[0] & m.s6_addr32[0])
								&& (l.s6_addr32[1] & m.s6_addr32[1]) ==  (r.s6_addr32[1] & m.s6_addr32[1])
								&& (l.s6_addr32[2] & m.s6_addr32[2]) ==  (r.s6_addr32[2] & m.s6_addr32[2])
								&& (l.s6_addr32[3] & m.s6_addr32[3]) ==  (r.s6_addr32[3] & m.s6_addr32[3])
								);

							if(pIil->pIfiDb != NULL)
								pIil->match = ifiDb_CheckAllow(pIil->afType, (char *)&pIil->ipv6, pIil->pIfiDb, &pIil->matchAllow, isLocal);
							else
							{
								pIil->match = isLocal;
								pIil->matchAllow = 1;
							}
						}
						break;
				}
				break;
		}
	}

	return (pIil->match == 0); // again
}

static void ifi_iterate(int (*pCallbackFn)(int, void *, void *, void *), void *pCtx)
{	struct ifaddrs *pIfAddrs = NULL;

	if(getifaddrs(&pIfAddrs) == 0)
	{	struct ifaddrs *pIf = pIfAddrs;
		int again = 1;

		while(pIf != NULL && again)
		{
			if((pIf->ifa_flags & (IFF_UP | IFF_LOOPBACK | IFF_POINTOPOINT)) && (pIf->ifa_flags & IFF_UP))
			{
				char *pInAddr = NULL;
				char *pInMask = NULL;

				switch(pIf->ifa_addr->sa_family)
				{
					case AF_INET:
						pInAddr = (char *)&((struct sockaddr_in *)pIf->ifa_addr)->sin_addr;
						pInMask = (char *)&((struct sockaddr_in *)pIf->ifa_netmask)->sin_addr;
						break;
					case AF_INET6:
						pInAddr = (char *)&((struct sockaddr_in6 *)pIf->ifa_addr)->sin6_addr;
						pInMask = (char *)&((struct sockaddr_in6 *)pIf->ifa_netmask)->sin6_addr;
						break;
				}

				if(pInAddr != NULL && pInMask != NULL)
					again = pCallbackFn(pIf->ifa_addr->sa_family, pInAddr, pInMask, pCtx);
			}

			pIf = pIf->ifa_next;
		}

		freeifaddrs(pIfAddrs);
	}
}

int ifi_islocalipAf(int afType, const char *pIn)
{	iil_t iil;

	iil.afType = afType;
	switch(afType)
	{
		case AF_INET: iil.ipv4 = *(struct in_addr *)pIn; break;
		case AF_INET6: iil.ipv6 = *(struct in6_addr *)pIn; break;
	}
	iil.match = 0;
	iil.matchType = MATCH_HOST;
	iil.pIfiDb = NULL;

	ifi_iterate(&ifi_callbackIsLocal, &iil);

	return (iil.match && iil.matchAllow);
}

int ifi_islocalnetAf(int afType, const char *pIn, const char *pDbPath)
{	iil_t iil;
	ifiDbCtx_t *pIfiDbCtx = ifiDb_Create("");

	if(ifiDb_Open(pIfiDbCtx, pDbPath))
	{
		ifiDb_BuildList(pIfiDbCtx);
		ifiDb_Close(pIfiDbCtx);
	}

	iil.afType = afType;
	switch(afType)
	{
		case AF_INET: iil.ipv4 = *(struct in_addr *)pIn; break;
		case AF_INET6: iil.ipv6 = *(struct in6_addr *)pIn; break;
	}
	iil.match = 0;
	iil.matchType = MATCH_NETWORK;
	iil.pIfiDb = pIfiDbCtx->pIfiDb;

	ifi_iterate(&ifi_callbackIsLocal, &iil);

	ifiDb_Destroy(&pIfiDbCtx);

	return (iil.match && iil.matchAllow);
}

#ifdef _UNIT_TEST_IFI
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

				if(phostent != NULL)
					printf("%s is localnet %u\n", *(argv+1), ifi_islocalnetAf(phostent->h_addrtype, (char *)phostent->h_addr, gDbpath));
				else
				{	struct in_addr ipv4;
					struct in6_addr ipv6;
					int afType = AF_UNSPEC;

					if(inet_pton(AF_INET, *(argv+1), &ipv4))
						afType = AF_INET;
					else if(inet_pton(AF_INET6, *(argv+1), &ipv6))
						afType = AF_INET6;

					if(afType != AF_UNSPEC)
						printf("%s is localnet %u\n", *(argv+1), ifi_islocalnetAf(afType, (afType == AF_INET ? (char *)&ipv4 :  (char *)&ipv6), gDbpath));
					else
						printf("%s unable to gethostbyname\n", *(argv+1));
				}
			}
			else if(strcasecmp(*argv,"--dbpath") == 0)
				gDbpath = *(argv+1);
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
