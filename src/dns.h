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

	int dns_query_rr_a_resp_v(const res_state statp, u_char *presp, size_t resplen, char *fmt, va_list vl);
	int dns_query_rr_a_resp(const res_state statp, u_char *presp, size_t resplen, char *fmt, ...);
	int dns_query_rr_a(const res_state statp, char *fmt, ...);

	int dns_query_rr_aaaa_resp_v(const res_state statp, u_char *presp, size_t resplen, char *fmt, va_list vl);
	int dns_query_rr_aaaa(const res_state statp, char *fmt, ...);

	int dns_rdnsbl_has_rr_a(const res_state statp, unsigned long ip, char *domain);

	int dns_hostname_ip_match(const res_state statp, char *hostname, unsigned long hostip);
#endif
