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
 *	CVSID:  $Id: dnsbl.c,v 1.39 2012/12/14 04:59:33 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		dnsbl.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: dnsbl.c,v 1.39 2012/12/14 04:59:33 neal Exp $";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "spamilter.h"
#include "dns.h"
#include "dnsbl.h"
#include "misc.h"
#include "smisc.h"
#include "inet.h"

char *dnsbl_str_stage[] = {"None","Conn","From","Rcpt","Hdr","Eom"};

void dnsbl_free_hosts(RBLLISTHOSTS *prlh)
{
	if(prlh != NULL)
	{
		if(prlh->plist != NULL)
			free(prlh->plist);
		free(prlh);
	}
}

void dnsbl_free_match(RBLLISTMATCH *pmatch)
{
	if(pmatch != NULL)
	{
		if(pmatch->ppmatch != NULL)
			free(pmatch->ppmatch);
		free(pmatch);
	}
}

// Open the RBL data base, parse, and build a structure array of RBLs
RBLLISTHOSTS *dnsbl_create(const char *pSessionId, char *dbpath)
{	RBLLISTHOSTS *prlh = calloc(1,sizeof(RBLLISTHOSTS));

	if(dbpath != NULL && *dbpath)
	{	char *str = NULL;
		int fd;

		asprintf(&str,"%s/db.rdnsbl",dbpath);
		fd = open(str,O_RDONLY);
		if(fd == -1)
			mlfi_debug(pSessionId,"dnsbl_open: unable to open RDNSBL host file '%s'\n",str);
		if(str != NULL)
			free(str);

		if(fd != -1)
		{	char buf[8192];

			lseek(fd,0l,SEEK_SET);
			while(mlfi_fdgets(fd,buf,sizeof(buf)) >= 0)
			{
				/* handle comments */
				str = strchr(buf,'#');
				if(str != NULL)
				{
					/* truncate at comment */
					*(str--) = '\0';
					/* trim right */
					while(str >= buf && (*str ==' ' || *str == '\t'))
						*(str--) = '\0';
				}

				if(strlen(buf))
				{	char numbuf[128];
					RBLHOST *prbl;

					prlh->plist = realloc(prlh->plist,sizeof(RBLHOST)*(prlh->qty+1));
					prbl = prlh->plist + prlh->qty;
					prlh->qty++;

					memset(prbl,0,sizeof(RBLHOST));
					str = mlfi_strcpyadv(prbl->hostname,sizeof(prbl->hostname),buf,'|');
					str = mlfi_strcpyadv(prbl->url,sizeof(prbl->url),str,'|');
					str = mlfi_strcpyadv(numbuf,sizeof(numbuf),str,'|');
					prbl->action =
						strcasecmp(numbuf,"tag") == 0 ? RBL_A_TAG :
						strcasecmp(numbuf,"reject") == 0 ? RBL_A_REJECT :
						RBL_A_NULL;
					str = mlfi_strcpyadv(numbuf,sizeof(numbuf),str,'|');
					prbl->stage =
						strcasecmp(numbuf,"conn") == 0 ? RBL_S_CONN :
						strcasecmp(numbuf,"from") == 0 ? RBL_S_FROM :
						strcasecmp(numbuf,"rcpt") == 0 ? RBL_S_RCPT :
						strcasecmp(numbuf,"hdr") == 0 ? RBL_S_HDR :
						strcasecmp(numbuf,"eom") == 0 ? RBL_S_EOM :
						RBL_S_NULL;

					/*
					mlfi_debug(pSessionId,"dnsbl_create: rbl '%s' '%s' '%s' '%s'\n"
						,prbl->hostname
						,prbl->url
						,(prbl->action == RBL_A_TAG ? "Tag" : prbl->action == RBL_A_REJECT ? "Reject" : "Unknown")
						,dnsbl_str_stage[prbl->stage]// = {"None","Conn","From","Rcpt","Hdr","Eom"};
					);
					*/
				}
			}
			close(fd);
		}
	}

	if(prlh != NULL && prlh->qty == 0)
	{
		dnsbl_free_hosts(prlh);
		prlh = NULL;
	}

	return prlh;
}

RBLHOST *dnsbl_action(const char *pSessionId, RBLHOST **prbls, int stage)
{	RBLHOST	*prblh = NULL;

	while(prblh == NULL && prbls != NULL && *prbls != NULL)
	{
		if((*prbls)->stage == stage && (*prbls)->action > RBL_A_NULL)
			prblh = *prbls;
		else
			prbls++;
	}
	mlfi_debug(pSessionId,"dnsbl_action: %s %s %s\n"
		,stage == RBL_S_CONN ? "connect" :
			stage == RBL_S_RCPT ? "rcpt" :
			stage == RBL_S_FROM ? "from" :
			stage == RBL_S_HDR ? "hdr" :
			stage == RBL_S_EOM ? "eom" :
			"Unknown"
		,prblh == NULL ? "no action" :
			prblh->hostname
		,prblh == NULL ? "" :
			prblh->action == RBL_A_TAG ? " Tag" :
			prblh->action == RBL_A_REJECT ? " Reject" :
			" Unknown"
		);

	return prblh;
}

typedef struct _dcrc_t
{
	int flagged;
	const char *pSessionId;
	const char *pArpaIpStr;
} dcrc_t; // Dnsbl_Check_Rbl_Callback

static int dnsbl_check_rbl_callback(dqrr_t *pDqrr, void *pdata)
{	dcrc_t *pDcrc = (dcrc_t *)pdata;
	int nsType = ns_rr_type(pDqrr->rr);
	char buf[INET6_ADDRSTRLEN+1];

	memset(buf,0,sizeof(buf));
	switch(nsType)
	{
		case ns_t_a:
			if(ns_rr_rdlen(pDqrr->rr) == NS_INADDRSZ)
				inet_ntop(AF_INET, ns_rr_rdata(pDqrr->rr), buf, sizeof(buf)-1);
			break;

		case ns_t_aaaa:
			if(ns_rr_rdlen(pDqrr->rr) == NS_IN6ADDRSZ)
				inet_ntop(AF_INET6, ns_rr_rdata(pDqrr->rr), buf, sizeof(buf)-1);
			break;
	}

	if(strlen(buf))
	{
		pDcrc->flagged++;
		mlfi_debug(pDcrc->pSessionId, "dnsbl_check_rbl '%s' - listed - %s\n", pDcrc->pArpaIpStr, buf);
	}

	return 1; // again
}

int dnsbl_check_rbl_af(const char *pSessionId, const res_state statp, int afType, const char *in, char *domain)
{	int flagged = 0;

	if(domain != NULL && *domain)
	{	dqrr_t *pDqrr = NULL;
		char *pIpStr = mlfi_inet_ntopAF(afType, in);
		char *pArpaIpStr = dns_inet_ptoarpa(pIpStr, afType, domain);
		int nsType = 0;
		int rc = -1;

		switch(afType)
		{
			case AF_INET:	nsType = ns_t_a;	break;
			case AF_INET6:	nsType = ns_t_aaaa;	break;
		}

		pDqrr = dns_query_rr_init(statp, nsType);
		rc = dns_query_rr_resp(pDqrr, pArpaIpStr);

		if(rc > 0)
		{
			dcrc_t dcrc;

			dcrc.flagged = flagged;
			dcrc.pSessionId = pSessionId;
			dcrc.pArpaIpStr = pArpaIpStr;

			dns_parse_response_answer(pDqrr, &dnsbl_check_rbl_callback, &dcrc);
			flagged = dcrc.flagged;
		}
		else
			mlfi_debug(pSessionId, "dnsbl_check_rbl '%s' - not listed\n", pArpaIpStr);
		dns_query_rr_free(pDqrr);

		if(pIpStr != NULL)
			free(pIpStr);

		if(pArpaIpStr != NULL)
			free(pArpaIpStr);
	}

	return flagged;
}

int dnsbl_check_rbl_sa(const char *pSessionId, const res_state statp, struct sockaddr *psa, char *domain)
{	int flagged = 0;

	if(psa != NULL)
	{	const char *in = NULL;

		switch(psa->sa_family)
		{
			case AF_INET: in = (char *) &((struct sockaddr_in *)psa)->sin_addr; break;
			case AF_INET6: in = (const char *)&((struct sockaddr_in6 *)psa)->sin6_addr; break;
		}

		flagged = dnsbl_check_rbl_af(pSessionId, statp, psa->sa_family, in, domain);
	}

	return flagged;
}

RBLLISTMATCH *dnsbl_check(const char *pSessionId, int stage, RBLLISTHOSTS *prbls, struct sockaddr *psa, res_state presstate)
{	RBLLISTMATCH *prblmatches = (RBLLISTMATCH*)calloc(1,sizeof(RBLLISTMATCH));

	if(prbls != NULL
		&& prbls->plist != NULL
		&& prbls->qty > 0
		&& psa != NULL
		&& !mlfi_isNonRoutableIpSA(psa) // don't bother testing non-routable ip addresses
		)
	{	char *pipstr = mlfi_inet_ntopSA(psa);
		RBLHOST *prblh;
		RBLHOST **prbl;
		int i;

		// copy the ip for later reference
		prblmatches->sock = *psa;
		// inital storage allocation enough for all rbls listed
		prbl = prblmatches->ppmatch = (RBLHOST **) calloc(prbls->qty,sizeof(RBLHOST *));
		// match the rbls to this ip
		for(i=0; i<prbls->qty; i++)
		{
			prblh = prbls->plist + i;
			if(prblh->stage == stage && dnsbl_check_rbl_sa(pSessionId, presstate, psa, prblh->hostname) > 0)
			{
				mlfi_debug(pSessionId,"dnsbl_check '%s.%s' - Blacklisted\n", pipstr, prblh->hostname);
				*prbl = prblh;
				prbl++;
				prblmatches->qty++;
			}
		}

		if(pipstr != NULL)
			free(pipstr);
	}

	if(prblmatches != NULL && prblmatches->qty == 0)
	{
		if(prblmatches->ppmatch != NULL)
			free(prblmatches->ppmatch);
		free(prblmatches);
		prblmatches = NULL;
	}

	return prblmatches;
}

void dnsbl_add_hdr(SMFICTX *ctx, RBLHOST *prbl)
{	mlfiPriv	*priv = MLFIPRIV;

	if(priv != NULL && prbl != NULL)
	{	char *pipstr = (priv->pip != NULL ? mlfi_inet_ntopSA(priv->pip) : NULL);
	
		//mlfi_addhdr_printf(ctx,"X-RDNSBL-Warning","Source IP tagged as spam source by %s",prbl->hostname);
		mlfi_addhdr_printf(ctx,"X-Milter","%s DataSet=RDNSBL-Warning; receiver=%s; ip=%s; rbl=%s;"
			,mlfi.xxfi_name ,gHostname
			,(pipstr != NULL ? pipstr : "")
			,prbl->hostname
			);

		if(pipstr != NULL)
			free(pipstr);
	}
}
