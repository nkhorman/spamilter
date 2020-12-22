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
 *	module:		geoipApi2.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: geoipApi2.c,v 1.12 2012/11/18 21:13:16 neal Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "geoipApi2.h"

#include "maxminddb.h"

typedef struct _geoipApi2Ctx_t
{
	MMDB_s mmdb;
	int dbOpen;
} geoipApi2Ctx_t;

// Create a context and open the DB
geoipApi2Ctx_t *geoipApi2_open(char const *pDbPath)
{
	geoipApi2Ctx_t *pCtx = NULL;
	
	if(pDbPath != NULL && *pDbPath)
	{
		char *pFname = NULL;

		asprintf(&pFname, "%s/GeoLite2-City.mmdb", pDbPath);
		if(pFname != NULL && *pFname)
		{
			pCtx = (geoipApi2Ctx_t *)calloc(1, sizeof(geoipApi2Ctx_t));
	
			if(pCtx != NULL)
				pCtx->dbOpen = (MMDB_open(pFname, MMDB_MODE_MMAP, &pCtx->mmdb) == MMDB_SUCCESS);

			free(pFname);
		}
	}

	return pCtx;
}

// Close the DB if open, and free context structure
geoipApi2Ctx_t *geoipApi2_close(geoipApi2Ctx_t *pCtx)
{
	if(pCtx != NULL)
	{
		if(pCtx->dbOpen)
				MMDB_close(&pCtx->mmdb);
		free(pCtx);
		pCtx = NULL;
	}

	return pCtx;
}

// The caller/consumer must free() returned string pointer when done
static char *MMDB_get_string(MMDB_lookup_result_s *result, ...)
{
	char *pStr = NULL;
	MMDB_entry_data_s entry_data;
	va_list vl;

	va_start(vl, result);
	if(MMDB_SUCCESS == MMDB_vget_value(&result->entry, &entry_data, vl))
	{
		switch(entry_data.type)
		{
			case  MMDB_DATA_TYPE_UTF8_STRING:
				pStr = strndup((char *)entry_data.utf8_string,  entry_data.data_size);
				break;
			case MMDB_DATA_TYPE_DOUBLE:
				asprintf(&pStr, "%f", entry_data.double_value);
				break;
		}
	}
	va_end(vl);

	return pStr;
}

static const char * _mk_NA( const char * p ) { return (p != NULL ? p : "N/A"); }

// The caller/consumer must free() returned string pointers when done by calling geoipApi2_ResFree()
static geoipApi2Res_t *geoipApi2_ResBuild(MMDB_lookup_result_s *pMmdbResult)
{
	geoipApi2Res_t *pRes = calloc(1, sizeof(geoipApi2Res_t));

	if(pMmdbResult != NULL && pMmdbResult->found_entry && pRes != NULL)
	{
		pRes->pCc		= MMDB_get_string(pMmdbResult, "country", "iso_code", NULL);
		pRes->pRegion	= MMDB_get_string(pMmdbResult, "subdivisions", "0", "iso_code", NULL);
		pRes->pCity		= MMDB_get_string(pMmdbResult, "city", "names", "en", NULL);
		pRes->pPostal	= MMDB_get_string(pMmdbResult, "postal", "code", NULL);
		pRes->pLat		= MMDB_get_string(pMmdbResult, "location", "latitude", NULL);
		pRes->pLong		= MMDB_get_string(pMmdbResult, "location", "longitude", NULL);

		asprintf(
			(char **)&pRes->pCountryCity
			, "%s, %s, %s, %s, %s, %s"
			, pRes->pCc
			, _mk_NA(pRes->pRegion)
			, _mk_NA(pRes->pCity)
			, _mk_NA(pRes->pPostal)
			, _mk_NA(pRes->pLat)
			, _mk_NA(pRes->pLong)
			);
	}

	if(pRes != NULL && pRes->pCc == NULL) // no result ?
		pRes->pCc = strdup("--"); // default "I dunn'o"

	return pRes;
}

// This routine calls MMDB_get_string(), which means...
// The caller/consumer must free() returned string pointers when done by calling geoipApi2_ResFree()
geoipApi2Res_t *geoipApi2_LookupCCByStr(geoipApi2Ctx_t *pCtx, char const *pStrIp)
{
	geoipApi2Res_t *pRes = NULL; // pre-set "no result"

	if(pCtx != NULL && pStrIp != NULL && *pStrIp && pCtx->dbOpen)
	{
		int gai_error;
		int mmdbError;
		MMDB_lookup_result_s mmdbResult =
			MMDB_lookup_string(&pCtx->mmdb, pStrIp, &gai_error, &mmdbError);

		if(gai_error == 0 && mmdbError == MMDB_SUCCESS)
			pRes = geoipApi2_ResBuild(&mmdbResult);
	}

	// if "no result", give back default "I dunn'o"
	return (pRes != NULL ? pRes : geoipApi2_ResBuild(NULL));
}

// This routine calls MMDB_get_string(), which means...
// The caller/consumer must free() returned string pointers when done by calling geoipApi2_ResFree()
geoipApi2Res_t *geoipApi2_LookupCCBySA(geoipApi2Ctx_t *pCtx, const struct sockaddr *pSa)
{
	geoipApi2Res_t *pRes = NULL; // pre-set "no result"

	if(pCtx != NULL && pSa != NULL && pCtx->dbOpen)
	{
		int mmdbError;
		MMDB_lookup_result_s mmdbResult;

		switch(pSa->sa_family)
		{
			case AF_INET:
			case AF_INET6:
				mmdbResult = MMDB_lookup_sockaddr(&pCtx->mmdb, pSa, &mmdbError);
				if(mmdbError == MMDB_SUCCESS)
					pRes = geoipApi2_ResBuild(&mmdbResult);
				break;
		}
	}

	// if "no result", give back default "I dunn'o"
	return (pRes != NULL ? pRes : geoipApi2_ResBuild(NULL));
}

// Construct a sockaddr_in or sockaddr_in6 and handoff to geoipApi2_LookupCCBySA()
geoipApi2Res_t *geoipApi2_LookupCCByAF(geoipApi2Ctx_t *pCtx, int afType, const char *in)
{
	geoipApi2Res_t *pRes = NULL; // pre-set "no result"

	if(pCtx != NULL && in != NULL)
	{
		struct sockaddr_in sa4 = {0};
		struct sockaddr_in6 sa6 = {0};

		switch(afType)
		{
			case AF_INET:
				sa4.sin_family = AF_INET;
				sa4.sin_len = sizeof(struct sockaddr_in);
				sa4.sin_addr = *(struct in_addr *)in;
				pRes = geoipApi2_LookupCCBySA(pCtx, (const struct sockaddr *)&sa4);
				break;
			case AF_INET6:
				sa6.sin6_family = AF_INET6;
				sa6.sin6_len = sizeof(struct sockaddr_in6);
				sa6.sin6_addr = *(struct in6_addr *)in;
				pRes = geoipApi2_LookupCCBySA(pCtx, (const struct sockaddr *)&sa6);
				break;
		}
	}

	// if "no result", give back default "I dunn'o"
	return (pRes != NULL ? pRes : geoipApi2_ResBuild(NULL));
}

// Construct a sockaddr_in or sockaddr_in6 and handoff to geoipApi2_LookupCCBySA()
// The caller/consumer must free() returned string pointers when done by calling geoipApi2_ResFree()
geoipApi2Res_t *geoipApi2_LookupCCByHostEnt(geoipApi2Ctx_t *pCtx, const struct hostent *pHostEnt)
{
	geoipApi2Res_t *pRes = NULL; // pre-set "no result"

	if(pHostEnt != NULL)
	{
		switch(pHostEnt->h_addrtype)
		{
			case AF_INET:
			case AF_INET6:
				pRes = geoipApi2_LookupCCByAF(pCtx, pHostEnt->h_addrtype, (char *)(pHostEnt->h_addr));
				break;
		}
	}

	// if "no result", give back default "I dunn'o"
	return (pRes != NULL ? pRes : geoipApi2_ResBuild(NULL));
}

#define FREESAFE(p) { if(p != NULL) { free((void *)p); p = NULL; } }

void geoipApi2_ResFree(geoipApi2Res_t *pRes)
{
	if(pRes != NULL)
	{
		FREESAFE(pRes->pCc);
		FREESAFE(pRes->pRegion);
		FREESAFE(pRes->pCity);
		FREESAFE(pRes->pPostal);
		FREESAFE(pRes->pLat);
		FREESAFE(pRes->pLong);
		FREESAFE(pRes->pCountryCity);
		FREESAFE(pRes);
	}
}
