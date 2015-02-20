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
 *	CVSID:  $Id: mx.c,v 1.15 2012/12/09 18:19:42 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		mx.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: mx.c,v 1.15 2012/12/09 18:19:42 neal Exp $";


#include <string.h>
#include <stdlib.h>
#include "config.h"
#include "mx.h"

// TODO - ipv6 - everywhere

static int mx_res_search(const res_state statp, const char *name, int class, int type, u_char **answer, ns_msg *phandle)
{	u_char	*resp = *answer = (u_char *)malloc(NS_PACKETSZ);
	int	i,respsize = NS_PACKETSZ;
	int	rc,rc_nip = -1;

	/* try hard to have a big enough buffer for the response */
	for(i=0; i<5 && resp != NULL && rc_nip < 0; i++)
	{
		rc = res_nsearch(statp, name, class, type, resp, respsize);
		if(rc > 0 && rc > respsize)
		{
			respsize += rc;
			resp = *answer = (u_char *)realloc(resp,respsize);
		}
		else if(rc > 0 && rc <= respsize)
		{
			rc_nip = ns_initparse(resp,rc,phandle);
			i = 0;
		}
	}

	return(rc_nip);
}

void mx_parse_host_query(mx_rr *mxrr, ns_msg handle, ns_sect section)
{	int	rrnum;
	ns_rr	rr;
	int	count = ns_msg_count(handle,section);

	for(rrnum=0; rrnum<count && rrnum<50; rrnum++)
	{
		if(ns_parserr(&handle, section, rrnum, &rr) == 0 && ns_rr_type(rr) == ns_t_a)
		{	int	i,dup;
			long	ip = ns_get32(ns_rr_rdata(rr));

			for(i=0,dup=0; i<mxrr->qty && !dup; i++)
				dup = (ip == mxrr->host[i].ip);

			if(!dup)
			{
				mxrr->host[mxrr->qty].ip = ip;
				mxrr->qty++;
			}
		}
	}
}

mx_rr *mx_get_rr_hosts(const res_state statp, mx_rr *mxrr)
{	ns_msg	handle;
	u_char	*resp;

	if(mx_res_search(statp, mxrr->name, ns_c_in, ns_t_a, &resp, &handle) > -1)
		mx_parse_host_query(mxrr,handle,ns_s_an);

	if(resp != NULL)
		free(resp);

	return(mxrr);
}

void mx_parse_rr_query(const res_state statp, mx_rr_list *rrl, ns_msg handle, ns_sect section)
{	int	rrnum;
	ns_rr	rr;
	char	hn[MAXDNAME];
	int	hp;
	u_char	*cp;

	for(rrnum=0; rrnum<ns_msg_count(handle,section) && rrnum<50; rrnum++)
	{
		if(ns_parserr(&handle, section, rrnum, &rr) == 0 && ns_rr_type(rr) == ns_t_mx)
		{
			cp = (u_char *)ns_rr_rdata(rr);
			NS_GET16(hp,cp);
			memset(hn,0,sizeof(hn));
			if(ns_name_uncompress(ns_msg_base(handle),ns_msg_end(handle),cp,hn,sizeof(hn)) != -1)
			{	int	i,dup;

				for(i=0,dup=0; i < rrl->qty && !dup; i++)
					dup = !strcasecmp(rrl->mx[i].name,hn);
				if(!dup)
				{
					strcpy(rrl->mx[rrl->qty].name,hn);
					rrl->mx[rrl->qty].pref = hp;
					mx_get_rr_hosts(statp, &rrl->mx[rrl->qty]);
					rrl->qty++;
				}
			}
		}
	}
}

mx_rr_list *mx_get_rr_bydomain(const res_state statp, mx_rr_list *rrl, const char *name)
{	int	i;
	ns_msg	handle;
	u_char	*resp = NULL;

	for(i=0; i<2 && rrl->qty == 0; i++)
	{
		if(mx_res_search(statp, name, ns_c_in, ns_t_mx, &resp, &handle) > -1)
		{
			mx_parse_rr_query(statp,rrl,handle,ns_s_an);
			strcpy(rrl->domain,name);
		}
	} 

	if(resp != NULL) 
		{free(resp); resp=NULL;}

	if(rrl->qty == 0)
	{
		strcpy(rrl->domain,name);
		/*
			Fake out an MX record per rfc974 (now superceeded by rfc2821)
			section "Iterpreting the List of MX RRs"

			"It is possible that the list of MXs in the response to the query will
			be empty.  This is a special case.  If the list is empty, mailers
			should treat it as if it contained one RR, an MX RR with a preference
			value of 0, and a host name of REMOTE.  (I.e., REMOTE is its only
			MX).  In addition, the mailer should do no further processing on the
			list, but should attempt to deliver the message to REMOTE.  ... "
		*/
		strcpy(rrl->mx[rrl->qty].name,name);	/* mx hostname */
		rrl->mx[rrl->qty].pref = 0;		/* mx host preference */
		mx_get_rr_hosts(statp, &rrl->mx[rrl->qty++]);	/* find A RRs for hosts */
		if(rrl->mx[0].qty == 0)	/* if no A RRs, */
			rrl->qty = 0;	/* then no MX RRs either */
	}

	return(rrl);
}

char *mx_get_host_ptr(const res_state statp, const char *ipstr, char *hostname, int hostnamelen)
{	ns_msg	handle;
	u_char	*resp = NULL;
	ns_rr	rr;
	char	hn[MAXDNAME];

	memset(hostname,0,hostnamelen);
	if(mx_res_search(statp, ipstr, ns_c_in, ns_t_ptr, &resp, &handle) > -1 &&
		ns_msg_count(handle,ns_s_an) == 1 &&
		ns_parserr(&handle,ns_s_an,0,&rr) == 0 &&
		ns_rr_type(rr) == ns_t_ptr &&
		ns_name_uncompress(ns_msg_base(handle),ns_msg_end(handle),ns_rr_rdata(rr),hn,sizeof(hn)) != -1
		)
		strncpy(hostname,hn,hostnamelen);

	if(resp != NULL)
		free(resp);

	return(strlen(hostname) ? hostname : NULL);
}

mx_rr_list *mx_get_rr_byipstr(const res_state statp, const char *ipstr, mx_rr_list *rrl)
{	char	buf[1024];
	int	buflen=sizeof(buf);

	if(mx_get_host_ptr(statp, ipstr,buf,buflen) != NULL)
	{
		strcpy(rrl->domain,buf);
		mx_get_rr_bydomain(statp,rrl,buf);
	}

	return(rrl);
}

mx_rr_list *mx_get_rr_byip(const res_state statp, mx_rr_list *rrl, long ip)
{	char	ipstr[1024];

	rrl->ip = ip;
	sprintf(ipstr,"%u.%u.%u.%u.in-addr.arpa",
		(int)((ip&0x000000ff)),(int)((ip&0x0000ff00)>>8),
		(int)((ip&0x00ff0000)>>16),(int)((ip&0xff000000)>>24));

	return(mx_get_rr_byipstr(statp,ipstr,rrl));
}

char ipbuf[50];
char *ip2str(long ip)
{
	sprintf(ipbuf,"%u.%u.%u.%u",
		(int)((ip&0xff000000)>>24),(int)((ip&0x00ff0000)>>16),
		(int)((ip&0x0000ff00)>>8),(int)((ip&0x000000ff)));

	return(ipbuf);
}

mx_rr_list *mx_show_rr(mx_rr_list *rrl)
{	int	i,j;
	mx_rr	*rr;

	if(rrl->qty == 0)
		printf("no mx records\n");
	else
	{
		for(i=0; i<rrl->qty; i++)
		{
			rr = &rrl->mx[i];
			printf("\tMX %u %s\n",rr->pref,rr->name);
			for(j=0; j<rr->qty; j++)
				printf("\t\tA %s\n",ip2str(rr->host[j].ip));
		}
	}

	return(rrl);
}

int mx_get_rr_match(mx_rr_list *rrl)
{	int	i,j;

	for(i=0; i<rrl->qty && !rrl->match; i++)
	{
		for(j=0; j<rrl->mx[i].qty && !rrl->match; j++)
			rrl->match = (rrl->mx[i].host[j].ip == rrl->ip);
	}

	return(rrl->match);
}

void mx_get_rr_recurse_host(const res_state statp, mx_rr_list *rrl)
{	char	*s,*d;

	while(!mx_get_rr_match(rrl) && strlen(rrl->domain))
	{
		d = rrl->domain;
		if(strlen(d))
		{
			s = strchr(d,'.');
			if(s != NULL)
			{
				s++;
				while(*s)
					*(d++) = *(s++);
			}
			*d = '\0';
			if(strlen(rrl->domain))
				mx_get_rr_bydomain(statp, rrl,rrl->domain);
		}
	}
}

mx_rr_list *mx_get_rr(const res_state statp, mx_rr_list *rrl, long ip, const char *name, int collect)
{

	memset(rrl,0,sizeof(mx_rr_list));
	if(ip)
		mx_get_rr_recurse_host(statp,mx_get_rr_byip(statp,rrl,ip));

	if(!rrl->match && name != NULL && strlen(name))
	{
		if(!collect)
		{
			memset(rrl,0,sizeof(mx_rr_list));
			rrl->ip = ip;
		}
		mx_get_rr_recurse_host(statp,mx_get_rr_bydomain(statp,rrl,name));
	}

	return(rrl);
}
