/*--------------------------------------------------------------------*
 *
 * Developed by;
 *	Neal Horman - http://www.wanlink.com
 *	Copyright (c) 2010 Neal Horman. All Rights Reserved
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
 *	CVSID:  $Id:$
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		ini.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id$";

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "ini.h"
#include "list.h"
#include "regexapi.h"

typedef struct _ov_t
{
	unsigned long keyHash;
	char const *pKeyName;
	int type; //OVT_xxx
	union
	{
		char const *pVal;
		uint32_t val;
	};
} ov_t; // Option Variadic Type

static list_t *gpListOptions = NULL;
static ik_t *gpIk = NULL;

#define PAT "[ \t]{0,}([a-zA-Z][a-zA-Z0-9_-]{0,})[ \t]{0,}=[ \t]{0,}([^\r\n]*)[\r\n]{0,2}$"

static ik_t *iniKeyFind(char const *pKeyName)
{	ik_t *pIk = gpIk;

	while(pIk && pIk->pName != NULL && strcasecmp(pKeyName, pIk->pName) != 0)
		pIk++;

	return (pIk && pIk->pName != NULL && strcasecmp(pKeyName, pIk->pName) == 0 ? pIk : NULL);
}

static int iniKeyNameType(char const *pKeyName)
{	ik_t *pIk = iniKeyFind(pKeyName);

	return (pIk != NULL ? pIk->type : OVT_NONE);
}

static unsigned long iniKeyHash(char const *pKeyName)
{	unsigned long keyHash = 0;
#define LARGENUMBER 6293815

	if(pKeyName != NULL && *pKeyName)
	{       unsigned long multiple = LARGENUMBER;

		for(int i=0, q=strlen(pKeyName); i<q; i++)
		{	unsigned char ch = pKeyName[i];

			if(ch != '\0')
			{
				keyHash += (multiple * i * ch);
				multiple *= LARGENUMBER;
			}
		}
	}

	return keyHash;
}

static void iniListItemValSet(ov_t *pOv, char const *pVal)
{
	switch(pOv->type)
	{
		case OVT_STR:
			if(pOv->pVal != NULL)
				free((void *)pOv->pVal);
			pOv->pVal = strdup(pVal);
			break;

		case OVT_BOOL:
			pOv->val = (strcasecmp(pVal, "1") == 0
				|| strcasecmp(pVal, "on") == 0
				|| strcasecmp(pVal, "true") == 0
				|| strcasecmp(pVal, "yes") == 0
				);
			break;

		case OVT_INT:
			pOv->val = atoi(pVal);
			break;
	}
}

static ov_t *iniListItemAlloc(int type, char const *pKey, char const *pVal)
{	ov_t *pOv = calloc(1, sizeof(ov_t));

	if(pOv != NULL)
	{
		pOv->type = type;
		pOv->keyHash = iniKeyHash(pKey);
		pOv->pKeyName = strdup(pKey);
		iniListItemValSet(pOv, pVal);
	}

	return pOv;
}

static int iniListItemFree(void *pItemData, void *pItemContext)
{	ov_t *pOv = (ov_t *)pItemData;

	(void)pItemContext;

	if(pOv->pKeyName != NULL)
		free((void *)pOv->pKeyName);
	if(pOv->type == OVT_STR)
		free((void *)pOv->pVal);

	return 1; // again
}

void iniFree(void)
{
	if(gpListOptions)
	{
		listDestroy(gpListOptions, &iniListItemFree, NULL);
		gpListOptions = NULL;
	}
}

void iniInit(ik_t *pIk)
{
	iniFree();
	gpListOptions = listCreate();
	gpIk = pIk;

	while(pIk != NULL && pIk->type != OVT_NONE)
	{
		listAdd(gpListOptions, iniListItemAlloc(pIk->type, pIk->pName, pIk->pValueDefault));
		pIk++;
	}
}

list_t *iniRead(char const *pFname)
{	list_t *pListInvalidOptions = NULL;
	FILE *fin = fopen(pFname,"r");

	if(fin != NULL)
	{	char buf[1024];
		char *str;

		while(!feof(fin))
		{
			if(fgets(buf, sizeof(buf), fin) != NULL)
			{
				// Comment removal and white space trimming if comment present
				str = strchr(buf,'#');
				if(str != NULL)
				{
					*(str--) = '\0';
					while(str >= buf && (*str ==' ' || *str == '\t'))
						*(str--) = '\0';
				}

				// If something left, grok it out, and validate the option keyname.
				// If validated, set the value of the key
				if(strlen(buf))
				{	regexapi_t *prat = regexapi_exec(buf, PAT, REGEX_DEFAULT_CFLAGS, 1);

					if(regexapi_matches(prat) && regexapi_nsubs(prat,0) == 2)
					{
						const char *pKey = regexapi_sub(prat,0,0);
						const char *pVal = regexapi_sub(prat,0,1);
						int keyType = iniKeyNameType(pKey);

						if(keyType != OVT_NONE)
							iniSet(pKey, pVal);
						else
						{
							if(pListInvalidOptions == NULL)
								pListInvalidOptions = listCreate();
							listAdd(pListInvalidOptions, strdup(pKey));
						}
					}
					regexapi_free(prat);
				}
			}
		}
		fclose(fin);
	}

	return pListInvalidOptions;
}

typedef struct _ioc_t
{
	unsigned long keyHash;
	ov_t *pOv;

} ioc_t;

static int callbackIniOvKeyNameMatch(void *pCallbackData, void *pCallbackCtx)
{	ov_t *pOv = (ov_t *)pCallbackData;
	ioc_t *pIoc = (ioc_t *)pCallbackCtx;

	if(pIoc->keyHash == pOv->keyHash)
		pIoc->pOv = pOv;

	return (pIoc->pOv == NULL); // again ?
}

void iniSet(char const *pKey, char const *pVal)
{	ioc_t ioc;

	ioc.keyHash = iniKeyHash(pKey);
	ioc.pOv = NULL;

	listForEach(gpListOptions, &callbackIniOvKeyNameMatch, &ioc);

	if(ioc.pOv != NULL)
		iniListItemValSet(ioc.pOv, pVal);
}

char const *iniGetStr(char const *pKey)
{	ioc_t ioc;

	ioc.keyHash = iniKeyHash(pKey);
	ioc.pOv = NULL;

	listForEach(gpListOptions, &callbackIniOvKeyNameMatch, &ioc);

	return (ioc.pOv != NULL ? ioc.pOv->pVal : NULL);
}

int iniGetInt(char const *pKey)
{	ioc_t ioc;

	ioc.keyHash = iniKeyHash(pKey);
	ioc.pOv = NULL;

	listForEach(gpListOptions, &callbackIniOvKeyNameMatch, &ioc);

	return (ioc.pOv != NULL ? ioc.pOv->val : 0);
}

