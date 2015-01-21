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
#include <sys/wait.h>
#include <pwd.h>

#include "spamilter.h"
#include "watcher.h"
#include "dnsbl.h"
#include "hndlrs.h"
#include "misc.h"

int	gDebug			= 0;
int	gForeground		= 0;
char	*gPolicyUrl		= "http://www.somedomain.com/policy.html";
char	*gDbpath		= "/var/db/spamilter";
char	*gConfpath		= "/etc/spamilter.rc";
char	*gMlficonn		= "inet:7726@localhost";
int	gDnsBlChk		= 1;
int	gSmtpSndrChk		= 1;
char	*gSmtpSndrChkAction	= "Reject";
int	gMtaHostChk		= 1;
int	gMtaHostChkAsIp		= 1;
int	gMtaHostIpfw		= 0;
int	gMtaHostIpfwNominate	= 0;
int	gMtaHostIpChk		= 0;
int	gMsExtChk		= 1;
char	*gMsExtChkAction	= "Reject";
char	gHostnameBuf[1024];	// hope this is big enough!
char	*gHostname		= gHostnameBuf;
char	*gUserName		= "nobody";
#ifdef SUPPORT_POPAUTH
char	*gPopAuthChk		= NULL;
#endif
#ifdef SUPPORT_LIBSPF
int	gMtaSpfChk		= 0;
#endif
#ifdef SUPPORT_VIRTUSER
char	*gVirtUserTableChk	= NULL;
#endif
#ifdef SUPPORT_ALIASES
char	*gAliasTableChk		= NULL;
#endif
#ifdef SUPPORT_LOCALUSER
int	gLocalUserTableChk	= 0;
#endif
#ifdef SUPPORT_GREYLIST
int gGreyListChk		= 0;
#endif
#ifdef SUPPORT_FWDHOSTCHK
int gRcptFwdHostChk		= 0;
#endif
int gHeaderChkReplyTo		= 0;
int gHeaderChkReceived		= 0;
#ifdef SUPPORT_GEOIP
char *gpGeoipDbPath		= NULL;
int gGeoIpCcChk			= 0;
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

#define SetKeyValStr(k,v,n,d) { if(strlen(k) && strlen(v) && strcasecmp((k),(n)) == 0) (d) = strdup((v)); }
#define SetKeyValInt(k,v,n,d) { if(strlen(k) && strlen(v) && strcasecmp((k),(n)) == 0) (d) = atoi((v)); }

#define ShowKeyValStr(k,v) { printf("%s = '%s'\n",(k),(v)); }
#define ShowKeyValInt(k,v) { printf("%s = %u\n",(k),(v)); }

void getconf(char *confpath)
{	FILE	*fin = fopen(confpath,"r");
	char	*str;
	char	buf[1024];
	char	key[1024];
	char	val[1024];

	if(fin != NULL)
	{
		while(!feof(fin))
		{
			fgets(buf,sizeof(buf),fin);
			
			str = strchr(buf,'#');
			if(str != NULL)
			{
				*(str--) = '\0';
				while(str >= buf && (*str ==' ' || *str == '\t'))
					*(str--) = '\0';
			}

			if(strlen(buf))
			{
				str = mlfi_strcpyadv(key,sizeof(key),buf,'=');
				str = mlfi_strcpyadv(val,sizeof(val),str,'=');

				SetKeyValStr(key,val,"UserName",gUserName);
				SetKeyValStr(key,val,"PolicyUrl",gPolicyUrl);
				SetKeyValStr(key,val,"DbPath",gDbpath);
				SetKeyValStr(key,val,"Conn",gMlficonn);
				SetKeyValInt(key,val,"DnsBlChk",gDnsBlChk);
				SetKeyValInt(key,val,"SmtpSndrChk",gSmtpSndrChk);
				SetKeyValStr(key,val,"SmtpSndrChkAction",gSmtpSndrChkAction);
				SetKeyValInt(key,val,"MtaHostChk",gMtaHostChk);
				SetKeyValInt(key,val,"MtaHostChkAsIp",gMtaHostChkAsIp);
				SetKeyValInt(key,val,"MtaHostIpfw",gMtaHostIpfw);
				SetKeyValInt(key,val,"MtaHostIpfwNominate",gMtaHostIpfwNominate);
				SetKeyValInt(key,val,"MtaHostIpChk",gMtaHostIpChk);
#ifdef SUPPORT_LIBSPF
				SetKeyValInt(key,val,"MtaSpfChk",gMtaSpfChk);
#endif

				SetKeyValInt(key,val,"MsExtChk",gMsExtChk);
				SetKeyValStr(key,val,"MsExtChkAction",gMsExtChkAction);
#ifdef SUPPORT_POPAUTH
				SetKeyValStr(key,val,"PopAuthChk",gPopAuthChk);
#endif
#ifdef SUPPORT_VIRTUSER
				SetKeyValStr(key,val,"VirtUserTableChk",gVirtUserTableChk);
#endif
#ifdef SUPPORT_ALIASES
				SetKeyValStr(key,val,"AliasTableChk",gAliasTableChk);
#endif
#ifdef SUPPORT_LOCALUSER
				SetKeyValInt(key,val,"LocalUserTableChk",gLocalUserTableChk);
#endif
#ifdef SUPPORT_GREYLIST
				SetKeyValInt(key,val,"GreyListChk",gGreyListChk);
#endif
#ifdef SUPPORT_FWDHOSTCHK
				SetKeyValInt(key,val,"RcptFwdHostChk",gRcptFwdHostChk);
#endif
				SetKeyValInt(key,val,"HeaderReplyToChk",gHeaderChkReplyTo);
				SetKeyValInt(key,val,"HeaderReceivedChk",gHeaderChkReceived);
#ifdef SUPPORT_GEOIP
				SetKeyValStr(key,val,"GeoIPDBPath",gpGeoipDbPath);
				SetKeyValInt(key,val,"GeoIPChk",gGeoIpCcChk);
#endif
			}
		}
		fclose(fin);
	}
	else
		printf("Warning! Unable to open config file '%s', using compiled defaults.\n",confpath);
}

void usage()
{
	printf("spamilter [-c config] [-d 1] \n");
	printf("\tWhere;\n\t-c specifies a config filename to read at startup\n");
	printf("\t-d debug mode.\n");
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

	// Process command line options
	while ((c = getopt(argc, argv, "fd:c:")) != -1)
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
			default:
				usage();
				exit(0);
				break;
		}
	}

	setproctitle("startup");

	getconf(gConfpath);

	if(gDebug)
		flags = LOG_PERROR;
	openlog(mlfi.xxfi_name, flags|LOG_NDELAY|LOG_PID, LOG_DAEMON);

	if(uid == 0) // if root, drop privs
	{
		if((pw = getpwnam(gUserName)) == NULL)
		{
			fprintf(stderr,"Fatal error - Unable to get user '%s' identity information",gUserName);
			exit(1);
		}
		else if(setgid(pw->pw_gid) != 0 || setuid(pw->pw_uid) != 0)
		{
			fprintf(stderr,"Fatal error - Unable to switch user identity to '%s'",gUserName);
			exit(2);
		}
		else
			uid = pw->pw_uid;
	}
	pw = getpwuid(uid);

	printf("\nStarting %s\n",mlfi.xxfi_name);
	printf("Running as user '%s/%s'\n",pw->pw_name,pw->pw_gecos);
	ShowKeyValStr("PolicyUrl",gPolicyUrl);
	ShowKeyValStr("DbPath",gDbpath);
	ShowKeyValStr("Conn",gMlficonn);
	ShowKeyValInt("DnsBlChk",gDnsBlChk);
	ShowKeyValInt("SmtpSndrChk",gSmtpSndrChk);
	ShowKeyValStr("SmtpSndrChkAction",gSmtpSndrChkAction);
	ShowKeyValInt("MtaHostChk",gMtaHostChk);
	ShowKeyValInt("MtaHostIpfw",gMtaHostIpfw);
	ShowKeyValInt("MtaHostIpfwNominate",gMtaHostIpfwNominate);
	ShowKeyValInt("MtaHostIpChk",gMtaHostIpChk);
#ifdef SUPPORT_LIBSPF
	ShowKeyValInt("MtaSpfChk",gMtaSpfChk);
#endif
	ShowKeyValInt("MsExtChk",gMsExtChk);
	ShowKeyValStr("MsExtChkAction",gMsExtChkAction);
#ifdef SUPPORT_POPAUTH
	ShowKeyValStr("PopAuthChk",gPopAuthChk);
#endif
#ifdef SUPPORT_VIRTUSER
	ShowKeyValStr("VirtUserTableChk",gVirtUserTableChk);
#endif
#ifdef SUPPORT_ALIASES
	ShowKeyValStr("AliasTableChk",gAliasTableChk);
#endif
#ifdef SUPPORT_LOCALUSER
	ShowKeyValInt("LocalUserTableChk",gLocalUserTableChk);
#endif
#ifdef SUPPORT_GREYLIST
	ShowKeyValInt("GreyListChk",gGreyListChk);
#endif
#ifdef SUPPORT_FWDHOSTCHK
	ShowKeyValInt("RcptFwdHostChk",gRcptFwdHostChk);
#endif
	ShowKeyValInt("HeaderReplyToChk",gHeaderChkReplyTo);
	ShowKeyValInt("HeaderReceivedChk",gHeaderChkReceived);
#ifdef SUPPORT_GEOIP
	ShowKeyValStr("GeoIPDBPath",gpGeoipDbPath);
	ShowKeyValInt("GeoIPChk",gGeoIpCcChk);
#endif

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
	if(!gMsExtChk)
		mlfi.xxfi_body = NULL;

	c = 0;
	smfi_setconn(gMlficonn);
	if (smfi_register(mlfi) == MI_FAILURE)
	{
		fprintf(stderr, "smfi_register failed\n");
		exit(EX_UNAVAILABLE);
	}
	else
		c = main_watcher(!gDebug & !gForeground, "/tmp/spamilter.pid");

	return c;
}
