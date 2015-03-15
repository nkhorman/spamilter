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
 *	CVSID:  $Id: hndlrs.c,v 1.185 2015/01/21 04:41:19 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		hndlrs.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: hndlrs.c,v 1.185 2015/01/21 04:41:19 neal Exp $";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <syslog.h>
#include <sys/wait.h>
#include <resolv.h>
#include <ctype.h>
#include <pthread.h>

#include "spamilter.h"
#include "hndlrs.h"
#include "smtp.h"
#include "dns.h"
#include "dnsbl.h"
#include "misc.h"
#include "bwlist.h"
#include "inet.h"
#include "badext.h"
#include "smisc.h"
#include "ifi.h"
#ifdef SUPPORT_POPAUTH
#include "popauth.h"
#endif
#if defined(SUPPORT_LIBSPF) || defined(SUPPORT_LIBSPF2)
#include "spfapi.h"
#endif
#ifdef SUPPORT_VIRTUSER
#include "virtusertable.h"
#endif
#ifdef SUPPORT_ALIASES
#include "aliastable.h"
#endif
#ifdef SUPPORT_LOCALUSER
#include <pwd.h>
#endif
#include "dupe.h"
#ifdef SUPPORT_FWDHOSTCHK
#include "fwdhostlist.h"
#endif
#include "regexapi.h"
#include "regexmisc.h"
#ifdef SUPPORT_GEOIP
#include "geoip.h"
#endif
#include "md5api.h"
#ifdef SUPPORT_DBL
#include "dbl.h"
#endif

char *str2lo(char *src)
{	char *p = src;

	while(p != NULL && *p)
	{
		*p = (char)tolower(*p);
		p++;
	}

	return(src);
}

void log_status(mlfiPriv *priv, const char *status, const char *reason)
{
	if(priv != NULL)
	{	char *sndr = (priv->replyto != NULL && *priv->replyto ? priv->replyto : priv->sndr);
		char *pf1 = priv->pSessionUuidStr;

		if(reason != NULL && priv->rcpt != NULL && sndr != NULL)
			syslog(LOG_INFO,"%s %s %s %s %s",status,pf1,sndr,priv->rcpt,reason);

		else if(priv->header_subject != NULL && priv->rcpt != NULL && sndr != NULL)
			syslog(LOG_INFO,"%s %s %s %s %s",status,pf1,sndr,priv->rcpt,priv->header_subject);

		else if(priv->rcpt != NULL && sndr != NULL)
			syslog(LOG_INFO,"%s %s %s %s",status,pf1,sndr,priv->rcpt);

		else if(sndr != NULL)
			syslog(LOG_INFO,"%s %s %s",status,pf1,sndr);

		else 
			syslog(LOG_INFO,"%s %s",status,pf1);
	}
}

enum { LOG_ACCEPTED, LOG_REJECTED, LOG_TEMPFAILED, LOG_DISCARDED, LOG_NOREPLY, LOG_SKIP };
static const char *logStr[] = { "Accepted", "Rejected", "TempFailed", "Discarded", "NoReply", "Skipped" };
#define LOG_STR(a) (logStr[(a)])
#define LOG_ACCEPTED_STR (LOG_STR(LOG_ACCEPTED))
#define LOG_REJECTED_STR (LOG_STR(LOG_REJECTED))
#define LOG_TEMPFAILED_STR (LOG_STR(LOG_TEMPFAILED))
#define LOG_DISCARDED_STR (LOG_STR(LOG_DISCARDED))
#define LOG_NOREPLY_STR (LOG_STR(LOG_NOREPLY))
#define LOG_SKIP_STR (LOG_STR(LOG_SKIP))

int mlfi_status_debug(mlfiPriv *priv, sfsistat *prs, const char *statusstr, const char *statusmsg, const char *debugmsg, ...)
{	va_list vl;

	log_status(priv,statusstr,statusmsg);
	if(prs != NULL)
	{
		if(statusstr == LOG_ACCEPTED_STR)
			*prs = SMFIS_ACCEPT;
		else if(statusstr == LOG_REJECTED_STR)
			*prs = SMFIS_REJECT;
		else if(statusstr == LOG_TEMPFAILED_STR)
			*prs = SMFIS_TEMPFAIL;
		else if(statusstr == LOG_DISCARDED_STR)
			*prs = SMFIS_DISCARD;
#ifdef SMFIS_NOREPLY
		else if(statusstr == LOG_NOREPLY_STR)
			*prs = SMFIS_NOREPLY;
#endif
#ifdef SMFIS_SKIP
		else if(statusstr == LOG_SKIP_STR)
			*prs = SMFIS_SKIP;
#endif
	}

	if(debugmsg != NULL)
	{
		va_start(vl,debugmsg);
		mlfi_vdebug(priv->pSessionUuidStr,debugmsg,vl);
		va_end(vl);
	}

	return (prs != NULL ? *prs == SMFIS_CONTINUE : 1);
}

void mlfi_MtaHostIpfwAction(mlfiPriv *priv, char *action)
{	int sd = NetSockOpenTcpPeer(INADDR_LOOPBACK,4739);

	if(sd != INVALID_SOCKET)
	{	char	buf[1024];

		mlfi_debug(priv->pSessionUuidStr,"mlfi_MtaHostIpfwAction: %s %s\n",priv->ipstr,action);
		while(NetSockGets(sd,buf,sizeof(buf)-1,1) > 0)
			;
		NetSockPrintf(sd,"%s,%s\r\n\r\n",action,priv->ipstr);
		NetSockClose(&sd);
	}
}

sfsistat mlfi_rdnsbl_reject(SMFICTX *ctx, sfsistat *rs, int stage, struct sockaddr *psa, RBLLISTHOSTS *prblhosts, RBLLISTMATCH **pprblmatch)
{
	if(*rs == SMFIS_CONTINUE && gDnsBlChk && psa != NULL && prblhosts != NULL && pprblmatch != NULL)
	{	mlfiPriv *priv = MLFIPRIV;

		if(!priv->islocalnethost)
		{	RBLHOST **ppmatch = NULL;
			RBLHOST *prblh = NULL;

			*pprblmatch = dnsbl_check(priv->pSessionUuidStr,stage,prblhosts,psa,priv->presstate);
			ppmatch = (*pprblmatch != NULL ? (*pprblmatch)->ppmatch : NULL);
			if(*pprblmatch != NULL
				&& (*pprblmatch)->qty
				&& (prblh = dnsbl_action(priv->pSessionUuidStr,ppmatch,stage)) != NULL
				&& prblh->action == RBL_A_REJECT)
			{	char *reason = NULL;

				mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - DNSBL, Please see: %s#dnsbl and %s",gPolicyUrl,prblh->url);
				asprintf(&reason,"DNSBL '%s'",prblh->url);
				mlfi_status_debug(priv,rs,LOG_REJECTED_STR,reason,"DNSBL rejected - %s\n",priv->ipstr);
				free(reason);

				if(gMtaHostIpfw)
					mlfi_MtaHostIpfwAction(priv,"add");
				else if(gMtaHostIpfwNominate) // multi-penalty for RBL hits
					mlfi_MtaHostIpfwAction(priv,"inculpate");
			}
		}
		else
			mlfi_debug(priv->pSessionUuidStr,"mlfi_rdnsbl_reject: islocalnethost: %u\n",priv->islocalnethost);
	}

	return *rs;
}

sfsistat mlfi_hndlr_exec(SMFICTX *ctx, int stage)
{	mlfiPriv	*priv = MLFIPRIV;
	sfsistat	rs = SMFIS_CONTINUE;
	char		*str = NULL;
	char		*reason = NULL;
	int		rc;

	if(priv != NULL)
	{	char *sndr = (priv->replyto != NULL && strlen(priv->replyto) ? priv->replyto : priv->sndr);
		char *actionexec = (priv->replyto != NULL && strlen(priv->replyto) ? priv->replytoactionexec : priv->sndractionexec);

		switch(stage)
		{
			case RBL_S_FROM:
				asprintf(&str,"%s '%s' '%s'",actionexec,priv->ipstr,sndr);
				break;
			case RBL_S_RCPT:
				asprintf(&str,"%s '%s' '%s' '%s'",priv->rcptactionexec,priv->ipstr,sndr,priv->rcpt);
				break;
			case RBL_S_CONN:
				asprintf(&str,"%s '%s' '%s' '%s'",priv->heloactionexec,priv->ipstr,sndr,priv->helo);
				break;
		}

		if(str != NULL)
		{
			mlfi_debug(priv->pSessionUuidStr,"mlfi_hndlr_exec: '%s'\n",str);
			rc = system(str);
			rc = WIFEXITED(rc) ? WEXITSTATUS(rc) : 2;
			mlfi_debug(priv->pSessionUuidStr,"mlfi_hndlr_exec: returned %d\n",rc);
			asprintf(&reason,"Exec '%s'",str);
			switch(rc)
			{
				case 1:
					mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy. Please see: %s#general",gPolicyUrl);
					mlfi_status_debug(priv,&rs,LOG_REJECTED_STR,NULL,NULL);
					break;
				case 127:
					// deliberate fall thru to case 2
				case 2:
					mlfi_status_debug(priv,&rs,LOG_TEMPFAILED_STR,NULL,NULL);
					break;
				case 3:
					mlfi_status_debug(priv,&rs,LOG_DISCARDED_STR,NULL,NULL);
					break;
				default:
					// deliberate fall thru to case 0
				case 0:
					rs = SMFIS_CONTINUE;
					break;
			}
			free(str);
			free(reason);
		}
	}

	return rs;
}

sfsistat mlfi_connect(SMFICTX *ctx, char *hostname, _SOCK_ADDR *hostaddr)
{	mlfiPriv	*priv = MLFIPRIV;
	sfsistat	rs = SMFIS_CONTINUE;

	// don't assume that we need to create a context structure
	if(priv == NULL)
		priv = calloc(1,sizeof(mlfiPriv));

	if(priv != NULL)
	{
#ifdef OS_FreeBSD
		struct uuid sessionUuid;
#endif
		char *pSessionUuidStr = NULL;

		smfi_setpriv(ctx, priv);

#ifdef OS_FreeBSD
		// session uuid
		if(uuidgen(&sessionUuid,1) == 0)
		{	uint32_t status = 0;

			uuid_to_string(&sessionUuid,&pSessionUuidStr,&status);
			if(status != uuid_s_ok && pSessionUuidStr != NULL)
			{
				free(pSessionUuidStr);
				pSessionUuidStr = NULL;
			}
		}

		if(pSessionUuidStr == NULL)
#endif
			asprintf(&pSessionUuidStr,"%04X",(unsigned int)pthread_self());

		if(pSessionUuidStr != NULL)
		{	MD5_CTX md5ctx;
			char md5digest[MD5_DIGEST_LENGTH];
			char md5ascii[(MD5_DIGEST_LENGTH*2)+10];
			char *pStr = NULL;

			memset(&md5ctx,0,sizeof(md5ctx));
			memset(&md5digest,0,sizeof(md5digest));
			memset(&md5ascii,0,sizeof(md5ascii));

			// This is damn silly, any given uuid is supposed to be unique!
			// but I've found instances where they aren't, so append time
			// before hasing, which btw, helps with the fall back scenario
			// where we use the thread id if no uuid is available.
			asprintf(&pStr,"%s0x%lX",pSessionUuidStr,(unsigned long)time(NULL));

			MD5Init(&md5ctx);
			MD5Update(&md5ctx,(const unsigned char *)pStr,strlen(pStr));
			MD5Final((unsigned char *)md5digest,&md5ctx);
			MD5Ascii((unsigned char *)md5digest,md5ascii);

			priv->pSessionUuidStr = strdup(md5ascii);
			free(pSessionUuidStr);
			pSessionUuidStr = NULL;
		}
		// TODO - oopsie, now what do we do? no session id is available

		priv->pbwlistctx = bwlistCreate(priv->pSessionUuidStr);
		priv->pDblCtx = dbl_Create(priv->pSessionUuidStr);
#ifdef SUPPORT_FWDHOSTCHK
		fwdhostlist_init(priv);
#endif
#ifdef SUPPORT_GEOIP
		geoip_init(ctx);
#endif
		dupe_init(priv);
		priv->presstate = RES_NALLOC(priv->presstate);
		if(priv->presstate != NULL)
			res_ninit(priv->presstate);
		badext_init(priv,gDbpath);

		// make sure that the b/w list action files are open
		bwlistOpen(priv->pbwlistctx,gDbpath);
		dbl_Open(priv->pDblCtx, gDbpath);
#ifdef SUPPORT_FWDHOSTCHK
		fwdhostlist_open(priv,gDbpath);
#endif
#ifdef SUPPORT_GEOIP
		geoip_open(ctx,gDbpath,gpGeoipDbPath);
#endif
		if (gDnsBlChk)
			priv->pdnsrblhosts = dnsbl_create(priv->pSessionUuidStr,gDbpath);

		if(hostaddr != NULL)
		{	// sa_len is supposed to be the length of the sockaddr structure, but it is zero instead,
			// so, use sa_family to pick the compile time sizeof() of the socket family structure and
			// if that fails, use the max
			// TODO - This should probably be a macro or a function instead
#ifdef OS_FreeBSD
			const unsigned int sa_len = (hostaddr->sa_len > 0 ?
				hostaddr->sa_len : hostaddr->sa_family == AF_INET ?
				sizeof(struct sockaddr_in) : hostaddr->sa_family == AF_INET6 ?
				sizeof(struct sockaddr_in6) : SOCK_MAXADDRLEN
				);
#else
#define sa_len sizeof(struct sockaddr_in6)
#endif

			priv->pip = (struct sockaddr *)calloc(1,sa_len);
			if(priv->pip != NULL)
				memcpy(priv->pip, hostaddr, sa_len);
			priv->ipstr = mlfi_inet_ntopSA(hostaddr);
		}

		if(hostname != NULL)
			priv->iphostname = strdup(hostname);

		mlfi_debug(priv->pSessionUuidStr,"mlfi_connect: '%s'/%s\n",hostname,priv->ipstr);

		priv->islocalnethost = 0;
		switch(priv->pip->sa_family)
		{
			case AF_INET:
				priv->islocalnethost = ( ntohl(((struct sockaddr_in *)priv->pip)->sin_addr.s_addr) == INADDR_LOOPBACK );
				if(!priv->islocalnethost)
					priv->islocalnethost = ifi_islocalnet(ntohl(((struct sockaddr_in *)priv->pip)->sin_addr.s_addr));
				break;
			case AF_INET6:
				{	struct in6_addr *pAddr = &((struct sockaddr_in6 *)priv->pip)->sin6_addr;

					priv->islocalnethost = (
						IN6_ARE_ADDR_EQUAL(pAddr, &in6addr_loopback)
						|| IN6_IS_ADDR_LINKLOCAL(pAddr) // fe80:xx
						|| IN6_IS_ADDR_SITELOCAL(pAddr) // fec0::xx
						);
					// TODO - ipv6 - need ifi_islocalnet() style tests
				}
				break;
		}

#ifdef SUPPORT_POPAUTH
		if(!priv->islocalnethost)
			priv->islocalnethost = (gPopAuthChk != NULL && popauth_validate(priv,gPopAuthChk));
#endif

		mlfi_debug(priv->pSessionUuidStr,"mlfi_connect: islocalnethost: %u\n",priv->islocalnethost);

		//mlfi_rdnsbl_reject(ctx, &rs, RBL_S_CONN, priv->pip, priv->pdnsrblhosts, &priv->pdnsrblmatch);

		// Connection rate limititing, by the fact that the ipfwmtad daemon
		// will firewall the source ip based on the number and rate of inculpate
		// operations
		if(gMtaHostIpfwNominate && !priv->islocalnethost)
			mlfi_MtaHostIpfwAction(priv,"inculpate");

#ifdef SUPPORT_GEOIP
		if(!priv->islocalnethost
			//&& gMtaHostChk ?
			)
		{	const char *pGeoipCC = NULL;

			if(!mlfi_isNonRoutableIpSA(priv->pip))
				pGeoipCC = geoip_result_addSA(ctx, priv->pip, geoip_LookupCCBySA(ctx, priv->pip));

			if(pGeoipCC != NULL)
			{
				mlfi_debug(priv->pSessionUuidStr,"mlfi_connect: geoip: %s, CC: %s\n"
					,priv->ipstr
					,pGeoipCC
					);
			}
		}
#endif
		priv->pListBodyHosts = listCreate();
	}
	else // no resources, beg out
		rs = SMFIS_TEMPFAIL;

	return rs;
}

sfsistat mlfi_helo(SMFICTX *ctx, char *helohost)
{	mlfiPriv *priv = MLFIPRIV;
	sfsistat rs = SMFIS_CONTINUE;

	mlfi_debug(priv->pSessionUuidStr,"mlfi_helo: '%s'\n",helohost);

	if(priv != NULL)
		priv->helo = str2lo(strdup(helohost));

	return(rs);
}

sfsistat mlfi_envfrom(SMFICTX *ctx, char **envfrom)
{	mlfiPriv	*priv = MLFIPRIV;
	sfsistat	rs = SMFIS_CONTINUE;

	mlfi_debug(priv->pSessionUuidStr,"mlfi_envfrom: '%s'\n",*envfrom);

	if(priv != NULL)
	{	char *pMbox = NULL;
		char *pDomain = NULL;
		// sendmail macros
//		char	*pstr[] = {"{auth_type}","{auth_authen}","{auth_ssf}","{auth_author}","{mail_mailer}"};
//		int	i,q=sizeof(pstr)/sizeof(pstr[0]);

		if(priv->sndr != NULL)
			free(priv->sndr);
		priv->sndr = str2lo(strdup(*envfrom));
		mlfi_regex_mboxsplit(priv->sndr,&pMbox,&pDomain);
		dupe_query(priv,*envfrom,DUPE_FROM);
		priv->sndraction = bwlistActionQuery(priv->pbwlistctx,BWL_L_SNDR,pDomain,pMbox,priv->sndractionexec);
		mlfi_debug(priv->pSessionUuidStr,"mlfi_envfrom: sndraction = %u/'%s'\n",priv->sndraction,gpBwlStrs[priv->sndraction]);

		priv->isAuthEn = (smfi_getsymval(ctx,"{auth_authen}") != NULL);
		mlfi_debug(priv->pSessionUuidStr,"mlfi_envfrom: isAuthEn %u\n",priv->isAuthEn);
		if(priv->isAuthEn)
		{
			priv->sndraction = BWL_A_ACCEPT;
			mlfi_debug(priv->pSessionUuidStr,"mlfi_envfrom: sndraction = %u/'%s'\n",priv->sndraction,gpBwlStrs[priv->sndraction]);
		}

		// Show all sendmail macro values
//		for(i=0; i<q; i++)
//			mlfi_debug(priv->pSessionUuidStr,"mlfi_envfrom: '%s':'%s'\n",pstr[i],smfi_getsymval(ctx,pstr[i]));

		switch(priv->sndraction)
		{
			case BWL_A_REJECT:
			case BWL_A_DISCARD:
			case BWL_A_TEMPFAIL:
			case BWL_A_ACCEPT:
			case BWL_A_TARPIT:
			case BWL_A_EXEC:
				// take no further action
				break;
			default:
				// deliberate fall thru to BWL_A_NULL case
			case BWL_A_NULL:
				// no acton specified, do other normal checks
				if(priv->helo == NULL || !*priv->helo)
				{
					mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - Helo or Ehlo MTA greeting expected, Please see: %s#helo",gPolicyUrl);
					mlfi_status_debug(priv,&rs,LOG_REJECTED_STR,"Missing Helo/Ehlo",
						"mlfi_envfrom: MTA hostname not specified\n");
				}
				break;
		}
		free(pMbox);
		free(pDomain);
	}

	return rs;
}

sfsistat mlfi_sndrchk(SMFICTX *ctx, const char *pMbox, const char *pDomain)
{	mlfiPriv *priv = MLFIPRIV;
	sfsistat rs = SMFIS_CONTINUE;

	if(gSmtpSndrChk && priv != NULL && !priv->SmtpSndrChkFail)
	{	int smtprc = 0;

		mlfi_debug(priv->pSessionUuidStr,"mlfi_sndrchk: %s@%s\n",pMbox,pDomain);
		priv->SmtpSndrChkFail = !smtp_email_address_is_deliverable(priv->pSessionUuidStr, priv->presstate, pMbox, pDomain, &smtprc, false);

		if(priv->SmtpSndrChkFail && strcasecmp(gSmtpSndrChkAction,"Reject") == 0)
		{
			if(smtprc == -1 || smtprc/100 == 4)
			{
				if(smtprc == -1)
					smtprc = 450;
				mlfi_setreply(ctx,smtprc,"4.7.1","Temporary failure - Unable to validate Sender address %s, Please see: %s#tempfailinvalidsender",priv->sndr,gPolicyUrl);
				mlfi_status_debug(priv,&rs,LOG_TEMPFAILED_STR,"Sender address verification",NULL);
			}
			else
			{
				mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - Unable to validate Sender address %s, Please see: %s#invalidsender",priv->sndr,gPolicyUrl);
				mlfi_status_debug(priv,&rs,LOG_REJECTED_STR,"Sender address verification",NULL);
			}
			mlfi_debug(priv->pSessionUuidStr,"mlfi_sndrchk: Return path '%s' not deliverable\n",priv->sndr);
		}
	}

	return rs;
}

sfsistat mlfi_replyto(SMFICTX *ctx)
{	mlfiPriv	*priv = MLFIPRIV;
	sfsistat	rs = SMFIS_CONTINUE;

	if(
		priv != NULL
		&& priv->header_replyto != NULL
		&& strlen(priv->header_replyto)
		&& strcmp(priv->header_replyto,"<>") != 0
	)
	{	char	*pMbox = NULL;
		char	*pDomain = NULL;
		char	*reason = NULL;
		int	action = BWL_A_NULL;

		if(mlfi_regex_mboxsplit(priv->header_replyto,&pMbox,&pDomain))
		{
			asprintf(&priv->replyto,"<%s@%s>",pMbox,pDomain);
			if(priv->replyto != NULL && *priv->replyto)
				str2lo(priv->replyto);
			mlfi_debug(priv->pSessionUuidStr,"mlfi_replyto: replyto set: %s\n",priv->replyto);

			if(priv->replyto != NULL && strcasecmp(priv->sndr,priv->replyto) != 0)
			{	
				action = bwlistActionQuery(priv->pbwlistctx,BWL_L_SNDR,pDomain,pMbox,priv->replytoactionexec);
			}
			else
			{
				free(priv->replyto);
				priv->replyto = NULL;
			}

			mlfi_debug(priv->pSessionUuidStr,"mlfi_replyto: to: '%s'/%s action = %u/'%s'\n"
				,priv->header_replyto
				,priv->replyto
				,action,gpBwlStrs[action]
				);

			switch(action)
			{
				case BWL_A_TARPIT:
					sleep(120);	// slow the bastard down!
					// deliberate fall thru to BWL_A_REJECT 
				case BWL_A_REJECT:
					mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - Sender '%s' has been blacklisted, Please see: %s#blacklistedsender"
						,priv->replyto,gPolicyUrl);
					asprintf(&reason,"Blacklisted Sender(Reply-To) '%s'",priv->replyto);
					rs = SMFIS_REJECT;
					mlfi_status_debug(priv,&rs,LOG_REJECTED_STR,reason
						,"mlfi_replyto: Blacklisted SMTP Sender(Reply-To), '%s'\n",priv->replyto);
					free(reason);
					if(gMtaHostIpfw)
						mlfi_MtaHostIpfwAction(priv,"add");
					break;
				case BWL_A_TEMPFAIL:
					rs = SMFIS_TEMPFAIL;
					mlfi_status_debug(priv,&rs,LOG_TEMPFAILED_STR,reason,NULL);
					break;
				case BWL_A_DISCARD:
					rs = SMFIS_DISCARD;
					mlfi_status_debug(priv,&rs,LOG_DISCARDED_STR,reason,NULL);
					break;
				case BWL_A_ACCEPT:
					// take no further action
					rs = SMFIS_ACCEPT;
					break;
				case BWL_A_EXEC:
					rs = mlfi_hndlr_exec(ctx,RBL_S_FROM);
					break;
				default:
					// deliberate fall thru to BWL_A_NULL case
				case BWL_A_NULL:
					// no acton specified, do other normal checks
					break;
			}
			priv->sndraction = action;

			if(rs == SMFIS_CONTINUE && pMbox != NULL && pDomain != NULL)
				rs = mlfi_sndrchk(ctx,pMbox,pDomain);
		}
		else
		{
			mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - Sender '%s' has been blacklisted, Please see: %s#blacklistedsender"
				,priv->header_replyto,gPolicyUrl);
			asprintf(&reason,"Blacklisted Sender(Malformed Reply-To) '%s'",priv->header_replyto);
			mlfi_status_debug(priv,&rs,LOG_REJECTED_STR,reason
				,"mlfi_replyto: Blacklisted SMTP Sender(Malformed Reply-To), '%s'\n",priv->header_replyto);
			free(reason);
			rs = SMFIS_REJECT;
		}

		free(pMbox);
		free(pDomain);
	}

	return rs;
}

#ifdef SUPPORT_LOCALUSER
int mlfi_envrcpt_islocaluser(mlfiPriv *priv, const char *user)
{	int		ok = 0;
	struct passwd	*pwd= NULL;

	if(user != NULL)
	{
		setpwent();
		ok = ((pwd = getpwnam(user)) != NULL);
		endpwent();
		mlfi_debug(priv->pSessionUuidStr,"mlfi_envrcpt_islocaluser: '%s' %d/%s\n",user,ok,(ok ? "Found" : "Not Found"));
	}

	return(ok);
}
#endif

#ifdef SUPPORT_GREYLIST

// For greylisting, do ip address globing, under the premise that a given sender might be
// using more than one MTA in close proximity to others as it relates to the ip address space.
//
// We glob ipv4 to /24, and ipv6 to /64
//
int mlfi_greylist(SMFICTX *ctx)
{	mlfiPriv	*priv = MLFIPRIV;
	int rc = 0;

	if(gGreyListChk)
	{	int sd = NetSockOpenUdp(0,0);

		if(sd != INVALID_SOCKET)
		{
			int inAfType = priv->pip->sa_family;
			char *pInAddr = NULL;
			struct in_addr in4_addr;
			struct in6_addr in6_addr;
			char *pipstr = priv->ipstr;

			switch(inAfType)
			{
				case AF_INET:
					in4_addr.s_addr = ntohl(((struct sockaddr_in *)priv->pip)->sin_addr.s_addr);
					in4_addr.s_addr &= 0xFFFFFF00; // mask for /24
					in4_addr.s_addr = htonl(in4_addr.s_addr);
					pInAddr = (char *)&in4_addr;
					break;

				case AF_INET6:
					{
#ifdef OS_FreeBSD
						size_t i;

						memset(&in6_addr, 0, sizeof(in6_addr));
						for(i=0; i<4; i++) // mask for /64, by only coping the top 64 bits
							in6_addr.__u6_addr.__u6_addr16[i] = ((struct sockaddr_in6 *)priv->pip)->sin6_addr.__u6_addr.__u6_addr16[i];
						pInAddr = (char *)&in6_addr;
#endif
					}
					break;
			}

			if(pInAddr != NULL && (pipstr = mlfi_inet_ntopAF(inAfType, pInAddr)) != NULL)
			{
				// TODO - add greylist host and port to config options
				if(NetSockPrintfTo(sd, 0x7f000001, 7892, "<%s> %s %s", pipstr, priv->sndr, priv->rcpt) != SOCKET_ERROR
					&& NetSockSelectOne(sd, 30000)
				)
				{	unsigned long ip;
					unsigned short port;
					char buf[128];
					int len = NetSockRecvFrom(sd,buf,sizeof(buf),&ip,&port,NULL);

					if(len != SOCKET_ERROR)
					{
						mlfi_debug(priv->pSessionUuidStr,"mlfi_greylist: query result '%*.*s'\n",len,len,buf);
						rc = (strncmp(buf,"<ok>",len) != 0 ? 1 : 2); // 1 = grey, 2 = white
					}
				}
				else
				{
					mlfi_debug(priv->pSessionUuidStr,"mlfi_greylist: query timeout\n");
					rc = -1;
				}

				free(pipstr);
			}
			NetSockClose(&sd);
		}
		else
		{
			mlfi_debug(priv->pSessionUuidStr,"mlfi_greylist: invalid socket\n");
			rc = -2;
		}
	}

	return rc;
}
#endif

sfsistat mlfi_contentfilter(mlfiPriv *priv, sfsistat *rs, int stage)
{
	if(1 && priv != NULL) // if content filtering
	{	char *pdup = (priv->header_subject != NULL ? str2lo(strdup(priv->header_subject)) : NULL);
		int reject = 0;

#include "hndlrs_contentfilter_local.inc"

		if(pdup != NULL)
			free(pdup);

		if(reject)
			*rs = SMFIS_REJECT;
	}

	return *rs;
}

sfsistat mlfi_envrcpt(SMFICTX *ctx, char **argv)
{	mlfiPriv *priv = MLFIPRIV;
	sfsistat rs = SMFIS_CONTINUE;

	mlfi_debug(priv->pSessionUuidStr,"mlfi_envrcpt: '%s'\n",*argv);

	if (priv != NULL)
	{
		if(priv->rcpt != NULL)
			free(priv->rcpt);
		priv->rcpt = str2lo(strdup(*argv));

		if(!priv->islocalnethost)
		{	char *pMbox = NULL;
			char *pDomain = NULL;
			char *reason = NULL;

			mlfi_regex_mboxsplit(priv->rcpt,&pMbox,&pDomain);
			dupe_query(priv,*argv,DUPE_RCPT);
			priv->rcptaction = bwlistActionQuery(priv->pbwlistctx,BWL_L_RCPT,pDomain,pMbox,priv->rcptactionexec);
			mlfi_debug(priv->pSessionUuidStr,"mlfi_envrcpt: rcptaction = %u/'%s'\n",priv->rcptaction,gpBwlStrs[priv->rcptaction]);
			asprintf(&reason,"Blacklisted recipient '%s@%s'",pMbox,pDomain);

			switch(priv->rcptaction)
			{
				case BWL_A_TARPIT:
					sleep(120);	// slow the bastard down!
					// deliberate fall thru to BWL_A_REJECT
				case BWL_A_REJECT:
					mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - Recipient has been blacklisted, Please see: %s#blacklistedrecipient",gPolicyUrl);
					mlfi_status_debug(priv,&rs,LOG_REJECTED_STR,reason
						,"mlfi_envrcpt: Blacklisted recipient '%s@%s'\n",pMbox,pDomain);
					if(gMtaHostIpfw)
						mlfi_MtaHostIpfwAction(priv,"add");
					break;
				case BWL_A_TEMPFAIL:
					mlfi_status_debug(priv,&rs,LOG_TEMPFAILED_STR,reason,NULL);
					break;
				case BWL_A_DISCARD:
					mlfi_status_debug(priv,&rs,LOG_DISCARDED_STR,reason,NULL);
					break;
				case BWL_A_ACCEPT:
					// take no further action
					break;
				case BWL_A_EXEC:
					rs = mlfi_hndlr_exec(ctx,RBL_S_RCPT);
					break;
				default:
					priv->rcptaction = BWL_A_NULL;
					// deliberate fall thru to BWL_A_NULL case
				case BWL_A_NULL:
					// no acton specified, do other normal checks
					if(priv->sndraction != BWL_A_ACCEPT)
					{
#if defined(SUPPORT_VIRTUSER) || defined(SUPPORT_ALIASES) || defined(SUPPORT_LOCALUSER)
						int deliverable = 0;
						int deliverableValid = 0;
#endif
#if defined(SUPPORT_VIRTUSER)
						deliverable |= (gVirtUserTableChk != NULL && virtusertable_validate_rcptdom(priv->pSessionUuidStr,pMbox,pDomain,gVirtUserTableChk));
						deliverableValid |= (gVirtUserTableChk != NULL);
#endif
#if defined(SUPPORT_ALIASES)
						deliverable |= (gAliasTableChk != NULL && aliastable_validate(priv->pSessionUuidStr,pMbox,gAliasTableChk));
						deliverableValid |= (gAliasTableChk != NULL);
#endif
#if defined(SUPPORT_LOCALUSER)
						deliverable |= (gLocalUserTableChk && mlfi_envrcpt_islocaluser(priv,pMbox));
						deliverableValid |= gLocalUserTableChk;
#endif
#if defined(SUPPORT_VIRTUSER) || defined(SUPPORT_ALIASES) || defined(SUPPORT_LOCALUSER)
						if(deliverableValid && !deliverable)
						{
							mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - Not deliverable, Please see: %s#undeliverable",gPolicyUrl);
							asprintf(&reason,"Not deliverable '%s@%s'",pMbox,pDomain);
							mlfi_status_debug(priv,&rs,LOG_REJECTED_STR,"Missing Helo/Ehlo",
								"mlfi_envrcpt: Not deliverable '%s@%s'\n",pMbox,pDomain);
							free(reason);
						}
						else
#endif
							rs = mlfi_rdnsbl_reject(ctx, &rs, RBL_S_RCPT, priv->pip, priv->pdnsrblhosts, &priv->pdnsrblmatch);
					}
				break;
			}

			free(pMbox);
			free(pDomain);
		}
	}

	return rs;
}

sfsistat mlfi_header(SMFICTX *ctx, char *headerf, char *headerv)
{	mlfiPriv *priv = MLFIPRIV;
	sfsistat rs = SMFIS_CONTINUE;
	char	*attachfname = NULL;
	char	*reason;

	if (priv != NULL)
	{
		if(strcasecmp(headerf,"X-Status") == 0 && strlen(headerv))
			priv->header_xstatus = strdup(headerv);
		else if(strcasecmp(headerf,"Subject") == 0)
		{
			// TODO - add UTF-8 handling
			priv->header_subject = strdup(headerv);
		}
		else if(gHeaderChkReplyTo && strcasecmp(headerf,"Reply-To") == 0 && strlen(headerv))
		{
			priv->header_replyto = strdup(headerv);
			mlfi_debug(priv->pSessionUuidStr,"mlfi_header: Reply-To: %s\n",priv->header_replyto);
		}
		else if(gHeaderChkReceived
			&& priv->sndraction != BWL_A_ACCEPT
			//&& priv->rcptaction != BWL_A_ACCEPT
			&& (
				strcasecmp(headerf,"Received") == 0
				|| strcasecmp(headerf,"X-Originating-IP") == 0 // hotmail does this
			)
			&& strlen(headerv)
			)
		// TODO - ipv6 - extraction from header and testing
		{	unsigned long ip = mlfi_regex_ipv4(headerv);
			int routeable = (ip != 0 && !mlfi_isNonRoutableIpV4(ip));
#ifdef SUPPORT_GEOIP
			const char *pGeoipCC = "--";
			if(routeable)
			{	
				pGeoipCC = geoip_result_addIpv4(ctx,ip, geoip_LookupCCByIpv4(ctx,ip));
				mlfi_debug(priv->pSessionUuidStr,"mlfi_header: geoip: %u.%u.%u.%u, CC: %s\n"
					,(ip>>24)&0xff ,(ip>>16)&0xff ,(ip>>8)&0xff ,ip&0xff
					,pGeoipCC
					);
			}
#endif

			mlfi_debug(priv->pSessionUuidStr,"mlfi_header: Received ip: %u.%u.%u.%u"
#ifdef SUPPORT_GEOIP
				", Country code: '%s'"
#endif
				", Routable %d, '%s'\n"
				,(ip&0xff000000)>>24 ,(ip&0x00ff0000)>>16 ,(ip&0x0000ff00)>>8 ,(ip&0x000000ff)
#ifdef SUPPORT_GEOIP
				,pGeoipCC
#endif
				,routeable ,headerv
				);

			// RBL check the ip in the "Recevied by/from:" header if it is routable
			if(routeable && ip != ntohl(((struct sockaddr_in *)priv->pip)->sin_addr.s_addr))
			{	struct sockaddr_in s;
				RBLLISTMATCH *prblmatch = NULL;

				s.sin_addr.s_addr = htonl(ip);
				s.sin_family = AF_INET;
				rs = mlfi_rdnsbl_reject(ctx, &rs, RBL_S_HDR, (struct sockaddr *)&s, priv->pdnsrblhosts, &prblmatch);
				dnsbl_free_match(prblmatch);
			}
		}
		else if(strcasecmp(headerf,"Content-Transfer-Encoding") == 0)
		{
			priv->bodyTransferEncoding = (strcasecmp(headerv,"base64") == 0 ? BODYENCODING_BASE64 : BODYENCODING_UNKNOWN);
		}

		if(rs == SMFIS_CONTINUE)
		{
			priv->MsExtFound = (gMsExtChk &&
				(mlfi_findBadExtHeader(priv->badextlist,priv->badextqty,headerf,"Content-Disposition:","filename=",headerv,&attachfname) ||
				(attachfname == NULL && mlfi_findBadExtHeader(priv->badextlist,priv->badextqty,headerf,"Content-Type:","name=",headerv,&attachfname)))
				);

			if(priv->MsExtFound && strcasecmp(gMsExtChkAction,"Reject") == 0)
			{
				mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - Inline executable attachement, Please see: %s#attachmentinline",gPolicyUrl);
				asprintf(&reason,"Attachement name '%s'",attachfname);
				mlfi_status_debug(priv,&rs,LOG_REJECTED_STR,reason,"mlfi_header: Attachement Reject\n");
				free(reason);
			}
		}

		if(attachfname != NULL)
			free(attachfname);
	}

	return rs;
}

typedef struct _cpr_t
{
	SMFICTX *ctx;
	sfsistat *prs;
	char **ppReason;
	int *pbContinueChecks;
}cpr_t;

#ifdef SUPPORT_DBL
int dblCheckCallbackHlo(const dblcb_t *pDblcb)
{	cpr_t *pcpr = (cpr_t *)pDblcb->pDblq->pCallbackData;
	SMFICTX *ctx = pcpr->ctx;
	mlfiPriv *priv = MLFIPRIV;

	mlfi_debug(priv->pSessionUuidStr,"HLO dbl_check '%s' - listed - %s via '%s'\n"
		,pDblcb->pDblq->pDomain
		,pDblcb->pDblResult
		,pDblcb->pDbl
		);

	mlfi_setreply(ctx, 550, "5.7.1", "Rejecting due to security policy - Blacklisted host, Please see: %s#blacklistedhostdbl", gPolicyUrl);
	asprintf(pcpr->ppReason, "Blacklisted host DBL '%s' via '%s'", pDblcb->pDblq->pDomain, pDblcb->pDbl);
	mlfi_status_debug(priv, pcpr->prs, LOG_REJECTED_STR, *pcpr->ppReason, "mlfi_hndlrs: Blacklisted host DBL\n");
	free(*pcpr->ppReason);
	*pcpr->pbContinueChecks = 0;

	return 0; // not again
}
#endif

#ifdef SUPPORT_GEOIP
typedef struct _lcgr_t
{
	cpr_t cpr;
	int *pContinueChecks;
}lcgr_t;

int listCallbackGeoipReject(void *pData, void *pCallbackData)
{	geoipResult *pResult = (geoipResult *)pData;
	lcgr_t *plcgr = (lcgr_t *)pCallbackData;
	SMFICTX *ctx = plcgr->cpr.ctx;
	mlfiPriv *priv = MLFIPRIV;
	int *pContinueChecks = plcgr->pContinueChecks;
	int action = geoip_query_action_cc(ctx, pResult->pCountryCode);

	asprintf(plcgr->cpr.ppReason,"Blacklisted Country Code '%s'",pResult->pCountryCode);
	mlfi_debug(priv->pSessionUuidStr,"mlfi_hndlrs: GeoIP action = %u/'%s'\n",action,gpBwlStrs[action]);
	switch(action)
	{
		case GEOIPLIST_A_TARPIT:
			sleep(120);	// slow the bastard down!
			// deliberate fall thru to GEOIPLIST_A_REJECT
		case GEOIPLIST_A_REJECT:
			mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - "
				"Country Code '%s' has been blacklisted, Please see: %s#blacklistedcountrycode"
				,pResult->pCountryCode,gPolicyUrl
				);
			*pContinueChecks = mlfi_status_debug(priv,plcgr->cpr.prs,LOG_REJECTED_STR,*plcgr->cpr.ppReason
				,"mlfi_hndlrs: Blacklisted Country Code, '%s'\n",pResult->pCountryCode);
			if(gMtaHostIpfw)
				mlfi_MtaHostIpfwAction(priv,"add");
			break;
		case GEOIPLIST_A_TEMPFAIL:
			*pContinueChecks = mlfi_status_debug(priv,plcgr->cpr.prs,LOG_TEMPFAILED_STR,*plcgr->cpr.ppReason,NULL);
			break;
		case GEOIPLIST_A_DISCARD:
			 *pContinueChecks = mlfi_status_debug(priv,plcgr->cpr.prs,LOG_DISCARDED_STR,*plcgr->cpr.ppReason,NULL);
			break;
		case GEOIPLIST_A_ACCEPT:
			// take no further action
			 *pContinueChecks = 0;
			break;
		//case GEOIPLIST_A_EXEC:
		//	rs = mlfi_hndlr_exec(ctx,RBL_S_FROM);
		//	 *pContinueCecks = (plcgr->cpr.prs == SMFIS_CONTINUE);
		//	break;
		default:
			// deliberate fall thru to GEOIPLIST_A_NULL case
		case GEOIPLIST_A_NULL:
			if(priv->isAuthEn)
				 *pContinueChecks = 0;
			break;
	}
	free(*plcgr->cpr.ppReason);

	return *pContinueChecks;
}
#endif

sfsistat mlfi_hndlrs(SMFICTX *ctx)
{	mlfiPriv *priv = MLFIPRIV;
	sfsistat rs = SMFIS_CONTINUE;

	mlfi_debug(priv->pSessionUuidStr,"mlfi_hndlrs\n");

	if(priv != NULL
		&& !(priv->islocalnethost || priv->isAuthEn)
		)
	{	int x,continue_checks = 1;
		char *reason = NULL;
		int ipfwMtaHostIpBanCandidate = 0;

#ifdef SUPPORT_FWDHOSTCHK
		// Policy enforcment
		// Can we deliver to the recipient ?
		if(continue_checks
			&& gRcptFwdHostChk
			)
		{	
			int smtprc = 0;
			int fhlrc, deliverable;
			char *rcptMbox = NULL;
			char *rcptDom = NULL;

			mlfi_regex_mboxsplit(priv->rcpt,&rcptMbox,&rcptDom);
			
			fhlrc = fwdhostlist_is_deliverable(priv,rcptMbox,rcptDom,&smtprc);
			deliverable = (fhlrc !=  0);

			mlfi_debug(priv->pSessionUuidStr,"mlfi_hndlrs: FwdHostChk '%s@%s' = %d, deliverable %d, smtprc %d\n",rcptMbox,rcptDom,fhlrc,deliverable,smtprc);
			if(!deliverable)
			{
				if(smtprc == -1 || smtprc/100 == 4)
				{
					if(smtprc == -1)
						smtprc = 450;
					mlfi_setreply(ctx,smtprc,"4.7.1","Temporary failure - Unable to validate recipient, Please see: %s#tempfailinvalidrecipient",gPolicyUrl);
					continue_checks = mlfi_status_debug(priv,&rs,LOG_TEMPFAILED_STR,"TempFailed","Recipient address verification",NULL);
				}
				else
				{
					mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - Recipient undeliverable, Please see: %s#undeliverablerecipient",gPolicyUrl);
					asprintf(&reason,"Undeliverable recipient %d",smtprc);
					continue_checks = mlfi_status_debug(priv,&rs,LOG_REJECTED_STR,reason,NULL);
					free(reason);
				}
			}
			free(rcptMbox);
			free(rcptDom);
		}
#endif

		// Policy enforcement
		// How should we handle the sender ?
		if(continue_checks
			//&& gxxxChk - TODO - should this be configurable ?
//			&& priv->rcptaction == BWL_A_NULL
			)
		{
			asprintf(&reason,"Blacklisted Sender '%s'",priv->sndr);
			mlfi_debug(priv->pSessionUuidStr,"mlfi_hndlrs: switch(sndraction) %u/'%s'\n",priv->sndraction,gpBwlStrs[priv->sndraction]);

			// this is done here instead of in envfrom so that the recpient can be collected for logging purposes
			switch(priv->sndraction)
			{
				case BWL_A_TARPIT:
					sleep(120);	// slow the bastard down!
					// deliberate fall thru to BWL_A_REJECT
				case BWL_A_REJECT:
					mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - Sender '%s' has been blacklisted, Please see: %s#blacklistedsender",priv->sndr,gPolicyUrl);
					continue_checks = mlfi_status_debug(priv,&rs,LOG_REJECTED_STR,reason
						,"mlfi_hndlrs: Blacklisted SMTP Sender, '%s'\n",priv->sndr);
					ipfwMtaHostIpBanCandidate = 1;
					break;
				case BWL_A_TEMPFAIL:
					continue_checks = mlfi_status_debug(priv,&rs,LOG_TEMPFAILED_STR,reason,NULL);
					break;
				case BWL_A_DISCARD:
					continue_checks = mlfi_status_debug(priv,&rs,LOG_DISCARDED_STR,reason,NULL);
					break;
				case BWL_A_ACCEPT:
					// take no further action
					continue_checks = 0;
					break;
				case BWL_A_EXEC:
					rs = mlfi_hndlr_exec(ctx,RBL_S_FROM);
					continue_checks = (rs == SMFIS_CONTINUE);
					break;
				default:
					priv->sndraction = BWL_A_NULL;
					// deliberate fall thru to BWL_A_NULL case
				case BWL_A_NULL:
					if(priv->isAuthEn)
						continue_checks = 0;
					break;
			}
			free(reason);
		}

		// Technical enforcement
		// The HLO MTA hostname should not be an ip address
		if(continue_checks
			&& (gMtaHostChk || gMtaHostChkAsIp)
			&& priv->helo != NULL
			&& inet_addr(priv->helo) != INADDR_NONE
			)
		{
			mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - Ip address used as MTA hostname '%s', Please see: %s#ipusedashostname",priv->helo,gPolicyUrl);
			asprintf(&reason,"Ip address used as MTA hostname '%s'",priv->helo);
			continue_checks = mlfi_status_debug(priv,&rs,LOG_REJECTED_STR,reason
				,"mlfi_hndlrs: Ip address used as MTA hostname '%s'\n",priv->helo);
			free(reason);

			ipfwMtaHostIpBanCandidate = 1;
			//if(gMtaHostIpfw)
			//	mlfi_MtaHostIpfwAction(priv,"add");
		}

		// Technical enforcement
		// The HLO MTA hostname should not be that of the recipient's domain
		if(continue_checks
			&& gMtaHostChk
			&& priv->helo != NULL
			)
		{
			char *mbox = NULL;
			char *dom = NULL;

			if(mlfi_regex_mboxsplit(priv->rcpt,&mbox,&dom)
				&& dom != NULL
				&& strcmp(priv->helo,dom) == 0
				)
			{
				mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - Invalid source hostname '%s', Please see: %s#invalidsourcehostname",priv->helo,gPolicyUrl);
				asprintf(&reason,"Invalid source MTA hostname '%s'",priv->helo);
				continue_checks = mlfi_status_debug(priv,&rs,LOG_REJECTED_STR,reason
					,"mlfi_hndlrs: Invalid source MTA hostname '%s'\n",priv->helo);
				free(reason);

				ipfwMtaHostIpBanCandidate = 1;
				//if(gMtaHostIpfw)
				//	mlfi_MtaHostIpfwAction(priv,"add");
			}

			free(mbox);
			free(dom);
		}

		// Technical enforcement - in reality, this varies wildly
		// The HLO MTA hostname should resolve to an ip address
		if(continue_checks
			&& gMtaHostChk
			&& !dns_query_rr(priv->presstate, ns_t_a, priv->helo)
			&& !dns_query_rr(priv->presstate, ns_t_aaaa, priv->helo)
			)
		{
			mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - Invalid hostname '%s', Please see: %s#invalidhostname",priv->helo,gPolicyUrl);
			asprintf(&reason,"Invalid MTA hostname '%s'",priv->helo);
			continue_checks = mlfi_status_debug(priv,&rs,LOG_REJECTED_STR,reason
				,"mlfi_hndlrs: Invalid MTA hostname '%s'\n",priv->helo);
			free(reason);

			ipfwMtaHostIpBanCandidate = 1;
			//if(gMtaHostIpfw)
			//	mlfi_MtaHostIpfwAction(priv,"add");
		}

		// Policy enforcement
		// Do BWL checking based on the HLO MTA hostname
		if(continue_checks
			&& gMtaHostChk
			&& (x = bwlistActionQuery(priv->pbwlistctx,BWL_L_SNDR,priv->helo,"",priv->sndractionexec)) != BWL_A_ACCEPT
			&& x != BWL_A_NULL
			)
		{
			asprintf(&reason,"Blacklisted host '%s'",priv->helo);

			switch(x)
			{
				case BWL_A_TARPIT:
					sleep(120);	// slow the bastard down!
					// deliberate fall thru to BWL_A_REJECT
				case BWL_A_REJECT:
					mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - Blacklisted host, Please see: %s#blacklistedhost",gPolicyUrl);
					continue_checks = mlfi_status_debug(priv,&rs,LOG_REJECTED_STR,reason
						,"mlfi_hndlrs: Blacklisted host '%s'\n",priv->helo);
					ipfwMtaHostIpBanCandidate = 1;
					break;
				case BWL_A_TEMPFAIL:
					continue_checks = mlfi_status_debug(priv,&rs,LOG_TEMPFAILED_STR,reason,NULL);
					break;
				case BWL_A_DISCARD:
					continue_checks = mlfi_status_debug(priv,&rs,LOG_DISCARDED_STR,reason,NULL);
					break;
				case BWL_A_EXEC:
					rs = mlfi_hndlr_exec(ctx,RBL_S_CONN);
					continue_checks = (rs == SMFIS_CONTINUE);
					break;
			}
			free(reason);
		}

#ifdef SUPPORT_DBL
		// Policy enforcement
		// Do DBL checking base on the HLO MTA hostname
		if(continue_checks
			&& gMtaHostChk
		)
		{	cpr_t cpr;
			dblq_t dblq;
		
			cpr.ctx = ctx;
			cpr.prs = &rs;
			cpr.ppReason = &reason;
			cpr.pbContinueChecks = &continue_checks;

			dblq.pDomain = priv->helo;
			dblq.pCallbackFn = &dblCheckCallbackHlo;
			dblq.pCallbackData = &cpr;
			dblq.pCallbackPolicyFn = &dbl_callback_policy_std;

			dbl_check_all(priv->pDblCtx, priv->presstate, &dblq);
			continue_checks = (rs == SMFIS_CONTINUE);
			if(continue_checks)
				mlfi_debug(priv->pSessionUuidStr,"HLO dbl_check '%s' - not listed\n",priv->helo);
		}
#endif

		// Technical enforcement - in reality, this varies wildly
		// The HLO MTA hostname should match the connecting ip address
		if(continue_checks
			&& gMtaHostIpChk
			&& strcmp(priv->helo,priv->iphostname) != 0
			&& !dns_hostname_ip_match_sa(priv->presstate, priv->helo, priv->pip)
			)
		{
			mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - Helo hostname/ip mismatch, Please see: %s#hostnameipmismatch",gPolicyUrl);
			asprintf(&reason,"Hostname/ip mismatch '%s'",priv->helo);
			continue_checks = mlfi_status_debug(priv,&rs,LOG_REJECTED_STR,reason
				,"mlfi_hndlrs: Helo hostname/ip mismatch\n");
			free(reason);
		}

#if defined(SUPPORT_LIBSPF) || defined(SUPPORT_LIBSPF2)
		// Policy enforcement
		// The MTA should pass SPF tests
		if(continue_checks
			&& gMtaSpfChk
			&& mlfi_spf_reject(priv,&rs) == SMFIS_REJECT
			)
		{
			mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - SPF %s - %s", priv->spf_rs, priv->spf_explain);
			asprintf(&reason,"SPF %s: '%s'",priv->spf_rs, priv->spf_error);
			continue_checks = mlfi_status_debug(priv, &rs, LOG_REJECTED_STR, reason
				,"mlfi_hndlrs: SPF %s: '%s'\n", priv->spf_rs, priv->spf_error);
			free(reason);
		}
#endif

		// Policy enforcement
		// DNS RBL checking - stage FROM
		if(continue_checks
			//&& gxxxChk - TODO - should this be configurable ?
			)
		{
			rs = mlfi_rdnsbl_reject(ctx, &rs, RBL_S_FROM, priv->pip, priv->pdnsrblhosts, &priv->pdnsrblmatch);
			continue_checks = (rs == SMFIS_CONTINUE);
		}

		// Policy enforcement
		// Validate the sndr return path
		if(continue_checks && gSmtpSndrChk)
		{	int isLoopBack = 0;

			switch(priv->pip->sa_family)
			{
				case AF_INET:
					isLoopBack = (ntohl(((struct sockaddr_in *)priv->pip)->sin_addr.s_addr) == INADDR_LOOPBACK);
					break;
				case AF_INET6:
					isLoopBack = IN6_ARE_ADDR_EQUAL( &((struct sockaddr_in6 *)priv->pip)->sin6_addr, &in6addr_loopback);
					break;
			}

			if(!isLoopBack && strcasecmp("<>",priv->sndr) != 0)
			{
				char *mbox = NULL;
				char *dom = NULL;

				mlfi_regex_mboxsplit(priv->sndr,&mbox,&dom);

				rs = mlfi_sndrchk(ctx,mbox,dom);
				continue_checks = (rs == SMFIS_CONTINUE);
				free(mbox);
				free(dom);
			}
		}

#ifdef SUPPORT_GEOIP
		// Policy enforcement
		// Do "Recieved by/from" header ip address resolution to GeoIP CountryCode checks
		if(continue_checks
			&& gGeoIpCcChk
			&& listQty(priv->pGeoipList) > 0
			)
		{	lcgr_t lcgr;

			lcgr.cpr.ctx = ctx;
			lcgr.cpr.prs = &rs;
			lcgr.cpr.ppReason = &reason;
			lcgr.pContinueChecks = &continue_checks;

			listForEach(priv->pGeoipList,&listCallbackGeoipReject,(void *)&lcgr);
		}
#endif

		// Policy enforcement
		// Do local content filtering checks
		if(continue_checks
			//&& gxxxChk - TODO - should this be configurable ?
			&& mlfi_contentfilter(priv,&rs,RBL_S_FROM) == SMFIS_REJECT
			)
		{
			mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - unathorized sender");
			continue_checks = mlfi_status_debug(priv,&rs,LOG_REJECTED_STR,"Unauthorized sender"
				,"mlfi_hndlrs: Unauthorized sender\n");
		}

		if(continue_checks
			//&& gReplyToChk
		)
		{
			rs = mlfi_replyto(ctx);
			continue_checks = (rs == SMFIS_CONTINUE && priv->rcptaction == BWL_A_NULL);
		}

		// Greylisting should be the last check ever done
#ifdef SUPPORT_GREYLIST
		// Policy enforcement
		// Do greylist checking
		if(continue_checks
			//&& gxxxChk - TODO - should this be configurable ?
			)
		{	int greyRc = mlfi_greylist(ctx);

			switch(greyRc)
			{
				case -2: // network connectivity issue
				case -1: // network timeout
				case 0: // status unknown
				case 2: // is white
					mlfi_debug(priv->pSessionUuidStr,"mlfi_hndlrs: Grey List - %d/ok\n",greyRc);
					break;
				case 1: // is grey
					mlfi_setreply(ctx,450,"4.7.1","Temporary failure - Grey Listed");
					continue_checks = mlfi_status_debug(priv,&rs,LOG_TEMPFAILED_STR,"Grey Listed"
						,"mlfi_hndlrs: Grey List - grey\n");
					break;
			}
		}
#endif
		if(gMtaHostIpfw && ipfwMtaHostIpBanCandidate)
			mlfi_MtaHostIpfwAction(priv,"add");
	}

	return rs;
}

sfsistat mlfi_eoh(SMFICTX *ctx)
{
	return mlfi_hndlrs(ctx);
}

sfsistat mlfi_body(SMFICTX *ctx, u_char *bodyp, size_t bodylen)
{	mlfiPriv *priv = MLFIPRIV;
	sfsistat rs = SMFIS_CONTINUE;

	// collect the body
	if (priv != NULL)
	{
		// scan for (http|https|ftp) links and store them for analysis later
		// NOTE:
		// Any body content checking or Content-Transfer-Encoding decoding
		// done here is problematic, and MUST be flying. ie. It must not depend
		// on being the first and only call to mlfi_body for this session
		// because this is called multiple times as the content is packetized /
		// chunked from the transport layer up through sendmail and eventually
		// to us.
		//
		switch(priv->bodyTransferEncoding)
		{
/*
			case BODYENCODING_UNKNOWN:
				break;

			case BODYENCODING_BASE64:
				{	char *pbodystr = (char *)uuDecode((unsigned char *)bodyp);

					if(pbodystr != NULL)
					{
						mlfi_regex_line_http(priv->pSessionUuidStr,(const char *)bodyp,priv->pListBodyHosts);
						free(pbodystr);
					}
				}
				break;
*/

			case BODYENCODING_NONE:
				// WARNING: doing this here ignores the possiblity that a URL might overlap the body chunk
				mlfi_regex_line_http(priv->pSessionUuidStr,(const char *)bodyp,priv->pListBodyHosts);
				break;
		}

		if(gMsExtChk)
		{	char	*p = (char *)realloc(priv->body, priv->bodylen + bodylen + 1);

			if(p != NULL)
			{
				priv->body = p;
				memcpy((priv->body+priv->bodylen), bodyp, bodylen);
				priv->bodylen += bodylen;
				priv->body[priv->bodylen] = '\0';
			}
		}
	}

	return(rs);
}

int mlfi_prependSubjectHeader(SMFICTX *ctx, char *fmt, ...)
{	mlfiPriv	*priv = MLFIPRIV;
	int		rc = 0;
	char		*str = NULL;
	va_list		vl;

	if(ctx != NULL && priv != NULL && fmt != NULL)
	{
		va_start(vl,fmt);
		rc = vasprintf(&str,fmt,vl);
		va_end(vl);

		if(str != NULL)
		{
			if(priv->header_subject != NULL)
			{	char	*s;

				asprintf(&s,"%s - %s",str,priv->header_subject);
				if(s != NULL)
				{
					smfi_chgheader(ctx,"Subject",1,s);
					free(s);
				}
			}
			else
				smfi_addheader(ctx,"Subject",str);
			free(str);
		}
		else
			rc = -1;
	}

	return(rc);
}

#ifdef SUPPORT_DBL
int dblCheckCallbackBody(const dblcb_t *pDblcb)
{	cpr_t *pcpr = (cpr_t *)pDblcb->pDblq->pCallbackData;
	SMFICTX *ctx = pcpr->ctx;
	mlfiPriv *priv = MLFIPRIV;

	mlfi_debug(priv->pSessionUuidStr, "dbl_check '%s' - listed - %s\n"
		,pDblcb->pDblq->pDomain
		,pDblcb->pDblResult
		);

	mlfi_setreply(ctx, 550, "5.7.1", "Rejecting due to security policy - Blacklisted body host, Please see: %s#bodyurldbl", gPolicyUrl);
	asprintf(pcpr->ppReason, "Body URL DBL host '%s' via '%s'", pDblcb->pDblq->pDomain, pDblcb->pDbl);
	mlfi_status_debug(priv, pcpr->prs, LOG_REJECTED_STR, *pcpr->ppReason, "mlfi_eom: Body URL DBL host\n");
	free(*pcpr->ppReason);
	*pcpr->prs = SMFIS_REJECT;
	*pcpr->pbContinueChecks = 0;

	return 0; // not again
}
#endif

typedef struct _lcbh_t
{
	cpr_t cpr;
	int bContinueChecks;
	int bDoRejects;
} lcbh_t;

int listCallbackBodyHosts(void *pData, void *pCallbackData)
{	const char *pStr = (const char *)pData;
	lcbh_t *pLcbh = (lcbh_t *)pCallbackData;
	SMFICTX *ctx = pLcbh->cpr.ctx;
	mlfiPriv *priv = MLFIPRIV;

	if(pStr != NULL && *pStr)
	{
		// add the stored body links as headers to the email for possible analysis by others
		mlfi_debug(priv->pSessionUuidStr,"mlfi_eom: Adding BodyHosts header: '%s'\n",pStr);
		mlfi_addhdr_printf(ctx,"X-Milter", "%s DataSet=BodyHost; reciever=%s; host='%s';"
			,mlfi.xxfi_name ,gHostname
			,pStr
			);

		if(
			pLcbh->bDoRejects
			&& priv->sndraction != BWL_A_ACCEPT
			&& priv->rcptaction != BWL_A_ACCEPT
			&& !priv->islocalnethost
			)
		{
			// sender BlackList testing
			switch(bwlistActionQuery(priv->pbwlistctx,BWL_L_SNDR,(char *)pStr,"",NULL))
			{
				case BWL_A_REJECT:
				case BWL_A_TARPIT:
				case BWL_A_DISCARD:
					mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - Blacklisted body URL, Please see: %s#bodyurl",gPolicyUrl);
					asprintf(pLcbh->cpr.ppReason,"Body URL host '%s'",pStr);
					mlfi_status_debug(priv,pLcbh->cpr.prs,LOG_REJECTED_STR,*pLcbh->cpr.ppReason,"mlfi_eom: Body URL host\n");
					free(*pLcbh->cpr.ppReason);
					*pLcbh->cpr.prs = SMFIS_REJECT;
					*pLcbh->cpr.pbContinueChecks = 0;
					break;
				case BWL_A_TEMPFAIL:
					break;
				case BWL_A_EXEC:
				case BWL_A_ACCEPT:
				case BWL_A_NULL:
					break;
			}

#ifdef SUPPORT_DBL
			if(*pLcbh->cpr.pbContinueChecks) // if BL testing passes
			{
				dblq_t dblq;

				dblq.pDomain = pStr;
				dblq.pCallbackFn = &dblCheckCallbackBody;
				dblq.pCallbackData = &pLcbh->cpr;
				dblq.pCallbackPolicyFn = &dbl_callback_policy_std;

				dbl_check_all(priv->pDblCtx, priv->presstate, &dblq);
				if(*pLcbh->cpr.pbContinueChecks)
					mlfi_debug(priv->pSessionUuidStr,"dbl_check '%s' - not listed\n",pStr);
			}
#endif

#ifdef SUPPORT_GEOIP
			if(*pLcbh->cpr.pbContinueChecks) // if BL testing passes, GEOIP testing
			{	struct hostent *phostent = gethostbyname(pStr);

				if(phostent != NULL && !mlfi_isNonRoutableIpHostEnt(phostent))
				{	const char *pCC = geoip_LookupCCByHostEnt(ctx, phostent);

					switch(geoip_query_action_cc(ctx, pCC))
					{
						case GEOIPLIST_A_REJECT:
						case GEOIPLIST_A_TARPIT:
						case GEOIPLIST_A_DISCARD:
							mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - Blacklisted body URL Country Code, Please see: %s#bodyurlcc", gPolicyUrl);
							asprintf(pLcbh->cpr.ppReason,"Body URL Country Code '%s' for host '%s'", pCC, pStr);
							mlfi_status_debug(priv, pLcbh->cpr.prs, LOG_REJECTED_STR, *pLcbh->cpr.ppReason, "mlfi_eom: Body URL host Country Code\n");
							free(*pLcbh->cpr.ppReason);
							*pLcbh->cpr.prs = SMFIS_REJECT;
							*pLcbh->cpr.pbContinueChecks = 0;
							break;
						case GEOIPLIST_A_TEMPFAIL:
							break;
						//case GEOIPLIST_A_EXEC:
						case GEOIPLIST_A_ACCEPT:
						case GEOIPLIST_A_NULL:
							break;
					}
				}
			}
#endif
		}
	}

	return *pLcbh->cpr.pbContinueChecks; // again
}

#ifdef SUPPORT_GEOIP
int listCallbackGeoipAddHdr(void *pData, void *pCallbackData)
{	geoipResult *pResult = (geoipResult *)pData;
	SMFICTX *ctx = (SMFICTX *)pCallbackData;
	char *pIpStr = mlfi_inet_ntopAF(pResult->ip.afType, (pResult->ip.afType == AF_INET ? (char *)&pResult->ip.ipv4 : (char *)&pResult->ip.ipv6));

	mlfi_addhdr_printf(ctx,"X-Milter", "%s DataSet=GeoIP; reciever=%s; ip=%s; CC=%s;"
		,mlfi.xxfi_name ,gHostname
		,pIpStr
		,(pResult->pCountryCity != NULL ? pResult->pCountryCity : pResult->pCountryCode)
	);

	return 1;
}
#endif

sfsistat mlfi_eom(SMFICTX *ctx)
{	mlfiPriv *priv = MLFIPRIV;
	sfsistat rs = SMFIS_CONTINUE;
	RBLHOST **prbls = NULL;
	char *attachfname = NULL;
	char *reason = NULL;
	int tag = 0;
	int continue_checks = 1;

	// per message, either _eom or _abort, not both
	if (priv != NULL)
	{
		mlfi_addhdr_printf(ctx,"X-Milter", "%s DataSet=SessionId; reciever=%s; sessionid='%s';"
			,mlfi.xxfi_name ,gHostname
			,priv->pSessionUuidStr
			);

		if(priv->sndraction != BWL_A_ACCEPT && priv->rcptaction != BWL_A_ACCEPT)
		{
			if(rs == SMFIS_CONTINUE)
			{
				priv->MsExtFound |= (gMsExtChk && priv->body != NULL && mlfi_findBadExtBody(priv->badextlist,priv->badextqty,priv->body,&attachfname));

				if(priv->MsExtFound)
				{
					if(strcasecmp(gMsExtChkAction,"Reject") == 0)
					{
						mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - Executable attachement, Please see: %s#attachmentbody",gPolicyUrl);
						asprintf(&reason,"Attachement name '%s'",attachfname);
						mlfi_status_debug(priv,&rs,LOG_REJECTED_STR,reason,"mlfi_eom: Attachement Reject\n");
						free(reason);
						continue_checks = 0;
					}
					else if(strcasecmp(gMsExtChkAction,"Tag") == 0)
					{
						mlfi_prependSubjectHeader(ctx,"Virus ?");

						mlfi_status_debug(priv,NULL,"Virus",NULL,"mlfi_eom: Flagged - Virus\n");
						tag++;
					}
				}
			}

			if(rs == SMFIS_CONTINUE && !priv->islocalnethost && mlfi_contentfilter(priv,&rs,RBL_S_EOM) == SMFIS_REJECT)
			{
				mlfi_setreply(ctx,550,"5.7.1","Rejecting due to security policy - Content violation");
				mlfi_status_debug(priv,&rs,LOG_REJECTED_STR,"Content violation","mlfi_eom: Content violation\n");
				continue_checks = 0;
			}


			if(rs == SMFIS_CONTINUE)
			{
				if(priv->SmtpSndrChkFail && strcasecmp(gSmtpSndrChkAction,"Tag") == 0)
				{
					mlfi_prependSubjectHeader(ctx,"Valid Sender ?");

					mlfi_status_debug(priv,NULL,"Sender",NULL,"mlfi_eom: Flagged - Invalid Sender\n");
					tag++;
				}

				if(priv->dnsblcount > 0)
				{
					mlfi_addhdr_printf(ctx,"X-Milter","%s DataSet=RDNSBL-Count; receiver=%s; count=%d"
						,mlfi.xxfi_name ,gHostname
						,priv->dnsblcount
						);
					for(prbls=(priv->pdnsrblmatch != NULL ? priv->pdnsrblmatch->ppmatch : NULL); prbls != NULL && *prbls != NULL; prbls++)
					{
						//if((*prbls)->stage == RBL_S_EOM && (*prbls)->action == RBL_A_TAG)
						if((*prbls)->stage == RBL_S_EOM)
							dnsbl_add_hdr(ctx,*prbls);
					}

					mlfi_prependSubjectHeader(ctx,"Spam ?");

					mlfi_status_debug(priv,NULL,"Spam",NULL,"mlfi_eom: Flagged - Spam\n");
					tag++;
				}
#if defined(SUPPORT_LIBSPF) || defined(SUPPORT_LIBSPF2)
				if(gMtaSpfChk && priv->spf_rs != NULL)
				{
					mlfi_addhdr_printf(ctx,"X-Milter", "%s DataSet=SPF; reciever=%s; client-ip=%s; envelope-from=%s; helo=%s; assesment=%s (%s);"
						,mlfi.xxfi_name,gHostname
						,priv->ipstr,priv->sndr,priv->helo
						,priv->spf_rs,priv->spf_error
						);
				}
#endif
				if(tag)
				{
					if(priv->header_xstatus != NULL)
						smfi_chgheader(ctx,"X-Status",1,"F");
					else
						smfi_addheader(ctx,"X-Status","F");

					smfi_addheader(ctx,"X-Keywords","$Label5");
				}

				// TODO - this is wrong in the context of the new Body URL rejection behavior below
				mlfi_status_debug(priv,NULL,LOG_ACCEPTED_STR,NULL,"mlfi_eom: Accepted\n");
			}
			else
				mlfi_debug(priv->pSessionUuidStr,"mlfi_eom: Rejected\n");

			if(attachfname != NULL)
				free(attachfname);
		}
		else
			// TODO - this is wrong in the context of the new Body URL rejection behavior below
			mlfi_status_debug(priv,NULL,LOG_ACCEPTED_STR,NULL,"mlfi_eom: Accepted\n");
	}

	// iterate the stored body links ?
	if(priv != NULL && listQty(priv->pListBodyHosts))
	{
		lcbh_t lcbh;

		lcbh.cpr.ctx = ctx;
		lcbh.cpr.prs = &rs;
		lcbh.cpr.ppReason = &reason;
		lcbh.cpr.pbContinueChecks = &lcbh.bContinueChecks;
		lcbh.bContinueChecks = lcbh.bDoRejects = continue_checks;

		listForEach(priv->pListBodyHosts,&listCallbackBodyHosts,&lcbh);
	}

#ifdef SUPPORT_GEOIP
	listForEach(priv->pGeoipList,&listCallbackGeoipAddHdr,(void *)ctx);
#endif

	mlfi_addhdr_printf(ctx,"X-Milter","%s DataSet=MTA-Peer; reciever=%s; sender-ip=%s; sender-helo=%s;"
		,mlfi.xxfi_name,gHostname,priv->ipstr,priv->helo
		);

	dupe_action(ctx,priv);

	if(rs == SMFIS_CONTINUE && gMtaHostIpfwNominate && !priv->islocalnethost)
		mlfi_MtaHostIpfwAction(priv,"exculpate");

	mlfi_cleanup(ctx);

	return rs;
}

sfsistat mlfi_abort(SMFICTX *ctx)
{	mlfiPriv *priv = MLFIPRIV;

	mlfi_debug(priv->pSessionUuidStr,"mlfi_abort\n");

	return mlfi_cleanup(ctx);
}

sfsistat mlfi_close(SMFICTX *ctx)
{	mlfiPriv *priv = MLFIPRIV;

	mlfi_debug(priv->pSessionUuidStr,"mlfi_close\n");

	if(priv != NULL)
	{
		if(priv->helo != NULL)
			free(priv->helo);
		if(priv->iphostname != NULL)
			free(priv->iphostname);
		if(priv->pip != NULL)
			free(priv->pip);
		if(priv->ipstr != NULL)
			free(priv->ipstr);
		if(priv->presstate != NULL)
		{
			res_nclose(priv->presstate);
			free(priv->presstate);
		}
#ifdef SUPPORT_GEOIP
		geoip_close(ctx);
#endif
#ifdef SUPPORT_FWDHOSTCHK
		fwdhostlist_close(priv);
#endif
		dbl_Close(priv->pDblCtx);
		dbl_Destroy(&priv->pDblCtx);
		bwlistClose(priv->pbwlistctx);
		bwlistDestroy(&priv->pbwlistctx);
		dnsbl_free_hosts(priv->pdnsrblhosts);
		dnsbl_free_match(priv->pdnsrblmatch);
		badext_close(priv);

		// should be last so that mlfi_debug continues to show session id
		if(priv->pSessionUuidStr != NULL)
		{
			free(priv->pSessionUuidStr);
			priv->pSessionUuidStr = NULL;
		}

		free(priv);
	}
	smfi_setpriv(ctx, NULL);

	return mlfi_cleanup(ctx);
}

int listCallbackDestroyBodyHosts(void *pData, void *pCallbackData)
{
	if(pData != NULL)
		free(pData);

	return 1; // again
}

sfsistat mlfi_cleanup(SMFICTX *ctx)
{	mlfiPriv *priv = MLFIPRIV;
	sfsistat rs = SMFIS_CONTINUE;

	// cleanup per message stuff, called from _abort or _eom
	if(priv != NULL)
	{
		if(priv->header_xstatus != NULL)
		{
			free(priv->header_xstatus);
			priv->header_xstatus = NULL;
		}
		if(priv->sndr != NULL)
		{
			free(priv->sndr);
			priv->sndr = NULL;
		}
		if(priv->rcpt != NULL)
		{
			free(priv->rcpt);
			priv->rcpt = NULL;
		}
		if(priv->header_subject != NULL)
		{
			free(priv->header_subject);
			priv->header_subject = NULL;
		}
		if(priv->header_replyto != NULL)
		{
			free(priv->header_replyto);
			priv->header_replyto = NULL;
		}
		if(priv->replyto != NULL)
		{
			free(priv->replyto);
			priv->replyto = NULL;
		}
		if(priv->body != NULL)
		{
			free(priv->body);
			priv->body = NULL;
		}
		if(priv->pListBodyHosts != NULL)
		{
			listDestroy(priv->pListBodyHosts,&listCallbackDestroyBodyHosts,NULL);
			priv->pListBodyHosts = NULL;
		}
#if defined(SUPPORT_LIBSPF) || defined(SUPPORT_LIBSPF2)
		if(priv->spf_rs != NULL)
		{
			free(priv->spf_rs);
			priv->spf_rs = NULL;
		}
		if(priv->spf_error != NULL)
		{
			free(priv->spf_error);
			priv->spf_error = NULL;
		}
		if(priv->spf_explain != NULL)
		{
			free(priv->spf_explain);
			priv->spf_explain = NULL;
		}
#endif
	}

	return rs;
}
