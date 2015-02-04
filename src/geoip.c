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
 *	CVSID$
 *
 * DESCRIPTION:
 *	application:	Spamilter
 *	module:		geoip.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: geoip.c,v 1.12 2012/11/18 21:13:16 neal Exp $";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "spamilter.h"
#include "misc.h"
#include "regexapi.h"
#include "list.h"
#include "geoip.h"
#include "GeoIP.h"
#include "GeoIP_internal.h"

void geoip_init(SMFICTX *ctx)
{	mlfiPriv *priv = MLFIPRIV;

	if(priv != NULL)
	{
		priv->pGeoipdb = NULL;
		priv->pGeoipCC = NULL;
		priv->pGeoipCity = NULL;
		priv->fdGeoipBWL = -1;
		priv->pGeoipList = listCreate();
	}
}

int geoip_open(SMFICTX *ctx, const char *dbpath, const char *pGeoipdbPath)
{	mlfiPriv *priv = MLFIPRIV;
	int ok = 0;

	if(priv != NULL && dbpath != NULL)
	{
		priv->pGeoipdb = GeoIP_setup_custom_directory(pGeoipdbPath);
		priv->pGeoipCC = ( GeoIP_db_avail(priv->pGeoipdb,GEOIP_COUNTRY_EDITION) ? GeoIP_open_type(priv->pGeoipdb,GEOIP_COUNTRY_EDITION,GEOIP_STANDARD) : NULL);
		priv->pGeoipCity = ( GeoIP_db_avail(priv->pGeoipdb,GEOIP_CITY_EDITION_REV0) ? GeoIP_open_type(priv->pGeoipdb,GEOIP_CITY_EDITION_REV0 ,GEOIP_STANDARD) : NULL);

		if(priv->fdGeoipBWL == -1)
		{	char	*fn = NULL;

			asprintf(&fn,"%s/db.geocc",dbpath);
			priv->fdGeoipBWL = open(fn,O_RDONLY);
			if(priv->fdGeoipBWL == -1)
				mlfi_debug(priv->pSessionUuidStr,"geoip: Unable to open Sender file '%s'\n",fn);
			if(fn != NULL)
				free(fn);
		}

		ok = (priv->pGeoipdb != NULL && priv->pGeoipCC != NULL && priv->fdGeoipBWL != -1);
	}

	mlfi_debug(priv->pSessionUuidStr,"geoip_open: ok %d\n",ok);

	return ok;
}

static int geoipResultFreeCallback(void *pData, void *pCallbackData)
{	geoipResult *pResult = (geoipResult *)pData;

	if(pResult->pGeoIPRecord != NULL)
		GeoIPRecord_delete(pResult->pGeoIPRecord);
	if(pResult->pCountryCity != NULL)
		free(pResult->pCountryCity);

	free(pResult);

	return 1; // again
}

void geoip_close(SMFICTX *ctx)
{	mlfiPriv *priv = MLFIPRIV;

	if(priv != NULL)
	{
		GeoIP_delete(priv->pGeoipCC);
		GeoIP_delete(priv->pGeoipCity);
		priv->pGeoipCC = NULL;
		priv->pGeoipCity = NULL;
		GeoIP_cleanup(&priv->pGeoipdb);

		if(priv->fdGeoipBWL != -1)
		{
			close(priv->fdGeoipBWL);
			priv->fdGeoipBWL = -1;
		}

		listDestroy(priv->pListBodyHosts,&geoipResultFreeCallback,NULL);
		priv->pGeoipList = NULL;
	}
}

// TODO - ipv6 - to be removed
const char *geoip_LookupCCByIpv4(SMFICTX *ctx, unsigned long ip)
{	mlfiPriv *priv = MLFIPRIV;
	const char *pCC = NULL;

	if(priv != NULL && priv->pGeoipCC != NULL)
		pCC = GeoIP_country_code_by_ipnum(priv->pGeoipCC, ip);

	return (pCC != NULL ? pCC : "--");
}

const char *geoip_LookupCCByIp(SMFICTX *ctx, const struct sockaddr *pip)
{	mlfiPriv *priv = MLFIPRIV;
	const char *pCC = NULL;

	if(priv != NULL && priv->pGeoipCC != NULL)
	{
		switch(pip->sa_family)
		{
			case AF_INET:
				pCC = GeoIP_country_code_by_ipnum(priv->pGeoipCC, ntohl(((struct sockaddr_in *)pip)->sin_addr.s_addr));
				break;
			case AF_INET6:
				// TODO - ipv6 - finish
				//pCC = GeoIP_country_code_by_ipnum_v6(priv->pGeoipCC, ipNum);
				break;
		}
	}

	return (pCC != NULL ? pCC : "--");
}

const char *geoip_LookupCCByHost(SMFICTX *ctx, const char *pHostName)
{	
	// TODO - ipv6
	return geoip_LookupCCByIpv4(ctx, (pHostName != NULL && *pHostName ? _GeoIP_lookupaddress(pHostName) : 0) );
}

static char *gpStrsBwlA[] =
{
	"None",
	"Accept",
	"Reject",
	"Discard",
	"TempFail",
	"TarPit",
	"Exec"
};

// This will return the action for the last match, not the first
// Note: The list should be in least to most specific match order
// otherwise, you might not get the results that you expect!
static int geoip_get_action(mlfiPriv *priv, const char *pCC)
{	int	rc = GEOIPLIST_A_NULL;
	char	buf[8192];
	char	acc[256],aaction[50];
	int	action;
	char	*str =NULL;
	int	fd = priv->fdGeoipBWL;

	if(fd != -1)
	{
		lseek(fd,0l,SEEK_SET);
		while(mlfi_fdgets(fd,buf,sizeof(buf)) >= 0)
		{
			// strip trailing comments and then white space
			str = strchr(buf,'#');
			if(str != NULL)
			{
				*(str--) = '\0';
				while(str >= buf && (*str ==' ' || *str == '\t'))
					*(str--) = '\0';
			}

			if(strlen(buf))
			{
				memset(acc,0,sizeof(acc));
				memset(aaction,0,sizeof(aaction));
			
				str = mlfi_strcpyadv(acc,sizeof(acc),buf,'|');
				str = mlfi_strcpyadv(aaction,sizeof(aaction),str,'|');

				action = (strcasecmp(aaction,"accept") == 0	? GEOIPLIST_A_ACCEPT :
					strcasecmp(aaction,"reject") == 0	? GEOIPLIST_A_REJECT :
					strcasecmp(aaction,"discard") == 0	? GEOIPLIST_A_DISCARD :
					strcasecmp(aaction,"fail") == 0		? GEOIPLIST_A_TEMPFAIL :
					strcasecmp(aaction,"tarpit") == 0	? GEOIPLIST_A_TARPIT :
					//strcasecmp(aaction,"ipfw") == 0		? GEOIPLIST_A_IPFW :
					GEOIPLIST_A_NULL);

				// use regex to do matching instead of strcasecmp
				if(regexapi(pCC,acc,REGEX_DEFAULT_CFLAGS))
					rc = action;
			}
		}
	}

	mlfi_debug(priv->pSessionUuidStr,"geoip_get_action: CC: %s - action %s\n",pCC,gpStrsBwlA[rc]);

	return rc;
}

// TODO - ipv6
int geoip_query_action(SMFICTX *ctx, unsigned long ip)
{	mlfiPriv *priv = MLFIPRIV;
	int	rc = GEOIPLIST_A_NULL;

	if(priv != NULL)
		rc = geoip_get_action(priv,geoip_LookupCCByIpv4(ctx,ip));

	return(rc);
}

static const char * _mk_NA( const char * p ) { return p ? p : "N/A"; }

const char *geoip_result_add(SMFICTX *ctx, unsigned long ip, const char *pCountryCode)
{	mlfiPriv *priv = MLFIPRIV;
	const char *pStr = NULL;

	if(priv->pGeoipList != NULL)
	{	geoipResult *pResult = calloc(1,sizeof(geoipResult));

		if(pResult != NULL)
		{
			pResult->ip = ip;
			pResult->pCountryCode = pCountryCode;
			pResult->pGeoIPRecord = (*pCountryCode != '-' && priv->pGeoipCity != NULL ? GeoIP_record_by_ipnum(priv->pGeoipCity, ip) : NULL);
			pResult->pCountryCity = NULL;
			if(pResult->pGeoIPRecord != NULL
				// matching CC ?
				&& strcmp(pResult->pCountryCode,pResult->pGeoIPRecord->country_code) == 0
			)
			{	GeoIPRecord *gir = pResult->pGeoIPRecord;

				asprintf(
					&pResult->pCountryCity
					,"%s, %s, %s, %s, %f, %f"
					,gir->country_code ,_mk_NA(gir->region) ,_mk_NA(gir->city), _mk_NA(gir->postal_code), gir->latitude, gir->longitude
					);
				pStr = pResult->pCountryCity;
			}
			else
				pStr = pCountryCode;

			listAdd(priv->pGeoipList,pResult);
		}
	}

	return pStr;
}
