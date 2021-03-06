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
 *	CVSID:  $Id: misc.h,v 1.16 2012/11/23 03:54:13 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		spamilter.c
 *--------------------------------------------------------------------*/

#ifndef _SPAMILTER_MISC_H_
#define _SPAMILTER_MISC_H_

	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <stdarg.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

	void mlfi_vdebug(const char *pSessionId, const char *pfmt, va_list vl);
	void mlfi_debug(const char *pSessionId, const char *fmt, ...);

	int mlfi_fdgets(int i, char *buf, int buflenmax);

	char *mlfi_strcpyadv(char *dst, int dstmax, char *src, char delim);
	char *mlfi_stradvtok(char **ppSrc, char delim);

	int mlfi_isNonRoutableIpV4(unsigned long ip);
	int mlfi_isNonRoutableIpAF(int af, const char *in);
	int mlfi_isNonRoutableIpHostEnt(const struct hostent *pHostEnt);
	int mlfi_isNonRoutableIpSA(const struct sockaddr *psa);

	unsigned long mlfi_regex_ipv4(const char *pstr);

	char *mlfi_inet_ntopAF(int afType, const char *in);
	char *mlfi_inet_ntopSA(const struct sockaddr *psa);

	int mlfi_inet_ptonAF(int *pAfType, char **ppAfAddr, const char *pIpStr);

	int mlfi_systemPrintf(char *fmt, ...);

	unsigned short mlfi_inet_ntosSA(struct sockaddr *pSa);
	void mlfi_inet_ntopsSA(struct sockaddr *pSa, char **ppIpStr, unsigned short *pPort);
#endif
