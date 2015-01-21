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
 *	CVSID:  $Id: tableDriverPsql.c,v 1.2 2012/11/18 21:13:16 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		tableDriverPsql
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: tableDriverPsql.c,v 1.2 2012/11/18 21:13:16 neal Exp $";

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "pgsql.h"

// private structure
typedef struct _tableDriverCtx_t
{
	size_t paramColQty;
	const char *pParamTableName;
	const char *pParamTableCols;
	const char *pParamTableWhere;

	char const *pDbHost;
	char const *pDbHostPort;
	char const *pDbDevice;
	char const *pDbUserName;
	char const *pDbUserPass;

	ppgsql_srvr pDbSrvr;
	ppgsql_query pDbQuery;
	int dbQueryRowNum;
	const char *pSessionId;

}tableDriverCtx_t;
#define TABLEDRIVERCTX_T

#include "tableDriverPsql.h"

// private prototypes
static int paramSetFn(tableDriverCtx_t *pDriverCtx, const char *pKey, va_list *vl);
static int openFn(tableDriverCtx_t *pDriverCtx);
static int closeFn(tableDriverCtx_t *pDriverCtx);
static int rowGetFn(tableDriverCtx_t *pDriverCtx, list_t **ppRow);
static int rewindFn(tableDriverCtx_t *pDriverCtx);
static int isOpenFn(tableDriverCtx_t *pDriverCtx);
static int listDestroyCallbackFn(void *pData, void *pCallbackData);

#define BUFFERSIZE 8192

#include "misc.h"

// public api
tableDriver_t *tableDriverPsqlCreate(const char *pSessionId)
{	tableDriver_t *pDriver = (tableDriver_t *)calloc(1,sizeof(tableDriver_t));

	if(pDriver != NULL)
	{
		//mlfi_debug(pSessionId,"tableDriverPsqlCreate: 1\n");
		pDriver->paramSetFn = &paramSetFn;
		pDriver->openFn = &openFn;
		pDriver->closeFn = &closeFn;
		pDriver->rowGetFn = &rowGetFn;
		pDriver->rewindFn = &rewindFn;
		pDriver->isOpenFn = &isOpenFn;
		pDriver->listDestroyCallbackFn = &listDestroyCallbackFn;
		pDriver->pDriverCtx = (tableDriverCtx_t *)calloc(1,sizeof(tableDriverCtx_t));
		if(pDriver->pDriverCtx != NULL)
		{
			//mlfi_debug(pSessionId,"tableDriverPsqlCreate: 2\n");
			pDriver->pDriverCtx->pDbHost = "localhost";
			pDriver->pDriverCtx->pDbHostPort = "5432";
			pDriver->pDriverCtx->pDbDevice = "spamilter";
			pDriver->pDriverCtx->pDbUserName = "spamilter";
			pDriver->pDriverCtx->pDbUserPass = "";
			pDriver->pDriverCtx->pDbSrvr = NULL;
			pDriver->pDriverCtx->pSessionId = pSessionId;
		}
	}
	else
		mlfi_debug(pSessionId,"tableDriverPsqlCreate: fail - invalid driver\n");

	return pDriver;
}

int tableDriverPsqlDestroy(tableDriver_t **ppDriver)
{	int rc = -1;

	if(ppDriver != NULL)
	{	tableDriver_t *pDriver = *ppDriver;

		if(pDriver != NULL)
		{
			if(pDriver->pDriverCtx != NULL)
			{
				closeFn(pDriver->pDriverCtx);
				free(pDriver->pDriverCtx);
			}
			free(pDriver);
			*ppDriver = NULL;
			rc = 0;
		}
	}

	return rc;
}

// private table driver api
int paramSetFn(tableDriverCtx_t *pDriverCtx, const char *pKey, va_list *pvl)
{	int rc = -1;

	if(pDriverCtx != NULL && pKey != NULL && *pKey)
	{
		if(strcasecmp(pKey,"colqty") == 0) { pDriverCtx->paramColQty = (size_t)va_arg(*pvl,size_t); rc = 0; }
		if(strcasecmp(pKey,"table") == 0) { pDriverCtx->pParamTableName = (const char *)va_arg(*pvl,const char *); rc = 0; }
		if(strcasecmp(pKey,"tableCols") == 0) { pDriverCtx->pParamTableCols = (const char *)va_arg(*pvl,const char *); rc = 0; }
		if(strcasecmp(pKey,"tableWhere") == 0) { pDriverCtx->pParamTableWhere = (const char *)va_arg(*pvl,const char *); rc = 0; }

		if(strcasecmp(pKey,"host") == 0) { pDriverCtx->pDbHost = (const char *)va_arg(*pvl,const char *); rc = 0; }
		if(strcasecmp(pKey,"hostPort") == 0) { pDriverCtx->pDbHostPort = (const char *)va_arg(*pvl,const char *); rc = 0; }
		if(strcasecmp(pKey,"Device") == 0) { pDriverCtx->pDbDevice = (const char *)va_arg(*pvl,const char *); rc = 0; }
		if(strcasecmp(pKey,"userName") == 0) { pDriverCtx->pDbUserName = (const char *)va_arg(*pvl,const char *); rc = 0; }
		if(strcasecmp(pKey,"userPass") == 0) { pDriverCtx->pDbUserPass = (const char *)va_arg(*pvl,const char *); rc = 0; }
	}

	return rc;
}

int openFn(tableDriverCtx_t *pDriverCtx)
{	int rc = -1;

	if(pDriverCtx != NULL)
	{
		pDriverCtx->pDbSrvr = pgsql_srvr_init(pDriverCtx->pDbHost,pDriverCtx->pDbHostPort,pDriverCtx->pDbDevice,pDriverCtx->pDbUserName,pDriverCtx->pDbUserPass);
		if(pDriverCtx->pDbSrvr != NULL)
		{
			//pDriverCtx->pDbSrvr->dbl->debug = 2;
			pgsql_open(pDriverCtx->pDbSrvr);
		}
		rc = isOpenFn(pDriverCtx);
		if(rc)
		{
			pDriverCtx->pDbQuery = pgsql_exec_printf(pDriverCtx->pDbSrvr->dbl
				,( pDriverCtx->pParamTableWhere != NULL && *pDriverCtx->pParamTableWhere ?
					"select %s from %s where %s" : "select %s from %s"
				)
				,pDriverCtx->pParamTableCols
				,pDriverCtx->pParamTableName
				,pDriverCtx->pParamTableWhere
				);
			pDriverCtx->dbQueryRowNum = 0;
		}
		else
			closeFn(pDriverCtx);
		//mlfi_debug(pDriverCtx->pSessionId,"tableDriverPsql:openFn '%s' %d %d\n",pDriverCtx->pParamFname,pDriverCtx->fd,rc);
	}
	else
		mlfi_debug(pDriverCtx->pSessionId,"tableDriverPsql:openFn fail - invalid context\n");

	return rc;
}

int isOpenFn(tableDriverCtx_t *pDriverCtx)
{
	return (pDriverCtx != NULL && pDriverCtx->pDbSrvr != NULL ? pgsql_IsOpen(pDriverCtx->pDbSrvr->dbl) != 0 : 0);
}

int closeFn(tableDriverCtx_t *pDriverCtx)
{	int rc = -1;

	if(pDriverCtx != NULL && pDriverCtx->pDbSrvr != NULL)
	{
		pgsql_freeResult(pDriverCtx->pDbQuery);
		pDriverCtx->pDbSrvr = pgsql_srvr_destroy(pDriverCtx->pDbSrvr,1);
		rc = 0;
	}

	return rc;
}

int listDestroyCallbackFn(void *pData, void *pCallbackData)
{
	if(pData != NULL)
		free(pData);

	return 1; // again
}

int rowGetFn(tableDriverCtx_t *pDriverCtx, list_t **ppRow)
{	int rc = -1;

	if(pDriverCtx != NULL && ppRow != NULL)
	{
		if(*ppRow != NULL)
		{
			listDestroy(*ppRow,&listDestroyCallbackFn,NULL);
			*ppRow = NULL;
		}

		if(isOpenFn(pDriverCtx)
			&& pDriverCtx->pDbQuery != NULL
			&& pDriverCtx->pDbQuery->ok
			&& pgsql_numFields(pDriverCtx->pDbQuery) == pDriverCtx->paramColQty
			&& pDriverCtx->dbQueryRowNum < pgsql_numRows(pDriverCtx->pDbQuery)
			)
		{	size_t i;

			*ppRow = listCreate();

			for(i=0; i<pDriverCtx->paramColQty; i++)
			{	char buf[BUFFERSIZE];
				char *pStr = NULL;

				memset(buf,0,sizeof(buf));
				if(pgsql_getFieldBuf(pDriverCtx->pDbQuery,pDriverCtx->dbQueryRowNum,i,buf,sizeof(buf)-1) && (pStr = strdup(buf)) != NULL)
				{
					listAdd(*ppRow,pStr);
				}
			}
			pDriverCtx->dbQueryRowNum++;
			rc=0;
		}
	}

	return rc;
}

int rewindFn(tableDriverCtx_t *pDriverCtx)
{	int rc = -1;

	if(pDriverCtx != NULL && pDriverCtx->pDbQuery != NULL)
	{
		pDriverCtx->dbQueryRowNum = 0;
		rc =  0;
	}

	return rc;
}

#ifdef UNIT_TEST_DRIVERPGSQL

int test1CallbackCol(void *pData, void *pCallbackData)
{
	printf("%s%s",(const char *)pData,"|");

	return 1;
}

int test1CallbackRow(void *pCallbackCtx, list_t *pRow)
{
	if(pRow != NULL)
	{
		listForEach(pRow,&test1CallbackCol,NULL);
		printf("\n");
	}

	return 1;
}

void test1(const char *fname)
{	tableDriver_t *pTd = tableDriverPsqlCreate();

	if(pTd != NULL)
	{	
		tableOpen(pTd,"table, colqty"
			",tableCols"
			",host" ",hostPort"
			",Device" ",userName" ",userPass"

			,fname,4
			,"domain,mbox,action,exec"
			,"localhost" ,"5432"
			,"spamilter" ,"spamilter" ,""
			);

		if(tableIsOpen(pTd))
		{
			tableForEachRow(pTd,&test1CallbackRow,NULL);
			tableClose(pTd);
		}
		else
			printf("Unable to open '%s'\n",fname);
		tableDriverPsqlDestroy(&pTd);
	}
}

int main(int argc, char **argv)
{	char opt;
	const char *optflags = "f:";
	const char *fname = NULL;

	while((opt = getopt(argc,argv,optflags)) != -1)
	{
		switch(opt)
		{
			case 'f': if(optarg != NULL && *optarg) fname = optarg; break;
			default:
				break;
		}
	}

	argc -= optind;
	argv += optind;

	test1(fname);
}
#endif
