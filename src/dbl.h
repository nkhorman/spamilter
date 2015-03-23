/*--------------------------------------------------------------------*
 *
 * Developed by;
 *	Neal Horman - http://www.wanlink.com
 *	Copyright (c) 2012 Neal Horman. All Rights Reserved
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
 *	CVSID:  $Id: dbl.h,v 1.2 2012/12/16 22:10:14 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		dsbl.h
 *--------------------------------------------------------------------*/

#ifndef _DBL_H_
#define _DBL_H_

	#include "dns.h"

	typedef struct _dblq_t dblq_t;
	typedef struct _dblcb_t dblcb_t;

	typedef int(*dbl_callback_t)(const dblcb_t *);
	typedef int(*dbl_callback_policy_t)(dblcb_t *);

	// The query parameters
	struct _dblq_t
	{
		const char *pDomain;
		dbl_callback_t pCallbackFn;
		void *pCallbackData;
		dbl_callback_policy_t pCallbackPolicyFn;
	};

	// Callback context
	struct _dblcb_t
	{
		const dblq_t *pDblq;
		const char *pDbl;
		dqrr_t *pDqrr;

		// NB. It's the callback policy function's reponsibility
		// to fill this out for the callback function's use.
		// This is meant to be the result of something like a
		// inet_ntop() based on the pNsrr content.
		char *pDblResult;
		// See dbl.c dbl_callback_policy_std() for meaning.
		int abused;
	};

	#include "table.h"
	typedef struct _dblCtx_t
	{
		tableDriver_t *pTableDriver;
		const char *pSessionId;
	}dblCtx_t;

	dblCtx_t *dbl_Create(const char *pSessionId);
	void dbl_Destroy(dblCtx_t **ppDblCtx);
	int dbl_Open(dblCtx_t *pDblCtx, const char *dbpath);
	void dbl_Close(dblCtx_t *pDblCtx);

	void dbl_check_all(dblCtx_t *pCtx, ds_t const *pDs, const dblq_t *pDblq);
	int dbl_callback_policy_std(dblcb_t *pDblcb);
#endif
