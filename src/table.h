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
 *	CVSID:  $Id: table.h,v 1.2 2012/06/25 01:39:34 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		table
 *--------------------------------------------------------------------*/

#ifndef _TABLE_H_
#define _TABLE_H_

 	#include <stdarg.h>
	#include "list.h"

	#ifndef TABLEDRIVERCTX_T
	typedef struct tableDriverCtx_t tableDriverCtx_t;
	#endif

	typedef int (*tableDriverParamSetFn_t)(tableDriverCtx_t *pDriver, const char *pKey, va_list *vl);
	typedef int (*tableDriverFn_t)(tableDriverCtx_t *pDriver);
	typedef int (*tableDriverRowGetFn_t)(tableDriverCtx_t *pDriver, list_t **ppRow);

	typedef int(*tableForEachCallback_t)(void *pCallbackCtx, list_t *pRow);

	typedef int(*tableDriverListDestroyFn_t)(void *pData, void *pCallbackData);

	typedef struct _tableDriver_t_
	{
		tableDriverParamSetFn_t paramSetFn;
		tableDriverFn_t openFn;
		tableDriverFn_t closeFn;
		tableDriverRowGetFn_t rowGetFn;
		tableDriverFn_t rewindFn;
		tableDriverFn_t isOpenFn;
		tableDriverListDestroyFn_t listDestroyCallbackFn;
		void *pListDestroyCallbackFnData;

		tableDriverCtx_t *pDriverCtx;
		list_t *pRow;
	}tableDriver_t;

	int tableOpen(tableDriver_t *pDriver, const char *pKeys, ...);
	int tableIsOpen(tableDriver_t *pDriver);
	int tableClose(tableDriver_t *pDriver);
	int tableForEachRow(tableDriver_t *pDriver, tableForEachCallback_t pCallbackFn, void *pCallbackCtx);

#endif
