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
 *	CVSID:  $Id: dns.c,v 1.18 2012/06/26 01:04:29 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		dns.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: dns.c,v 1.18 2012/06/26 01:04:29 neal Exp $";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <netdb.h>

#include "spamilter.h"
#include "dns.h"

// Do a query of a specified type, return the result in the presp buffer
// The consumer provides the response buffer
int dns_query_rr_resp_v(const res_state statp, u_char *presp, size_t respLen, int nsType, char *fmt, va_list vl)
{	int	rc = 0;

	if(fmt != NULL && *fmt)
	{	char *hostName = NULL;
		int x = vasprintf(&hostName,fmt,vl);

		if(hostName != NULL && x > 0)
		{
			rc = res_nquery(statp, hostName, ns_c_in, nsType, presp, respLen);
			if(rc == -1 && h_errno == NETDB_SUCCESS)
				rc = 0;
		}

		if(hostName != NULL)
			free(hostName);
	}

	return rc;
}

// Do a query of a specified type, returning the result in the presp buffer
// The consumer provides the response buffer, because they want to parse it
int dns_query_rr_resp(const res_state statp, u_char *presp, size_t respLen, int nsType, char *fmt, ...)
{	va_list	vl;
	int rc;

	va_start(vl,fmt);
	rc = dns_query_rr_resp_v(statp, presp, respLen, nsType, fmt, vl);
	va_end(vl);

	return rc;
}

// Do a query of a specified type, returning 1 if there was at least one result
// The consumer doesn't care about the response content
int dns_query_rr(const res_state statp, int nsType, char *fmt, ...)
{	va_list	vl;
	int rc;
	u_char resp[NS_PACKETSZ];

	va_start(vl,fmt);
	rc = dns_query_rr_resp_v(statp, &resp[0], sizeof(resp), nsType, fmt, vl);
	va_end(vl);

	return (rc > 0);
}

// Make an in-addr.arpa style ipv4 query, and return 1 if found
int dns_rdnsbl_has_rr_a(const res_state statp, unsigned long ip, char *domain)
{
	// this is an in-addr.arpa style lookup
	return dns_query_rr(statp, ns_t_a, "%u.%u.%u.%u.%s",
			((ip&0x000000ff)), ((ip&0x0000ff00)>>8), ((ip&0x00ff0000)>>16), ((ip&0xff000000)>>24), domain
			);
}

// Iterate a given section of a response
void dns_parse_response(u_char *presp, size_t respLen, ns_sect nsSect, int (*pCallbackFn)(nsrr_t *, void *), void *pCallbackData)
{	ns_msg	handle;

	if(respLen && ns_initparse(presp, respLen, &handle) > -1)
	{	int rrnum;
		int count = ns_msg_count(handle, nsSect);
		int again = 1;
		nsrr_t nsrr;

		nsrr.pResp = presp;
		nsrr.respLen = respLen;
		for(rrnum=0; rrnum<count && again; rrnum++)
		{
			if(ns_parserr(&handle, nsSect, rrnum, &nsrr.rr) == 0)
				again = pCallbackFn(&nsrr, pCallbackData);
		}
	}
}

// Iterate the Answer Section of a response
void dns_parse_response_answer(u_char *presp, size_t respLen, int (*pCallbackFn)(nsrr_t *, void *), void *pCallbackData)
{
	dns_parse_response(presp, respLen, ns_s_an, pCallbackFn, pCallbackData);
}

typedef struct _dmi_t
{
	unsigned long ip;
	int match;
}dmi_t;

static int dnsMatchIpCallback(nsrr_t *pNsrr, void *pdata)
{
	dmi_t *pDmi = (dmi_t *)pdata;

	pDmi->match = (ns_rr_type(pNsrr->rr) == ns_t_a && ns_get32(ns_rr_rdata(pNsrr->rr)) == pDmi->ip);

	return (pDmi->match == 0); // again ?
}

// Query a hostname and find an ipv4 match
int dns_hostname_ip_match(const res_state statp, char *hostname, unsigned long hostip)
{	dmi_t dmi;

	dmi.ip = hostip;
	dmi.match = 0;

	if(hostname != NULL && hostip != 0)
	{
		u_char	resp[NS_PACKETSZ];
		int	rc = dns_query_rr_resp(statp, &resp[0], sizeof(resp), ns_t_a, "%s",hostname);

		if(rc > 0)
			dns_parse_response_answer(resp, rc, &dnsMatchIpCallback, &dmi);
	}

	return dmi.match;
}

#ifdef UNIT_TEST
// The consumer must free() return value
char *inet_ptoarpa(const char *pHost, int afNet)
{	char *pArpa = NULL;

	if(pHost != NULL)
	{
		char bufstr[8192];
		char bufnet[sizeof(struct in6_addr)];
		struct in_addr *pIp4 = (struct in_addr *)bufnet;
		struct in6_addr *pIp6 = (struct in6_addr *)bufnet;
		int i;

		memset(bufstr,0,sizeof(bufstr));
		memset(bufnet,0,sizeof(bufnet));

		switch(afNet)
		{
			case AF_INET:
				if(inet_pton(AF_INET, pHost, pIp4))
				{
					for(i=3; i>=0; i--)
						snprintf(bufstr+strlen(bufstr), sizeof(bufstr)-strlen(bufstr)
							, (i>0 ? "%u." : "%u"), ((char *)pIp4)[i] & 0xff
							);
					snprintf(bufstr+strlen(bufstr), sizeof(bufstr)-strlen(bufstr), ".in-addr.arpa");
				}
				break;
			case AF_INET6:
				if(inet_pton(AF_INET6, pHost, pIp6))
				{
					for(i=15; i>=0; i--)
						snprintf(bufstr+strlen(bufstr), sizeof(bufstr)-strlen(bufstr)
							, (i>0 ? "%x.%x." : "%x.%x")
							, pIp6->__u6_addr.__u6_addr8[i] & 0x0f
							, (pIp6->__u6_addr.__u6_addr8[i] & 0xf0) >> 4
							);
					snprintf(bufstr+strlen(bufstr), sizeof(bufstr)-strlen(bufstr), ".ip6.arpa");
				}
				break;
		}

		if(strlen(bufstr))
			pArpa = strdup(bufstr);
	}

	return pArpa;
}

// Show the text version of a given  RR
int dnsParseResponseCallback(nsrr_t *pNsrr, void *pdata)
{
	(void)pdata;
	int nsType = ns_rr_type(pNsrr->rr);
	switch(nsType)
	{
		case ns_t_a:
			if(ns_rr_rdlen(pNsrr->rr) == NS_INADDRSZ)
			{
				char buf[INET6_ADDRSTRLEN+1];
				memset(buf,0,sizeof(buf));
				inet_ntop(AF_INET, ns_rr_rdata(pNsrr->rr), buf, sizeof(buf)-1);
				printf("\tA %s\n",buf);
			}
			break;

		case ns_t_aaaa:
			if(ns_rr_rdlen(pNsrr->rr) == NS_IN6ADDRSZ)
			{
				char buf[INET6_ADDRSTRLEN+1];
				memset(buf,0,sizeof(buf));
				inet_ntop(AF_INET6, ns_rr_rdata(pNsrr->rr), buf, sizeof(buf)-1);

				printf("\tAAAA %s\n",buf);
			}
			break;

		case ns_t_ptr:
			{	char namebuf[NS_MAXDNAME];
				int rc;

				memset(namebuf, 0, sizeof(namebuf));

				// Once again, no resolver documentation on freebsd
				// http://docstore.mik.ua/orelly/networking_2ndEd/dns/ch15_02.htm
				// for ns_name_uncompress documentation
				// and http://www.libspf2.org/docs/html/spf__dns__resolv_8c-source.html
				// they've alread gone through this.
				rc = ns_name_uncompress(pNsrr->pResp, pNsrr->pResp + pNsrr->respLen, ns_rr_rdata(pNsrr->rr), namebuf, sizeof(namebuf));
				if(rc != -1)
					printf("\tPTR %s\n", namebuf);
			}
			break;

		default:
			printf("dnsParseResponseCallback - unhandled type %d\n", nsType);
			break;
	}

	return 1; // again
}

res_state gpRes = NULL;

void test1(int argc, char **argv)
{
	int i, rc_a, rc_aaaa, rc_ptr, rc_cn;
	u_char respA[NS_PACKETSZ];
	u_char respAAAA[NS_PACKETSZ];
	u_char respPTR[NS_PACKETSZ];
	u_char respCN[NS_PACKETSZ];

	// Query some common types of a given record name
	for(i=1; i<argc; i++)
	{
		rc_a = dns_query_rr_resp(gpRes, respA, sizeof(respA), ns_t_a, "%s", argv[i]);
		if(rc_a == -1)
			herror("a");

		rc_aaaa = dns_query_rr_resp(gpRes, respAAAA, sizeof(respAAAA), ns_t_aaaa, "%s", argv[i]);
		if(rc_aaaa == -1)
			herror("aaaa");

		rc_ptr = dns_query_rr_resp(gpRes, respPTR, sizeof(respPTR), ns_t_ptr, "%s", argv[i]);
		if(rc_ptr == -1)
			herror("ptr");

		rc_cn = dns_query_rr_resp(gpRes, respCN, sizeof(respCN), ns_t_cname, "%s", argv[i]);
		if(rc_cn == -1)
			herror("cn");

		if(rc_a == 0 && rc_aaaa == 0 && rc_ptr == 0 && rc_cn == 0)
		{	char *pstr = inet_ptoarpa(argv[i], AF_INET);

			if(pstr == NULL)
				pstr = inet_ptoarpa(argv[i], AF_INET6);

			if(pstr != NULL)
			{
				rc_ptr = dns_query_rr_resp(gpRes, respPTR, sizeof(respPTR), ns_t_ptr, "%s", pstr);
				if(rc_ptr == -1)
					herror("ptr");
				free(pstr);
			}
		}

		if(rc_a == 0 && rc_aaaa == 0 && rc_ptr == 0 && rc_cn == 0)
			printf("%s - no A, AAAA, PTR, or CNAME record\n", argv[i]);
		else
		{
			printf("%s\n", argv[i]);
			if(rc_a)
				dns_parse_response_answer(respA, rc_a, &dnsParseResponseCallback, NULL);

			if(rc_aaaa)
				dns_parse_response_answer(respAAAA, rc_aaaa, &dnsParseResponseCallback, NULL);

			if(rc_ptr)
				dns_parse_response_answer(respPTR, rc_ptr, &dnsParseResponseCallback, NULL);

			if(rc_cn)
				dns_parse_response_answer(respCN, rc_cn, &dnsParseResponseCallback, NULL);
		}
	}
}

int main(int argc, char **argv)
{
	gpRes = RES_NALLOC(gpRes);

	if(gpRes != NULL)
	{

		res_ninit(gpRes);

		test1(argc, argv);

		res_nclose(gpRes);
		free(gpRes);
	}

	return 0;
}
#endif
