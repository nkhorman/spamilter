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
 *	CVSID:  $Id: list.h,v 1.2 2012/06/14 04:21:35 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		list
 *--------------------------------------------------------------------*/

#ifndef _LIST_H_
#define _LIST_H_

// Sigh.... Yet another linked list construct. It sure
// would have been nice if libc included a few collections.

#ifdef _IS_LISTAPI_
	typedef struct _listElement_t
	{
		void *pData;
		struct _listElement_t *pNext;
	}listElement_t;

	typedef struct _list_t
	{
		unsigned long qty;
		listElement_t *pFirst;
		listElement_t *pLast;
	}list_t;
#else
	typedef struct list_t list_t;
#endif

	list_t *listCreate(void);
	void listDestroy(list_t *pList, int(*pCallback)(void *, void *), void *pCallbackData);

	unsigned long listQty(list_t *pList);

	int listAdd(list_t *pList, void *pData);

	void listForEach(list_t *pList, int(*pCallback)(void *, void *), void *pCallbackData);
	void *listGetAt(list_t *pList, size_t index); // index is one relative


#endif
