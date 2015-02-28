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
 *	CVSID:  $Id: misc.c,v 1.24 2012/12/07 19:38:33 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		misc.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: misc.c,v 1.24 2012/12/07 19:38:33 neal Exp $";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <syslog.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern int gDebug;

#include "misc.h"

void mlfi_vdebug(const char *pSessionId, const char *pfmt, va_list vl)
{
	if(pfmt != NULL)
	{	char *pfmt2 = NULL;

		if(asprintf(&pfmt2,"[%s] %s",pSessionId,pfmt) != -1 && pfmt2 != NULL)
		{
#ifdef DEBUG_FMT_STR_LOGGING
			FILE *fout = fopen("/tmp/spamilter.dbg.txt","a");

			if(fout != NULL)
			{
				fprintf(fout,"--pfmt\t'%s'\n--pfmt2\t'%s'\n",pfmt,pfmt2);
				fflush(fout);
				fclose(fout);
			}
#endif

			vsyslog(LOG_DEBUG,pfmt2,vl);
		}
		if(pfmt2 != NULL)
			free(pfmt2);
	}
}

void mlfi_debug(const char *pSessionId, const char *pfmt, ...)
{
	if(pfmt != NULL)
	{	va_list	vl;

		va_start(vl,pfmt);
		mlfi_vdebug(pSessionId,pfmt,vl);
		va_end(vl);
	}
}

// our own fgets... cept for handles
int mlfi_fdgets(int i, char *buf, int buflenmax)
{	char	*str = buf;
	int	rc = 0;

	// reset buffer for next iteration
	memset(buf,0,buflenmax);

	// read chars until EOL or EOF
	while((rc = read(i,str,1)) == 1 && *str != '\n' && str < buf+buflenmax-1)
		str++;

	// if we have a complete line, clean it up, and handle it
	if(rc == 1)
	{
		if(strlen(buf))
		{
			// trim right
			while((*str == ' ' || *str == '\t' || *str =='\r' || *str == '\n') && str > buf)
				*(str--) = '\0';
			// trim left
			str = buf;
			while(*str==' ' || *str =='\t' || *str=='\r' || *str=='\n')
				str++;
			// left justify the buffer
			if(*str && str > buf)
			{	char *dst = buf;

				while(*str)
					*(dst++) = *(str++);
				*dst = '\0';
			}
			rc = strlen(buf);
		}
		else
			rc = 0;
	}
	else
		rc = -1;

	return(rc);
}

char *mlfi_strcpyadv(char *dst, int dstmax, char *src, char delim)
{
	if(dst != NULL)
		memset(dst,0,dstmax);
	while(src != NULL && *src && *src != delim && dstmax--)
	{
		if(*src != ' ' && *src != '\t' && *src != '\r' && *src != '\n')
			*(dst++) = *(src++);
		else
			src++;
	}

	if(*src == delim)
		src++;

	return(src);
}

char *mlfi_stradvtok(char **ppSrc, char delim)
{	char *dst = *ppSrc;
	char *src = *ppSrc;

	while(src != NULL && *src && *src != delim)
	{
		if(dst == src && *src != delim && (*src == ' ' || *src == '\t' || *src == '\r' || *src == '\n'))
			dst++;
		src++;
	}

	if(*src == delim)
	{
		*src = '\0';
		src++;
		while(*src == ' ' || *src == '\t' || *src == '\r' || *src == '\n')
			src++;
	}

	*ppSrc = src;

	return dst;
}

int mlfi_isNonRoutableIpV4(unsigned long ip)
{
	return mlfi_isNonRoutableIpAF(AF_INET, (const char *)&ip);
}

int mlfi_isNonRoutableIpAF(int af, const char *in)
{	int nonRoutable = 1;

	switch(af)
	{
		case AF_INET:
			{	int rc = 0;
				unsigned long ip = ntohl(*(unsigned long*)in);

				if(ip != 0)
				{
					/* always allow localhost */
					rc |= ((ip&0xff000000l) == 0x7f000000l);	/* ip = 127.0.0.0/8 */

					/* rfc1918 networks */
					rc |= ((ip&0xff000000l) == 0x0a000000l);	/* ip = 10.0.0.0/8 */
					rc |= ((ip&0xfff00000l) == 0xac100000l);	/* ip = 172.16.0.0/12 */
					rc |= ((ip&0xffff0000l) == 0xc0a80000l);	/* ip = 192.168.0.0/16 */

					// Link-Local Address - rfc3927
					rc |= ((ip&0xffff0000l) == 0xa9fe0000l);	/* ip = 169.254.0.0/16 */

					nonRoutable = rc;
				}
			}
			break;
		// TODO - ipv6 - is this enough ?
		case AF_INET6:
			{	struct in6_addr *pAddr = (struct in6_addr *)in;

				nonRoutable = (
					IN6_ARE_ADDR_EQUAL(pAddr, &in6addr_loopback)
					|| IN6_IS_ADDR_LINKLOCAL(pAddr) // fe80:xx
					|| IN6_IS_ADDR_SITELOCAL(pAddr) // fec0::xx
					);
			}
			break;
	}

	return nonRoutable;
}

int mlfi_isNonRoutableIpHostEnt(const struct hostent *pHostEnt)
{
	return(pHostEnt != NULL ? mlfi_isNonRoutableIpAF(pHostEnt->h_addrtype, pHostEnt->h_addr ) : 1);
}

int mlfi_isNonRoutableIpSA(const struct sockaddr *psa)
{	int nonRoutable = 1;

	switch(psa->sa_family)
	{
		case AF_INET:
			nonRoutable = mlfi_isNonRoutableIpAF(psa->sa_family, (const char *)&((struct sockaddr_in *)psa)->sin_addr.s_addr);
			break;
		case AF_INET6:
			nonRoutable = mlfi_isNonRoutableIpAF(psa->sa_family, (const char *)&((struct sockaddr_in6 *)psa)->sin6_addr);
			break;
	}

	return nonRoutable;
}

char *mlfi_inet_ntopAF(int afType, const char *in)
{	char *pstr = NULL;

	if(in != NULL)
	{	size_t s = 0;

		switch(afType)
		{
			case AF_INET: s = INET_ADDRSTRLEN; break;
			case AF_INET6: s = INET6_ADDRSTRLEN; break;
		}

		if(s > 0 && (pstr = calloc(1,s)) != NULL)
			pstr = (char *)inet_ntop(afType, in, pstr, s);
	}

	return pstr;
}

char *mlfi_inet_ntopSA(const struct sockaddr *psa)
{	char *pstr = NULL;

	if(psa != NULL)
	{	const char *in = NULL;

		switch(psa->sa_family)
		{
			case AF_INET: in = (char *) &((struct sockaddr_in *)psa)->sin_addr; break;
			case AF_INET6: in = (const char *)&((struct sockaddr_in6 *)psa)->sin6_addr; break;
		}

		pstr = mlfi_inet_ntopAF(psa->sa_family, in);
	}

	return pstr;
}

int mlfi_systemPrintf(char *fmt, ...)
{	int	rc = -1;

	if(fmt != NULL)
	{	char	*str;
		va_list	vl;

		va_start(vl,fmt);
		rc = vasprintf(&str,fmt,vl);

		if(str != NULL)
		{
			if(rc != -1)
			{
				rc = system(str);
				rc = WIFEXITED(rc) ? WEXITSTATUS(rc) : -1;
			}
			free(str);
		}
		va_end(vl);
	}

	return rc;
}

