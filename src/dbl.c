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

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <netdb.h>

#include "dbl.h"
#include "misc.h"

#ifndef s6_addr32
#define s6_addr32 __u6_addr.__u6_addr32
#endif

// double your insanity
typedef struct _dbldns_t
{
	dblcb_t dblcb;
	int again;
} dbldns_t;

// The dns response RR iterator calls us, so we
// in turn call the consumers callback abstracting the dns
// iterator's callback function protyping from the consumer
// as well as calling the policy function callback, which
// might seem silly on the surface of it, but by using a policy
// callback, you an maintain the DBL return result processing in
// one place instead of dealing with it in a billion callback
// functions that only care about when the DBL failed the queried
// Domain.
// (That is one long runon sentence...)
static int dbl_check_callback(nsrr_t *pNsrr, void *pData)
{	dbldns_t *pDbldns = (dbldns_t *)pData;

	// convience pointers
	dblcb_t *pDblcb = &pDbldns->dblcb;
	const dblq_t *pDblq = pDblcb->pDblq;

	pDblcb->pNsrr = pNsrr;

	switch(ns_rr_type(pDblcb->pNsrr->rr))
	{
		// valid DBL answer types
		case ns_t_a: // 0x7f0001xx
		case ns_t_aaaa: // ::FFFF:7F00:2
		case ns_t_txt:
			// It's our job to make sure this is empty, and free it afterwards
			pDblcb->pDblResult = NULL;

			if(
				// the consumer knows how to deal wth the ns_rr
				pDblq->pCallbackPolicyFn == NULL
				// let the policy callback deal with ns_rr testing
				|| (pDblq->pCallbackPolicyFn != NULL && pDblq->pCallbackPolicyFn(pDblcb))
			)
			{
				pDbldns->again = pDblq->pCallbackFn(pDblcb);
			}

			// Because the policy callback filled this out, we are supposed to free it
			if(pDblcb->pDblResult != NULL)
			{
				// We're in trouble f this is not heap!
				free((void *)pDblcb->pDblResult);
					pDblcb->pDblResult = NULL;
			}
			break;

		// all other answers
		default:
			break;
	}

	return 1; // again
}

// Our consumer, gives us a context to pass to the callback for every RR in the dns response.
// So, since we ourselves, use an iterator for the RR's, we have to stuff the consumer foo, in
// a context struct, to pass through the iterator to use when calling the consumer's callback.
// Double your pleasure, double your fun... :)
int dbl_check(const res_state statp, const char *pDbl, const dblq_t *pDblq)
{	u_char resp[NS_PACKETSZ];
	int rc = dns_query_rr_resp(statp, &resp[0], sizeof(resp), ns_t_a, "%s.%s", pDblq->pDomain, pDbl);
	int again = 1;

	if(rc > 0)
	{
		dbldns_t dbldns;

		memset(&dbldns, 0, sizeof(dbldns));
		dbldns.dblcb.pDblq = pDblq;
		dbldns.dblcb.pDbl = pDbl;
		dbldns.again = 1;

		dns_parse_response_answer(resp, rc, &dbl_check_callback, &dbldns);

		again = dbldns.again;
	}

	return again;
}

// Iterate a list of DBLs using a callback
void dbl_check_list(const res_state statp, const char **ppDbl, const dblq_t *pDblq)
{	int again = 1;

	while(*ppDbl && again)
		again = dbl_check(statp,*(ppDbl++), pDblq);
}

// A predefined list of DBLs to iterate using a callback
// TODO - Move this list out into a config table, using
// the table driver and tableForEachRow()... oooh, another callback!
// tripple your insanity
void dbl_check_all(const res_state statp, const dblq_t *pDblq)
{
	const char *pDbls[] =
	{
		"dbl.spamhaus.org",
		"multi.surbl.org",
		"black.uribl.com",
		NULL
	};

	dbl_check_list(statp,pDbls, pDblq);
}

int dbl_callback_policy_std(dblcb_t *pDblcb)
{	int bCallbackProceed = 0;
	nsrr_t *pNsrr = pDblcb->pNsrr;

	// http://www.spamhaus.org/faq/section/Spamhaus%20DBL#277
	// http://www.rfc-editor.org/rfc/rfc5782.txt
	//
	// http://www.spamhaus.org/faq/section/Spamhaus%20DBL#291
	// Return Codes	Data Source
	// 127.0.1.2	spam domain
	// 127.0.1.4	phish domain
	// 127.0.1.5	malware domain
	// 127.0.1.6	botnet C&C domain
	// http://www.spamhaus.org/faq/section/Spamhaus%20DBL#411
	// 127.0.1.102	abused legit spam
	// 127.0.1.103	abused spammed redirector domain
	// 127.0.1.104	abused legit phish
	// 127.0.1.105	abused legit malware
	// 127.0.1.106	abused legit botnet C&C
	// 127.0.1.255	IP queries prohibited!

	switch(ns_rr_type(pNsrr->rr))
	{
		case ns_t_a: // 0x7f0001xx
			if(ns_rr_rdlen(pNsrr->rr) == NS_INADDRSZ)
			{	unsigned long ip = ns_get32(ns_rr_rdata(pNsrr->rr));

				if((ip&0xffffff00) == 0x7f000100)
				{
					pDblcb->abused = ((ip & 0x000000ff) >= 100); // an abused domain

					if(!pDblcb->abused)
						pDblcb->pDblResult = mlfi_inet_ntopAF(AF_INET, (char *)ns_rr_rdata(pNsrr->rr));

					bCallbackProceed = !pDblcb->abused;
				}
			}
			break;

		case ns_t_aaaa: // ::FFFF:7F00:2
			// TODO - none of this logic is validated
			if(ns_rr_rdlen(pNsrr->rr) == NS_IN6ADDRSZ)
			{
				struct in6_addr *pIp6 = (struct in6_addr *)ns_rr_rdata(pNsrr->rr);

				// 0x0000_0000 0000_0000 0000_ffff 7f00_0002
				if(
					pIp6->s6_addr32[0] == 0
					&& pIp6->s6_addr32[1] == 0
					&& pIp6->s6_addr32[2] == 0x0000ffff
					&& (pIp6->s6_addr32[3] & 0xffffff00) == 0x7f000100
					)
				{
					pDblcb->abused = ((pIp6->s6_addr32[3] & 0x000000ff) >= 100); // an abused domain

					if(!pDblcb->abused)
						pDblcb->pDblResult = mlfi_inet_ntopAF(AF_INET6, (char *)pIp6);
				}
			}
			break;

		//case ns_t_txt: // TODO
		//	break;

		default:
			break;
	}

	//if(pDblcb->pDblResult != NULL)
	//	printf("pDblResult %s\n", pDblcb->pDblResult);

	return bCallbackProceed;
}

#ifdef _UNIT_TEST
static int callback(const dblcb_t *pDblcb)
{
	printf("DBL '%s' query '%s' returned '%s'%s\n"
		, pDblcb->pDbl
		, pDblcb->pDblq->pDomain
		, pDblcb->pDblResult
		, (pDblcb->abused ? " - abused" : "")
		);

	return 1; // again
}

int main(int argc, char **argv)
{	char *pDbl = NULL;//"dbl.spamhaus.org";
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
			printf("using DBL '%s' for lookup of '%s'\n",pDbl,pDomain);

		res_state presstate = NULL;

		presstate = RES_NALLOC(presstate);

		if(presstate != NULL)
		{	dblq_t dblq;

			res_ninit(presstate);

			dblq.pDomain = pDomain;
			dblq.pCallbackFn = &callback;
			dblq.pCallbackData = NULL;
			dblq.pCallbackPolicyFn = &dbl_callback_policy_std;

			if(pDbl != NULL)
				dbl_check(presstate, pDbl, &dblq);
			else
				dbl_check_all(presstate, &dblq);

			res_nclose(presstate);
			free(presstate);
		}
		else
			printf("enable to init resolver\n");
	}
	else
		printf("no domain specified - use -h [host/domain name]\n");

	return 0;
}
#endif
