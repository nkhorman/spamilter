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
 *	CVSID:  $Id: inet.h,v 1.10 2012/05/04 00:14:06 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		inet.h
 *--------------------------------------------------------------------*/

#ifndef _SPAMILTER_INET_H_
#define _SPAMILTER_INET_H_

	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <sys/ioctl.h>
	#include <errno.h>

	#ifndef satosin
	#define satosin(sa)     ((struct sockaddr_in *)(sa))
	#endif
	#ifndef sintosa
	#define sintosa(sin)    ((struct sockaddr *)(sin))
	#endif

	#define INVALID_SOCKET -1
	#define SOCKET_ERROR -1

	void NetSockInitAddr(struct sockaddr_in *pSock, long ip, short port);

	int NetSockBind(int *pSd, int sockType, long ip, short port);

	int NetSockOpt(int sd, int optname, int optval);
	int NetSockOptNoLinger(int sd);

	int NetSockOpenUdp(unsigned long ip, unsigned short port);
	int NetSockOpenTcpPeer(long ip, short port);
	int NetSockOpenTcpListen(long ip, short port);

	int NetSockSelectOne(int sd, int timeOutMs);

	int NetSockRecv(int sd, char *buf, int buflen, int *penabled);
	int NetSockRecvFrom(int sd, char *buf, int buflen, unsigned long *pip, unsigned short *pport, int *penabled);

	int NetSockSend(int sd, char *buf, int buflen);
	int NetSockSendTo(int sd, char *buf, int buflen, unsigned long ip, unsigned short port);

	void NetSockClose(int *pSd);

	int NetSockGets(int sd, char *buf, int buflenmax, int timeout);
	int NetSockPrintf(int sd, char *fmt, ...);
	int NetSockVPrintf(int sd, char *fmt, va_list vl);

	int NetSockPrintfTo(int sd, unsigned long ip, unsigned short port, char *fmt, ...);
#endif
