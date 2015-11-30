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
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "inet.h"
#include "misc.h"
#include "key.h"
#include "list.h"
#include "ifidb.h"

#include "ipfw_direct.h"
#include "ipfw_mtacli.h"

#ifdef SUPPORT_PAM
#include "pam.h"
#else
#include "uam.h"
#endif

typedef struct _MTAINFO
{
	char	ip[INET6_ADDRSTRLEN+1];
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
	char		*pIpStr;
	unsigned short	ipPort;
} CLIENT, *PCLIENT;

enum { AL_NONE, AL_FULL };

char *opcodes[] = {"None\t", "Pending Add", "Inculpate", "Pending Del", "Blocked\t", "Inculpate Blocked", "Pending Exculpate", "Pending FW Remove", "FW Removed"};

enum { OPCODE_NONE, OPCODE_PENDING_ADD, OPCODE_PENDING_INCULPATE, OPCODE_PENDING_DEL, OPCODE_BLOCKED, OPCODE_INCULPATE_BLOCKED, OPCODE_PENDING_EXCULPATE, OPCODE_PENDING_FWREMOVE, OPCODE_FWREMOVED, };

int	gAction		= 0;	// deny = 0, add = 1
int	gPortNum	= 25;
int	gRuleNum	= 90;
int	gDebug		= 0;
int	debugmode	= 0;
int	gSdTcp		= INVALID_SOCKET;
MTAINFO	*gpMtaInfo	= NULL;
const char *gpMtaDbFname	= "/tmp/ipfwmtad.db";
RSA	*gChildRsa	= NULL;
char	*gChildRsaPKey	= NULL;
fd_set	gChildFds;
CLIENT	gChildClients[FD_SETSIZE];

ifiDbCtx_t *gpClientACLCtx = NULL;
const char *gpClientACLFname = CONF_DIR"/ipfwmtad.acl";

#define TIME24HOURS (60 * 60 * 24)

void MtaInfoIpfwSync(int needDelete)
{	PMTAINFO pinfo = gpMtaInfo;
	unsigned short mask = 0;
	char tmp[sizeof(struct in6_addr)+1];

#if defined(USEIPFWDIRECT) && defined(OS_FreeBSD)
	ipfw_startup();
#endif

	if(debugmode>1)
		printf("MtaInfoIpfwSync %s delete\n", needDelete ? "with" : "no");

#if defined(OS_FreeBSD)
	// delete the mta rules
	if(needDelete)
#if defined(USEIPFWDIRECT)
		ipfw_del(gRuleNum);
#else
		mlfi_systemPrintf("ipfw delete %u\n", gRuleNum);
#endif
#endif

	// add/remove the mta rules
	while(pinfo != NULL)
	{
		mask = (inet_pton(AF_INET, pinfo->ip, &tmp) ? 32 :
			inet_pton(AF_INET6, pinfo->ip, &tmp) ? 128 :
			0
			);

		switch(pinfo->opcode)
		{
			case OPCODE_PENDING_FWREMOVE:
#ifdef OS_Linux
				mlfi_systemPrintf("iptables -D SPAMILTER -s %s/%u -j DROP (or -j REJECT)\n", pinfo->ip, mask);
#endif
				pinfo->opcode = OPCODE_FWREMOVED;
				break;

			case OPCODE_INCULPATE_BLOCKED:
				// deliberate fall thru to OPCODE_BLOCKED
			case OPCODE_BLOCKED:
				if(pinfo->needAdd || needDelete)
				{
#if defined(OS_FreeBSD)
#if defined(USEIPFWDIRECT)
					ipfw_add(gRuleNum, pinfo->ip, gPortNum, gAction);
#else
					mlfi_systemPrintf("ipfw -q add %u deny tcp from %s to any %u\n", gRuleNum, pinfo->ip, gPortNum);
#endif
#endif
#ifdef OS_Linux
					mlfi_systemPrintf("iptables -I SPAMILTER -s %u.%u.%u.%u/%u -j DROP (or -j REJECT)\n", pinfo->ip, mask);
#endif
					pinfo->needAdd = 0;
				}
				break;
		}
		pinfo = pinfo->next;
	}
#if defined(USEIPFWDIRECT) && defined(OS_FreeBSD)
	ipfw_shutdown();
#endif
}

void MtaInfoWriteDb(const char *fname)
{	PMTAINFO	pinfo = gpMtaInfo;
	int		fdout;
	FILE		*fout;
	char		*oldfname;
	char		tempfname[1024];

	strcpy(tempfname, "/tmp/ipfwmtadXXXXXXXX");

	// create a new db from scratch
	if((fdout = mkstemp(tempfname)) > -1 && (fout = fdopen(fdout, "w")) != NULL)
	{
		if(debugmode > 2)
			printf("MtaInfoWriteDb\n");
		while(pinfo != NULL)
		{
			if(pinfo->opcode != OPCODE_NONE)
			{
				fprintf(fout, "%s %lu %lu %lu %u\n"
					, pinfo->ip, (long)pinfo->timeFirst, (long)pinfo->timeExpire, pinfo->count, pinfo->opcode);
			}

			pinfo = pinfo->next;
		}
		fflush(fout);
		fclose(fout);

		// save the existing db in case things go wrong
		asprintf(&oldfname, "%s.bak", fname);
		if(oldfname != NULL && strlen(oldfname))
		{
			unlink(oldfname);
			rename(fname, oldfname);
			unlink(fname);
			rename(tempfname, fname);
			free(oldfname);
		}
		unlink(tempfname);
	}
}

void MtaInfoDumpItem(PMTAINFO pinfo)
{	char timestr[256];

	if(pinfo != NULL)
	{
		strcpy(timestr, ctime(&pinfo->timeExpire));
		*(timestr+strlen(timestr)-1) = '\0';
		
		printf("%s\t%lu\t%s\t%s\n", pinfo->ip, pinfo->count, timestr, opcodes[pinfo->opcode]);
	}
}

void MtaInfoReadDb(const char *fname)
{	FILE		*fin;
	char		buf[2048];
	char		ipbuf[1024];
	time_t		first, expire, now = time(NULL);
	long		count, opcode;
	int		rc;
	PMTAINFO	pinfo;
	struct stat	fstat;

	memset(&fstat, 0, sizeof(fstat));
	if(stat(fname, &fstat) == 0)
	{
		if(fstat.st_mode == (S_IRUSR|S_IWUSR|S_IFREG) && fstat.st_uid == 0 && fstat.st_gid == 0)
		{
			fin = fopen(fname, "r");
			while(fin != NULL && !feof(fin) && fgets(buf, sizeof(buf), fin) != NULL)
			{
				*(buf+strlen(buf)-1) = '\0';
				opcode = OPCODE_NONE;
				rc = sscanf(buf, "%s %lu %lu %lu %lu", ipbuf, (long *)&first, (long *)&expire, &count, &opcode);
				// serialize the disk cache into memory
				if(opcode != OPCODE_NONE && (rc == 1 || rc == 4 || rc ==5))
				{
					if(rc == 4 || rc == 5)
					{
						pinfo = calloc(1, sizeof(MTAINFO));
						pinfo->next		= gpMtaInfo;
						strncpy(pinfo->ip, ipbuf, sizeof(pinfo->ip));
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
						pinfo = calloc(1, sizeof(MTAINFO));
						pinfo->next		= gpMtaInfo;
						strncpy(pinfo->ip, ipbuf, sizeof(pinfo->ip));
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
			syslog(LOG_ERR, "Warning - %s must be 0600/root:wheel only. Vulnerable database file not read!\n", fname);
	}
}

PMTAINFO MtaInfoFind(char *pIpStr)
{	PMTAINFO pinfo = NULL;

	if(pIpStr != NULL)
	{
		pinfo = gpMtaInfo;

		while(pinfo != NULL && strcmp(pinfo->ip, pIpStr) != 0)
			pinfo = pinfo->next;
	}

	return pinfo;
}

int MtaInfoSet(int afType, char *pAfAddr, long interval, int opcode, int rate)
{
	char *pIpStr = mlfi_inet_ntopAF(afType, pAfAddr);
	PMTAINFO pinfo = MtaInfoFind(pIpStr);
	int rc = 500;

	if(pinfo == NULL)
	{
		pinfo = calloc(1, sizeof(MTAINFO));
		if(pinfo != NULL)
		{
			strncpy(pinfo->ip, pIpStr, sizeof(pinfo->ip));
			pinfo->timeFirst = time(NULL);
			pinfo->next = gpMtaInfo;
			gpMtaInfo = pinfo;
		}
	}

	if(pinfo != NULL)
	{
		switch(opcode)
		{
			case OPCODE_PENDING_ADD:
				pinfo->count ++;
				pinfo->timeInterval = interval;
				pinfo->timeExpire = time(NULL) + interval;
				pinfo->opcode = opcode;
				if(debugmode > 0)
					printf("MtaInfoSet: add %s\n", pIpStr);

				rc = 220;
				break;

			case OPCODE_PENDING_DEL:
				pinfo->opcode = opcode;
				if(debugmode > 0)
					printf("MtaInfoSet: del %s\n", pIpStr);
				rc = 220;
				break;

			case OPCODE_PENDING_INCULPATE:
				if(pinfo->opcode == OPCODE_PENDING_INCULPATE || pinfo->opcode == OPCODE_NONE)
				{
					pinfo->count ++;
					pinfo->timeInterval = interval;
					pinfo->timeExpire = time(NULL) + interval;
					pinfo->opcode = opcode;
					pinfo->rate = rate;
					pinfo->update = 1;
					if(debugmode > 0)
						printf("MtaInfoSet: inculpate %s\n", pIpStr);
					rc = 220;
				}
				break;

			case OPCODE_PENDING_EXCULPATE:
				if(pinfo->count > 0 && pinfo->opcode == OPCODE_PENDING_INCULPATE)
				{
					pinfo->count --;
					pinfo->timeInterval = interval;
					pinfo->timeExpire -= interval;
					if(pinfo->count == 0)
						pinfo->opcode	= opcode;
					pinfo->rate = rate;
					pinfo->update = 1;
					if(debugmode > 0)
						printf("MtaInfoSet: exculpate %s\n", pIpStr);
					rc = 220;
				}
				break;
		}
	}

	if(pIpStr != NULL)
		free(pIpStr);

	return rc;
}

int MtaInfoDelete(PMTAINFO pinfodel)
{	PMTAINFO	pinfo = gpMtaInfo;
	int		needDelete = 0;

	while(pinfo != NULL && !needDelete)
	{
		// if the current mta host matches the one to be deleted, then delete it
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

	return needDelete;
}

void MtaInfoStateMachineUpdate(const char *fname)
{	PMTAINFO	pinfo = gpMtaInfo;
	time_t		now = time(NULL);
	int		needDelete = 0;
	int		needAdd = 0;
	int		needUpdate = 0;

	// handle MTAs in the list based on rules
	pinfo = gpMtaInfo;
	while(pinfo != NULL)
	{
		switch(pinfo->opcode)
		{
			case OPCODE_PENDING_ADD:
				if(now >= pinfo->timeFirst + 15)
				{
					if(debugmode > 0)
						printf("MtaInfoStateMachineUpdate: add %s\n", pinfo->ip);
					pinfo->opcode = OPCODE_BLOCKED;
					pinfo->needAdd = needAdd = 1;
				}
				break;

			case OPCODE_PENDING_INCULPATE:
				if(pinfo->count > 0 && now > pinfo->timeFirst + (60 * 60))
				{
					if(debugmode > 0)
						printf("MtaInfoStateMachineUpdate: reset inculpated %s\n", pinfo->ip);
					needUpdate = MtaInfoDelete(pinfo);
				}
				// more than 'interval' times per minute = broken server / persitent spammer / pain in the ass!
				else if(pinfo->count > 0 && pinfo->rate > 0 && ((60 * 60) / pinfo->count) < ((60 * 60) / pinfo->rate))
				{
					if(debugmode > 0)
						printf("MtaInfoStateMachineUpdate: add inculpated %s - calculated: %lu, rate: %u\n",
							pinfo->ip, (60 / pinfo->count), (60 / pinfo->rate)
							);
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
						printf("MtaInfoStateMachineUpdate: exculpate %s\n", pinfo->ip);
					if(pinfo->count == 0)
					{
						needDelete = 1;
						pinfo->opcode = OPCODE_PENDING_FWREMOVE;
					}
					else
						pinfo->update = 0;
					needUpdate = 1;
				}
				break;

			case OPCODE_PENDING_DEL:
				if(debugmode > 0)
					printf("MtaInfoStateMachineUpdate: delete %s\n", pinfo->ip);
				needDelete = 1;
				pinfo->opcode = OPCODE_PENDING_FWREMOVE;
				break;

			case OPCODE_INCULPATE_BLOCKED:
				// deliberate fall thru to OPCODE_BLOCKED
			case OPCODE_BLOCKED:
				if(now >= pinfo->timeExpire)
				{
					if(debugmode > 0)
						printf("MtaInfoStateMachineUpdate: expire %s\n", pinfo->ip);
					needDelete = 1;
					pinfo->opcode = OPCODE_PENDING_FWREMOVE;
				}
				break;

			case OPCODE_FWREMOVED:
				MtaInfoDelete(pinfo);
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

int MtaInfoDump(int sd, int afType, char *pAfAddr, int stat)
{	PMTAINFO pinfo = gpMtaInfo;
	char *pIpStr = mlfi_inet_ntopAF(afType, pAfAddr);
	int rc = (sd != -1 && pinfo != NULL ? 220 : 500);
	char timestr[256];
	int done = 0;

	while(pinfo != NULL && !done)
	{
		if(pIpStr == NULL || (pIpStr != NULL && strcmp(pinfo->ip, pIpStr) == 0))
		{
			done = (pIpStr != NULL && strcmp(pinfo->ip, pIpStr) == 0);
			strcpy(timestr, ctime(&pinfo->timeExpire));
			*(timestr+strlen(timestr)-1) = '\0';
			NetSockPrintf(sd, "%03u%c%s\t%u\t%s\t%s\n", stat, pinfo->next != NULL && !done ? '-' : ' ',
				pinfo->ip, pinfo->count, timestr, opcodes[pinfo->opcode]
				);
		}
		pinfo = pinfo->next;
	}
	if(afType != AF_UNSPEC && !done)
		NetSockPrintf(sd, "220 Not found\n");

	if(pIpStr != NULL)
		free(pIpStr);

	return rc;
}

void clientSessionAuth(int sd, char *buf)
{	PCLIENT	pclient = &gChildClients[sd];

	// read the client key, and respond with encrypted challenge
	if(strncasecmp(buf, "key,rsa ", 8) == 0)
	{	char	*p = buf+8;

		// setup for the client key
		if(pclient->pclikey == NULL)
			pclient->pclikey = key_new();

		if(pclient->pclikey != NULL && key_read(pclient->pclikey, (unsigned char **)&p))
		{	char	*pkt = NULL;
			BN_CTX	*pbnctx = BN_CTX_new();

			// generate challenge for the client session
			if(pbnctx != NULL)
			{	BIGNUM *pencchallenge = BN_new();

				// clear/free old
				if(pclient->pchallenge != NULL)
					BN_clear_free(pclient->pchallenge);
				// new challenge
				if(pclient->pchallengestr != NULL)
				{
					free(pclient->pchallengestr);
					pclient->pchallengestr = NULL;
				}
				if((pclient->pchallenge = BN_new()) != NULL)
				{
					BN_rand(pclient->pchallenge, 256, 0, 0);
					BN_mod(pclient->pchallenge, pclient->pchallenge, pclient->pclikey->n, pbnctx);
					pclient->pchallengestr = BN_bn2hex(pclient->pchallenge);
				}

				// encrypted
				if(pencchallenge != NULL && pclient->pchallenge != NULL && key_bn_encrypt(pclient->pclikey, pclient->pchallenge, pencchallenge))
					// in ascii form
					pkt = BN_bn2dec(pencchallenge);
				BN_clear_free(pencchallenge);
			}

			if(pkt != NULL)
			{
				NetSockPrintf(sd, "220-rnd %s\r\n220 OK\r\n", pkt);
				memset(pkt, 0, strlen(pkt));
				free(pkt);
			}
			else
				NetSockPrintf(sd, "221 error - Unable to generate challenge\r\n");
		}
		else
			NetSockPrintf(sd, "221 error - Unable to read key\r\n");
	}
	else if(strncasecmp(buf, "auth,", 5) == 0)
	{	int	encrndlen = 0;
		char	*pencrnd = (char *)key_asctobin((unsigned char *)buf+5, &encrndlen);
		char	*pdecrnd = NULL;
		int	decrndlen = 0;

		decrndlen = key_decrypt(gChildRsa, (unsigned char *)pencrnd, encrndlen, (unsigned char **)&pdecrnd);

		if(decrndlen > 0)
		{	char *psig = strchr(pdecrnd, ';');
			char *passwd = strchr(pdecrnd, ':');

			if(psig != NULL)
				*(psig++) = '\0';
			if(passwd != NULL)
				*(passwd++) = '\0';
			if(psig != NULL && strcmp(psig, pclient->pchallengestr) == 0 && passwd != NULL &&
#ifdef SUPPORT_PAM
				pam_authuserpass(pdecrnd, passwd)
#else
				uam_authuserpass(pdecrnd, passwd)
#endif
				)
			{
				NetSockPrintf(sd, "220 OK - Authenticated\r\n");
				pclient->authlevel = AL_FULL;
			}
			else
				NetSockPrintf(sd, "221 error - Invalid signature\r\n");
		}
		else
			NetSockPrintf(sd, "221 error - unable to decrypt auth\r\n");
	}
}

void clientSessionReadLine(int sd, char *buf)
{	unsigned int rc = 500;
	int	afType = AF_UNSPEC;
	char	*pAfAddr = NULL;
	long	interval = 2 * TIME24HOURS;
	int	rate = 14;
	char	bufcmd[10];
	char	bufip[50];
	char	bufinterval[50];
	char	bufrate[50];
	PCLIENT	pclient = &gChildClients[sd];

	if(debugmode > 1)
		printf("clientReadLine: %u/%s:%u '%s'\n", sd, pclient->pIpStr, pclient->ipPort, buf);

	/* syntax;
	 *	cmd, ip address [, interval [, rate]]
	 * where;
	 *	cmd		= [ add | del | inculpate | exculpate ]
	 *	ip address	= an ipv4 dotted quad
	 *	interval	= an integer specifiying number of days to block (nb. 2 days is default if not specified)
	 *	rate		= connections per minute threshold if cmd = inculpate (nb. 14 is default if not specified)
	 */

	clientSessionAuth(sd, buf);

	buf = mlfi_strcpyadv(bufcmd, sizeof(bufcmd), buf, ',');
	buf = mlfi_strcpyadv(bufip, sizeof(bufip), buf, ',');
	buf = mlfi_strcpyadv(bufinterval, sizeof(bufinterval), buf, ',');
	buf = mlfi_strcpyadv(bufrate, sizeof(bufrate), buf, ',');

	if(strlen(bufip))
		mlfi_inet_ptonAF(&afType, &pAfAddr, bufip);

	if(strlen(bufinterval))
		interval = TIME24HOURS * atoi(bufinterval);

	if(strlen(bufrate))
		rate = atoi(bufrate);

	if(gChildClients[sd].authlevel == AL_FULL)
	{
		if(afType != AF_UNSPEC)
		{
			if(strcasecmp(bufcmd, "add") == 0)
				rc = MtaInfoSet(afType, pAfAddr, interval, OPCODE_PENDING_ADD, 0);
			else if(strcasecmp(bufcmd, "del") == 0)
				rc = MtaInfoSet(afType, pAfAddr, interval, OPCODE_PENDING_DEL, 0);
			else if(strcasecmp(bufcmd, "inculpate") == 0 || strcasecmp(bufcmd, "nominate") == 0)
				rc = MtaInfoSet(afType, pAfAddr, interval, OPCODE_PENDING_INCULPATE, rate);
			else if(strcasecmp(bufcmd, "exculpate") == 0)
				rc = MtaInfoSet(afType, pAfAddr, interval, OPCODE_PENDING_EXCULPATE, rate);

			if(rc != 500)
				NetSockPrintf(sd, "%03u %s\r\n", rc, bufcmd);
		}
	}
	if(strcasecmp(bufcmd, "status") == 0)
		rc = MtaInfoDump(sd, afType, pAfAddr, 250);

	if(gChildClients[sd].authlevel != AL_FULL && rc == 500)
		NetSockPrintf(sd, "221 error - unauthorized\r\n");

	if(pAfAddr != NULL)
		free(pAfAddr);
}

void clientSessionReadLines(int i, fd_set *fds)
{	char	buff[1024];
	int	rc;

	// this loop gives opportunity for a DOS that would starve other clients
	while((rc = NetSockGets(i, buff, sizeof(buff), 1)) > 0)
		clientSessionReadLine(i, buff);

	if(rc == 0)	// remove the peer from the socket list
	{	PCLIENT	pclient = &gChildClients[i];

		if(debugmode>1)
			printf("clientReadSessionLines: client disconnect: %u/%s:%u\n", i, pclient->pIpStr, pclient->ipPort);
		FD_CLR(i, fds);
		shutdown(i, SHUT_RDWR);
		close(i);

		if(pclient->pchallenge != NULL)
			BN_clear_free(pclient->pchallenge);
		if(pclient->pchallengestr != NULL)
			free(pclient->pchallengestr);
		if(pclient->pclikey != NULL)
			key_free(pclient->pclikey);
		if(pclient->pIpStr != NULL)
			free(pclient->pIpStr);

		// clean up for next use
		memset(pclient, 0, sizeof(CLIENT));
	}
}

void clientSessionAccept(int sdTcp, fd_set *fds)
{	struct sockaddr_in6	sa;
	socklen_t		salen = sizeof(sa);
	int			sd = accept(sdTcp, (struct sockaddr *)&sa, &salen);

	if(sd != SOCKET_ERROR)
	{	PCLIENT	pclient = &gChildClients[sd];
		pclient->ipPort = 0;
		mlfi_inet_ntopsSA((struct sockaddr *)&sa, &pclient->pIpStr, &pclient->ipPort);
		int allow = 0;
		int match = ifiDb_CheckAllowSA((struct sockaddr *)&sa, gpClientACLCtx->pIfiDb, &allow, 1);
		int reject = !(match && allow);
		
		// socket setup
		NetSockOptNoLinger(sd);
		NetSockOpt(sd, SO_KEEPALIVE, 1);

		if(debugmode>1)
			printf("clientAccept: client connect %u/%s:%u reject %u\n", sd, pclient->pIpStr, pclient->ipPort, reject);

		if(!reject)
		{
			FD_SET(sd, fds);

			// set authentictation level
			pclient->authlevel = AL_NONE;
			switch(((struct sockaddr *)&sa)->sa_family)
			{
				case AF_INET:
					pclient->authlevel = (ntohl(((struct sockaddr_in *)&sa)->sin_addr.s_addr) == INADDR_LOOPBACK) ? AL_FULL : AL_NONE;
					break;
				case AF_INET6:
					pclient->authlevel = (IN6_ARE_ADDR_EQUAL(&((struct sockaddr_in6 *)&sa)->sin6_addr, &in6addr_loopback) ? AL_FULL : AL_NONE);
					break;
			}

			if(pclient->authlevel == AL_NONE)
				NetSockPrintf(sd, "220-agent ipfwmtad/0.5\r\n220-key %s\r\n", gChildRsaPKey);
			else
				NetSockPrintf(sd, "220-agent ipfwmtad/0.5\r\n");
			NetSockPrintf(sd, "220 OK\r\n");
		}
		else
		{
			NetSockPrintf(sd, "221 Connection not allowed.\r\n");
			NetSockClose(&sd);
		}
	}
}

void socketScan(fd_set *fds, int sdTcp)
{	fd_set		sfds;
	struct timeval	tv;
	int		i, rc;

	tv.tv_sec = 1;
	tv.tv_usec = 0;
	memcpy(&sfds, fds, sizeof(sfds));
	rc = select(FD_SETSIZE, &sfds, NULL, NULL, &tv);

	// find a socket that needs attention
	for(i=0; rc != SOCKET_ERROR && i<FD_SETSIZE; i++)
	{
		if(FD_ISSET(i, &sfds))
		{
			if(i == sdTcp)	// add a connecting peer to the socket list
				clientSessionAccept(sdTcp, fds);
			else	// service a connected peer
				clientSessionReadLines(i, fds);
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
			syslog(LOG_ERR, "Signal %u received\n", signo);
			break;
		case SIGTERM:
		case SIGQUIT:
		case SIGINT:
		case SIGSEGV:
			syslog(LOG_ERR, "Shutdown with signal %u\n", signo);
			childShutdown();
			exit(signo);
			break;
	}
}

void childShutdown()
{	int	i;

	// close all open ports
	for(i=0; i<FD_SETSIZE; i++)
	{
		if(FD_ISSET(i, &gChildFds))
		{
			shutdown(i, SHUT_RDWR);
			close(i);
		}
	}

	closelog();
}

int childMain(int afType, char const *afAddr, unsigned short port)
{	int	quit	= 0;
	int	rc = -1;
	int	mtaUpdateInterval = 1;
	long	mtaUpdate = time(NULL) + mtaUpdateInterval;

	signal(SIGTERM, child_signal_hndlr);
	signal(SIGQUIT, child_signal_hndlr);
	signal(SIGINT, child_signal_hndlr);
	signal(SIGHUP, child_signal_hndlr);
	signal(SIGSEGV, child_signal_hndlr);
	signal(SIGPIPE, child_signal_hndlr);

	syslog(LOG_ERR, "Child: Started using '%u'", port );

	gChildRsa = key_generate(2048);
	gChildRsaPKey = (char *)key_pubkeytoasc(gChildRsa);

	memset(&gChildClients, 0, sizeof(gChildClients));

	gSdTcp= NetSockOpenTcpListenAf(afType, afAddr, port);

	if(gSdTcp != INVALID_SOCKET)
	{
		FD_ZERO(&gChildFds);
		FD_SET(gSdTcp, &gChildFds);

		MtaInfoReadDb(gpMtaDbFname);
#ifdef OS_Linux
		mlfi_systemPrintf("iptables -F SPAMILTER; iptables -X SPAMILTER; iptables -D INPUT 1 -p tcp --dport 25 -j SPAMILTER\n");
		mlfi_systemPrintf("iptables -N SPAMILTER; iptables -A SPAMILTER -j ACCEPT; iptables -I INPUT 1 -p tcp --dport 25 -j SPAMILTER\n");
#endif
		MtaInfoIpfwSync(1);

		gpClientACLCtx = ifiDb_Create("");
		if(ifiDb_Open(gpClientACLCtx, gpClientACLFname))
		{
			ifiDb_BuildList(gpClientACLCtx);
			ifiDb_Close(gpClientACLCtx);
		}

		while(!quit)
		{
			socketScan(&gChildFds, gSdTcp);
			if(time(NULL) > mtaUpdate)
			{
				MtaInfoStateMachineUpdate(gpMtaDbFname);
				mtaUpdate = time(NULL) + mtaUpdateInterval;
			}
		}

		ifiDb_Destroy(&gpClientACLCtx);

		rc = 0;
		syslog(LOG_ERR, "Child: Exiting: normal");
	}
	else
		syslog(LOG_ERR, "Child: Exiting: Invalid Tcp socket");

	childShutdown();

	return rc;
}

void childStart(int afType, char const *afAddr, unsigned short port, int count, int forked)
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
		openlog("ipfwmtad", flags|LOG_NDELAY|LOG_PID, LOG_DAEMON);
		if(childMain(afType, afAddr, port) == -1)
		{
			syslog(LOG_ERR, "Error starting child, sleeping 30 seconds, %u retries left.", count);
			sleep(30000);
		}
	}
}

void usage()
{
	printf("usage: [-d] "
			"[-A client ACL config file] "
			"[-I server ip address] "
			"[-p port number] "
			"[-n fname] "
			"[-u rule number] "
			"[-U auth user name] "
			"[-P auth user password] "
			"| [-i fname] "
			"| [-a ipaddress] "
			"| [-r ipaddress] "
			"| [-q ipaddress]\n"
		"\t-d - debug mode\n"
		"\t-A - server mode Client ACL config file - default /usr/local/etc/spamilter/ipfwmtad.acl\n"
		"\t-I - server mode ip address to bind to, imeadiate mode server ip address - default 127.0.0.1\n"
		"\t-p - server tcp port number - default 4739\n"
		"\t-n - server mode - ip database file name - default '%s'\n"
		"\t-u - server mode - ipfw rule number - default 90\n"
//		"\t-b - server mode - ipfw action (add/deny)\n"
		"\t-i - imeadiate mode - ipfw resync using the ip database file name\n"
		"\t-r - imeadiate mode - queue ipaddress for removal\n"
		"\t-a - imeadiate mode - queue ipaddress for addition\n"
		"\t-q - imeadiate mode - query ipaddress\n"
		"\t-U - imeadiate mode - auth user name\n"
		"\t-P - imeadiate mode - auth user password\n"
		"\t-? - man page or options summary\n"
		, gpMtaDbFname
		);
}

int main(int argc, char **argv)
{	int	count = 3;
	unsigned long ip = htonl(INADDR_LOOPBACK);
	int afType = AF_INET;
	char const *afAddr = (char *)&ip;
	unsigned short port = 4739;
	int	forking = 1;
	int	servermode = 1;
	char	opt;
	char	*optflags = "d:p:n:u:i:r:a:q:o:b:I:U:P:A:";
	char	*username = "";
	char	*userpass = "";

	if(getuid() != 0)
	{
		if(argc > 1 && strcmp(argv[1], "-?") == 0)
			mlfi_systemPrintf("%s", "man ipfwmtad");
		else
		{
			printf("Not ROOT, exiting.\n");
			usage();
		}
		exit(0);
	}

	while((opt = getopt(argc, argv, optflags)) != -1)
	{
		switch(opt)
		{
			case 'd':
				if(optarg != NULL && *optarg)
					forking = 1 - ((debugmode = atoi(optarg))>0);
				if(debugmode)
					printf("debug mode\n");
				break;

			// server mode stuff
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
					gAction = (strcasecmp("add", optarg) == 0);
				break;
			case 'A':
				if(optarg != NULL && *optarg)
					gpClientACLFname = optarg;
				break;

			// imeadiate mode stuff
			case 'i':
				if(optarg != NULL && *optarg)
					gpMtaDbFname = optarg;
				MtaInfoReadDb(gpMtaDbFname);
				MtaInfoIpfwSync(0);
				return 0;
				break;
			case 'a':
				if(optarg != NULL && *optarg)
				{
					servermode = 0;
					cliIpfwActionAF(afType, afAddr, port, username, userpass, optarg, "add", debugmode);
				}
				break;
			case 'r':
				if(optarg != NULL && *optarg)
				{
					servermode = 0;
					cliIpfwActionAF(afType, afAddr, port, username, userpass, optarg, "del", debugmode);
				}
				break;
			case 'q':
				if(optarg != NULL && *optarg)
				{
					servermode = 0;
					cliIpfwActionAF(afType, afAddr, port, username, userpass, optarg, "status", debugmode);
				}
				break;
			case 'U':
				if(optarg != NULL && *optarg)
					username = optarg;
				break;
			case 'P':
				if(optarg != NULL && *optarg)
					userpass = optarg;
				break;
			case 'I':
				if(optarg != NULL && *optarg)
				{	struct hostent *pHostent = gethostbyname(optarg);

					if(pHostent != NULL)
					{
						afType = pHostent->h_addrtype;
						afAddr = (char *)pHostent->h_addr_list[0];
					}
					else
					{
						printf("Error - unable to resolve -I paramater '%s'\n", optarg);
						return 1;
					}
				}
				break;
			case '?':
				// show man page, if they arent' trying to figure out other cli params
				if(argc > 2 || mlfi_systemPrintf("%s", "man ipfwmtad"))
					usage();
				return 1;
				break;
			default:
				printf("Invalid argument '%c'\n\n", opt);
				usage();
				return 1;
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
				childStart(afType, afAddr, port, count, 1);
		}
		else
			childStart(afType, afAddr, port, count, 0);
	}

	return 0;
}
