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
 *	CVSID:  $Id: mx.c,v 1.15 2012/12/09 18:19:42 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		mx.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: mx.c,v 1.15 2012/12/09 18:19:42 neal Exp $";


#include <string.h>
#include <stdlib.h>
#include "config.h"

#include "dns.h"
#include "mx.h"
#include "misc.h"

// collect A and AAAA record info
static int mx_get_rr_hosts_Callback(dqrr_t *pDqrr, void *pdata)
{	mx_rr *mxrr = (mx_rr *)pdata;
	int nsType = ns_rr_type(pDqrr->rr);

	switch(nsType)
	{
		case ns_t_a:
			if(ns_rr_rdlen(pDqrr->rr) == NS_INADDRSZ)
			{	int i, isDuplicate;
				unsigned long ip = ns_get32(ns_rr_rdata(pDqrr->rr));

				for(i=0,isDuplicate=0; i<mxrr->qty && !isDuplicate; i++)
					isDuplicate = (ip == mxrr->host[i].ipv4);

				if(!isDuplicate)
				{
					mxrr->host[mxrr->qty].ipv4 = ip;
					mxrr->host[mxrr->qty].nsType = nsType;
					mxrr->qty++;
				}
			}
			break;

		case ns_t_aaaa:
			if(ns_rr_rdlen(pDqrr->rr) == NS_IN6ADDRSZ)
			{
				int i, isDuplicate;
				struct in6_addr *pIp6 = (struct in6_addr *)ns_rr_rdata(pDqrr->rr);

				for(i=0,isDuplicate=0; i<mxrr->qty && !isDuplicate; i++)
					isDuplicate = (memcmp(pIp6, &mxrr->host[i].ipv6, sizeof(mxrr->host[i].ipv6)) == 0);

				if(!isDuplicate)
				{
					mxrr->host[mxrr->qty].ipv6 = *pIp6;
					mxrr->host[mxrr->qty].nsType = nsType;
					mxrr->qty++;
				}
			}
			break;
	}

	return (mxrr->qty < MAX_MXHOSTS); // again
}

// find A and AAAA host records
mx_rr *mx_get_rr_hosts(const res_state statp, mx_rr *mxrr)
{
	dqrr_t *pDqrr = dns_query_rr_init(statp, ns_t_a);

	// search for A records
	if(dns_query_rr_resp(pDqrr, mxrr->name) > -1)
		dns_parse_response_answer(pDqrr, &mx_get_rr_hosts_Callback, mxrr);

	// search for AAAA records
	if(mxrr->qty < MAX_MXHOSTS && dns_query_rr_resp(dns_query_rr_reinit(pDqrr, ns_t_aaaa), mxrr->name) > -1)
		dns_parse_response_answer(pDqrr, &mx_get_rr_hosts_Callback, mxrr);

	dns_query_rr_free(pDqrr);

	return mxrr;
}

// collect the mx record info
static int mx_get_rr_bydomainCallback(dqrr_t *pDqrr, void *pdata)
{	mx_rr_list *rrl = (mx_rr_list *)pdata;
	int nsType = ns_rr_type(pDqrr->rr);

	switch(nsType)
	{
		case ns_t_mx:
			{	char hostName[NS_MAXDNAME];
				int rc;
				const u_char *rrData = ns_rr_rdata(pDqrr->rr);
				unsigned short hostPreference = 0;

				memset(hostName, 0, sizeof(hostName));
				NS_GET16(hostPreference, rrData);

				rc = ns_name_uncompress(ns_msg_base(pDqrr->rrMsg), ns_msg_end(pDqrr->rrMsg), rrData, hostName, sizeof(hostName));
				if(rc != -1)
				{	int i,isDuplicate;

					// search for duplicate mx records
					for(i=0,isDuplicate=0; i < rrl->qty && !isDuplicate; i++)
						isDuplicate = !strcasecmp(rrl->mx[i].name, hostName);

					// if not duplicate
					if(!isDuplicate)
					{
						// save the mx record info
						strcpy(rrl->mx[rrl->qty].name, hostName);
						rrl->mx[rrl->qty].pref = hostPreference;

						// find all the hosts for the listed mx
						mx_get_rr_hosts(pDqrr->statp, &rrl->mx[rrl->qty]);
						rrl->qty++;
					}
				}
			}
			break;
	}

	return 1; // find all mx records
}

// find mx records and coresponding A and AAAA records per host for a given domain
mx_rr_list *mx_get_rr_bydomain(const res_state statp, mx_rr_list *rrl, const char *name)
{	dqrr_t *pDqrr = dns_query_rr_init(statp, ns_t_mx);

	// find mx records
	if(pDqrr != NULL)
	{	int rc = dns_query_rr_resp(pDqrr, name);

		if(rc > 0)
		{
			dns_parse_response_answer(pDqrr, &mx_get_rr_bydomainCallback, rrl);
			if(rrl->qty)
				strcpy(rrl->domain,name);
		}
		dns_query_rr_free(pDqrr);
	} 

	// no mx records found, build an entry to satisfy rfc2821
	if(rrl->qty == 0)
	{
		strcpy(rrl->domain,name);
		/*
			Fake out an MX record per rfc974 (now superceeded by rfc2821)
			section "Iterpreting the List of MX RRs"

			"It is possible that the list of MXs in the response to the query will
			be empty.  This is a special case.  If the list is empty, mailers
			should treat it as if it contained one RR, an MX RR with a preference
			value of 0, and a host name of REMOTE.  (I.e., REMOTE is its only
			MX).  In addition, the mailer should do no further processing on the
			list, but should attempt to deliver the message to REMOTE.  ... "
		*/
		strcpy(rrl->mx[rrl->qty].name,name);	// mx hostname
		rrl->mx[rrl->qty].pref = 0;		// mx host preference

		// find all the hosts for the listed mx
		mx_get_rr_hosts(statp, &rrl->mx[rrl->qty++]);

		if(rrl->mx[0].qty == 0)	// if no A or AAAA RRs,
			rrl->qty = 0;	// then no MX RRs either
	}

	return rrl;
}
