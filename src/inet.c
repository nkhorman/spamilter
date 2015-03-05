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
 *	CVSID:  $Id: inet.c,v 1.13 2011/07/29 21:23:17 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		inet.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: inet.c,v 1.13 2011/07/29 21:23:17 neal Exp $";

#include "config.h"

#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>

#ifdef OS_SunOS
#include <sys/filio.h>
#endif

#include "inet.h"

#ifdef __FreeBSD__
#define HAVE_SOCKADDR_SA_LEN 1
#endif

struct sockaddr *NetSockInitAf(struct sockaddr *pSa, int afType, const char *in, unsigned short port)
{
	switch(afType)
	{
		case AF_INET:
			{	struct sockaddr_in *pSin = (struct sockaddr_in *)(pSa != NULL ? pSa : calloc(1, sizeof(struct sockaddr_in)));

				if(pSin != NULL)
				{
					pSa = (struct sockaddr *)pSin;
#ifdef HAVE_SOCKADDR_SA_LEN
					pSin->sin_len = sizeof(struct sockaddr_in);
#endif
					pSin->sin_family = afType;
					pSin->sin_port = htons(port);

					if(in != NULL)
						pSin->sin_addr = *(struct in_addr *)in;
					else
						pSin->sin_addr.s_addr = INADDR_ANY;
				}
			}
			break;

		case AF_INET6:
			{	struct sockaddr_in6 *pSin6 = (struct sockaddr_in6 *)(pSa != NULL ? pSa : calloc(1, sizeof(struct sockaddr_in6)));

				if(pSin6 != NULL)
				{
					pSa = (struct sockaddr *)pSin6;
#ifdef HAVE_SOCKADDR_SA_LEN
					pSin6->sin6_len = sizeof(struct sockaddr_in6);
#endif
					pSin6->sin6_family = afType;
					pSin6->sin6_port = htons(port);
					if(in != NULL)
						pSin6->sin6_addr = *(struct in6_addr *)in; // TODO - ipv6 - INADDR6_ANY
				}
			}
			break;
	}

	return pSa;
}

void NetSockClose(int *pSd)
{
	if(*pSd != INVALID_SOCKET )
	{	int	rc;

		shutdown(*pSd,SHUT_RDWR);
		while((rc = close(*pSd)) != 0 && (errno == EAGAIN || errno == EINPROGRESS || errno == EALREADY))
			sleep(10);
		*pSd = INVALID_SOCKET;
	}
}

int NetSockBindAf(int *pSd, int sockType, int afType, const char *in, unsigned short port)
{	int ok = ((*pSd = socket(afType, sockType, 0)) != INVALID_SOCKET);

	if(ok)
	{	struct sockaddr *pSa = NetSockInitAf(NULL, afType, in, port);

		if(pSa != NULL)
		{
#ifdef HAVE_SOCKADDR_SA_LEN
#define SALEN pSa->sa_len
#else
#define SALEN (afType == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6))
#endif
			ok = (bind(*pSd, pSa, SALEN) != SOCKET_ERROR);
			free(pSa);
		}
	}

	return ok;
}

int NetSockOptNoLinger(int sd)
{	struct linger	optlinger;

	optlinger.l_onoff = 0;
	optlinger.l_linger = 0;

	return(setsockopt(sd,SOL_SOCKET,SO_LINGER,(char *)&optlinger,sizeof(optlinger)) == 0);
}

int NetSockOpt(int sd, int optname, int optval)
{
	return(setsockopt(sd,SOL_SOCKET,optname,(char *)&optval,sizeof(int)) == 0);
}

int NetSockIOCtl(int sd, long cmd, unsigned long *pArg)
{
	return(ioctl(sd,cmd,pArg) != SOCKET_ERROR);
}

int NetSockOpenUdpAf(int afType, const char *in, unsigned short port)
{	int			sd = INVALID_SOCKET;
	unsigned long		arg_on = 1;

	if(!(
		NetSockBindAf(&sd, SOCK_DGRAM, afType, in, port)
		&& NetSockIOCtl(sd, FIONBIO, &arg_on)
		&& NetSockOpt(sd, SO_BROADCAST, 1)
	))
	{
		NetSockClose(&sd);
	}

	return sd;
}

int NetSockOpenUdp(unsigned long ip, unsigned short port)
{
	ip = htonl(ip);
	return NetSockOpenUdpAf(AF_INET, (const char *)&ip, port);
}

int NetSockOpenTcpPeerAf(int afType, const char *in, unsigned short port)
{	int			sd	= INVALID_SOCKET;
	unsigned long		arg_on	= 1;
	int			ok	= (NetSockBindAf(&sd, SOCK_STREAM, afType, NULL, 0) && (ioctl(sd, FIONBIO, &arg_on) == 0));

	if(ok)
	{	struct sockaddr *pSa = NetSockInitAf(NULL, afType, in, port);

		if(pSa != NULL)
		{
			ok = NetSockOptNoLinger(sd);

#ifdef HAVE_SOCKADDR_SA_LEN
#define SALEN pSa->sa_len
#else
#define SALEN (afType == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6))
#endif
			if(ok)
			{	int err = (connect(sd, pSa, SALEN) == SOCKET_ERROR ? errno : 0);

				ok = (err == 0 || err == EINPROGRESS || err == EWOULDBLOCK);
			}

			free(pSa);
		}
	}

	if(!ok)
		NetSockClose(&sd);

	return sd;
}

int NetSockOpenTcpPeer(unsigned long ip, unsigned short port)
{
	ip = htonl(ip);
	return NetSockOpenTcpPeerAf(AF_INET, (const char *)&ip, port);
}

int NetSockOpenTcpListenAf(int afType, const char *in, unsigned short port)
{	int			sd,ok;
	unsigned long		arg_on	= 1;
	struct sockaddr *	pSa = NetSockInitAf(NULL, afType, in, port);

#ifdef HAVE_SOCKADDR_SA_LEN
#define SALEN pSa->sa_len
#else
#define SALEN (afType == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6))
#endif
	ok = (pSa != NULL) &&
		((sd = socket(AF_INET, SOCK_STREAM, 0)) != INVALID_SOCKET) &&
		NetSockOptNoLinger(sd) &&
		NetSockOpt(sd, SO_REUSEADDR, 1) &&
		(ioctl(sd, FIONBIO, &arg_on) == 0) &&
		(bind(sd, pSa, SALEN) != SOCKET_ERROR) &&
		(listen(sd, 1) != SOCKET_ERROR);

	if(!ok)
		NetSockClose(&sd);

	return sd;
}

int NetSockOpenTcpListen(unsigned long ip, unsigned short port)
{
	ip = htonl(ip);
	return NetSockOpenTcpListenAf(AF_INET, (const char *)&ip, port);
}


/* buffer up lines of data from peer until 1 second time-out */
int NetSockGets(int sd, char *buf, int buflenmax, int timeout)
{	char		*str = buf;
	fd_set		sfds;
	struct timeval	tv;
	int		rc = 0;
	int		done = 0;

	/* reset buffer for next iteration */
	memset(buf,0,buflenmax);
	while(!done)
	{

		/* our own fgets... cept for handles */
		while((rc = read(sd,str,1)) == 1 && *str != '\n' && str < buf+buflenmax-1)
			str++;

		/* if we have a complete line, clean it up, and handle it */
		if(rc == 1 && strlen(buf) && *str=='\n')
		{
			/* trim right */
			while((*str =='\r' || *str == '\n') && str > buf)
				*(str--) = '\0';
			/* trim left */
			str = buf;
			while(*str==' ' || *str =='\t' || *str=='\r' || *str=='\n')
				str++;
			/* handle complete line by breaking loop */
			if(*str)
			{	char *dst = buf;

				/* left justify the buffer */
				if(str>dst)
				{
					while(*str)
						*(dst++) = *(str++);
					*dst = '\0';
				}
				done = 1;
				rc = strlen(buf);
			}
			else
			{
				done = 1;
				rc = 0;
			}
		}
		else if(rc == 0)
			done = 1;
		else	/* try to wait for more */
		{
			tv.tv_sec = timeout;
			tv.tv_usec = 0;
			FD_ZERO(&sfds);
			FD_SET(sd,&sfds);
			rc = select(FD_SETSIZE,&sfds,NULL,NULL,&tv);
			done = (rc == 0 || rc == -1);
			if(done)
				rc = -1;
		}
	}

	return(rc);
}

int NetSockSendToAf(int sd, char *buf, int buflen, int afType, const char * in, unsigned short port)
{	int rc = 0;

	if(sd != INVALID_SOCKET && buf != NULL && buflen > 0 && port != 0)
	{	struct sockaddr *pSa = NetSockInitAf(NULL, afType, in, port);

		if(pSa != NULL)
		{
#ifdef HAVE_SOCKADDR_SA_LEN
#define SALEN pSa->sa_len
#else
#define SALEN (afType == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6))
#endif
			rc = sendto(sd, buf, buflen, 0, pSa, SALEN);
			free(pSa);
		}
	}

	return rc;
}

int NetSockSendTo(int sd, char *buf, int buflen, unsigned long ip, unsigned short port)
{
	ip = htonl(ip);
	return NetSockSendToAf(sd, buf, buflen, AF_INET, (const char *)&ip, port);
}

int NetSockVPrintfToAf(int sd, int afType, const char *in, unsigned short port, const char *fmt, va_list vl)
{	int rc = 0;

	if(fmt != NULL)
	{	char *str = NULL;

		rc = vasprintf(&str,fmt,vl);
		if(str != NULL && rc != -1)
			rc = NetSockSendToAf(sd, str, rc, afType, in, port);
		if(str != NULL)
			free(str);
	}

	return rc;
}

int NetSockPrintfToAf(int sd, int afType, const char *in, unsigned short port, const char *fmt, ...)
{	int rc = -1;

	if(fmt != NULL)
	{	va_list vl;

		va_start(vl,fmt);
		rc = NetSockVPrintfToAf(sd, afType, in, port, fmt, vl);
		va_end(vl);
	}

	return rc;
}

int NetSockPrintfTo(int sd, unsigned long ip, unsigned short port, const char *fmt, ...)
{	int rc;
	va_list vl;

	ip = htonl(ip);
	va_start(vl, fmt);
	rc = NetSockVPrintfToAf(sd, AF_INET, (const char *)&ip, port, fmt, vl);
	va_end(vl);

	return rc;
}

int NetSockSend(int sd, char *buf, int buflen)
{	int rc = 0;

	if(sd != INVALID_SOCKET && buf != NULL && buflen)
	{
		while((rc = send(sd,buf,buflen,0)) == SOCKET_ERROR && (errno == EAGAIN || errno == EINPROGRESS || errno == EALREADY))
			usleep(10000);
	}

	return(rc);
}

int NetSockVPrintf(int sd, const char *fmt, va_list vl)
{	char	*str;
	int	rc = -1;

	if(sd != INVALID_SOCKET)
	{
		rc = vasprintf(&str,fmt,vl);
		if(str != NULL && rc != -1)
			rc = NetSockSend(sd,str,rc);
		if(str != NULL)
			free(str);
	}

	return(rc);
}

int NetSockPrintf(int sd, const char *fmt, ...)
{	va_list	vl;
	int	rc;

	va_start(vl,fmt);
	rc = NetSockVPrintf(sd,fmt,vl);
	va_end(vl);

	return(rc);
}

int NetSockSelectOne(int sd, int timeOutMs)
{	int rc = 0;

	if(sd != INVALID_SOCKET)
	{	fd_set fds;
		struct timeval tv;
		int selrc;

		tv.tv_sec  = timeOutMs / 1000;
		tv.tv_usec = (timeOutMs % 1000) * 1000;
		FD_ZERO(&fds);
		FD_SET(sd, &fds);

		rc = ((selrc = select(FD_SETSIZE,&fds,NULL,NULL,&tv)) != 0 && selrc != SOCKET_ERROR && FD_ISSET(sd,&fds));
	}
	else
		usleep(timeOutMs * 1000);

	return(rc);
}

int NetSockRecv(int sd, char *buf, int buflen, int *penabled)
{	int rc = -1;
	int rxlen = 0;

	if(sd != INVALID_SOCKET && buf != NULL && buflen)
	{
		rc = 0;
		while(rxlen < buflen && rc != SOCKET_ERROR && (penabled == NULL || (penabled != NULL && *penabled)))
		{
			while((rc = recv(sd,buf+rxlen,buflen-rxlen,0)) == SOCKET_ERROR && 
				(errno == EAGAIN || errno == EINPROGRESS || errno == EALREADY))
				usleep(10000);

			if(rc != SOCKET_ERROR)
			{
				if(rc == 0)
					buflen = rxlen;
				else
					rxlen += rc;
			}
		}
	}

	return(rc != SOCKET_ERROR ? rxlen : rc);
}

int NetSockRecvFrom(int sd, char *buf, int buflen, unsigned long *pip, unsigned short *pport, int *penabled)
{	int rc = -1;
	int rxlen = 0;

	if(sd != INVALID_SOCKET && buf != NULL && buflen)
	{	struct sockaddr_in sin;
		unsigned int sinlen=sizeof(sin);
		memset(&sin,0,sinlen);
		memset(buf,0,buflen);

		while(
			(penabled == NULL || (penabled != NULL && *penabled))
			&& (rc=recvfrom(sd,buf+rxlen,buflen,0,(struct sockaddr*)&sin,&sinlen)) == SOCKET_ERROR
			&& (errno == EAGAIN || errno == EINPROGRESS || errno == EALREADY)
		)
		{
			usleep(10000);
		}

		if(rc != SOCKET_ERROR)
		{
			if(pip != NULL)
				*pip = ntohl(sin.sin_addr.s_addr);
			if(pport != NULL)
				*pport = ntohs(sin.sin_port);
			rxlen += rc;
		}
	}

	return(rc != SOCKET_ERROR ? rxlen : rc);
}
