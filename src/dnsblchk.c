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
 *	CVSID:  $Id: dnsblchk.c,v 1.22 2012/11/23 03:54:13 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		dnsblchk.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: dnsblchk.c,v 1.22 2012/11/23 03:54:13 neal Exp $";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sysexits.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>

#include "spamilter.h"
#include "misc.h"
#include "smisc.h"
#include "mx.h"
#include "inet.h"
#include "smtp.h"
#include "dns.h"
#include "dnsbl.h"

char	gHostnameBuf[1024];
char	*gHostname = gHostnameBuf;

int gDebug = 0;

res_state	gStatp = NULL;

// Test if a host is RBL'd
int testip_af(int afType, const char *in, const char *dbpath)
{	char		*str;
	char		buf[8192];
	char		hostbuf[1024];
	int		fd;
	int		count = 0;
	char		sessionId[10];

	sprintf(sessionId,"%04X",(unsigned int)pthread_self());

	asprintf(&str, "%s/db.rdnsbl", dbpath);
	fd = open(str,O_RDONLY);
	if(fd == -1)
		mlfi_debug(sessionId,"hostListOpen: unable to open RDNSBL host file '%s'\n",str);
	else
		mlfi_debug(sessionId,"%s - opened\n",str);
	free(str);

	if(fd != -1)
	{
		lseek(fd,0l,SEEK_SET);
		while(mlfi_fdgets(fd,buf,sizeof(buf)) >= 0)
		{
			str = strchr(buf,'#');
			if(str != NULL)
			{
				*(str--) = '\0';
				while(str >= buf && (*str ==' ' || *str == '\t'))
					*(str--) = '\0';
			}

			if(strlen(buf))
			{
				mlfi_strcpyadv(hostbuf,sizeof(hostbuf),buf,'|');
				//mlfi_debug(sessinId,"checking - %s %s\n",hostbuf,buf);
				if(strlen(hostbuf) && dnsbl_check_rbl_af(sessionId, gStatp, afType, in, hostbuf))
				{
					/*
					mlfi_debug(sessinId,"\t\t'%u.%u.%u.%u.%s' - Blacklisted\n",
						((ip&0x000000ff)),
						((ip&0x0000ff00)>>8),
						((ip&0x00ff0000)>>16),
						((ip&0xff000000)>>24),
						hostbuf);
						*/
					count ++;
				}
			}
		}
		close(fd);
	}

	return(count);
}

// Test a single hostname to see if it is RBL'd
int testipstr(const char *pIpStr, const char *path)
{	int	rc = -1;

	if(pIpStr != NULL && path != NULL)
	{
		char bufnet[sizeof(struct in6_addr)];
		struct in_addr *pIp4 = (struct in_addr *)bufnet;
		struct in6_addr *pIp6 = (struct in6_addr *)bufnet;

		memset(bufnet,0,sizeof(bufnet));

		// Test an ipv4 host
		if(inet_pton(AF_INET, pIpStr, pIp4))
			rc = testip_af(AF_INET, (char *)pIp4, path);
		// Test an ipv6 host
		else if(inet_pton(AF_INET6, pIpStr, pIp6))
			rc = testip_af(AF_INET6, (char *)pIp6, path);
	}

	return rc;
}

int mx_get_rr_match(mx_rr_list *rrl)
{	int	i,j;

	for(i=0; i<rrl->qty && !rrl->match; i++)
	{
		for(j=0; j<rrl->mx[i].qty && !rrl->match; j++)
		{
			switch(rrl->mx[i].host[j].nsType)
			{
				case ns_t_a:
					rrl->match = (rrl->mx[i].host[j].nsType == rrl->nsType && rrl->mx[i].host[j].ipv4 == rrl->ipv4);
					break;

				case ns_t_aaaa:
					rrl->match = (rrl->mx[i].host[j].nsType == rrl->nsType
						&& IN6_ARE_ADDR_EQUAL(&rrl->mx[i].host[j].ipv6, &rrl->ipv6)
						);
					break;
			}
		}
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

// test if mx hosts for a given domain are RBL'd
void testdomainmx(char *domain, char *path)
{	mx_rr_list	mxrrl;
	mx_rr_list	*rrl = &mxrrl;
	int	i,j;
	mx_rr	*rr;

	memset(rrl, 0, sizeof(mx_rr_list));
	mx_get_rr_recurse_host(gStatp, mx_get_rr_bydomain(gStatp, rrl, domain));
	
	if(rrl->qty == 0)
		printf("no mx records\n");
	else
	{
		printf("Domain: %s\n",domain);
		// for each of the mx records
		for(i=0; i<rrl->qty; i++)
		{
			rr = &rrl->mx[i];
			printf("\tMX %u %s\n",rr->pref,rr->name);
			// for each of the hosts of the mx records
			for(j=0; j<rr->qty; j++)
			{
				switch(rr->host[j].nsType)
				{
					case ns_t_a:
						{	struct in_addr ip;
							char *pStr = NULL;

							ip.s_addr = htonl(rr->host[j].ipv4);
							pStr = mlfi_inet_ntopAF(AF_INET, (char *)&ip);

							// test if the host is RBL'd
							printf("\t\tA %s\n", pStr);
							if(testip_af(AF_INET, (char *)&ip, path) == 0)
								printf("\t\tPassed\n");
							free(pStr);
						}
						break;

					case ns_t_aaaa:
						{
							char *pStr = mlfi_inet_ntopAF(AF_INET6, (char *)&rr->host[j].ipv6);

							// test if the host is RBL'd
							printf("\t\tAAAA %s\n", pStr);
							if(testip_af(AF_INET6, (char *)&rr->host[j].ipv6, path) == 0)
								printf("\t\tPassed\n");
							free(pStr);
						}
						break;
				}
			}
		}
	}
}

void mboxdomainsplit(char *mbox, char **domain)
{	char	*s,*d;
	int	i,l;

	if(domain != NULL)
	{
		*domain = NULL;
		for(i=0,l=strlen(mbox),d=mbox,s=mbox; i<l; i++)
		{
			if(*s != '<' && *s != '>')
			{
				*(d++) = *(s++);

				if(*(d-1) == '@')
				{
					*domain = d;
					*(d-1) = '\0';
				}
			}
			else
			{
				*d = '\0';
				s++;
			}
		}
	}
}

void usage(void)
{
	printf("dnsblchk [-d] [-p db.rdnsbl file pathname] [-i ip address] [-e mbox@domain] [domain] ...\n"
		"\t-d - debug on\n"
		"\t-p - specify directory path to db.rdnsbl file - default = /var/db/spamilter/db.rdnsbl\n"
		"\t-i - test ip address to see if it is listed at any of the RBLs from db.rdnsbl\n"
		"\t-e - test email delivery\n"
		"\t-? - man page or options summary\n"
		"\t[domain] - test all of the listed MX hosts of a given domain to see if they are listed at any of the RBLs from db.rdnsbl\n"
	);
}

int main(int argc, char **argv)
{	int	c;
	int	smtprc;
	int	rc;
	char	*dbpath = "/var/db/spamilter";
	char		sessionId[10];

	sprintf(sessionId,"%04X",(unsigned int)pthread_self());

	if(argc < 2)
	{
		usage();
		exit(0);
	}

	gDebug = 0;
	gStatp = RES_NALLOC(gStatp);
	res_ninit(gStatp);

	gethostname(gHostnameBuf,sizeof(gHostnameBuf)-1);
	printf("Hostname: '%s'\n",gHostnameBuf);

	while ((c = getopt(argc, argv, "i:e:dp:?")) != -1)
	{
		switch (c)
		{
			case 'd':
				gDebug = 1;
				openlog("dnsblchk", LOG_PERROR|LOG_NDELAY|LOG_PID, LOG_DAEMON);
				break;
			case 'p':
				if(optarg != NULL && *optarg)
					dbpath = optarg;
				break;
			case 'i':
				if(optarg != NULL && *optarg)
				{
					if(testipstr(optarg,dbpath) == 0)
						printf("\t\tPassed\n");
				}
				else
					printf("Bogus/Missing command line arguement\n");
				break;
			case 'e':
				if(optarg != NULL && *optarg)
				{	char	*mbox = strdup(optarg);
					char	*dom = NULL;

					mboxdomainsplit(mbox,&dom);
					rc = smtp_email_address_is_deliverable(sessionId, gStatp, mbox, dom, &smtprc, true);
					printf("mx deliverable: %d, returned: %d\n",rc,smtprc);
				}
				else
					printf("Bogus/Missing command line arguement\n");
				break;
			case '?':
				// show man page, if they arent' trying to figure out other cli params
				if(argc > 2 || mlfi_systemPrintf("%s", "man dnsblchk"))
					usage();
				break;
		}
	}

	argc -= optind;
	argv += optind;

	while(argc > 0)
	{
		testdomainmx(*argv,dbpath);
		argc--;
		argv++;
	}

	res_nclose(gStatp);

	return 0;
}
