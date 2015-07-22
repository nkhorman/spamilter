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
 *	CVSID:  $Id: spamilter.c,v 1.54 2013/07/13 23:31:37 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		spamilter.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: spamilter.c,v 1.54 2013/07/13 23:31:37 neal Exp $";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <sysexits.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>

#include "spamilter.h"
#include "ini.h"
#include "watcher.h"
#include "dnsbl.h"
#include "hndlrs.h"
#include "misc.h"

int	gDebug			= 0;
const char	*gConfpath		= PATH_CONFIG;
int	gForeground		= 0;
char	gHostnameBuf[1024];	// hope this is big enough!
const char *gHostname		= gHostnameBuf;

#ifdef NEED_GETPROGNAME
static const char *__gpProgname = NULL;

#include <libgen.h>

const char *getprogname(void)
{
	return __gpProgname;
}
#endif

struct smfiDesc mlfi =
{
	"Spamilter",	// filter name
	SMFI_VERSION,	// version code -- do not change
	SMFIF_ADDHDRS | SMFIF_CHGHDRS | SMFIF_CHGBODY | SMFIF_ADDRCPT | SMFIF_DELRCPT,	// flags
	mlfi_connect,	// connection info filter
	mlfi_helo,	// SMTP HELO command filter
	mlfi_envfrom,	// envelope sender filter
	mlfi_envrcpt,	// envelope recipient filter
	mlfi_header,	// header filter
	mlfi_eoh,	// end of header
	mlfi_body,	// body block filter
	mlfi_eom,	// end of message
	mlfi_abort,	// message aborted
	mlfi_close	// connection cleanup
};

ik_t gpIk[] =
{
	{ OVT_STR, OPT_USERNAME,			"nobody" },
	{ OVT_STR, OPT_POLICYURL,			"http://somedomain.com/policy.html" },
	{ OVT_STR, OPT_DBPATH,				"/var/db/spamilter" },
	{ OVT_STR, OPT_CONN,				"inet:7726@localhost" },
	{ OVT_BOOL, OPT_DNSBLCHK,			"yes" },
	{ OVT_BOOL, OPT_SMTPSNDRCHK,			"yes" },
	{ OVT_BOOL, OPT_SMTPSNDRCHKACTION,		"Reject" },
	{ OVT_BOOL, OPT_MTAHOSTCHK,			"yes" },
	{ OVT_BOOL, OPT_MTAHOSTCHKASIP,			"no" },
	{ OVT_BOOL, OPT_MTAHOSTIPFW,			"no" },
	{ OVT_BOOL, OPT_MTAHOSTIPFWNOMINATE,		"no" },
	{ OVT_BOOL, OPT_MTAHOSTIPCHK,			"no" },
#if defined(SUPPORT_LIBSPF) || defined(SUPPORT_LIBSPF2)
	{ OVT_BOOL, OPT_MTASPFCHK,			"yes" },
	{ OVT_BOOL, OPT_MTASPFCHKSOFTFAILASHARD,	"no" },
#endif
	{ OVT_INT, OPT_MSEXTCHK,			"2" },
	{ OVT_BOOL, OPT_MSEXTCHKACTION,			"Reject" },
#ifdef SUPPORT_POPAUTH
	{ OVT_BOOL, OPT_POPAUTHCHK,			"" },
#endif
#ifdef SUPPORT_VIRTUSER
	{ OVT_BOOL, OPT_VIRTUSERTABLECHK,		"/etc/mail/virtuser.db" },
#endif
#ifdef SUPPORT_ALIASES
	{ OVT_BOOL, OPT_ALIASTABLECHK,			"/etc/mail/aliases.db" },
#endif
#ifdef SUPPORT_LOCALUSER
	{ OVT_BOOL, OPT_LOCALUSERTABLECHK,		"yes" },
#endif
#ifdef SUPPORT_GREYLIST
	{ OVT_BOOL, OPT_GREYLISTCHK,			"yes" },
#endif
#ifdef SUPPORT_FWDHOSTCHK
#endif
	{ OVT_BOOL, OPT_HEADERREPLYTOCHK,		"yes" },
	{ OVT_BOOL, OPT_HEADERRECEIVEDCHK,		"no" },
#ifdef SUPPORT_FWDHOSTCHK
	{ OVT_BOOL, OPT_GEOIPDBPATH,			"/var/db/spamilter/geoip" },
	{ OVT_BOOL, OPT_GEOIPCHK,			"yes" },
#endif
//	{ OVT_, "", "" },
	{ OVT_NONE, NULL, NULL },
};

static int callbackInvalidOptionShow(void *pCallbackData, void *pCallbackCtx)
{
	printf("Unused or Invalid Option '%s'\n", (char *)pCallbackData);

	return 1; // again
}

static int callbackInvalidOptionFree(void *pCallbackData, void *pCallbackCtx)
{
	free((char *)pCallbackData);

	return 1; // again
}

void getconf(char const *pFin)
{
	ik_t *pIk = &gpIk[0];
	list_t *pListInvalidOptions = NULL;

	iniInit(pIk);
	pListInvalidOptions = iniRead(pFin);

	while(pIk != NULL && pIk->type != OVT_NONE)
	{
		switch(pIk->type)
		{
			case OVT_STR:
				printf("%s = '%s'\n", pIk->pName, iniGetStr(pIk->pName));
				break;
			case OVT_BOOL:
				printf("%s = %s\n", pIk->pName, iniGetInt(pIk->pName) ? "yes" : "no" );
				break;
			case OVT_INT:
				printf("%s = %u\n", pIk->pName, iniGetInt(pIk->pName));
				break;
		}
		pIk++;
	}

	listForEach(pListInvalidOptions, &callbackInvalidOptionShow, NULL);
	listDestroy(pListInvalidOptions, &callbackInvalidOptionFree, NULL);
}

void usage()
{
	printf("spamilter [-c config] [-d 1] [-f] [-?]\n"
		"\tWhere;\n\t-c - config filename\n"
		"\t-d - debug mode\n"
		"\t-f - run in foreground\n"
		"\t-? - man page or options summary\n"
		);
}

int worker_main()
{
	return smfi_main();
}

int main(int argc, char *argv[])
{	int		c;
	struct passwd	*pw = NULL;
	int		uid = getuid();
	int		flags = 0;

#ifdef NEED_GETPROGNAME
	__gpProgname = basename(strdup(argv[0]));
#endif
#if defined(HAVE_SETPROCTITLE) && defined(OS_Linux)
	spt_init(argc,argv[0]);
#endif

	// Process command line options
	while ((c = getopt(argc, argv, "fd:c:?")) != -1)
	{
		switch (c)
		{
			case 'c':
				if (optarg != NULL && *optarg != '\0')
					gConfpath = optarg;
				break;
			case 'd': // debug
				if(optarg != NULL && *optarg)
					gDebug = atoi(optarg);
				break;
			case 'f': // forground
				gDebug = 0;
				gForeground = 1;
				break;
			case '?':
				// show man page, if they arent' trying to figure out other cli params
				if(argc > 2 || mlfi_systemPrintf("%s", "man spamilter"))
					usage();
				exit(0);
				break;
			default:
				usage();
				exit(0);
				break;
		}
	}

#ifdef HAVE_SETPROCTITLE
	setproctitle("startup");
#endif
	printf("\nStarting %s\n",mlfi.xxfi_name);

	getconf(gConfpath);

	if(gDebug)
		flags = LOG_PERROR;
	openlog(mlfi.xxfi_name, flags|LOG_NDELAY|LOG_PID, LOG_DAEMON);

	if(uid == 0) // if root, drop privs
	{	const char *pUserName = iniGetStr(OPT_USERNAME);

		if((pw = getpwnam(pUserName)) == NULL)
		{
			fprintf(stderr,"Fatal error - Unable to get user '%s' identity information",pUserName);
			exit(1);
		}
		else if(setgid(pw->pw_gid) != 0 || setuid(pw->pw_uid) != 0)
		{
			fprintf(stderr,"Fatal error - Unable to switch user identity to '%s'",pUserName);
			exit(2);
		}
		else
			uid = pw->pw_uid;
	}
	pw = getpwuid(uid);

	printf("Running as user '%s/%s'\n",pw->pw_name,pw->pw_gecos);

	gethostname(gHostnameBuf,sizeof(gHostnameBuf)-1);

	// for silly hosts that don't fully quallify their hostname
	if(strchr(gHostnameBuf,'.') == NULL)
	{
		// append the default domain name from the resolver library
		res_init();
		strncat(gHostnameBuf,".",(sizeof(gHostnameBuf)-1)-strlen(gHostnameBuf));
		strncat(gHostnameBuf,_res.defdname,(sizeof(gHostnameBuf)-1)-strlen(gHostnameBuf));
	}
	printf("Hostname: '%s'\n",gHostname);


	// if not doing attachment checks, don't do body processing
	if(!iniGetInt(OPT_MSEXTCHK))
		mlfi.xxfi_body = NULL;

	c = 0;
	smfi_setconn(iniGetStr(OPT_CONN));
	if (smfi_register(mlfi) == MI_FAILURE)
	{
		fprintf(stderr, "smfi_register failed\n");
		exit(EX_UNAVAILABLE);
	}
	else
		c = main_watcher(!gDebug & !gForeground, "/tmp/spamilter.pid");

	return c;
}
