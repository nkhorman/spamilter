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

	#include "GeoIP.h"
	#include "GeoIPCity.h"

	enum { GEOIPLIST_A_NULL, GEOIPLIST_A_ACCEPT, GEOIPLIST_A_REJECT, GEOIPLIST_A_DISCARD, GEOIPLIST_A_TEMPFAIL, GEOIPLIST_A_TARPIT/*, GEOIPLIST_A_IPFW*/ };

	typedef struct _geoipresult_t
	{
		unsigned long ip;
		const char *pCountryCode;
		char *pCountryCity;
		GeoIPRecord *pGeoIPRecord;
	}geoipResult;

	void geoip_init(SMFICTX *ctx);
	int geoip_open(SMFICTX *ctx, const char *dbpath, const char *pGeoipdbPath);
	void geoip_close(SMFICTX *ctx);
	int geoip_query_action(SMFICTX *ctx, unsigned long ip);

	const char *geoip_LookupCCByIp(SMFICTX *ctx, unsigned long ipNum);
	const char *geoip_LookupCCByHost(SMFICTX *ctx, const char *pHostName);

	const char *geoip_result_add(SMFICTX *ctx, unsigned long ip, const char *pCountryCode);
#endif
