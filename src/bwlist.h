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
 *	CVSID:  $Id: bwlist.h,v 1.16 2014/02/28 05:37:57 neal Exp $
 *
 * DESCRIPTION:
 *	application:	Spamilter
 *	module:		bwlist.h
 *--------------------------------------------------------------------*/

#ifndef _SPAMILTER_BWLIST_H_
#define _SPAMILTER_BWLIST_H_

	enum { BWL_A_NULL, BWL_A_ACCEPT, BWL_A_REJECT, BWL_A_DISCARD, BWL_A_TEMPFAIL, BWL_A_TARPIT, BWL_A_EXEC, BWL_A_IPFW };
	enum { BWL_L_NULL, BWL_L_SNDR, BWL_L_RCPT };

	extern char *gpBwlStrs[];

#ifndef BWLIST_API
	typedef struct bwlistCtx_t bwlistCtx_t;
#else
	#include "table.h"
	typedef struct _bwlistCtx_t
	{
		tableDriver_t *pTdSndr;
#ifdef SUPPORT_AUTO_WHITELIST
		tableDriver_t *pTdSndrAuto;
#endif
		tableDriver_t *pTdRcpt;
		const char *pSessionId;
	}bwlistCtx_t;
#endif

	bwlistCtx_t *bwlistCreate(const char *pSessionId);
	void bwlistDestroy(bwlistCtx_t **ppBwlistCtx);

	int bwlistOpen(bwlistCtx_t *pBwListCtx, const char *dbpath);
	void bwlistClose(bwlistCtx_t *pBwlistCtx);

	int bwlistActionQuery(bwlistCtx_t *pBwlistCtx, int list, const char *dom, const char *mbox, char *exec);
#endif
