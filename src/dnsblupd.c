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
 *	CVSID:  $Id: dnsblupd.c,v 1.13 2011/07/29 21:23:16 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		dnsblupd.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: dnsblupd.c,v 1.13 2011/07/29 21:23:16 neal Exp $";

#include "config.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "config.h"
#include "dnsupdate.h"
#include "dns.h"

void usage()
{
	printf("dnsblupd [-d 0|1] [-z RDNSBL zone name] [[-a xxx.xxx.xxx.xxx] [-i xxx.xxx.xxx.xxx]] [-r xxx.xxx.xxx.xxx] [-l xxxx.xxxx.xxxx]\n");
	printf("\tWhere;\n");
	printf("\t-d debug mode.\n");
	printf("\t-z RDNSBL zone name. must preceed -i, -a, or -l.\n");
	printf("\t-i inserts an ip address into the RDNSBL zone\n");
	printf("\t-a is the A record address used on Insert, 127.0.0.1 otherwise\n");
	printf("\t-r removes an ip address from the RDNSBL zone\n");
	printf("\t-l lookup ip address in RDNSBL zone\n");
}

void lookup(res_state statp, char *dname, long ip, char *zone)
{
	printf("A record for %s %s\n",dname, dns_rdnsbl_has_rr_a(statp,ip, zone) ? "exists." : "was not found.");
}

int main(int argc, char **argv)
{	char		*zone		= "rdnsbl.wanlink.net";
	char		*r_addr		= "127.0.0.1";
	char		*r_dname	= NULL;
	int		c, debug	= 0;
	int		r_opcode	= -1;
	u_int32_t	r_ttl		= 0ul;
	int		ipa,ipb,ipc,ipd;
	res_state	statp = RES_NALLOC(statp);

	if(argc == 1)
	{
		usage();
		exit(0);
	}

	res_ninit(statp);

	while ((c = getopt(argc, argv, "i:a:r:dz:l:")) != -1)
	{
		switch (c)
		{
			case 'i':
				if (optarg != NULL && *optarg && sscanf(optarg,"%u.%u.%u.%u",&ipa,&ipb,&ipc,&ipd) == 4)
				{
					asprintf(&r_dname,"%u.%u.%u.%u.%s",ipd,ipc,ipb,ipa,zone);
					printf("Insert request for %s\n",r_dname);
					r_opcode = 1;
					r_ttl = 3600ul;
				}
				else
					printf("Invalid ip address format\n");
				break;
			case 'r':
				if (optarg != NULL && *optarg && sscanf(optarg,"%u.%u.%u.%u",&ipa,&ipb,&ipc,&ipd) == 4)
				{
					asprintf(&r_dname,"%u.%u.%u.%u.%s",ipd,ipc,ipb,ipa,zone);
					printf("Remove request for %s\n",r_dname);
					r_opcode = 0;
				}
				else
					printf("Invalid ip address format\n");
				break;
			case 'd':
				debug = 1;
				break;
			case 'a':
				if (optarg != NULL && *optarg)
					r_addr = optarg;
				break;
			case 'l':
				if (optarg != NULL && *optarg && sscanf(optarg,"%u.%u.%u.%u",&ipa,&ipb,&ipc,&ipd) == 4)
				{
					asprintf(&r_dname,"%u.%u.%u.%u.%s",ipd,ipc,ipb,ipa,zone);
					lookup(statp,r_dname,mkip(ipa,ipb,ipc,ipd),zone);
					free(r_dname);
					r_dname = NULL;
				}
				break;
			case 'z':
				if (optarg != NULL && *optarg)
					zone = optarg;
				break;
			default:
				usage();
				exit(0);
				break;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0 && r_opcode != -1 && r_dname != NULL && r_addr != NULL && strlen(r_dname) && strlen(r_addr)) 
	{
		printf("Update request %scompleted\n",dns_update_rr_a(debug,r_opcode,r_dname,r_ttl,r_addr) == -1 ? "not " : "");
		lookup(statp,r_dname,mkip(ipa,ipb,ipc,ipd),zone);
	}
	return(0);
}
