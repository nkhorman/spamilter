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
 *	RCSID:  $Id$
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		ifidb.c
 *--------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <netdb.h>

#include "ifidb.h"
#include "misc.h"

#ifdef SUPPORT_TABLE_FLATFILE
#include "tableDriverFlatFile.h"
#endif
#ifdef SUPPORT_TABLE_PGSQL
#include "tableDriverPsql.h"
#endif


#ifndef s6_addr32
#define s6_addr32 __u6_addr.__u6_addr32
#endif

enum
{
	TBL_COL_ALLOWDENY,
	TBL_COL_ADDRESS,
	TBL_COL_EXECEPTION,
	TBL_COL_QTY
};

typedef struct _idrc_t
{
	char *pCols[TBL_COL_QTY];
	int colIndex;
}idrc_t; // Ifi Db Row Context Type

typedef struct _ira_t
{
	int afType;
	union
	{
		struct in_addr ipv4;
		struct in6_addr ipv6;
	};
	int maskLen;
}ira_t; // Ifi Row Address Type

enum
{
	IFIDB_NONE,
	IFIDB_ALLOW,
	IFIDB_DENY
};

typedef struct _ir_t
{
	int allowDeny;
	list_t *pListAddress;
	list_t *pListException;
}ir_t; // Ifi Row Type

ifiDbCtx_t *ifiDb_Create(const char *pSessionId)
{	ifiDbCtx_t *pIfiDbCtx = (ifiDbCtx_t *)calloc(1,sizeof(ifiDbCtx_t));

	if(pIfiDbCtx != NULL)
	{
		// setup the table drivers
#ifdef SUPPORT_TABLE_FLATFILE
		pIfiDbCtx->pTableDriver = tableDriverFlatFileCreate(pSessionId);
#endif
#ifdef SUPPORT_TABLE_PGSQL
		pIfiDbCtx->pTableDriver = tableDriverPsqlCreate(pSessionId);
#endif
		pIfiDbCtx->pSessionId = pSessionId;
	}

	return pIfiDbCtx;
}

void ifiDb_Destroy(ifiDbCtx_t **ppIfiDbCtx)
{
	if(ppIfiDbCtx != NULL && *ppIfiDbCtx != NULL)
	{
#ifdef SUPPORT_TABLE_FLATFILE
		tableDriverFlatFileDestroy(&(*ppIfiDbCtx)->pTableDriver);
#endif
#ifdef SUPPORT_TABLE_PGSQL
		tableDriverPsqlDestroy(&(*ppIfiDbCtx)->pTableDriver);
#endif
		free(*ppIfiDbCtx);
		*ppIfiDbCtx = NULL;
	}
}

int ifiDb_Open(ifiDbCtx_t *pIfiDbCtx, const char *dbpath)
{
	if(pIfiDbCtx != NULL && dbpath != NULL)
	{
#ifdef SUPPORT_TABLE_FLATFILE
		char	*fn;
#endif

		if(!tableIsOpen(pIfiDbCtx->pTableDriver))
		{
#ifdef SUPPORT_TABLE_FLATFILE
			asprintf(&fn,"%s/db.ifi",dbpath); tableOpen(pIfiDbCtx->pTableDriver,"table, colqty",fn,TBL_COL_QTY);
			if(!tableIsOpen(pIfiDbCtx->pTableDriver))
				mlfi_debug(pIfiDbCtx->pSessionId,"ifiDb: Unable to open file '%s'\n",fn);
			free(fn);
#endif
#ifdef SUPPORT_TABLE_PGSQL
			tableOpen(pIfiDbCtx->pTableDriver
				"table, colqty"
				",tableCols"
				",host" ",hostPort"
				",Device" ",userName" ",userPass"

				,"ifiDb",TBL_COL_QTY
				,"allowdeny,address,exception"
				,"localhost" ,"5432"
				,"spamilter" ,"spamilter" ,""
				);
			if(!tableIsOpen(pIfiDbCtx->pTableDriver))
				mlfi_debug(pIfiDbCtx->pSessionId,"ifiDb: Unable to open\n");
#endif
		}
	}

	return (pIfiDbCtx != NULL && tableIsOpen(pIfiDbCtx->pTableDriver));
}

void ifiDb_Close(ifiDbCtx_t *pIfiDbCtx)
{
	if(pIfiDbCtx != NULL)
	{
		if(tableIsOpen(pIfiDbCtx->pTableDriver))
			tableClose(pIfiDbCtx->pTableDriver);
	}
}

#ifdef _UNIT_TEST_IFIDB
#define SAFESTR(a) ((a) != NULL ? (a) : "")

static void ifiDb_ShowRow(idrc_t *pIdrc)
{
	if(pIdrc != NULL)
	{
		printf("%s|%s|%s\n"
			, SAFESTR(pIdrc->pCols[TBL_COL_ALLOWDENY])
			, SAFESTR(pIdrc->pCols[TBL_COL_ADDRESS])
			, SAFESTR(pIdrc->pCols[TBL_COL_EXECEPTION])
		);
	}
}
#endif

static list_t* ifiDb_IrBuildAddressList(char *pStr, int *pAfType)
{	list_t *pList = NULL;

	if(pStr != NULL)
	{	char *p1;
		char *p2;

		pList = listCreate();
		while((p1 = mlfi_stradvtok(&pStr, ',')) != NULL && *p1)
		{
			p2 = p1;
			p1 = mlfi_stradvtok(&p2,'/');

			if(p1 != NULL && p2 != NULL && *p1 && p2)
			{	ira_t ira;

				ira.maskLen = atoi(p2);
				if((*pAfType == AF_UNSPEC || *pAfType == AF_INET)
					&& ira.maskLen <=32
					&& inet_pton(AF_INET, p1, &ira.ipv4)
					)
				{
					*pAfType = ira.afType = AF_INET;
				}
				else if(
					(*pAfType == AF_UNSPEC || *pAfType == AF_INET6)
					&& ira.maskLen <= 128
					&& inet_pton(AF_INET6, p1, &ira.ipv6)
					)
				{
					*pAfType = ira.afType = AF_INET6;
				}
				else
				{
					*pAfType = ira.afType = AF_UNSPEC;
					ira.maskLen = 0;
				}

				if(ira.afType != AF_UNSPEC)
				{	ira_t *pIra = calloc(1,sizeof(ira_t));

					if(pIra != NULL)
					{
						*pIra = ira;
						listAdd(pList, pIra);
					}
				}
			}
		}

		if(listQty(pList) == 0)
			listDestroy(pList, NULL, NULL);
	}

	return pList;
}

static int ifiDb_IrFreeCallback(void *pData, void *pCallbackCtx)
{	
	(void)pCallbackCtx;
	free(pData);

	return 1;
}

static void ifiDb_IrFree(ir_t *pIr)
{
	if(pIr != NULL)
	{
		listDestroy(pIr->pListAddress, &ifiDb_IrFreeCallback, NULL);
		listDestroy(pIr->pListException, &ifiDb_IrFreeCallback, NULL);
		free(pIr);
	}
}

ir_t *ifiDb_IrBuild(char *pCols[])
{	ir_t *pIr = calloc(1, sizeof(ir_t));

	if(pIr != NULL)
	{	char *pStr = pCols[TBL_COL_ALLOWDENY];

		pIr->allowDeny = (strcasecmp(pStr, "allow") == 0
			? IFIDB_ALLOW : strcasecmp(pStr, "deny") == 0
			? IFIDB_DENY
			: IFIDB_NONE
			);

		if(pIr->allowDeny != IFIDB_NONE)
		{	int afType[2] = { AF_UNSPEC, AF_UNSPEC };

			pIr->pListAddress = ifiDb_IrBuildAddressList(pCols[TBL_COL_ADDRESS], &afType[0]);
			pIr->pListException = ifiDb_IrBuildAddressList(pCols[TBL_COL_EXECEPTION], &afType[1]);

			// missing address
			if(pIr->pListAddress == NULL)
			{
				ifiDb_IrFree(pIr);
				pIr = NULL;
				printf("free 3\n"); // TODO - mlfi_debug
			}
			// mixed address types ?
			else if(afType[1] != AF_UNSPEC && afType[0] != afType[1])
			{
				ifiDb_IrFree(pIr);
				pIr = NULL;
				printf("free 1\n"); // TODO - mlfi_debug
			}
		}
		else // invalid action
		{
			ifiDb_IrFree(pIr);
			pIr = NULL;
			printf("free 2\n"); // TODO - mlfi_debug
		}
	}

	return pIr;
}

// collect the column pointers
static int ifiDbCallbackBuildListCol(void *pData, void *pCallbackData)
{	idrc_t *pIrc = (idrc_t *)pCallbackData;
	int again = 1; // iterate for more columns

	if(pIrc->colIndex < TBL_COL_QTY)
		pIrc->pCols[pIrc->colIndex++] = (char *)pData;
	else
		again = 0; // reached max column count, stop

	return again;
}

static int ifiDbCallbackBuildList(void *pCallbackCtx, list_t *pRow)
{	int again = 1;

	if(pRow != NULL)
	{	ifiDbCtx_t *pCtx = (ifiDbCtx_t *)pCallbackCtx;
		idrc_t idrc;

		memset(&idrc, 0, sizeof(idrc));
		// iterate the cols
		listForEach(pRow, &ifiDbCallbackBuildListCol, &idrc);

		// we need at least the first two of the three columns
		if(idrc.colIndex >= TBL_COL_QTY-1)
		{	ir_t *pIr = NULL;
#ifdef _UNIT_TEST_IFIDB
			ifiDb_ShowRow(&idrc);
#endif
			pIr = ifiDb_IrBuild(idrc.pCols);
			if(pIr != NULL)
				listAdd(pCtx->pIfiDb, pIr);
		}
	}

	return again;
}

void ifiDb_BuildList(ifiDbCtx_t *pCtx)
{
	if(pCtx->pIfiDb == NULL)
		pCtx->pIfiDb = listCreate();
	tableForEachRow(pCtx->pTableDriver, &ifiDbCallbackBuildList, pCtx);
}

typedef struct _idc_t
{
	int afType;
	const char *pIn;
	int match;
	int action;
}idc_t; // Ifi Db Check Type

#define BITSHIFT(a,b) \
{ \
	int i; \
	for(i=0; i<(b); i++) \
		(a) <<= 1; \
}

#define BITMASK(a,b) \
{ \
	(a).s6_addr32[0] &= (b).s6_addr32[0]; \
	(a).s6_addr32[1] &= (b).s6_addr32[1]; \
	(a).s6_addr32[2] &= (b).s6_addr32[2]; \
	(a).s6_addr32[3] &= (b).s6_addr32[3]; \
}

int ifiDb_CheckAddressCallback(void *pData, void *pCallbackCtx)
{	ira_t *pIra = (ira_t *)pData;
	idc_t *pIdc = (idc_t *)pCallbackCtx;

	if(pIdc->afType == pIra->afType)
	{
		switch(pIdc->afType)
		{
			case AF_INET:
				{	uint32_t mask = ~0;

					BITSHIFT(mask, 32-pIra->maskLen);
					mask = htonl(mask);

					pIdc->match = ((((struct in_addr *)pIdc->pIn)->s_addr & mask) == (pIra->ipv4.s_addr & mask));
				}
				break;
			case AF_INET6:
				{	struct in6_addr l = *(struct in6_addr *)pIdc->pIn;
					struct in6_addr r = pIra->ipv6;
					struct in6_addr mask;

					memset(&mask, 0, sizeof(mask));

					if(pIra->maskLen <=32)
					{
						mask.s6_addr32[0] = ~0;
						BITSHIFT(mask.s6_addr32[0], 32-pIra->maskLen)
					}
					else if(pIra->maskLen <=64)
					{
						mask.s6_addr32[0] = ~0;
						mask.s6_addr32[1] = ~0;
						BITSHIFT(mask.s6_addr32[1], 64-pIra->maskLen)
					}
					else if(pIra->maskLen <=96)
					{
						mask.s6_addr32[0] = ~0;
						mask.s6_addr32[1] = ~0;
						mask.s6_addr32[2] = ~0;
						BITSHIFT(mask.s6_addr32[2], 96-pIra->maskLen)
					}
					else if(pIra->maskLen <=128)
					{
						mask.s6_addr32[0] = ~0;
						mask.s6_addr32[1] = ~0;
						mask.s6_addr32[2] = ~0;
						mask.s6_addr32[3] = ~0;
						BITSHIFT(mask.s6_addr32[3], 128-pIra->maskLen)
					}

					BITMASK(l, mask);
					BITMASK(r, mask);
					pIdc->match = IN6_ARE_ADDR_EQUAL(&l, &r);
				}
				break;
		}
	}

	return !pIdc->match;
}

int ifiDb_CheckRowCallback(void *pData, void *pCallbackCtx)
{	ir_t *pIr = (ir_t *)pData;
	idc_t *pIdc = (idc_t *)pCallbackCtx;

	listForEach(pIr->pListAddress, ifiDb_CheckAddressCallback, pIdc);

	if(pIdc->match)
	{	idc_t idc;

		pIdc->action = pIr->allowDeny;

		idc.afType = pIdc->afType;
		idc.pIn = pIdc->pIn;
		idc.match = 0;
		idc.action = IFIDB_NONE;

		listForEach(pIr->pListException, ifiDb_CheckAddressCallback, &idc);

		if(idc.match)
		{
			switch(pIdc->action)
			{
				case IFIDB_ALLOW: pIdc->action = IFIDB_DENY; break;
				case IFIDB_DENY: pIdc->action = IFIDB_ALLOW; break;
			}
		}
	}

	return !pIdc->match;
}

int ifiDb_CheckAllow(int afType, const char *pIn, list_t *pIfiDb, int *pAllow)
{	idc_t idc;

	idc.afType = afType;
	idc.pIn = pIn;
	idc.match = 0;
	idc.action = IFIDB_NONE;

	listForEach(pIfiDb, &ifiDb_CheckRowCallback, &idc);

	*pAllow = (idc.action == IFIDB_ALLOW);

	return idc.match;
}

#ifdef _UNIT_TEST_IFIDB
#include <syslog.h>

static int cacbAddress(void *pData, void *pCallbackCtx)
{	ira_t *pIra = (ira_t *)pData;

	char *p1 = mlfi_inet_ntopAF(pIra->afType, (char *)&pIra->ipv4);
	printf("%s/%u, ", p1, pIra->maskLen);
	free(p1);

	return 1;
}

static int cacbRow(void *pData, void *pCallbackCtx)
{	ir_t *pIr = (ir_t *)pData;

	printf("%s | ", pIr->allowDeny == IFIDB_ALLOW ? "Allow" : "Deny");
	listForEach(pIr->pListAddress, &cacbAddress, NULL);
	printf(" | ");
	listForEach(pIr->pListException, &cacbAddress, NULL);
	printf("\n");

	return 1;
}

void test1(list_t *pList)
{
	listForEach(pList, &cacbRow, NULL);
}

int main(int argc, char **argv)
{
	int c;
	list_t *pIfiDb = NULL;

	while ((c = getopt(argc, argv, "")) != -1)
	{
		switch (c)
		{
		}
	}
	argc -= optind;
	argv += optind;


	openlog("ifiDb", LOG_PERROR|LOG_NDELAY|LOG_PID, LOG_DAEMON);

	ifiDbCtx_t *pIfiDbCtx = ifiDb_Create("");
	if(ifiDb_Open(pIfiDbCtx, "/var/db/spamilter"))
	{
		ifiDb_BuildList(pIfiDbCtx);
		ifiDb_Close(pIfiDbCtx);
		pIfiDb = pIfiDbCtx->pIfiDb;
	}
	ifiDb_Destroy(&pIfiDbCtx);

	test1(pIfiDb);

	return 0;
}
#endif