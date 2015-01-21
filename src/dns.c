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
#include <arpa/nameser.h>
#include <resolv.h>
#include <netdb.h>

#include "spamilter.h"

int dns_query_rr_aaaa_resp_v(const res_state statp, u_char *presp, size_t resplen, char *fmt, va_list vl)
{	int	haveRR = 0;
	char	*hostName = NULL;

	if(fmt != NULL && *fmt)
	{	int x = vasprintf(&hostName,fmt,vl);

		if(hostName != NULL && x > 0)
			haveRR = res_nquery(statp, hostName, ns_c_in, ns_t_aaaa, presp, resplen);

		if(hostName != NULL)
			free(hostName);
	}

	return haveRR;
}

int dns_query_rr_a_resp_v(const res_state statp, u_char *presp, size_t resplen, char *fmt, va_list vl)
{	int	haveRR = 0;
	char	*hostName = NULL;

	if(fmt != NULL && *fmt)
	{	int x = vasprintf(&hostName,fmt,vl);

		if(hostName != NULL && x > 0)
			haveRR = res_nquery(statp, hostName, ns_c_in, ns_t_a, presp, resplen);

		if(hostName != NULL)
			free(hostName);
	}

	return haveRR;
}

int dns_query_rr_a_resp(const res_state statp, u_char *presp, size_t resplen, char *fmt, ...)
{	va_list	vl;
	int rc;

	va_start(vl,fmt);
	rc = dns_query_rr_a_resp_v(statp,presp,resplen,fmt,vl);
	va_end(vl);

	return rc;
}

int dns_query_rr_aaaa(const res_state statp, char *fmt, ...)
{	va_list	vl;
	int rc;
	u_char resp[NS_PACKETSZ];

	va_start(vl,fmt);
	rc = dns_query_rr_aaaa_resp_v(statp,&resp[0],sizeof(resp),fmt,vl);
	va_end(vl);

	return rc;
}

int dns_query_rr_a(const res_state statp, char *fmt, ...)
{	va_list	vl;
	int rc;
	u_char resp[NS_PACKETSZ];

	va_start(vl,fmt);
	rc = dns_query_rr_a_resp_v(statp,&resp[0],sizeof(resp),fmt,vl);
	va_end(vl);

	return rc;
}

int dns_rdnsbl_has_rr_a(const res_state statp, unsigned long ip, char *domain)
{
	/* this is an in_addr.arpa style lookup! */
	int rc = dns_query_rr_a(statp, "%u.%u.%u.%u.%s",
			((ip&0x000000ff)), ((ip&0x0000ff00)>>8), ((ip&0x00ff0000)>>16), ((ip&0xff000000)>>24), domain
			);

	return (rc == -1 ? 0 : 1);
}

int dns_hostname_ip_parse_query(unsigned long hostip, ns_msg handle, ns_sect section)
{	int	rrnum;
	ns_rr	rr;
	int	count = ns_msg_count(handle,section);
	int	match = 0;

	for(rrnum=0; rrnum<count && !match; rrnum++)
	{
		if(ns_parserr(&handle, section, rrnum, &rr) == 0 && ns_rr_type(rr) == ns_t_a)
			match = (ns_get32(ns_rr_rdata(rr)) == hostip);
	}

	return(match);
}

int dns_hostname_ip_match(const res_state statp, char *hostname, unsigned long hostip)
{	int	match = 0;

	if(hostname != NULL && hostip != 0)
	{	ns_msg	handle;
		u_char	resp[NS_PACKETSZ];
		int	rc = dns_query_rr_a_resp(statp, &resp[0], sizeof(resp), "%s",hostname);

		if(rc > 0 && ns_initparse(resp,rc,&handle) > -1)
			match = dns_hostname_ip_parse_query(hostip,handle,ns_s_an);
	}

	return(match);
}

#ifdef UNIT_TEST
int main(int argc, char **argv)
{	int i;
	res_state pRes = RES_NALLOC(NULL);

	if(pRes != NULL)
	{
		res_ninit(pRes);

		for(i=1; i<argc; i++)
			printf("%s = %d\n",argv[i],dns_query_rr_a(pRes,"%s",argv[i]));

		res_nclose(pRes);
		free(pRes);
	}

	return 0;
}
#endif
