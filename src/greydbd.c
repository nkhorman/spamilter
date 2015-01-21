/*--------------------------------------------------------------------*
 *
 * Developed by;
 *	Neal Horman - http://www.wanlink.com
 *	Copyright (c) 2009 Neal Horman. All Rights Reserved
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
 *	CVSID:  $Id: greydbd.c,v 1.11 2013/01/04 02:37:49 neal Exp $
 *
 * DESCRIPTION:
 *	application:	greydbd
 *	module:		greydbd.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: greydbd.c,v 1.11 2013/01/04 02:37:49 neal Exp $";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h> 
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>

#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "md5api.h"


#include "inet.h"
#include "pgsql.h"

int		gDebug		= 0;
int		gSd		= INVALID_SOCKET;
ppgsql_srvr	gpDbsrvr	= NULL;

char const	*gpDbHost	= "localhost";
char const	*gpDbHostPort	= "5432";
char const	*gpDbDevice	= "spamilter";
char const	*gpDbUserName	= "spamilter";
char const	*gpDbUserPass	= "";

ppgsql_srvr dbinit()
{

	gpDbsrvr = pgsql_srvr_destroy(gpDbsrvr,1);
	gpDbsrvr = pgsql_srvr_init(gpDbHost,gpDbHostPort,gpDbDevice,gpDbUserName,gpDbUserPass);
	if(gpDbsrvr != NULL)
		pgsql_open(gpDbsrvr);

	return gpDbsrvr;
}

int hashIsWhite(char *phash, unsigned int hashlen)
{	unsigned long count = 0;

	if(phash != NULL && hashlen != 0)
	{
		if(gpDbsrvr == NULL || !pgsql_IsOpen(gpDbsrvr->dbl))
			dbinit();

		if(gpDbsrvr != NULL && gpDbsrvr->dbl != NULL)
		{	ppgsql_query dblq = pgsql_exec_printf(gpDbsrvr->dbl,"select greylist('%*.*s')",hashlen,hashlen,phash);
			int dbreset = 1;

			if(dblq != NULL)
			{
				if(dblq->ok)
				{
					if(dblq->numRows > 0)
					{	char buf[128];

						memset(buf,0,sizeof(buf));
						if(pgsql_getFieldBuf(dblq,0,0,buf,sizeof(buf)-1))
							count = atol(buf);
					}
					dbreset = 0;
				}
				else
					syslog(LOG_DEBUG,"hashIsWhite: dblq %c= NULL && dbl->ok %u && dblq->numRows %u\n"
						,(dblq != NULL ? '=' : '!')
						,(dblq != NULL ? dblq->ok : 0)
						,(dblq != NULL ? dblq->numRows : 0)
						);

				pgsql_freeResult(dblq);
			}

			if(dbreset)
			{
				syslog(LOG_INFO,"hashIsWHite: psql_srvr_destroy");
				gpDbsrvr = pgsql_srvr_destroy(gpDbsrvr,1);
			}
		}
	}

	return (count > 1);
}

void processSocket(int sd)
{	char sockbuf[1024];

	memset(&sockbuf,0,sizeof(sockbuf));
	if(NetSockSelectOne(sd,10))
	{	unsigned long peerIp = 0;
		unsigned short peerPort = 0;
		int rc = NetSockRecvFrom(sd,sockbuf,sizeof(sockbuf), &peerIp, &peerPort, NULL);

		if(rc != SOCKET_ERROR && peerIp == 0x7f000001)
		{
			int i;
			char hashbuf[MD5_DIGEST_STRING_LENGTH];
			char *pStrHashResult[] = {"ok", "fail"};
			int hashIndex = 0;

			for(i=0; i<rc; i++)
				sockbuf[i]=tolower(sockbuf[i]);

			memset(&hashbuf,0,sizeof(hashbuf));
			hashIndex = !hashIsWhite(MD5Hash(sockbuf,rc,hashbuf),MD5_DIGEST_STRING_LENGTH-1);
			NetSockPrintfTo(sd,peerIp,peerPort,"<%s>",pStrHashResult[hashIndex]);

			syslog(LOG_INFO,"%*.*s = %*.*s - %s - %u.%u.%u.%u:%u '<%s>'\n"
				,rc,rc,sockbuf
				,MD5_DIGEST_STRING_LENGTH-1,MD5_DIGEST_STRING_LENGTH-1,hashbuf
				,(hashIndex ? "grey" : "white")
				,(unsigned int)(peerIp>>24)&0xff,(unsigned int)(peerIp>>16)&0xff,(unsigned int)(peerIp>>8)&0xff,(unsigned int)peerIp&0xff,peerPort
				,pStrHashResult[hashIndex]
				);
		}
	}
}

void childShutdown();

void child_signal_hndlr(int signo)
{
	switch(signo)
	{
		case SIGHUP:
			syslog(LOG_ERR,"Signal %u received\n",signo);
			break;
		case SIGTERM:
		case SIGQUIT:
		case SIGINT:
		case SIGSEGV:
			syslog(LOG_ERR,"Shutdown with signal %u\n",signo);
			childShutdown();
			exit(signo);
			break;
	}
}

void childShutdown()
{
	NetSockClose(&gSd);
	if(gpDbsrvr != NULL)
		gpDbsrvr = pgsql_srvr_destroy(gpDbsrvr,1);
	closelog();
}

int childMain(short port)
{	int	quit	= 0;
	int	rc = -1;

	signal(SIGTERM,child_signal_hndlr);
	signal(SIGQUIT,child_signal_hndlr);
	signal(SIGINT,child_signal_hndlr);
	signal(SIGHUP,child_signal_hndlr);
	signal(SIGSEGV,child_signal_hndlr);

	syslog(LOG_ERR,"Child: Started using listening port '%u'",port );

	gSd = NetSockOpenUdp(0x7f000001,port);
	dbinit();
	if(gSd != INVALID_SOCKET && gpDbsrvr != NULL)
	{
		while(!quit)
		{
			processSocket(gSd);
			usleep(10000);
		}

		rc = 0;
		syslog(LOG_ERR,"Child: Exiting: normal");
	}

	if(gSd == INVALID_SOCKET)
		syslog(LOG_ERR,"Child: Exiting: Invalid Tcp socket");
	if(gpDbsrvr == NULL)
		syslog(LOG_ERR,"Childe: exiting: NULL pgsql connection");

	childShutdown();

	return(rc);
}

void childStart(short port, int count, int forked)
{	int	flags = 0;

	if(forked)
	{
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
	}
	else
		flags = LOG_PERROR;

	while(count--)
	{
		openlog("greydbd",flags|LOG_NDELAY|LOG_PID,LOG_DAEMON);
		if(childMain(port) == -1)
		{
			syslog(LOG_ERR,"Error starting child, sleeping 30 seconds, %u retries left.",count);
			sleep(30000);
		}
	}
}

void showVersion()
{
	printf("greydbd - Version 1.0 - spamilter greylist database support service\n");
}

void showUsage()
{
	showVersion();
	printf("usage:\n"
		"\t-v = Service Version Information\n"
		"\t-d = Service Debug Mode\n"
		"\t-p = Serivce Port Number\n"
		"\t-h = PostgreSql Host Name\n"
		"\t-i = PostgreSql Host Port\n"
		"\t-j = PostgreSql DataBase Device Name\n"
		"\t-k = PostgreSql DataBase User Name\n"
		"\t-l = PostgreSql DataBase User Password\n"
		);
}

int main(int argc, char **argv)
{	int	count = 3;
	short	port = (short)7892;
	int	forking = 1;
	char	opt;
	char	*optflags = "d:p:h:i:j:k:l:v?";

	while((opt = getopt(argc,argv,optflags)) != -1)
	{
		switch(opt)
		{
			case 'd':
				if(optarg != NULL && *optarg)
					forking = 1 - ((gDebug = atoi(optarg))>0);
				if(gDebug)
					printf("debug mode\n");
				break;

			/* server mode stuff */
			case 'p':
				if(optarg != NULL && *optarg)
					port = (short)atoi(optarg);
				break;
			case 'h':
				if(optarg != NULL && *optarg)
					gpDbHost = strdup(optarg);
				break;
			case 'i':
				if(optarg != NULL && *optarg)
					gpDbHostPort = strdup(optarg);
				break;
			case 'j':
				if(optarg != NULL && *optarg)
					gpDbDevice = strdup(optarg);
				break;
			case 'k':
				if(optarg != NULL && *optarg)
					gpDbUserName = strdup(optarg);
				break;
			case 'l':
				if(optarg != NULL && *optarg)
					gpDbUserPass = strdup(optarg);
				break;
			case 'v':
				showVersion();
				return 0;
				break;
			case '?':
			default:
				showUsage();
				return 1;
				break;
		}
	}
	argc -= optind;
	argv += optind;

	if(forking)
	{
		if(fork() == 0)
			childStart(port,count,1);
	}
	else
		childStart(port,count,0);

	return 0;
}
