/*--------------------------------------------------------------------*
 *
 * Developed by;
 *	Neal Horman - http://www.wanlink.com
 *	Copyright (c) 2020 Neal Horman. All Rights Reserved
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
 *	CVSID:  $Id:$
 *
 * DESCRIPTION:
 *	application:	Spamilter
 *	module:		geoipApi2.h
 *--------------------------------------------------------------------*/

#ifndef _SPAMILTER_GEOIPAPI2_H_
#define _SPAMILTER_GEOIPAPI2_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct _geoipApi2Ctx_t geoipApi2Ctx_t; // opaque

// you must call geoipApi2_ResFree() when done with an intance of this
typedef struct _geopipApi2Res_t
{
	// The _ip_t structure is not filled in the geoipApi2.c module,
	// but rather in the geoip.c module in geoip_result_addAF()
	struct _ip_t
	{
		int afType;
		union
		{
			unsigned long ipv4;
			struct in6_addr ipv6;
		};
	} ip;

	char const *pCc;
	char const *pRegion;
	char const *pCity;
	char const *pPostal;
	char const *pLat;
	char const *pLong;
	char const *pCountryCity;
} geoipApi2Res_t;

geoipApi2Ctx_t * geoipApi2_open(char const *pDbFname);
geoipApi2Ctx_t *geoipApi2_close(geoipApi2Ctx_t *pCtx);

geoipApi2Res_t *geoipApi2_LookupCCByStr(geoipApi2Ctx_t *pCtx, char const *pStrIp);
geoipApi2Res_t *geoipApi2_LookupCCByHostEnt(geoipApi2Ctx_t *pCtx, const struct hostent *pHostEnt);
geoipApi2Res_t *geoipApi2_LookupCCBySA(geoipApi2Ctx_t *pCtx, const struct sockaddr *pSa);
geoipApi2Res_t *geoipApi2_LookupCCByAF(geoipApi2Ctx_t *pCtx, int afType, const char *in);

void geoipApi2_ResFree(geoipApi2Res_t *pRes);

#ifdef __cplusplus
}
#endif

#endif
