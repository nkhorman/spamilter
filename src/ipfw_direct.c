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
 *	CVSID:  $Id: ipfw_direct.c,v 1.12 2011/07/29 21:23:17 neal Exp $
 *
 * DESCRIPTION:
 *	application:	ipfwmtad
 *	module:		ipfw_direct.c
 *--------------------------------------------------------------------*/

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_fw.h>
#include <net/route.h> // def. of struct route
#include <netinet/ip_dummynet.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "misc.h"

extern int debugmode;

int ipfw_sd = -1;

int ipfw_add(int rulenum, const char *pIpStr, unsigned short port, int action)
{	int rc = -1;
	int afType = AF_UNSPEC;
	char *pAfAddr = NULL;


#if defined(IPFW2) || (__FreeBSD_version >= 600000)
	if(ipfw_sd != -1
		&& mlfi_inet_ptonAF(&afType, &pAfAddr, pIpStr)
		&& (afType == AF_INET || afType == AF_INET6)
		)
	{	socklen_t socklen;
		uint32_t rulebuf[255];
		struct ip_fw *rule = (struct ip_fw *)rulebuf;
		ipfw_insn *cmd = rule->cmd;

		bzero(rulebuf, sizeof(rulebuf));

		rule->rulenum = rulenum;

		// protocol
		cmd->len =  (cmd->len & (F_NOT | F_OR)) | 1;
		cmd->opcode = O_PROTO;
		cmd->arg1 = 6;	// tcp
		cmd += F_LEN(cmd);
		bzero(cmd, sizeof(*cmd));

		// source IP
		switch(afType)
		{
			case AF_INET:
				((ipfw_insn_ip *)cmd)->o.len &= ~F_LEN_MASK;	// zero len
				{	uint32_t *da = ((ipfw_insn_u32 *)cmd)->d;

					da[0] = *(unsigned long *)pAfAddr;//htonl(ip);
					// since /32, this is unessecary
					//da[1] = htonl(~0);	// force /32
					//da[0] &= da[1];		// mask base address with mask
				}
				((ipfw_insn_ip *)cmd)->o.len |= F_INSN_SIZE(ipfw_insn_u32);//*2;
				cmd->opcode = O_IP_SRC;//O_IP_SRC_MASK;
				break;
			case AF_INET6:
				((ipfw_insn_ip6 *)cmd)->o.len &= ~F_LEN_MASK;	// zero len
				{	struct in6_addr *d = &((ipfw_insn_ip6 *)cmd)->addr6;

					memcpy(d, pAfAddr, sizeof(struct in6_addr));
					// since /128, this is unessecary
					//memset(&d[1], 0xFF, sizeof(struct in6_addr)); // force /128
					//APPLY_MASK(&d[0], &d[1]);
				}
				((ipfw_insn_ip6 *)cmd)->o.len |= F_INSN_SIZE(ipfw_insn) + F_INSN_SIZE(struct in6_addr);//*2;
				cmd->opcode = O_IP6_SRC;//O_IP6_SRC_MASK;
				break;
		}
		cmd += F_LEN(cmd);
		bzero(cmd, sizeof(*cmd));

		// destination IP
		((ipfw_insn_ip *)cmd)->o.len &= ~F_LEN_MASK;	// zero len
		cmd->opcode = O_IP_DST;
		cmd += F_LEN(cmd);
		bzero(cmd, sizeof(*cmd));

		// destination port
		{	ipfw_insn_u16 *cmd16 = (ipfw_insn_u16 *)cmd;
			uint16_t *p = cmd16->ports;

			p[0] = p[1] = port;
			cmd16->o.len |= 2; // leave F_NOT and F_OR untouched
		}
		cmd->opcode = O_IP_DSTPORT;
		cmd += F_LEN(cmd);
		bzero(cmd, sizeof(*cmd));

		// start action section
		rule->act_ofs = cmd - rule->cmd;

		// action
		cmd->len = 1;
		cmd->opcode = (action==1? O_ACCEPT : O_DENY);
		cmd->arg1 = 0;
		cmd += F_LEN(cmd);
		bzero(cmd, sizeof(*cmd));

		rule->cmd_len = (uint32_t *)cmd - (uint32_t *)(rule->cmd);
		socklen = (char *)cmd - (char *)rule;
		rc = getsockopt(ipfw_sd, IPPROTO_IP, IP_FW_ADD, rule, &socklen);
#else
	if(ipfw_sd != -1
		&& mlfi_inet_ptonAF(&afType, &pAfAddr, pIpStr)
		&& afType == AF_INET
		)
	{	socklen_t socklen;
		struct ip_fw rule;
	
		memset(&rule, 0, sizeof(rule));
		rule.fw_number	= rulenum;
		rule.fw_flg	|= (action==1? IP_FW_F_ACCEPT : IP_FW_F_DENY);
		rule.fw_prot	= IPPROTO_TCP;

		// from
		rule.fw_src.s_addr	= *(unsigned long *)pAfAddr;//htonl(ip);
		rule.fw_smsk.s_addr	= htonl(~0);

		// to
		rule.fw_dst.s_addr	= 0;
		rule.fw_dmsk.s_addr	= 0;

		// dst port
		*(rule.fw_uar.fw_pts+IP_FW_GETNSRCP(&rule)) = port;
		IP_FW_SETNDSTP(&rule, 1);

		// No direction specified -> do both directions
		rule.fw_flg |= (IP_FW_F_OUT|IP_FW_F_IN);

		i = sizeof(rule);
		rc = getsockopt(ipfw_sd, IPPROTO_IP, IP_FW_ADD, &rule, &i);
#endif

		if(debugmode > 1)
			printf("ipfw_add: %s port %u = %d/%s errno %d\n", pIpStr, port, rc, (rc == 0 ? "Success":"Fail"), errno);
	}
#ifdef _UNIT_TEST
	else
		printf("ip address '%s' invalid\n", pIpStr);
#endif

	if(pAfAddr != NULL)
		free(pAfAddr);

	return(rc == 0);
}

int ipfw_del(int rulenum)
{	int	rc = -1;

	if(ipfw_sd != -1)
	{
#if defined(IPFW2) || (__FreeBSD_version >= 600000)
		uint32_t rule = rulenum;
#else
		struct ip_fw	rule;

		memset(&rule, 0, sizeof rule);
		rule.fw_number = rulenum;
#endif
		rc = setsockopt(ipfw_sd, IPPROTO_IP, IP_FW_DEL, &rule, sizeof(rule));
	}

	return(rc == 0);
}

void ipfw_startup()
{	
	if(ipfw_sd == -1)
		ipfw_sd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
#ifdef _UNIT_TEST
	else
		printf("Unable to open ipfw socket\n");
#endif
}

void ipfw_shutdown()
{
	if(ipfw_sd != -1)
	{
		close(ipfw_sd);
		ipfw_sd = -1;
	}
}

#ifdef _UNIT_TEST
int debugmode=1;
int main(int argc, char *argv[])
{	char *ipaddr_add = NULL;
	int ipaddr_del = 0;
	int ip_rule = 70;
	char c;

	while ((c = getopt(argc, argv, "a:d")) != -1)
	{
		switch (c)
		{
			case 'a':
				if(optarg != NULL && *optarg)
					ipaddr_add = optarg;
				break;
			case 'd':
				ipaddr_del = 1;
				break;
		}
	}
	argc -= optind;
	argv += optind;

	if(ipaddr_add != NULL || ipaddr_del)
	{
		if(getuid() == 0)
		{
			ipfw_startup();

			if(ipaddr_add != NULL)
				ipfw_add(ip_rule, ipaddr_add, 25, 0);
			else if(ipaddr_del)
				ipfw_del(ip_rule);

			ipfw_shutdown();
		}
		else
			printf("Must be root user\n");
	}

	return 0;
}
#endif
