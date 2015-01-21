/*--------------------------------------------------------------------*
 *
 * Developed by;
 *	Neal Horman - http://www.wanlink.com
 *	Copyright (c) 2002 Neal Horman. All Rights Reserved
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
 *	CVSID:  $Id: ipfwmtad.c,v 1.22 2011/10/27 18:16:53 neal Exp $
 *
 * DESCRIPTION:
 *	application:	ipfwmtad
 *	module:		main.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: ipfwmtad.c,v 1.22 2011/10/27 18:16:53 neal Exp $";

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

#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#include "inet.h"
#include "misc.h"
#include "ipfw_direct.h"
#include "key.h"

#ifdef SUPPORT_PAM
#include "pam.h"
#else
#include "uam.h"
#endif

typedef struct _MTAINFO
{
	long	ip;
	time_t	timeFirst;
	time_t	timeExpire;
	long	timeInterval;
	long	count;
	int	opcode;
	int	rate;
	int	update;
	int	needAdd;
	struct _MTAINFO *next;
} MTAINFO, *PMTAINFO;

typedef struct _CLIENTINFO
{
	int		authlevel;
	BIGNUM		*pchallenge;
	char		*pchallengestr;
	RSA		*pclikey;
} CLIENT, *PCLIENT;

enum { AL_NONE, AL_FULL };

char *opcodes[] = {"None\t","Add\t","Inculpate","Del\t","Blocked\t","Inculpate Blocked","Exculpate"};

enum { OPCODE_NONE, OPCODE_PENDING_ADD, OPCODE_PENDING_INCULPATE, OPCODE_PENDING_DEL, OPCODE_BLOCKED, OPCODE_INCULPATE_BLOCKED, OPCODE_PENDING_EXCULPATE };

int	gAction		= 0;	/* deny = 0, add = 1*/
int	gPortNum	= 25;
int	gRuleNum	= 90;
int	gDebug		= 0;
int	debugmode	= 0;
int	gSdTcp		= INVALID_SOCKET;
MTAINFO	*gpMtaInfo	= NULL;
char	*gpMtaDbFname	= "/tmp/ipfwmtad.db";
RSA	*gChildRsa	= NULL;
char	*gChildRsaPKey	= NULL;
fd_set	gChildFds;
CLIENT	gChildClients[FD_SETSIZE];

#define TIME24HOURS (60 * 60 * 24)
#define USEIPFWDIRECT

#ifndef USEIPFWDIRECT
int serverIpfwAction(char *fmt, ...)
{	va_list	vl;
	char	*str;
	int	rc = -1;
	int	ec;

	if(fmt != NULL)
	{
		va_start(vl,fmt);
		rc = vasprintf(&str,fmt,vl);
		if(str != NULL || rc == -1)
		{
			ec = system(str);
			ec = WIFEXITED(ec) ? WEXITSTATUS(ec) : -1;
			if(debugmode > 2)
				printf("IpfwAction: rc: %d - %s",ec,str);
			free(str);
		}
		va_end(vl);
	}

	return(rc);
}
#endif

void MtaInfoIpfwSync(int needDelete)
{	PMTAINFO	pinfo = gpMtaInfo;
#ifdef USEIPFWDIRECT
	ipfw_startup();
#endif

	if(debugmode>1)
		printf("MtaInfoIpfwSync %s delete\n",needDelete ? "with" : "no");

	/* delete the mta rules */
	if(needDelete)
#ifdef USEIPFWDIRECT
		ipfw_del(gRuleNum);
#else
		serverIpfwAction("ipfw delete %u\n",gRuleNum);
#endif

	/* re-create/add the mta rules */
	while(pinfo != NULL)
	{
		switch(pinfo->opcode)
		{
			case OPCODE_INCULPATE_BLOCKED:
				/* deliberate fall thru to OPCODE_BLOCKED */
			case OPCODE_BLOCKED:
				if(pinfo->needAdd || needDelete)
				{
#ifdef USEIPFWDIRECT
					ipfw_add(gRuleNum,pinfo->ip,gPortNum,gAction);
#else
					serverIpfwAction("ipfw -q add %u deny tcp from %u.%u.%u.%u to any %u\n",gRuleNum,
						((pinfo->ip&0xff000000)>>24),((pinfo->ip&0xff0000)>>16),((pinfo->ip&0xff00)>>8),(pinfo->ip&0xff),gPortNum);
#endif
					pinfo->needAdd = 0;
				}
				break;
		}
		pinfo = pinfo->next;
	}
#ifdef USEIPFWDIRECT
	ipfw_shutdown();
#endif
}

void MtaInfoWriteDb(char *fname)
{	PMTAINFO	pinfo = gpMtaInfo;
	int		fdout;
	FILE		*fout;
	long		ip;
	char		*oldfname;
	char		tempfname[1024];

	strcpy(tempfname,"/tmp/ipfwmtadXXXXXXXX");

	/* create a new db from scratch */
	if((fdout = mkstemp(tempfname)) > -1 && (fout = fdopen(fdout,"w")) != NULL)
	{
		if(debugmode > 2)
			printf("MtaInfoWriteDb\n");
		while(pinfo != NULL)
		{
			ip = pinfo->ip;
			if(pinfo->opcode != OPCODE_NONE)
			{
				fprintf(fout,"%u.%u.%u.%u %lu %lu %lu %u\n",
					(int)((ip&0xff000000)>>24),(int)((ip&0xff0000)>>16),(int)((ip&0xff00)>>8),(int)(ip&0xff),
					(long)pinfo->timeFirst,(long)pinfo->timeExpire,pinfo->count,pinfo->opcode);
			}
			pinfo = pinfo->next;
		}
		fflush(fout);
		fclose(fout);

		/* save the existing db in case things go wrong */
		asprintf(&oldfname,"%s.bak",fname);
		if(oldfname != NULL && strlen(oldfname))
		{
			unlink(oldfname);
			rename(fname,oldfname);
			unlink(fname);
			rename(tempfname,fname);
			free(oldfname);
		}
		unlink(tempfname);
	}
}

void MtaInfoDumpItem(PMTAINFO pinfo)
{	char timestr[256];

	if(pinfo != NULL)
	{
		strcpy(timestr,ctime(&pinfo->timeExpire));
		*(timestr+strlen(timestr)-1) = '\0';
		
		printf("%03u.%03u.%03u.%03u\t%lu\t%s\t%s\n",
			(int)((pinfo->ip&0xff000000)>>24),(int)((pinfo->ip&0xff0000)>>16),(int)((pinfo->ip&0xff00)>>8),(int)(pinfo->ip&0xff),
			pinfo->count,timestr,opcodes[pinfo->opcode]);
	}
}

void MtaInfoReadDb(char *fname)
{	FILE		*fin;
	char		buf[1024];
	char		ipbuf[1024];
	time_t		first,expire,now = time(NULL);
	long		count,opcode;
	int		rc;
	PMTAINFO	pinfo;
	struct stat	fstat;

	memset(&fstat,0,sizeof(fstat));
	if(stat(fname,&fstat) == 0)
	{
		if(fstat.st_mode == (S_IRUSR|S_IWUSR|S_IFREG) && fstat.st_uid == 0 && fstat.st_gid == 0)
		{
			fin = fopen(fname,"r");
			while(fin != NULL && !feof(fin) && fgets(buf,sizeof(buf),fin) != NULL)
			{
				*(buf+strlen(buf)-1) = '\0';
				opcode = OPCODE_NONE;
				rc = sscanf(buf,"%s %lu %lu %lu %lu",ipbuf,(long *)&first,(long *)&expire,&count,&opcode);
				/* serialize the disk cache into memory */
				if(opcode != OPCODE_NONE && (rc == 1 || rc == 4 || rc ==5))
				{
					if(rc == 4 || rc == 5)
					{
						pinfo = calloc(1,sizeof(MTAINFO));
						pinfo->next		= gpMtaInfo;
						pinfo->ip		= ntohl(inet_addr(ipbuf));
						pinfo->timeFirst	= first;
						pinfo->timeExpire	= expire;
						pinfo->count		= count;
						if(rc == 4)
							pinfo->opcode	= OPCODE_BLOCKED;
						else
							pinfo->opcode	= opcode;
						if(pinfo->opcode == OPCODE_PENDING_EXCULPATE)
							pinfo->opcode = OPCODE_PENDING_INCULPATE;
						gpMtaInfo = pinfo;
					}
					else if(rc == 1)
					{
						pinfo = calloc(1,sizeof(MTAINFO));
						pinfo->next		= gpMtaInfo;
						pinfo->ip		= ntohl(inet_addr(ipbuf));
						pinfo->timeFirst	= now;
						pinfo->timeExpire	= now;
						pinfo->count		= 1;
						pinfo->opcode		= OPCODE_BLOCKED;
						gpMtaInfo = pinfo;
					}
					if(debugmode > 0)
						MtaInfoDumpItem(pinfo);
				}
			}
			if(fin != NULL)
				fclose(fin);
		}
		else
			syslog(LOG_ERR,"Warning - %s must be 0600/root:wheel only. Vulnerable database file not read!\n",fname);
	}
}

PMTAINFO MtaInfoFind(long ip)
{	PMTAINFO pinfo = gpMtaInfo;

	while(pinfo != NULL && pinfo->ip != ip)
		pinfo = pinfo->next;

	return(pinfo);
}

int MtaInfoSet(long ip, long interval, int opcode, int rate)
{	PMTAINFO	pinfo = MtaInfoFind(ip);
	int		rc = 500;

	if(pinfo == NULL)
	{
		pinfo = calloc(1,sizeof(MTAINFO));
		if(pinfo != NULL)
		{
			pinfo->ip		= ip;
			pinfo->timeFirst	= time(NULL);
			pinfo->next		= gpMtaInfo;
			gpMtaInfo = pinfo;
		}
	}

	if(pinfo != NULL)
	{
		switch(opcode)
		{
			case OPCODE_PENDING_ADD:
				pinfo->count ++;
				pinfo->timeInterval	= interval;
				pinfo->timeExpire	= time(NULL) + interval;
				pinfo->opcode		= opcode;
				if(debugmode > 0)
					printf("MtaInfoSet: add %u.%u.%u.%u\n",
						(int)((pinfo->ip&0xff000000)>>24),(int)((pinfo->ip&0xff0000)>>16),(int)((pinfo->ip&0xff00)>>8),(int)(pinfo->ip&0xff));
				rc = 220;
				break;

			case OPCODE_PENDING_DEL:
				pinfo->opcode		= opcode;
				if(debugmode > 0)
					printf("MtaInfoSet: del %u.%u.%u.%u\n",
						(int)((pinfo->ip&0xff000000)>>24),(int)((pinfo->ip&0xff0000)>>16),(int)((pinfo->ip&0xff00)>>8),(int)(pinfo->ip&0xff));
				rc = 220;
				break;

			case OPCODE_PENDING_INCULPATE:
				if(pinfo->opcode == OPCODE_PENDING_INCULPATE || pinfo->opcode == OPCODE_NONE)
				{
					pinfo->count ++;
					pinfo->timeInterval	= interval;
					pinfo->timeExpire	= time(NULL) + interval;
					pinfo->opcode		= opcode;
					pinfo->rate		= rate;
					pinfo->update		= 1;
					if(debugmode > 0)
						printf("MtaInfoSet: inculpate %u.%u.%u.%u\n",
							(int)((pinfo->ip&0xff000000)>>24),(int)((pinfo->ip&0xff0000)>>16),(int)((pinfo->ip&0xff00)>>8),(int)(pinfo->ip&0xff));
					rc = 220;
				}
				break;

			case OPCODE_PENDING_EXCULPATE:
				if(pinfo->count > 0 && pinfo->opcode == OPCODE_PENDING_INCULPATE)
				{
					pinfo->count --;
					pinfo->timeInterval	= interval;
					pinfo->timeExpire	-= interval;
					if(pinfo->count == 0)
						pinfo->opcode	= opcode;
					pinfo->rate		= rate;
					pinfo->update		= 1;
					if(debugmode > 0)
						printf("MtaInfoSet: exculpate %u.%u.%u.%u\n",
							(int)((pinfo->ip&0xff000000)>>24),(int)((pinfo->ip&0xff0000)>>16),(int)((pinfo->ip&0xff00)>>8),(int)(pinfo->ip&0xff));
					rc = 220;
				}
				break;
		}
	}

	return(rc);
}

int MtaInfoDelete(PMTAINFO pinfodel)
{	PMTAINFO	pinfo = gpMtaInfo;
	int		needDelete = 0;

	while(pinfo != NULL && !needDelete)
	{
		/* if the current mta host matches the one to be deleted, then delete it */
		if(pinfodel != NULL && pinfo == pinfodel)
		{
			pinfo = gpMtaInfo = pinfo->next;
			free(pinfodel);
			needDelete = 1;
		}
		else if(pinfodel != NULL && pinfo->next == pinfodel)
		{
			pinfo->next = pinfodel->next;
			free(pinfodel);
			needDelete = 1;
		}

		if(pinfo != NULL)
			pinfo = pinfo->next;
	}

	return(needDelete);
}

void MtaInfoStateMachineUpdate(char *fname)
{	PMTAINFO	pinfo = gpMtaInfo;
	time_t		now = time(NULL);
	int		needDelete = 0;
	int		needAdd = 0;
	int		needUpdate = 0;

	/* handle MTAs in the list based on rules */
	pinfo = gpMtaInfo;
	while(pinfo != NULL)
	{
		switch(pinfo->opcode)
		{
			case OPCODE_PENDING_ADD:
				if(now >= pinfo->timeFirst + 15)
				{
					if(debugmode > 0)
						printf("MtaInfoStateMachineUpdate: add %u.%u.%u.%u\n",
							(int)((pinfo->ip&0xff000000)>>24),(int)((pinfo->ip&0xff0000)>>16),(int)((pinfo->ip&0xff00)>>8),(int)(pinfo->ip&0xff));
					pinfo->opcode = OPCODE_BLOCKED;
					pinfo->needAdd = needAdd = 1;
				}
				break;

			case OPCODE_PENDING_INCULPATE:
				if(pinfo->count > 0 && now > pinfo->timeFirst + (60 * 60))
				{
					if(debugmode > 0)
						printf("MtaInfoStateMachineUpdate: reset inculpated %u.%u.%u.%u\n",
							(int)((pinfo->ip&0xff000000)>>24),(int)((pinfo->ip&0xff0000)>>16),(int)((pinfo->ip&0xff00)>>8),(int)(pinfo->ip&0xff));
					needUpdate = MtaInfoDelete(pinfo);
				}
				/* more than 'interval' times per minute = broken server / persitent spammer / pain in the ass! */
				else if(pinfo->count > 0 && pinfo->rate > 0 && ((60 * 60) / pinfo->count) < ((60 * 60) / pinfo->rate))
				{
					if(debugmode > 0)
						printf("MtaInfoStateMachineUpdate: add inculpated %u.%u.%u.%u - calculated: %lu, rate: %u\n",
							(int)((pinfo->ip&0xff000000)>>24),(int)((pinfo->ip&0xff0000)>>16),(int)((pinfo->ip&0xff00)>>8),(int)(pinfo->ip&0xff),
							(60 / pinfo->count),(60 / pinfo->rate));
					pinfo->opcode = OPCODE_INCULPATE_BLOCKED;
					pinfo->update = 0;
					pinfo->needAdd = needAdd = 1;
				}
				else if(pinfo->update)
				{
					pinfo->update = 0;
					needUpdate = 1;
				}
				break;

			case OPCODE_PENDING_EXCULPATE:
				if(pinfo->update)
				{
					if(debugmode > 0)
						printf("MtaInfoStateMachineUpdate: exculpate %u.%u.%u.%u\n",
							(int)((pinfo->ip&0xff000000)>>24),(int)((pinfo->ip&0xff0000)>>16),(int)((pinfo->ip&0xff00)>>8),(int)(pinfo->ip&0xff));
					if(pinfo->count == 0)
						needDelete = MtaInfoDelete(pinfo);
					else
						pinfo->update = 0;
					needUpdate = 1;
				}
				break;

			case OPCODE_PENDING_DEL:
				if(debugmode > 0)
					printf("MtaInfoStateMachineUpdate: delete %u.%u.%u.%u\n",
						(int)((pinfo->ip&0xff000000)>>24),(int)((pinfo->ip&0xff0000)>>16),(int)((pinfo->ip&0xff00)>>8),(int)(pinfo->ip&0xff));
				needDelete = MtaInfoDelete(pinfo);
				break;

			case OPCODE_INCULPATE_BLOCKED:
				/* deliberate fall thru to OPCODE_BLOCKED */
			case OPCODE_BLOCKED:
				if(now >= pinfo->timeExpire)
				{
					if(debugmode > 0)
						printf("MtaInfoStateMachineUpdate: expire %u.%u.%u.%u\n",
							(int)((pinfo->ip&0xff000000)>>24),(int)((pinfo->ip&0xff0000)>>16),(int)((pinfo->ip&0xff00)>>8),(int)(pinfo->ip&0xff));
					needDelete = MtaInfoDelete(pinfo);
				}
				break;
		}
		pinfo = pinfo->next;
	}

	if(needDelete || needAdd || needUpdate)
	{
		if(needDelete || needAdd)
			MtaInfoIpfwSync(needDelete);
		MtaInfoWriteDb(fname);
	}
}

int MtaInfoDump(int sd, long ip, int stat)
{	PMTAINFO	pinfo = gpMtaInfo;
	int		rc = sd != -1 && pinfo != NULL ? 220 : 500;
	char		timestr[256];
	int		done = 0;

	while(pinfo != NULL && !done)
	{
		if(ip == -1 || ip == 0 || pinfo->ip == ip)
		{
			done = pinfo->ip == ip;
			strcpy(timestr,ctime(&pinfo->timeExpire));
			*(timestr+strlen(timestr)-1) = '\0';
			NetSockPrintf(sd,"%03u%c%03u.%03u.%03u.%03u\t%u\t%s\t%s\n",stat,pinfo->next != NULL && !done ? '-' : ' ',
				(int)((pinfo->ip&0xff000000)>>24),(int)((pinfo->ip&0xff0000)>>16),(int)((pinfo->ip&0xff00)>>8),(int)(pinfo->ip&0xff),
				pinfo->count,timestr,opcodes[pinfo->opcode]);
		}
		pinfo = pinfo->next;
	}
	if(ip > 0 && !done)
		NetSockPrintf(sd,"220 Not found\n");

	return(rc);
}

void clientSessionAuth(int sd, char *buf)
{	PCLIENT	pclient = &gChildClients[sd];

	/* read the client key, and respond with encrypted challenge */
	if(strncasecmp(buf,"key,rsa ",8) == 0)
	{	char	*p = buf+8;

		/* setup for the client key */
		if(pclient->pclikey == NULL)
			pclient->pclikey = key_new();

		if(pclient->pclikey != NULL && key_read(pclient->pclikey,(unsigned char **)&p))
		{	char	*pkt = NULL;
			BN_CTX	*pbnctx = BN_CTX_new();

			/* generate challenge for the client session */
			if(pbnctx != NULL)
			{	BIGNUM *pencchallenge = BN_new();

				/* clear/free old */
				if(pclient->pchallenge != NULL)
					BN_clear_free(pclient->pchallenge);
				/* new challenge */
				if(pclient->pchallengestr != NULL)
				{
					free(pclient->pchallengestr);
					pclient->pchallengestr = NULL;
				}
				if((pclient->pchallenge = BN_new()) != NULL)
				{
					BN_rand(pclient->pchallenge,256,0,0);
					BN_mod(pclient->pchallenge,pclient->pchallenge,pclient->pclikey->n,pbnctx);
					pclient->pchallengestr = BN_bn2hex(pclient->pchallenge);
				}

				/* encrypted */
				if(pencchallenge != NULL && pclient->pchallenge != NULL && key_bn_encrypt(pclient->pclikey,pclient->pchallenge,pencchallenge))
					/* in ascii form */
					pkt = BN_bn2dec(pencchallenge);
				BN_clear_free(pencchallenge);
			}

			if(pkt != NULL)
			{
				NetSockPrintf(sd,"220-rnd %s\r\n220 OK\r\n",pkt);
				memset(pkt,0,strlen(pkt));
				free(pkt);
			}
			else
				NetSockPrintf(sd,"221 error - Unable to generate challenge\r\n");
		}
		else
			NetSockPrintf(sd,"221 error - Unable to read key\r\n");
	}
	else if(strncasecmp(buf,"auth,",5) == 0)
	{	int	encrndlen = 0;
		char	*pencrnd = (char *)key_asctobin((unsigned char *)buf+5,&encrndlen);
		char	*pdecrnd = NULL;
		int	decrndlen = 0;

		decrndlen = key_decrypt(gChildRsa,(unsigned char *)pencrnd,encrndlen,(unsigned char **)&pdecrnd);

		if(decrndlen > 0)
		{	char *psig = strchr(pdecrnd,';');
			char *passwd = strchr(pdecrnd,':');

			if(psig != NULL)
				*(psig++) = '\0';
			if(passwd != NULL)
				*(passwd++) = '\0';
			if(psig != NULL && strcmp(psig,pclient->pchallengestr) == 0 && passwd != NULL &&
#ifdef SUPPORT_PAM
				pam_authuserpass(pdecrnd,passwd)
#else
				uam_authuserpass(pdecrnd,passwd)
#endif
				)
			{
				NetSockPrintf(sd,"220 OK - Authenticated\r\n");
				pclient->authlevel = AL_FULL;
			}
			else
				NetSockPrintf(sd,"221 error - Invalid signature\r\n");
		}
		else
			NetSockPrintf(sd,"221 error - unable to decrypt auth\r\n");
	}
}

void clientSessionReadLine(int sd, char *buf)
{	int	rc = 500;
	long	ip = -1;
	long	interval = 2 * TIME24HOURS;
	int	rate = 14;
	char	bufcmd[10];
	char	bufip[50];
	char	bufinterval[50];
	char	bufrate[50];

	if(debugmode > 1)
		printf("clientReadLine: %u '%s'\n",sd,buf);

	/* syntax;
	 *	cmd , ip address [, interval [, rate]]
	 * where;
	 *	cmd		= [ add | del | inculpate | exculpate ]
	 *	ip address	= an ipv4 dotted quad
	 *	interval	= an integer specifiying number of days to block (nb. 2 days is default if not specified)
	 *	rate		= connections per minute threshold if cmd = inculpate (nb. 14 is default if not specified)
	 */

	clientSessionAuth(sd,buf);

	buf = mlfi_strcpyadv(bufcmd,sizeof(bufcmd),buf,',');
	buf = mlfi_strcpyadv(bufip,sizeof(bufip),buf,',');
	buf = mlfi_strcpyadv(bufinterval,sizeof(bufinterval),buf,',');
	buf = mlfi_strcpyadv(bufrate,sizeof(bufrate),buf,',');

	if(strlen(bufip))
		ip = ntohl(inet_addr(bufip));

	if(strlen(bufinterval))
		interval = TIME24HOURS * atoi(bufinterval);

	if(strlen(bufrate))
		rate = atoi(bufrate);

	if(gChildClients[sd].authlevel == AL_FULL && ip != -1 && ip != 0)
	{
		if(strcasecmp(bufcmd,"add") == 0)
			rc = MtaInfoSet(ip,interval,OPCODE_PENDING_ADD,0);
		else if(strcasecmp(bufcmd,"del") == 0)
			rc = MtaInfoSet(ip,interval,OPCODE_PENDING_DEL,0);
		else if(strcasecmp(bufcmd,"inculpate") == 0 || strcasecmp(bufcmd,"nominate") == 0)
			rc = MtaInfoSet(ip,interval,OPCODE_PENDING_INCULPATE,rate);
		else if(strcasecmp(bufcmd,"exculpate") == 0)
			rc = MtaInfoSet(ip,interval,OPCODE_PENDING_EXCULPATE,rate);
	}

	if(strcasecmp(bufcmd,"status") == 0)
		rc = MtaInfoDump(sd,ip,250);
}

void clientSessionReadLines(int i, fd_set *fds)
{	char	buff[1024];
	int	rc;

	/* this loop gives opportunity for a DOS that would starve other clients */
	while((rc = NetSockGets(i,buff,sizeof(buff),1)) > 0)
		clientSessionReadLine(i,buff);

	if(rc == 0)	/* remove the peer from the socket list */
	{	PCLIENT	pclient = &gChildClients[i];

		if(debugmode>1)
			printf("clientReadSessionLines: client disconnect: %u\n",i);
		FD_CLR(i,fds);
		shutdown(i,SHUT_RDWR);
		close(i);

		if(pclient->pchallenge != NULL)
			BN_clear_free(pclient->pchallenge);
		if(pclient->pchallengestr != NULL)
			free(pclient->pchallengestr);
		if(pclient->pclikey != NULL)
			key_free(pclient->pclikey);

		/* clean up for next use */
		memset(pclient,0,sizeof(CLIENT));
	}
}

void clientSessionAccept(int sdTcp, fd_set *fds)
{	struct sockaddr_in	sin;
	socklen_t		sinlen = sizeof(sin);
	int			rc;

	rc = accept(sdTcp,(struct sockaddr *)&sin,&sinlen);
	if(rc != SOCKET_ERROR)
	{	PCLIENT	pclient = &gChildClients[rc];
		
		/* socket setup */
		/*NetSockOpt(rc,TCP_NODELAY,1); */
		NetSockOptNoLinger(rc);
		NetSockOpt(rc,SO_KEEPALIVE,1);
		FD_SET(rc,fds);
		if(debugmode>1)
			printf("clientAccept: client connect: %u\n",rc);

		/* set authentictation level */
		pclient->authlevel = (sin.sin_family == AF_INET && ntohl(sin.sin_addr.s_addr) == INADDR_LOOPBACK) ? AL_FULL : AL_NONE;
		/*pclient->authlevel = AL_NONE;*/

		if(pclient->authlevel == AL_NONE)
			NetSockPrintf(rc,"220-agent ipfwmtad/0.4\r\n220-key %s\r\n",gChildRsaPKey);
		else
			NetSockPrintf(rc,"220-agent ipfwmtad/0.4\r\n");
		NetSockPrintf(rc,"220 OK\r\n");
	}
}

void socketScan(fd_set *fds, int sdTcp)
{	fd_set		sfds;
	struct timeval	tv;
	int		i,rc;

	tv.tv_sec = 1;
	tv.tv_usec = 0;
	memcpy(&sfds,fds,sizeof(sfds));
	rc = select(FD_SETSIZE,&sfds,NULL,NULL,&tv);

	/* find a socket that needs attention */
	for(i=0; rc != SOCKET_ERROR && i<FD_SETSIZE; i++)
	{
		if(FD_ISSET(i,&sfds))
		{
			if(i == sdTcp)	/* add a connecting peer to the socket list */
				clientSessionAccept(sdTcp,fds);
			else	/* service a connected peer */
				clientSessionReadLines(i,fds);
		}
	}
}

void childShutdown();

void child_signal_hndlr(int signo)
{
	switch(signo)
	{
		case SIGHUP:
		case SIGPIPE:
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
{	int	i;

	/* close all open ports */
	for(i=0; i<FD_SETSIZE; i++)
	{
		if(FD_ISSET(i,&gChildFds))
		{
			shutdown(i,SHUT_RDWR);
			close(i);
		}
	}

	closelog();
}

int childMain(short port)
{	int	quit	= 0;
	int	rc = -1;
	int	mtaUpdateInterval = 1;
	long	mtaUpdate = time(NULL) + mtaUpdateInterval;

	signal(SIGTERM,child_signal_hndlr);
	signal(SIGQUIT,child_signal_hndlr);
	signal(SIGINT,child_signal_hndlr);
	signal(SIGHUP,child_signal_hndlr);
	signal(SIGSEGV,child_signal_hndlr);
	signal(SIGPIPE,child_signal_hndlr);

	syslog(LOG_ERR,"Child: Started using '%u'",port );

	gChildRsa = key_generate(1024);
	gChildRsaPKey = (char *)key_pubkeytoasc(gChildRsa);

	memset(&gChildClients,0,sizeof(gChildClients));

	gSdTcp	= NetSockOpenTcpListen(INADDR_LOOPBACK,port);

	if(gSdTcp != INVALID_SOCKET)
	{
		FD_ZERO(&gChildFds);
		FD_SET(gSdTcp,&gChildFds);

		MtaInfoReadDb(gpMtaDbFname);
		MtaInfoIpfwSync(1);

		while(!quit)
		{
			socketScan(&gChildFds,gSdTcp);
			if(time(NULL) > mtaUpdate)
			{
				MtaInfoStateMachineUpdate(gpMtaDbFname);
				mtaUpdate = time(NULL) + mtaUpdateInterval;
			}
		}
		rc = 0;
		syslog(LOG_ERR,"Child: Exiting: normal");
	}
	else
	{
		if(gSdTcp == INVALID_SOCKET)
			syslog(LOG_ERR,"Child: Exiting: Invalid Tcp socket");
	}

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
		openlog("ipfwmtad",flags|LOG_NDELAY|LOG_PID,LOG_DAEMON);
		if(childMain(port) == -1)
		{
			syslog(LOG_ERR,"Error starting child, sleeping 30 seconds, %u retries left.",count);
			sleep(30000);
		}
	}
}

void cliIpfwAction(char *ipstr, char *action)
{	int sd = NetSockOpenTcpPeer(INADDR_LOOPBACK,4739);
	char	*user = "neal";
	char	*pass = "neal";

	if(sd != INVALID_SOCKET)
	{	char	buf[8192];
		RSA	*psrvkey = key_new();
		RSA	*pclikey = key_generate(512);
		char	*pclikeystr = (char *)key_pubkeytoasc(pclikey);
		char	*psrvrnd = NULL;
		int	done = 0;

		while(!done && psrvrnd == NULL && NetSockGets(sd,buf,sizeof(buf)-1,5) > 0)
		{	char *p = buf;

			/* read in the server public key, and send ours in response */
			if(strncasecmp(buf,"220-key rsa ",12) == 0 && *(p+=12) && key_read(psrvkey,(unsigned char **)&p) && pclikeystr != NULL)
				NetSockPrintf(sd,"key,%s\r\n",pclikeystr);
			/* read in the server entropy and decrypt it */
			if(strncasecmp(buf,"220-rnd ",8) == 0)
			{	BIGNUM	*bni	= NULL;
				BIGNUM	*bno	= BN_new();

				BN_dec2bn(&bni,buf+8);
				key_bn_decrypt(pclikey,bni,bno);
				psrvrnd = BN_bn2hex(bno);
			}
			done = (*(buf+3) == ' ');
		}

		/* if we have entropy from the server, use it to login */
		if(psrvrnd != NULL && user != NULL && pass != NULL)
		{	char	*pkt;

			asprintf(&pkt,"%s:%s;%s",user,pass,psrvrnd);
			pkt = (char *)key_encrypttoasc(psrvkey,(unsigned char *)pkt,strlen(pkt));
			if(pkt != NULL)
			{
				NetSockPrintf(sd,"auth,%s\r\n",pkt);
				free(pkt);
			}

			memset(buf,0,sizeof(buf));
			done = 0;
			while(!done && NetSockGets(sd,buf,sizeof(buf)-1,5) > 0)
				done = *(buf+3) == ' ';

		}

		if(done)
			NetSockPrintf(sd,"%s,%s\r\n",action,ipstr);

		memset(buf,0,sizeof(buf));
		while(NetSockGets(sd,buf,sizeof(buf)-1,1) > 0)
		{
			if(debugmode>1)
				printf("%s\n",buf);
		}
		NetSockPrintf(sd,"\r\n");
		NetSockClose(&sd);
	}
}

void usage()
{
	printf("usage: [-dp -n fname] [-u rule number] | [-i fname] | [-r ipaddress] | [-q ipaddress]\n"
		"\t-d = debug mode\n"
		"\t-p = server tcp port number\n"
		"\t-n = server mode - ip database file name\n"
		"\t-u = server mode - ipfw rule number\n"
		"\t-p = server mode - ipfw port number\n"
		"\t-b = server mode - ipfw action (add/deny)\n"
		"\t-i = imeadiate mode - ipfw resync using the ip database file name\n"
		"\t-r = imeadiate mode - queue ipaddress for removal\n"
		"\t-a = imeadiate mode - queue ipaddress for addition\n"
		"\t-q = imeadiate mode - query ipaddress\n"
		);
}

int main(int argc, char **argv)
{	int	count = 3;
	short	port = (short)4739;
	int	forking = 1;
	int	servermode = 1;
	char	opt;
	char	*optflags = "d:p:n:u:i:r:a:q:o:b:";

	if(getuid() != 0)
	{
		printf("Not ROOT, exiting.\n");
		exit(0);
	}

	while((opt = getopt(argc,argv,optflags)) != -1)
	{
		switch(opt)
		{
			case 'd':
				if(optarg != NULL && *optarg)
					forking = 1 - ((debugmode = atoi(optarg))>0);
				if(debugmode)
					printf("debug mode\n");
				break;

			/* server mode stuff */
			case 'p':
				if(optarg != NULL && *optarg)
					port = (short)atoi(optarg);
				break;
			case 'n':
				if(optarg != NULL && *optarg)
					gpMtaDbFname = optarg;
				break;
			case 'u':
				if(optarg != NULL && *optarg)
					gRuleNum = atoi(optarg);
				break;
			case 'o':
				if(optarg != NULL && *optarg)
					gPortNum = atoi(optarg);
				break;
			case 'b':
				if(optarg != NULL && *optarg)
					gAction = (strcasecmp("add",optarg) == 0);
				break;

			/* imeadiate mode stuff */
			case 'i':
				if(optarg != NULL && *optarg)
					gpMtaDbFname = optarg;
				MtaInfoReadDb(gpMtaDbFname);
				MtaInfoIpfwSync(0);
				return(0);
				break;
			case 'a':
				if(optarg != NULL && *optarg)
				{
					servermode = 0;
					cliIpfwAction(optarg, "add");
				}
				break;
			case 'r':
				if(optarg != NULL && *optarg)
				{
					servermode = 0;
					cliIpfwAction(optarg, "del");
				}
				break;
			case 'q':
				if(optarg != NULL && *optarg)
				{
					servermode = 0;
					cliIpfwAction(optarg, "status");
				}
				break;
			case '?':
			default:
				usage();
				return(1);
				break;
		}
	}
	argc -= optind;
	argv += optind;

	if(servermode)
	{

		if(forking)
		{
			if(fork() == 0)
				childStart(port,count,1);
		}
		else
			childStart(port,count,0);
	}

	return(0);
}
