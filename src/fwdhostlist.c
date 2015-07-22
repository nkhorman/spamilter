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
 *	CVSID:  $Id: fwdhostlist.c,v 1.5 2014/02/28 05:37:57 neal Exp $
 *
 * DESCRIPTION:
 *	application:	Spamilter
 *	module:		fwdhostlist.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: fwdhostlist.c,v 1.5 2014/02/28 05:37:57 neal Exp $";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>
//#include <syslog.h>

#include "spamilter.h"
#include "smtp.h"
#include "fwdhostlist.h"
#include "misc.h"

void fwdhostlist_init(mlfiPriv *priv)
{
	if(priv != NULL)
		priv->fwdhostlistfd = -1;
}

int fwdhostlist_open(mlfiPriv *priv, const char *dbpath)
{
	if(priv != NULL && dbpath != NULL)
	{	char	*fn;

		if(priv->fwdhostlistfd == -1)
		{
			asprintf(&fn,"%s/db.fwdhost",dbpath);
			priv->fwdhostlistfd = open(fn,O_RDONLY);
			if(priv->fwdhostlistfd == -1)
				mlfi_debug(priv->pSessionUuidStr,"fwdhostlist: Unable to open Sender file '%s'\n",fn);
			free(fn);
		}
	}

	return(priv != NULL && priv->fwdhostlistfd != -1);
}

void fwdhostlist_close(mlfiPriv *priv)
{
	if(priv != NULL && priv->fwdhostlistfd != -1)
	{
		close(priv->fwdhostlistfd);
		priv->fwdhostlistfd = -1;
	}
}

// Kind of like strtok, but better.
// Returns a pointer to a string delimited by "delim",
// left and right trimmed for white space, and zero
// terminated. Then pSrc will be advanced to point to
// the next string in the buffer.
static char *stradvtrim(char **pSrc, char delim)
{	char *pStr = NULL;

	if(pSrc != NULL && *pSrc != NULL && **pSrc)
	{	char *pDelim;
		char *pEnd;

		pStr = *pSrc;

		// trim left
		while(*pStr && (*pStr == ' ' || *pStr == '\t'))
			pStr++;

		// find delimiter
		pDelim = strchr(pStr,delim);
		if(pDelim != NULL)
		{
			*pDelim = '\0';
			*pSrc = pDelim + 1;
		}
		else
			*pSrc = NULL;

		// trim right
		pEnd = (pStr + strlen(pStr) - 1);
		while(pEnd >= pStr && *pEnd && (*pEnd == ' ' || *pEnd == '\t' || *pEnd == '\r' || *pEnd == '\n'))
			*(pEnd--) = '\0';
	}

	return pStr;
}

// Find the "Domain" in the db.fwdhost file, resolve
// it to an ip address, and return the address family
// type, and address to the comsumer via pAfType, and ppIn
// The consumer must free the ip address allocated in *ppIn
static int fwdhostlist_domain_addr_get(mlfiPriv *priv, const char *pDomain, char *pFwdHostBuf, size_t fwdHostBufLen, int *pAfType, char **ppIn)
{	int bFwdHostMatchFound = 0;

	if(priv->fwdhostlistfd != -1)
	{	char buf[8192];
		char *pStr;

		lseek(priv->fwdhostlistfd,0l,SEEK_SET);
		// read through the file
		while(bFwdHostMatchFound == 0 && mlfi_fdgets(priv->fwdhostlistfd,buf,sizeof(buf)) >= 0)
		{
			pStr = strchr(buf,'#');
			if(pStr != NULL)
			{
				*(pStr--) = '\0';
				while(pStr >= buf && (*pStr ==' ' || *pStr == '\t'))
					*(pStr--) = '\0';
			}
			else
				pStr = buf;

			if(strlen(pStr))
			{	char *pFwdDomain = stradvtrim(&pStr,'|');
				char *pFwdHost = stradvtrim(&pStr,'|');

				// if there is a domain match, and there is a forwading host
				if(strcasecmp(pDomain,pFwdDomain) == 0 && strlen(pFwdHost))
				{	struct hostent *phostent = gethostbyname(pFwdHost); // resolve the hostname

					if(phostent != NULL) // if the hostname was resolved
					{
						// tell the consumer what host the address is for
						strncpy(pFwdHostBuf, pFwdHost, fwdHostBufLen-1);
						pFwdHostBuf[fwdHostBufLen] = 0; // make sure it's terminated

						// allocate a network order ip address structure based on the address family type
						switch(phostent->h_addrtype)
						{
							case AF_INET: *ppIn = calloc(1,sizeof(struct in_addr)); break;
							case AF_INET6: *ppIn = calloc(1,sizeof(struct in6_addr)); break;
						}

						// copy the ip address
						if(*ppIn != NULL)
						{
							*pAfType = phostent->h_addrtype;
							memcpy(*ppIn, phostent->h_addr, phostent->h_length);
							bFwdHostMatchFound = 1;
						}
					}
					else
						mlfi_debug(priv->pSessionUuidStr, "%s: unable to resolve host '%s'", __func__, pFwdHost);
				}
			}
		}
	}

	return bFwdHostMatchFound;
}

// Test if an email address is deliverable via a forwarding host.
// This will return true, if there is no Forwarding host listed,
// for the given email address domain, or, if the forwarding host
// name was resolvable, and the host will accept delivery for the
// specified email address.
int fwdhostlist_is_deliverable(mlfiPriv *priv, char *rcpt, char *pDomain, int *pSmtprc)
{	int bDeliverable = 0;

	if(priv != NULL && pDomain != NULL && pSmtprc != NULL)
	{	char fwdHostStr[512];
		int afType = AF_UNSPEC;
		char *pIn = NULL;
		
		memset(fwdHostStr,0,sizeof(fwdHostStr));

		// Find ip address information for a matching domain
		if(fwdhostlist_domain_addr_get(priv, pDomain, fwdHostStr, sizeof(fwdHostStr), &afType, &pIn))
		{
			// if we have ip address info, test deliverability
			if(afType != AF_UNSPEC && pIn != NULL)
			{	char *pStr = mlfi_inet_ntopAF(afType, pIn); // turn the ip address into a string for display purposes
				int tst = -1;

				// show the ip address
				mlfi_debug(priv->pSessionUuidStr, "%s: '%s@%s' fwdhost: '%s'", __func__, rcpt, pDomain, fwdHostStr);
				mlfi_debug(priv->pSessionUuidStr, "\t\t%s %s\n", (afType == AF_INET ? "A" : "AAAA"), pStr);
				free(pStr);

				// connect to ip address and test devliverability
				tst = smtp_host_is_deliverable_af(priv->pSessionUuidStr, rcpt, pDomain, afType, pIn, pSmtprc);
				bDeliverable = (tst > 0 && *pSmtprc == 250); // is it deliverable ?

				// show test results
				mlfi_debug(priv->pSessionUuidStr, "\t\t%s: %d", (tst > 0 ? "Passed" : tst == 0 ? "Failed" : "Unreacable"), *pSmtprc);
			}
			else
				*pSmtprc = -1; // smtp errored, because we couldn't check it
		}
		else
			bDeliverable = 1; // say that it is deliverable, if we couldn't find a forwarding host

		// free the ip address allocated by fwdhostlist_dom_addr()
		if(pIn != NULL)
			free(pIn);
	}

	return bDeliverable;
}
