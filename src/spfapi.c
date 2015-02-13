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
 *	CVSID:  $Id: spfapi.c,v 1.9 2012/12/07 19:37:16 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		spfapi.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: spfapi.c,v 1.9 2012/12/07 19:37:16 neal Exp $";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "spamilter.h"
#include "spfapi.h"
#include "misc.h"

#ifdef SUPPORT_LIBSPF2
#include "libspf/build2/config.h"
extern void (*SPF_error_handler)( const char *, int, const char * ) __attribute__ ((noreturn));
extern void (*SPF_warning_handler)( const char *, int, const char * );
extern void (*SPF_info_handler)( const char *, int, const char * );
extern void (*SPF_debug_handler)( const char *, int, const char * );
#endif
#include "spf.h"

#ifdef SUPPORT_LIBSPF
static char *trim_right(char *src)
{	char *str = (src != NULL && *src ? src+strlen(src)-1 : NULL);

	while(str != NULL && str >= src && (*str == '\r' || *str == '\n'))
		*(str--)='\0';

	return(src);
}

static char *printfEscapeAndTrim(char *pSrc)
{	size_t l = (pSrc != NULL ? strlen(pSrc) : 0);
	char *pDst = (l > 0 ? calloc(1,++l) : NULL);

	if(pDst != NULL)
	{	char *pTmp = pDst;

		while(*pSrc)
		{
			switch(*pSrc)
			{
				case '\r':
				case '\n':
					break;
				case '%':
					l ++;
					pDst = realloc(pDst,l);
					*(pTmp++) = *pSrc;
					// deliberate fall thru
				default:
					*(pTmp++) = *pSrc;
					break;
			}
			pSrc++;
		}
	}

	return pDst;//trim_right(pDst);
}
#endif

sfsistat mlfi_spf_reject(mlfiPriv *priv, sfsistat *rs)
{
	if (priv != NULL)
	{
#ifdef SUPPORT_LIBSPF
		confg.level = gDebug;

		peer_info_t *spf_info = SPF_init(gHostname, priv->ipstr, NULL, NULL, NULL, 0, 0);
	
		if(spf_info != NULL)
		{	char *pSpfRs = NULL;
			char *pSpfEr = NULL;

			SPF_smtp_helo(spf_info, priv->helo);
			SPF_smtp_from(spf_info, priv->sndr);

			spf_info->RES = SPF_policy_main(spf_info);
			pSpfRs = printfEscapeAndTrim(spf_info->rs);
			pSpfEr = printfEscapeAndTrim(spf_info->error);

			mlfi_debug(priv->pSessionUuidStr,"spf: %s\n",pSpfEr);

			switch(spf_info->RES)
			{
				case SPF_PASS:		priv->spf_rc = SSPF_PASS;	break;
				case SPF_NONE:		priv->spf_rc = SSPF_NONE;	break;
				case SPF_S_FAIL:	priv->spf_rc = SSPF_S_FAIL;	break;
				case SPF_ERROR:		priv->spf_rc = SSPF_ERROR;	break;
				case SPF_NEUTRAL:	priv->spf_rc = SSPF_NEUTRAL;	break;
				case SPF_UNKNOWN:	priv->spf_rc = SSPF_UNKNOWN;	break;
				case SPF_UNMECH:	priv->spf_rc = SSPF_UNMECH;	break;
				case SPF_H_FAIL:	priv->spf_rc = SSPF_H_FAIL;	break;
			}

			priv->spf_rs		= pSpfRs;
			priv->spf_error		= pSpfEr;
			priv->spf_explain	= trim_right(SPF_get_explain(spf_info));

			SPF_close(spf_info);
		}
#endif
#ifdef SUPPORT_LIBSPF2
		SPF_server_t *spf_server = SPF_server_new(SPF_DNS_CACHE, gDebug != 0);
		SPF_request_t *spf_request = (spf_server != NULL ? SPF_request_new(spf_server) : NULL);
		SPF_response_t *spf_response = NULL;

		// disable debug output routines from the libspf2 library
		if(gDebug == 0)
		{
			SPF_error_handler = NULL;
			SPF_warning_handler = NULL;
			SPF_info_handler = NULL;
			SPF_debug_handler = NULL;
		}

		if(spf_request != NULL)
		{
			if(
				(
					(1/*ipv4*/ && SPF_request_set_ipv4_str(spf_request, priv->ipstr ) == 0)
					||(0/*ipv6*/ && SPF_request_set_ipv6_str(spf_request, priv->ipstr ) == 0)
				)
				&& SPF_request_set_helo_dom( spf_request, priv->helo ) == 0
				&& SPF_request_set_env_from( spf_request, priv->sndr ) == 0
			)
			{
				SPF_request_query_mailfrom(spf_request, &spf_response);

				if(spf_response != NULL)
				{
					switch(SPF_response_result(spf_response))
					{
						case SPF_RESULT_NEUTRAL:	priv->spf_rc = SSPF_NEUTRAL;	break;
						case SPF_RESULT_PASS:		priv->spf_rc = SSPF_PASS;	break;
						case SPF_RESULT_FAIL:		priv->spf_rc = SSPF_H_FAIL;	break;
						case SPF_RESULT_SOFTFAIL:	priv->spf_rc = SSPF_S_FAIL;	break;
						case SPF_RESULT_NONE:		priv->spf_rc = SSPF_NONE;	break;
						case SPF_RESULT_TEMPERROR:	priv->spf_rc = SSPF_TEMPERROR;	break;
						case SPF_RESULT_PERMERROR:	priv->spf_rc = SSPF_UNKNOWN;	break;
						case SPF_RESULT_INVALID:	break;
					}

#define VALID_STR(x) (x ? x : "")
					priv->spf_rs = strdup( SPF_strresult( SPF_response_result(spf_response)));
					asprintf(&priv->spf_error
						, "policy result: %s (%s)"
						, priv->spf_rs
						, VALID_STR(SPF_response_get_header_comment(spf_response))
						);
					priv->spf_explain = strdup( VALID_STR(SPF_response_get_smtp_comment(spf_response)));

					SPF_response_free(spf_response);
				}
			}

			SPF_request_free(spf_request);
		}
		if(spf_server != NULL)
			SPF_server_free(spf_server);
#endif

		// reject policy
		switch(priv->spf_rc)
		{
			case SSPF_S_FAIL:
				if(!gMtaSpfChkSoftFailAsFail) // if soft fail != reject
					break;
			case SSPF_H_FAIL:
				*rs = SMFIS_REJECT;
				break;
			case SSPF_TEMPERROR:
				*rs = SMFIS_TEMPFAIL;
				break;
		}
	}

	return(*rs);
}
