/*--------------------------------------------------------------------*
 *
 * Developed by;
 *	Neal Horman - http://www.wanlink.com
 *	Copyright (c) 2012 Neal Horman. All Rights Reserved
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
 *	CVSID:  $Id: dbl.c,v 1.3 2014/09/01 16:09:47 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		dsbl.c
 *--------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "dbl.h"

int dbl_check(const res_state statp, const char *pDbl, const char *pDomain, dbl_callback_t pCallback, void *pCallbackData)
{	u_char resp[NS_PACKETSZ];
	int rc = dns_query_rr_a_resp(statp,&resp[0],sizeof(resp),"%s.%s",pDomain,pDbl);
	int again = 1;

	if(rc > 0)
	{	ns_msg	handle;
	
		if(ns_initparse(resp,rc,&handle) > -1)
		{	int	rrnum;
			ns_rr	rr;
			int	count = ns_msg_count(handle,ns_s_an);

			for(rrnum=0; rrnum<count; rrnum++)
			{
				if(ns_parserr(&handle, ns_s_an, rrnum, &rr) == 0)
				{
					switch((unsigned int)ns_rr_type(rr)) // the cast quells the compiler "enumeration not handled" warning
					{
						case ns_t_a:
							again = pCallback(pDbl,pDomain,ns_get32(ns_rr_rdata(rr)),pCallbackData);
							break;
						case ns_t_aaaa:
							break;
					}
				}
			}
		}
	}

	return again;
}

void dbl_check_list(const res_state statp, const char **ppDbl, const char *pDomain, dbl_callback_t pCallback, void *pCallbackData)
{	int again = 1;

	while(*ppDbl && again)
		again = dbl_check(statp,*(ppDbl++),pDomain,pCallback,pCallbackData);
}

void dbl_check_all(const res_state statp, const char *pDomain, dbl_callback_t pCallback, void *pCallbackData)
{
	const char *pDbls[] =
	{
		"dbl.spamhaus.org",
		"multi.surbl.org",
		"black.uribl.com",
		NULL
	};

	dbl_check_list(statp,pDbls,pDomain,pCallback,pCallbackData);
}

#ifdef _UNIT_TEST
int main(int argc, char **argv)
{	char *pDbl = "dbl.spamhaus.org";
	char *pDomain = NULL;
	int c;

	while ((c = getopt(argc, argv, "h:l:")) != -1)
	{
		switch (c)
		{
			case 'l':
				if(optarg != NULL && *optarg)
					pDbl = optarg;
			case 'h':
				if(optarg != NULL && *optarg)
					pDomain = optarg;
				break;
		}
	}
	argc -= optind;
	argv += optind;


	if(pDomain != NULL)
	{
		if(pDbl != NULL)
		{
			res_state presstate = NULL;
		
			presstate = RES_NALLOC(presstate);

			if(presstate != NULL)
			{	int rc;
				unsigned long ip = 0;

				printf("using DBL '%s' for lookup of '%s'\n",pDbl,pDomain);

				int callback(const char *pDbl, const char *pDomain, unsigned ip, void *pData)
				{
					if((ip&0xffffff00) == 0x7f000100)
						printf("'%s' '%s' - %u.%u.%u.%u\n" ,pDbl ,pDomain ,((ip&0xff000000)>>24) ,((ip&0x00ff0000)>>16) ,((ip&0x0000ff00)>>8) ,((ip&0x000000ff)));

					return 1; // again
				}

				res_ninit(presstate);

				if(pDbl != NULL)
					dbl_check(presstate,pDbl,pDomain,&callback,NULL);
				else
					dbl_check_all(presstate,pDomain,&callback,NULL);

				res_nclose(presstate);
				free(presstate);
			}
			else
				printf("enable to init resolver\n");
		}
		else
			printf("no DBL specified, use -l [DBL hostname]\n");
	}
	else
		printf("no domain specified - use -h [host/domain name]\n");

	return 0;
}
#endif
