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
 *	CVSID:  $Id: smtp.c,v 1.30 2012/11/23 03:27:01 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		smtp.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: smtp.c,v 1.30 2012/11/23 03:27:01 neal Exp $";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sysexits.h>
#include <string.h>
#include <stdarg.h>

#include "spamilter.h"
#include "mx.h"
#include "inet.h"
#include "smtp.h"
#include "ifi.h"
#include "misc.h"

int smtp_get_resp(const char *pSessionId, int sd, int timeout, int rcode)
{	char	buf[8192];
	int	l;
	int	rc = 0;

	do
	{
		l = NetSockGets(sd, buf, sizeof(buf), timeout);
	} while(l > 0 && buf[3] == '-');

	if(l > 0 && buf[3] == ' ')
	{
		buf[3] = '\0';
		rc = atoi(buf);
		if(rc != rcode)
		{
			buf[3]= ' ';
			mlfi_debug(pSessionId,"\t\t'%s'\n",buf);
		}
	
	}
	else
		rc = -1;

	return(rc);
}

int smtp_send_get_resp(const char *pSessionId, int sd, int timeout, int rcode, char *fmt, ...)
{	va_list	vl;
	int	rc = 0;

	va_start(vl,fmt);

	rc = NetSockVPrintf(sd,fmt,vl);
	if(rc > 0)
		rc = smtp_get_resp(pSessionId,sd,timeout,rcode);
	else
		mlfi_debug(pSessionId,"smtp_send_get_resp: send failure\n");

	va_end(vl);

	return(rc);
}

int smtp_host_test_mailfrom_rcptto(const char *pSessionId, int sd, int timeout, const char *mailfrom, const char *mbox, const char *dom, int *smtprc)
{	int	rc = -1;

	if((*smtprc = smtp_send_get_resp(pSessionId,sd,timeout,250,"helo %s\r\n",gHostname)) == 250 &&
		(*smtprc = smtp_send_get_resp(pSessionId,sd,timeout,250,"mail from:<%s>\r\n",mailfrom)) == 250 &&
		(*smtprc = smtp_send_get_resp(pSessionId,sd,timeout,250,"rcpt to:<%s@%s>\r\n",mbox,dom)) == 250)
		rc = 1;
	else
		rc = (*smtprc > 0 ? 0 : -1);

	return(rc);
}

int smtp_host_is_deliverable_af(const char *pSessionId, const char *mbox, const char *dom, int afType, const char *in, int *smtprc)
{	int	rc = -1;	/* 1 = success, 0 = failure, -1 = unreachable */
	int	sd = NetSockOpenTcpPeerAf(afType, in, 25);
	int	timeout = 90;

	if(sd != INVALID_SOCKET && (*smtprc = smtp_get_resp(pSessionId,sd,90,220)) == 220)
	{
		if((rc = smtp_host_test_mailfrom_rcptto(pSessionId,sd,timeout,"",mbox,dom,smtprc)) == 0 &&
			smtp_send_get_resp(pSessionId,sd,timeout,250,"rset\r\n") == 250)
		{	char	*adr;
		
			asprintf(&adr,"postmaster@%s",gHostname);
			if(adr != NULL)
			{
				rc = smtp_host_test_mailfrom_rcptto(pSessionId,sd,timeout,adr,mbox,dom,smtprc);
				free(adr);
			}
		}
	}
	else if(sd == INVALID_SOCKET)
		rc = (errno == ECONNREFUSED ? 0 : -1);
	else
		rc = (*smtprc > 0 ? 0 : -1);

	if(sd != INVALID_SOCKET)
		NetSockPrintf(sd,"quit\r\n");

	NetSockClose(&sd);

	return rc;
}

int smtp_mx_is_deliverable(const char *pSessionId, mx_rr *rr, const char *mbox, const char *dom, int *smtprc, int bTestAll)
{	int j;
	int lastSuccess = -1;

	mlfi_debug(pSessionId,"\tMX %u %s\n",rr->pref,rr->name);
	rr->visited = 1;

	for(j=0; (lastSuccess == -1 || bTestAll) && j < rr->qty; j++)
	{	struct in_addr ipv4;
		const char *in = NULL;
		char *pStr = NULL;
		int afType = 0;
		int tst = -1;

		switch(rr->host[j].nsType)
		{
			case ns_t_a:
				ipv4.s_addr = htonl(rr->host[j].ipv4);
				in = (const char *)&ipv4;
				afType = AF_INET;
				break;

			case ns_t_aaaa:
				in = (const char *)&rr->host[j].ipv6;
				afType = AF_INET6;
				break;
		}

		pStr = mlfi_inet_ntopAF(afType, in);
		mlfi_debug(pSessionId,"\t\t%s %s\n", (afType == AF_INET ? "A" : "AAAA"), pStr);
		free(pStr);

		if(ifi_islocalipAf(afType, in))
		{
			tst = 2;
			*smtprc = 250;
		}
		else
			tst = smtp_host_is_deliverable_af(pSessionId, mbox, dom, afType, in, smtprc);

		switch(tst)
		{
			case 2:
				mlfi_debug(pSessionId,"\t\tPassed - Local Host\n");
				lastSuccess = tst;
				break;
			case 1:
				mlfi_debug(pSessionId,"\t\tPassed - Foreign Host: %d\n",*smtprc);
				lastSuccess = tst;
				break;
			case 0:
				mlfi_debug(pSessionId,"\t\tFailed: %d\n",*smtprc);
				lastSuccess = tst;
				break;
			default:
				mlfi_debug(pSessionId,"\t\tUnreachable\n");
				break;
		}
	}

	return lastSuccess;
}

int smtp_email_address_is_deliverable(const char *pSessionId, const res_state statp, const char *mbox, const char *dom, int *smtprc, int bTestAll)
{	int		rc = 0;
	mx_rr_list	*rrl = calloc(1,sizeof(mx_rr_list));
	int		i,j,l,tst = -1;
	mx_rr		*rr;

#ifdef OS_hpux
	sigignore(SIGPIPE);
#endif

	if(rrl != NULL && mbox != NULL && dom != NULL)
	{
		memset(rrl,0,sizeof(mx_rr_list));
		mx_get_rr_bydomain(statp,rrl,dom);

		if(rrl->qty == 0)
			mlfi_debug(pSessionId,"no mx records\n");
		else
		{
			/* test all MX RRs */
			for(i=0; tst==-1 && i<rrl->qty; i++)
			{
				/* find highest priority non-visited MX RR */
				for(rr=NULL,j=0,l=-1; j<rrl->qty; j++)
				{
					if(!rrl->mx[j].visited && (l == -1 || rrl->mx[j].pref <= l))
					{
						l = rrl->mx[j].pref;
						rr = &rrl->mx[j];
					}
				}

				/* test MX */
				if(rr != NULL)
					tst = smtp_mx_is_deliverable(pSessionId, rr, mbox, dom, smtprc, bTestAll);
			}
			if(tst == 1 || tst == 2)
				rc = 1;
		}
	}

	if(rrl != NULL)
		free(rrl);

	return rc;
}
