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

void NetSockInitAddr(struct sockaddr_in *pSock, long ip, short port)
{
	// setup the socket address structure
	memset((void *)pSock,0,sizeof(struct sockaddr_in));
	pSock->sin_family	= AF_INET;
	pSock->sin_port		= port != 0 ? htons(port) : 0;
	pSock->sin_addr.s_addr	= ip == INADDR_ANY ? INADDR_ANY : htonl(ip);
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

int NetSockBind(int *pSd, int sockType, long ip, short port)
{	int			ok = ((*pSd = socket(AF_INET,sockType,0)) != INVALID_SOCKET);
	struct sockaddr_in	socks;

	if(ok)
	{
		NetSockInitAddr(&socks,ip,port);

		// bind the source port
		ok = (bind(*pSd,(struct sockaddr *)&socks,sizeof(socks)) != SOCKET_ERROR);
	}

	return(ok);
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
int NetSockOpenUdp(unsigned long ip, unsigned short port)
{	int			sd = INVALID_SOCKET;
	unsigned long		arg_on = 1;

	if(!(
		NetSockBind(&sd, SOCK_DGRAM, (ip == 0 ? INADDR_ANY : ip), port)
		&& NetSockIOCtl(sd,FIONBIO,&arg_on)
		&& NetSockOpt(sd,SO_BROADCAST,1)
	))
	{
		NetSockClose(&sd);
	}

	return(sd);
}

int NetSockOpenTcpPeer(long ip, short port)
{	int			sd	= INVALID_SOCKET;
	unsigned long		arg_on	= 1;
	int			ok	= NetSockBind(&sd,SOCK_STREAM,INADDR_ANY,0) && (ioctl(sd,FIONBIO,&arg_on) == 0);
	int			err;
	struct sockaddr_in	socks;

	if(ok)
	{
		NetSockInitAddr(&socks,ip,port);
		ok = NetSockOptNoLinger(sd);

		if(ok)
		{
			if((err = connect(sd,(struct sockaddr *)&socks,sizeof(socks))) == SOCKET_ERROR)
				err = errno;
			ok = (err == 0 || err == EINPROGRESS || err == EWOULDBLOCK);
		}
	}

	if(!ok)
		NetSockClose(&sd);

	return(sd);
}

int NetSockOpenTcpListen(long ip, short port)
{	int			sd,ok;
	unsigned long		arg_on	= 1;
	struct sockaddr_in	socks;

	NetSockInitAddr(&socks,ip,port);
	ok = ((sd = socket(AF_INET,SOCK_STREAM,0)) != INVALID_SOCKET) &&
		NetSockOptNoLinger(sd) &&
		NetSockOpt(sd,SO_REUSEADDR,1) &&
		(ioctl(sd,FIONBIO,&arg_on) == 0) &&
		(bind(sd,(struct sockaddr *)&socks,sizeof(socks)) != SOCKET_ERROR) &&
		(listen(sd,1) != SOCKET_ERROR);

	if(!ok)
		NetSockClose(&sd);

	return(sd);
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

int NetSockSendTo(int sd, char *buf, int buflen, unsigned long ip, unsigned short port)
{	int rc = 0;

	if(sd != INVALID_SOCKET && buflen > 0 && ip != 0 && port != 0)
	{	struct sockaddr_in sock;

		memset(&sock,0,sizeof(sock));
		sock.sin_family		= AF_INET;
		sock.sin_port		= htons(port);
		sock.sin_addr.s_addr	= htonl(ip);

		rc = sendto(sd,buf,buflen,0,(struct sockaddr *)&sock,sizeof(sock));
	}

	return(rc);
}

int NetSockPrintfTo(int sd, unsigned long ip, unsigned short port, char *fmt, ...)
{	int rc = -1;

	if(sd != INVALID_SOCKET && ip != 0 && port != 0 && fmt != NULL)
	{	va_list vl;
		char *str = NULL;

		va_start(vl,fmt);
		rc = vasprintf(&str,fmt,vl);
		if(str != NULL && rc != -1)
			rc = NetSockSendTo(sd,str,rc,ip,port);
		if(str != NULL)
			free(str);
	}

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

int NetSockVPrintf(int sd, char *fmt, va_list vl)
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

int NetSockPrintf(int sd, char *fmt, ...)
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
