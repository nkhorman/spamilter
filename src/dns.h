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

	// primatives
	// The consumer provides the response buffer
	int dns_query_rr_resp_v(const res_state statp, u_char *presp, size_t respLen, int nsType, char *fmt, va_list vl);
	// The consumer provides the response buffer, because they want to parse it
	int dns_query_rr_resp(const res_state statp, u_char *presp, size_t respLen, int nsType, char *fmt, ...);
	// The consumer doesn't care about the response content
	int dns_query_rr(const res_state statp, int nsType, char *fmt, ...);

	// Make an in-addr.arpa style ipv4 query, and return 1 if found
	int dns_rdnsbl_has_rr_a(const res_state statp, unsigned long ip, char *domain);

	// Query a hostname and find an ipv4 match
	int dns_hostname_ip_match(const res_state statp, char *hostname, unsigned long hostip);

	typedef struct _nsrrr_t
	{
		u_char *pResp;
		size_t respLen;
		ns_rr rr;
	} nsrr_t;

	// Iterate a given section of a response
	void dns_parse_response(u_char *presp, size_t respLen, ns_sect nsSection, int (*pCallbackFn)(nsrr_t *, void *), void *pCallbackData);
	// Iterate the Answer Section of a response
	void dns_parse_response_answer(u_char *presp, size_t respLen, int (*pCallbackFn)(nsrr_t *, void *), void *pCallbackData);

	// Build an arpa request for an ipv4 or ipv6 address
	// The consumer must free() the return value.
	char *dns_inet_ptoarpa(const char *pHost, int afNet);
#endif
