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
#include "misc.h"

void usage()
{
	printf("dnsblupd [-d] [-z RDNSBL zone name] [-a IP address] [-i IP address] [-r IP address] [-l IP adress] [-?]\n"
		"\tWhere;\n"
		"\t-d - debug mode.\n"
		"\t-z - RDNSBL zone name. must preceed -i, -a, or -l.\n"
		"\t-i - inserts an ip address into the RDNSBL zone\n"
		"\t-a - is the A or AAAA record address used on Insert, 127.0.0.1 or ::1 otherwise\n"
		"\t-r - removes an ip address from the RDNSBL zone\n"
		"\t-l - lookup ip address in RDNSBL zone\n"
		"\t-? - man page\n"
		);
}

int lookup(ds_t *pDs, char *pHost)
{
	int ra = dns_query_rr(pDs, ns_t_a, pHost);
	int raaaa = (ra == 0 ? dns_query_rr(pDs, ns_t_aaaa, pHost) : 0);
	int rr = ra + raaaa;

	printf("%s record for %s %s\n", (ra ? "A" : (raaaa ? "AAAA" : "No")), pHost, (rr ? "exists." : "found."));

	return (rr > 0);
}

int main(int argc, char **argv)
{	char		*zone		= NULL;
	char		*r_addr		= "127.0.0.1";
	char		*r_dname	= NULL;
	int		c, debug	= 0;
	int		r_opcode	= -1;
	u_int32_t	r_ttl		= 0ul;
	res_state	statp		= RES_NALLOC(statp);
	int		aFamily		= AF_INET;
	int		lookupSuccess	= 0;
	ds_t ds;

	if(argc == 1)
	{
		usage();
		exit(0);
	}

	res_ninit(statp);

	ds.pSessionId = "";
	ds.statp = statp;
	ds.bLoggingEnabled = 0;

	while ((c = getopt(argc, argv, "i:a:r:dz:l:?")) != -1)
	{
		switch (c)
		{
			case 'i':
				if (optarg != NULL && *optarg)
				{
					if(zone!= NULL)
					{
						r_dname = dns_inet_ptoarpa(optarg, AF_INET, zone);
						if(r_dname == NULL)
						{
							r_dname = dns_inet_ptoarpa(optarg, AF_INET6, zone);
							if(r_dname != NULL)
								aFamily = AF_INET6;
						}
						if(r_dname != NULL)
						{
							printf("Insert request for %s\n",r_dname);
							r_opcode = 1;
							r_ttl = 3600ul;
						}
						else
							printf("Invalid IP address\n");
					}
					else
						printf("No Zone specified.\n");
				}
				else
					printf("IP address missing\n");
				break;
			case 'r':
				if (optarg != NULL && *optarg)
				{
					if(zone != NULL)
					{
						r_dname = dns_inet_ptoarpa(optarg, AF_INET, zone);
						if(r_dname == NULL)
						{
							r_dname = dns_inet_ptoarpa(optarg, AF_INET6, zone);
							if(r_dname != NULL)
								aFamily = AF_INET6;
						}
						if(r_dname != NULL)
						{
							printf("Remove request for %s\n",r_dname);
							r_opcode = 0;
						}
						else
							printf("Invalid IP address\n");
					}
					else
						printf("No Zone specified.\n");
				}
				else
					printf("IP address missing\n");
				break;
			case 'd':
				debug = 1;
				break;
			case 'a':
				if (optarg != NULL && *optarg)
				{
					r_addr = optarg;
					aFamily = 0;
				}
				break;
			case 'l':
				if (optarg != NULL && *optarg)
				{
					if(zone != NULL)
					{
						r_dname = dns_inet_ptoarpa(optarg, AF_INET, zone);
						if(r_dname == NULL)
						{
							r_dname = dns_inet_ptoarpa(optarg, AF_INET6, zone);
							if(r_dname != NULL)
								aFamily = AF_INET6;
						}
						if(r_dname != NULL)
						{
							lookupSuccess = lookup(&ds, r_dname);
							free(r_dname);
							r_dname = NULL;
						}

					}
					else
						printf("No Zone specified.\n");
				}
				break;
			case 'z':
				if (optarg != NULL && *optarg)
					zone = optarg;
				break;
			case '?':
				// show man page, if they arent' trying to figure out other cli params
				if(argc > 2 || mlfi_systemPrintf("%s", "man dnsblupd"))
					usage();
				exit(0);
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
		if(aFamily == AF_INET6)
			r_addr = "::1";

		printf("Update request %scompleted\n", dns_update_rr_a(debug, r_opcode, r_dname, r_ttl, r_addr, aFamily) == -1 ? "not " : "");
		lookupSuccess = lookup(&ds, r_dname);
		free(r_dname);
		r_dname = NULL;
	}

	return (lookupSuccess ? 0 : 1); // exit(1) if lookup fails
}
