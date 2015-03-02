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
 *	CVSID:  $Id: dns.h,v 1.10 2012/05/04 00:14:06 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		dns.h
 *--------------------------------------------------------------------*/

#ifndef _SPAMILTER_DNS_H_
#define _SPAMILTER_DNS_H_

 	#include "config.h"

	#include <stdarg.h>

	#define mkip(a,b,c,d) ((((a)&0xff)<<24)|(((b)&0xff)<<16)|(((c)&0xff)<<8)|((d)&0xff))

	typedef struct _dqrr_t
	{
		res_state statp;
		int nsType;

		// the response buffer
		u_char *pResp;
		size_t respLen;

		// the ns parser
		ns_rr rr;
		int rrNum;
		ns_msg rrMsg;
		int rrCount;
	} dqrr_t; // Dns Query Rr Response Type

	// Create a query structure, and init with a specified type
	dqrr_t *dns_query_rr_init(const res_state statp, int nsType);
	// Reinitialize the query structure with a speciied type
	dqrr_t * dns_query_rr_reinit(dqrr_t *pDqrr, int nsType);
	// Free a query structure
	void dns_query_rr_free(dqrr_t *pDqrr);

	// Execute a query with a specified type
	int dns_query_rr_resp(dqrr_t *pDqrr, const char *pQuery);
	// Execute a vprintf style query with a specified type
	int dns_query_rr_resp_vprintf(dqrr_t *pDqrr, const char *pFmt, va_list vl);
	// Execute a printf style query with a specified type
	int dns_query_rr_resp_printf(dqrr_t *pDqrr, const char *pFmt, ...);

	// Do a query of a specified type, returning 1 if there was at least one result
	int dns_query_rr(const res_state statp, int nsType, const char *pQuery);

	// Iterate a given section of a response
	void dns_parse_response(dqrr_t *pDqrr, ns_sect nsSect, int (*pCallbackFn)(dqrr_t *, void *), void *pCallbackData);
	// Iterate the Answer Section of a response
	void dns_parse_response_answer(dqrr_t *pDqrr, int (*pCallbackFn)(dqrr_t *, void *), void *pCallbackData);

	// Query a hostname of a specified type and find a match
	int dns_hostname_ip_match_af(const res_state statp, const char *hostname, int afType, const char *in);
	int dns_hostname_ip_match_sa(const res_state statp, const char *hostname, struct sockaddr *psa);

	// Build an arpa request for an ipv4 or ipv6 address.
	// The consumer must free() the return value.
	char *dns_inet_ptoarpa(const char *pHost, int afType, const char *pDomainRoot);

#endif
