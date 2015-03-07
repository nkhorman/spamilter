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

#include "dns.h"

// Create a query structure, and init with a specified type
dqrr_t *dns_query_rr_init(const res_state statp, int nsType)
{	dqrr_t *pDqrr = calloc(1, sizeof(dqrr_t));

	if(pDqrr != NULL)
	{
		pDqrr->statp = statp;
		pDqrr->nsType = nsType;
		pDqrr->respLen = NS_PACKETSZ;
		pDqrr->pResp = calloc(1, pDqrr->respLen);
		pDqrr->tries = 2;
	}

	return pDqrr;
}

// Reinitialize the query structure with a speciied type
dqrr_t * dns_query_rr_reinit(dqrr_t *pDqrr, int nsType)
{
	if(pDqrr != NULL)
	{
		if(pDqrr->pResp != NULL)
			free(pDqrr->pResp);
		pDqrr->nsType = nsType;
		pDqrr->respLen = NS_PACKETSZ;
		pDqrr->pResp = calloc(1, pDqrr->respLen);
		pDqrr->tries = 2;
	}

	return pDqrr;
}

// Free a query structure
void dns_query_rr_free(dqrr_t *pDqrr)
{
	if(pDqrr != NULL)
	{
		if(pDqrr->pResp != NULL)
			free(pDqrr->pResp);

		free(pDqrr);
	}
}

#ifdef UNIT_TEST
typedef struct _nsType_t
{
	int type;
	const char *name;
} nsType_t;

nsType_t gNsTypes[] = { {ns_t_a, "A"}, {ns_t_aaaa, "AAAA"}, {ns_t_ptr, "PTR"}, {ns_t_cname, "CNAME"} };

const char *nsTypeLookup(int nsType)
{	int i;
	const char *pStr = NULL;

	for(i=0; pStr == NULL && i<sizeof(gNsTypes)/sizeof(gNsTypes[0]); i++)
		pStr = (gNsTypes[i].type == nsType ? gNsTypes[i].name : NULL);

	return (pStr != NULL ? pStr : "Unknown");
}
#endif

// Execute a query with a specified type
int dns_query_rr_resp(dqrr_t *pDqrr, const char *pQuery)
{	int rc = -1;

	if(pDqrr != NULL && pQuery != NULL && *pQuery)
	{	int i,tries = pDqrr->tries;
		const char *pNsTypeStr = nsTypeLookup(pDqrr->nsType);

		// try hard to have a big enough buffer for the response
		for(i=0; i<5 && pDqrr->pResp != NULL && rc < 0 && tries; i++)
		{
#ifdef UNIT_TEST
			printf("%s:%s:%d type %u/%s retrys %u %s\n", __FILE__, __func__, __LINE__, pDqrr->nsType, pNsTypeStr, tries, pQuery);
#endif
			rc = res_nquery(pDqrr->statp, pQuery, ns_c_in, pDqrr->nsType, pDqrr->pResp, pDqrr->respLen);

			if(rc > 0)
			{
				if(rc > pDqrr->respLen)
				{
					pDqrr->respLen += rc;
					pDqrr->pResp = realloc(pDqrr->pResp, pDqrr->respLen);
				}
				else
					pDqrr->respLen = rc;
			}
			else if(rc == -1)
			{
				switch(h_errno)
				{
					case HOST_NOT_FOUND: // athorative answer
					case NO_RECOVERY: // Non recoverable errors, FORMERR, REFUSED, NOTIMP
					case NO_DATA: // valid name, no data record of requested type
						// deliberate fall thru
						rc = 0;
						break;

					case NETDB_SUCCESS: // no problem - this is NXDOMAIN
					case TRY_AGAIN: // Non-Authoritative Host not found, or SERVERFAIL
						if(tries) // attempt to query again ?
						{
							tries--;
							// cause another query
							i = 0;
							rc = (tries ?  -1 : 0);
						}
						break;
				}
			}
		}
	}

	return rc;
}

// Execute a vprintf style query with a specified type
int dns_query_rr_resp_vprintf(dqrr_t *pDqrr, const char *pFmt, va_list vl)
{	int	rc = 0;

	if(pFmt != NULL && *pFmt)
	{	char *pQuery = NULL;
		int x = vasprintf(&pQuery, pFmt, vl);

		if(pQuery != NULL && x > 0)
			rc = dns_query_rr_resp(pDqrr, pQuery);

		if(pQuery != NULL)
			free(pQuery);
	}

	return rc;
}

// Execute a printf style query with a specified type
int dns_query_rr_resp_printf(dqrr_t *pDqrr, const char *pFmt, ...)
{	va_list	vl;
	int rc;

	va_start(vl, pFmt);
	rc = dns_query_rr_resp_vprintf(pDqrr, pFmt, vl);
	va_end(vl);

	return rc;
}

// Do a query of a specified type, returning 1 if there was at least one result
// The consumer doesn't care about the response content
int dns_query_rr(const res_state statp, int nsType, const char *pQuery)
{	int rc = -1;
	dqrr_t *pDqrr = dns_query_rr_init(statp, nsType);

	if(pDqrr != NULL)
	{
		rc = dns_query_rr_resp(pDqrr, pQuery);
		dns_query_rr_free(pDqrr);
	}

	return (rc > 0);
}

// Iterate a given section of a response
void dns_parse_response(dqrr_t *pDqrr, ns_sect nsSect, int (*pCallbackFn)(dqrr_t *, void *), void *pCallbackData)
{
	if(pDqrr != NULL && pDqrr->respLen && ns_initparse(pDqrr->pResp, pDqrr->respLen, &pDqrr->rrMsg) > -1)
	{	int again = 1;

		pDqrr->rrCount = ns_msg_count(pDqrr->rrMsg, nsSect);

		for(pDqrr->rrNum=0; pDqrr->rrNum<pDqrr->rrCount && again; pDqrr->rrNum++)
		{
			if(ns_parserr(&pDqrr->rrMsg, nsSect, pDqrr->rrNum, &pDqrr->rr) == 0)
				again = pCallbackFn(pDqrr, pCallbackData);
		}
	}
}

// Iterate the Answer Section of a response
void dns_parse_response_answer(dqrr_t *pDqrr, int (*pCallbackFn)(dqrr_t *, void *), void *pCallbackData)
{
	dns_parse_response(pDqrr, ns_s_an, pCallbackFn, pCallbackData);
}

typedef struct _dmi_t
{
	int nsType;
	union
	{
		struct in_addr *pIpv4;
		struct in6_addr *pIpv6;
	};
	int match;
}dmi_t; // Dns Match Ip Type

static int dnsMatchIpCallback(dqrr_t *pDqrr, void *pdata)
{	dmi_t *pDmi = (dmi_t *)pdata;

	if(ns_rr_type(pDqrr->rr) == pDmi->nsType)
	{
		switch(pDmi->nsType)
		{
			case ns_t_a:
				if(ns_rr_rdlen(pDqrr->rr) == NS_INADDRSZ)
					pDmi->match = (ns_get32(ns_rr_rdata(pDqrr->rr)) == ntohl(pDmi->pIpv4->s_addr));
				break;
			case ns_t_aaaa:
				if(ns_rr_rdlen(pDqrr->rr) == NS_IN6ADDRSZ)
				{	struct in6_addr *pIp6 = (struct in6_addr *)ns_rr_rdata(pDqrr->rr);

					pDmi->match = (memcmp(pIp6, pDmi->pIpv6, sizeof(struct in6_addr)) == 0);
				}
				break;
		}
	}

	return (pDmi->match == 0); // again ?
}

// Query a hostname of a specified type and find a match
int dns_hostname_ip_match_af(const res_state statp, const char *hostname, int afType, const char *in)
{	dmi_t dmi;

	memset(&dmi, 0, sizeof(dmi));
	switch(afType)
	{
		case AF_INET:	dmi.nsType = ns_t_a;	dmi.pIpv4 = (struct in_addr *)in;	break;
		case AF_INET6:	dmi.nsType = ns_t_aaaa;	dmi.pIpv6 = (struct in6_addr *)in;	break;
	}

	if(hostname != NULL)
	{	dqrr_t *pDqrr = dns_query_rr_init(statp, dmi.nsType);
		int rc = dns_query_rr_resp(pDqrr, hostname);

		if(rc > 0)
			dns_parse_response_answer(pDqrr, &dnsMatchIpCallback, &dmi);

		dns_query_rr_free(pDqrr);
	}

	return dmi.match;
}

// Query a hostname of a specified type and find a match
int dns_hostname_ip_match_sa(const res_state statp, const char *hostname, struct sockaddr *psa)
{	const char *in = NULL;

	switch(psa->sa_family)
	{
		case AF_INET: in = (char *) &((struct sockaddr_in *)psa)->sin_addr; break;
		case AF_INET6: in = (const char *)&((struct sockaddr_in6 *)psa)->sin6_addr; break;
	}

	return (in != NULL ? dns_hostname_ip_match_af(statp, hostname, psa->sa_family, in) : 0);
}

// Build an arpa request for an ipv4 or ipv6 address.
// The consumer must free() the return value.
char *dns_inet_ptoarpa(const char *pHost, int afType, const char *pDomainRoot)
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

		switch(afType)
		{
			case AF_INET:
				if(inet_pton(AF_INET, pHost, pIp4))
				{
					for(i=3; i>=0; i--)
						snprintf(bufstr+strlen(bufstr), sizeof(bufstr)-strlen(bufstr)
							, (i>0 ? "%u." : "%u"), ((char *)pIp4)[i] & 0xff
							);
					snprintf(bufstr+strlen(bufstr), sizeof(bufstr)-strlen(bufstr), ".%s"
						, (pDomainRoot == NULL ? "in-addr.arpa" : pDomainRoot)
						);
				}
				break;
			case AF_INET6:
				if(inet_pton(AF_INET6, pHost, pIp6))
				{
					for(i=15; i>=0; i--)
						snprintf(bufstr+strlen(bufstr), sizeof(bufstr)-strlen(bufstr)
							, (i>0 ? "%x.%x." : "%x.%x")
							, pIp6->s6_addr[i] & 0x0f
							, (pIp6->s6_addr[i] & 0xf0) >> 4
							);
					snprintf(bufstr+strlen(bufstr), sizeof(bufstr)-strlen(bufstr), ".%s"
						, (pDomainRoot == NULL ? "ip6.arpa" : pDomainRoot)
						);
				}
				break;
		}

		if(strlen(bufstr))
			pArpa = strdup(bufstr);
	}

	return pArpa;
}

#ifdef UNIT_TEST
// Show the text version of a given  RR
int dnsParseResponseCallback(dqrr_t *pDqrr, void *pdata)
{
	(void)pdata;
	int nsType = ns_rr_type(pDqrr->rr);

	switch(nsType)
	{
		case ns_t_a:
			if(ns_rr_rdlen(pDqrr->rr) == NS_INADDRSZ)
			{
				char buf[INET6_ADDRSTRLEN+1];
				memset(buf,0,sizeof(buf));
				inet_ntop(AF_INET, ns_rr_rdata(pDqrr->rr), buf, sizeof(buf)-1);
				printf("\tA %s\n",buf);
			}
			break;

		case ns_t_aaaa:
			if(ns_rr_rdlen(pDqrr->rr) == NS_IN6ADDRSZ)
			{
				char buf[INET6_ADDRSTRLEN+1];
				memset(buf,0,sizeof(buf));
				inet_ntop(AF_INET6, ns_rr_rdata(pDqrr->rr), buf, sizeof(buf)-1);

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
				rc = ns_name_uncompress(pDqrr->pResp, pDqrr->pResp + pDqrr->respLen, ns_rr_rdata(pDqrr->rr), namebuf, sizeof(namebuf));
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

int testType(nsType_t *pType, const char *pQuery)
{	int rc = 0;
	dqrr_t *pDqrr = dns_query_rr_init(gpRes, pType->type);

	if(pDqrr != NULL)
	{
		rc = dns_query_rr_resp(pDqrr, pQuery);
		if(rc > 0)
			dns_parse_response_answer(pDqrr, &dnsParseResponseCallback, NULL);
		else if(rc == -1)
			herror(pType->name);
		dns_query_rr_free(pDqrr);
	}

	return (rc > 0);
}

void test1(int argc, char **argv)
{	int i,j,rc;

	// Query some common types of a given record name
	for(i=1; i<argc; i++)
	{
		printf("%s\n", argv[i]);

		for(j=0,rc=0; j<sizeof(gNsTypes)/sizeof(gNsTypes[0]); j++)
			rc += testType(&gNsTypes[j], argv[i]);

		//if(rc == 0)
		{	char *pstr = dns_inet_ptoarpa(argv[i], AF_INET, NULL);

			if(pstr == NULL)
				pstr = dns_inet_ptoarpa(argv[i], AF_INET6, NULL);

			if(pstr != NULL)
			{
				rc += testType(&gNsTypes[2], pstr);
				free(pstr);
			}
		}

		if(rc == 0)
			printf("%s - no A, AAAA, PTR, or CNAME record\n", argv[i]);
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
