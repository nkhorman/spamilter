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
 *	CVSID:  $Id: list.c,v 1.3 2012/06/14 04:21:35 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		list
 *--------------------------------------------------------------------*/

#include <stdlib.h>

#define _IS_LISTAPI_
#include "list.h"

list_t *listCreate(void)
{
	return (list_t *)calloc(1,sizeof(list_t));
}

void listDestroy(list_t *pList, int(*pCallback)(void *, void *), void *pCallbackData)
{
	if(pList != NULL)
	{	int i;
		listElement_t *pE1 = pList->pFirst;
		listElement_t *pE2 = NULL;

		for(i=0; i<pList->qty; i++)
		{
			if(pCallback != NULL)
				pCallback(pE1->pData,pCallbackData);
			pE2 = pE1->pNext;
			free(pE1);
			pE1 = pE2;
		}

		free(pList);
	}
}

unsigned long listQty(list_t *pList)
{
	return (pList != NULL ? pList->qty : 0);
}

int listAdd(list_t *pList, void *pData)
{	int ok = 0;

	if(pList != NULL)
	{	listElement_t *pE = (listElement_t *)calloc(1,sizeof(listElement_t));

		if(pList->pFirst == NULL)
			pList->pFirst = pE;
		if(pList->pLast != NULL)
			pList->pLast->pNext = pE;

		if(pE != NULL)
		{
			pList->qty ++;
			pList->pLast = pE;
			pE->pData = pData;
			ok = 1;
		}
	}

	return ok;
}

void listForEach(list_t *pList, int(*pCallback)(void *, void *), void *pCallbackData)
{
	if(pList != NULL)
	{	int i,again=1;
		listElement_t *pE = pList->pFirst;

		for(i=0; i<pList->qty && again && pE != NULL; i++)
		{
			again = pCallback(pE->pData,pCallbackData);
			pE = pE->pNext;
		}
	}
}

// index is one relative
void *listGetAt(list_t *pList, size_t index)
{	void *pData = NULL;

	if(pList != NULL && index > 0 && index <= pList->qty)
	{	listElement_t *pE = pList->pFirst;

		while(pE != NULL && index)
		{
			index--;
			if(index == 0)
				pData = pE->pData;
			pE = pE->pNext;
		}
	}

	return pData;
}
