/*--------------------------------------------------------------------*
 *
 * Developed by;
 *	Neal Horman - http://www.wanlink.com
 *	Copyright (c) 2015 Neal Horman. All Rights Reserved
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
 *	CVSID:  $Id$
 *
 * DESCRIPTION:
 *	application:	ipfwmtad
 *	module:		ipfw_mtacli.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id$";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>

#include "ipfw_mtacli.h"

#include "misc.h"
#include "inet.h"
#include "key.h"

int cliIpfwActionSd(int sd, char const *user, char const *pass, char *ipstr, char *action, int debugmode)
{	int rc = 0;

	if(sd != INVALID_SOCKET)
	{	char	buf[8192];
		RSA	*psrvkey = key_new();
		RSA	*pclikey = key_generate(2048);
		char	*pclikeystr = (char *)key_pubkeytoasc(pclikey);
		char	*psrvrnd = NULL;
		int	done = 0;
		int	needrnd = 0;

		while(!done && NetSockGets(sd, buf, sizeof(buf)-1, 5) > 0)
		{	char *p = buf;

			if(debugmode>1)
				printf("<--%s\n", buf);
			// read in the server public key, and send ours in response
			if(strncasecmp(buf, "220-key rsa ", 12) == 0 && *(p+=12) && key_read(psrvkey, (unsigned char **)&p) && pclikeystr != NULL)
			{
				NetSockPrintf(sd, "key,%s\r\n", pclikeystr);
				if(debugmode>1)
					printf("-->key,%s\n", pclikeystr);
				needrnd = 1;
			}

			// read in the server entropy and decrypt it
			if(strncasecmp(buf, "220-rnd ", 8) == 0)
			{	BIGNUM	*bni	= NULL;
				BIGNUM	*bno	= BN_new();

				BN_dec2bn(&bni, buf+8);
				key_bn_decrypt(pclikey, bni, bno);
				psrvrnd = BN_bn2hex(bno);
				needrnd = 0;
			}
			done = (*(buf+3) == ' ') && !needrnd;
		}

		// if we have entropy from the server, use it to login
		if(psrvrnd != NULL && user != NULL && pass != NULL)
		{	char	*pkt;

			asprintf(&pkt, "%s:%s;%s", user, pass, psrvrnd);
			pkt = (char *)key_encrypttoasc(psrvkey, (unsigned char *)pkt, strlen(pkt));
			if(pkt != NULL)
			{
				NetSockPrintf(sd, "auth,%s\r\n", pkt);
				if(debugmode>1)
					printf("-->auth,%s\n", pkt);
				free(pkt);
			}

			memset(buf, 0, sizeof(buf));
			done = 0;
			while(!done && NetSockGets(sd, buf, sizeof(buf)-1, 5) > 0)
			{
				if(debugmode>1)
					printf("<--%s\n", buf);
				done = *(buf+3) == ' ';
			}

		}

		if(done)
		{
			NetSockPrintf(sd, "%s,%s\r\n", action, ipstr);
			if(debugmode)
				printf("-->%s,%s\n", action, ipstr);
		}

		memset(buf, 0, sizeof(buf));
		while(NetSockGets(sd, buf, sizeof(buf)-1, 1) > 0)
		{
			if(debugmode)
				printf("<--%s\n", buf);
		}
		NetSockPrintf(sd, "\r\n");

		rc = done;
	}

	return rc;
}

int cliIpfwActionIpv4(unsigned long ip, unsigned short port, char const *user, char const *pass, char *ipstr, char *action, int debugmode)
{	int sd = NetSockOpenTcpPeer(ip, port);
	int rc = cliIpfwActionSd(sd, user, pass, ipstr, action, debugmode);

	NetSockClose(&sd);

	return rc;
}

int cliIpfwActionAF(int afType, char const *afAddr, unsigned short port, char const *user, char const *pass, char *ipstr, char *action, int debugmode)
{	int sd = NetSockOpenTcpPeerAf(afType, afAddr, port);
	int rc = cliIpfwActionSd(sd, user, pass, ipstr, action, debugmode);

	NetSockClose(&sd);

	return rc;
}
