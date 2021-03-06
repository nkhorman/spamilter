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
 *	CVSID:  $Id: geoip.h,v 1.3 2012/05/03 19:44:22 neal Exp $
 *
 * DESCRIPTION:
 *	application:	Spamilter
 *	module:		geoip.h
 *--------------------------------------------------------------------*/

#ifndef _SPAMILTER_GEOIP_H_
#define _SPAMILTER_GEOIP_H_

#include "geoipApi2.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


	enum { GEOIPLIST_A_NULL, GEOIPLIST_A_ACCEPT, GEOIPLIST_A_REJECT, GEOIPLIST_A_DISCARD, GEOIPLIST_A_TEMPFAIL, GEOIPLIST_A_TARPIT/*, GEOIPLIST_A_IPFW*/ };


	void geoip_init(SMFICTX *ctx);
	int geoip_open(SMFICTX *ctx, const char *dbpath, const char *pGeoipdbPath);
	void geoip_close(SMFICTX *ctx);

	int geoip_query_action_cc(SMFICTX *ctx, const char *pCC);

	const char *geoip_LookupCCByAF(SMFICTX *ctx, int af, const char *in);
	const char *geoip_LookupCCByHostEnt(SMFICTX *ctx, const struct hostent *pHostEnt);
	const char *geoip_LookupCCByHostName(SMFICTX *ctx, const char *pHostName);
	const char *geoip_LookupCCBySA(SMFICTX *ctx, const struct sockaddr *pip);

	const char *geoip_result_addIpv4(SMFICTX *ctx, unsigned long ip, const char *pCountryCode);
	const char *geoip_result_addSA(SMFICTX *ctx, struct sockaddr *psa, const char *pCountryCode);
	const char *geoip_result_addAF(SMFICTX *ctx, int afType, const char *in, const char *pCountryCode);
#endif
