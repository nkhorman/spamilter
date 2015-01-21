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
 *	CVSID:  $Id: table.c,v 1.2 2012/06/25 01:39:34 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		table
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: table.c,v 1.2 2012/06/25 01:39:34 neal Exp $";

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "table.h"
#include "misc.h"

int tableOpen(tableDriver_t *pDriver, const char *pKeys, ...)
{	int rc = -1;

	if(pDriver != NULL && pDriver->openFn != NULL)
	{
		if(pDriver->paramSetFn != NULL && pKeys != NULL && *pKeys)
		{	char *str1 = strdup(pKeys);;
			char *str2 = str1;
			va_list vl;

			va_start(vl,pKeys);
			while(str1 != NULL && *str1)
				pDriver->paramSetFn(pDriver->pDriverCtx,mlfi_stradvtok(&str1,','),&vl);
			va_end(vl);

			if(str2 != NULL)
				free(str2);
		}

		rc = pDriver->openFn(pDriver->pDriverCtx);
	}

	return rc;
}

int tableIsOpen(tableDriver_t *pDriver)
{
	return (pDriver != NULL && pDriver->isOpenFn != NULL ? pDriver->isOpenFn(pDriver->pDriverCtx) : 0);
}

int tableClose(tableDriver_t *pDriver)
{	int rc = -1;

	if(pDriver != NULL)
	{
		if(pDriver->closeFn != NULL)
			rc = pDriver->closeFn(pDriver->pDriverCtx);
		if(pDriver->pRow != NULL)
		{
			listDestroy(pDriver->pRow,pDriver->listDestroyCallbackFn,pDriver->pListDestroyCallbackFnData);
			pDriver->pRow = NULL;
		}
	}

	return rc;
}

int tableForEachRow(tableDriver_t *pDriver, tableForEachCallback_t pCallbackFn, void *pCallbackCtx)
{	int rc = -1;

	if(pDriver != NULL && pDriver->rowGetFn != NULL)
	{	int again = 1;

		rc = (pDriver->rewindFn != NULL ? pDriver->rewindFn(pDriver->pDriverCtx) : 0);
		while(rc > -1 && again)
		{
			rc = pDriver->rowGetFn(pDriver->pDriverCtx,&pDriver->pRow);
			if(rc > -1 && listQty(pDriver->pRow) > 0)
				again = pCallbackFn(pCallbackCtx,pDriver->pRow);
		}
	}

	return rc;
}
