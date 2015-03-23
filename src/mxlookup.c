/*--------------------------------------------------------------------*
 *
 * Developed by;
 *	Neal Horman - http://www.wanlink.com
 *	Copyright (c) 2003 Neal Horman. All Rights Reserved
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
 *	CVSID:  $Id: mxlookup.c,v 1.4 2011/07/29 21:23:17 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		mxlookup.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: mxlookup.c,v 1.4 2011/07/29 21:23:17 neal Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "mx.h"
#include "misc.h"

#define mkip(a,b,c,d) ((((a)&0xff)<<24)|(((b)&0xff)<<16)|(((c)&0xff)<<8)|((d)&0xff))

int gbTerse = 0;

void mxsrr(mx_rr_list *rrl)
{	int	i,j,l;
	mx_rr	*rr;

	for(i=0; i<rrl->qty; i++)
	{
		/* find highest priority non-visited MX RR */
		for(rr=NULL,j=0,l=-1; j<rrl->qty; j++)
		{
			if(!rrl->mx[j].visited && (l == -1 || rrl->mx[j].pref <= l))
			{
				l = rrl->mx[j].pref;
				rr = &rrl->mx[j];
			}
		}

		if(rr != NULL)
		{
			if(!gbTerse)
				printf("\tMX %u %s\n",rr->pref,rr->name);
			for(j=0; j<rr->qty; j++)
			{
				switch(rr->host[j].nsType)
				{
					case ns_t_a:
						{	struct in_addr ip;
							char *pStr = NULL;

							ip.s_addr = htonl(rr->host[j].ipv4);
							pStr = mlfi_inet_ntopAF(AF_INET, (char *)&ip);

							printf(gbTerse ? "%s\n" : "\t\tA %s\n", pStr);
							free(pStr);
						}
						break;
					case ns_t_aaaa:
						{
							char *pStr = mlfi_inet_ntopAF(AF_INET6, (char *)&rr->host[j].ipv6);

							printf(gbTerse ? "%s\n" : "\t\tAAAA %s\n", pStr);
							free(pStr);
						}
						break;
				}
			}
			rr->visited = 1;
		}
	}
}

void mxbsrr(char *domain)
{	mx_rr_list	mxrrl;
	res_state	statp = RES_NALLOC(statp);

	if(domain != NULL)
	{	ds_t ds;

		ds.statp = statp;
		ds.pSessionId = "";
		ds.bLoggingEnabled = 0;

		res_ninit(statp);

		if(!gbTerse)
			printf("%s\n",domain);

		memset(&mxrrl,0,sizeof(mx_rr_list));
		mxsrr(mx_get_rr_bydomain(&ds, &mxrrl,domain));
	}
}

void usage()
{
	printf("usage: [-t] domain.com ...\n"
		"\t-t = terse mode - just ip addresses\n"
		);

	exit(1);
}

int main(int argc, char **argv)
{	int		c;

	while ((c = getopt(argc, argv, "t")) != -1)
	{
		switch (c)
		{
			case 't':
				gbTerse = 1;
				break;
			case '?':
			default:
				usage();
				break;
		}
	}

	argc -= optind;
	argv += optind;

	if(argc)
	{
		while(argc > 0)
		{
			mxbsrr(*argv);
			argc--;
			argv++;
		}
	}
	else
		usage();

	return(0);
}
