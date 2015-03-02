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
 *	CVSID:  $Id: dnsbl.h,v 1.13 2012/11/18 21:13:16 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		dnsbl.h
 *--------------------------------------------------------------------*/

#ifndef _SPAMILTER_DNSBL_H_
#define _SPAMILTER_DNSBL_H_

	enum { RBL_A_NULL, RBL_A_TAG, RBL_A_REJECT };
	enum { RBL_S_NULL, RBL_S_CONN, RBL_S_FROM, RBL_S_RCPT, RBL_S_HDR, RBL_S_EOM };

	typedef struct _rblhost
	{
		char	hostname[256];
		char	url[512];
		int	action;
		int	stage;
	} RBLHOST;

	typedef struct _rbllisthosts_t
	{
		size_t qty;
		RBLHOST *plist;
	}RBLLISTHOSTS;

	typedef struct _rbllistmatch_t
	{
		struct sockaddr sock;
		size_t qty;
		RBLHOST **ppmatch;
	}RBLLISTMATCH;

	int dnsbl_check_rbl_af(const char *pSessionId, const res_state statp, int afType, const char *in, char *domain);
	int dnsbl_check_rbl_sa(const char *pSessionId, const res_state statp, struct sockaddr *psa, char *domain);

	RBLLISTMATCH *dnsbl_check(const char *pSessionId, int stage, RBLLISTHOSTS *prbls, struct sockaddr *psa, res_state presstate);
	RBLHOST *dnsbl_action(const char *pSessionId, RBLHOST **prbls, int stage);

	void dnsbl_add_hdr(SMFICTX *ctx, RBLHOST *prbl);

	void dnsbl_free_match(RBLLISTMATCH *pmatch);
	void dnsbl_free_hosts(RBLLISTHOSTS *plist);
	RBLLISTHOSTS *dnsbl_create(const char *pSessionId, char *dbpath);
#endif
