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
 *	CVSID:  $Id: tableDriverFlatFile.c,v 1.3 2012/11/18 21:13:16 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		tableDriverFlatFile
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: tableDriverFlatFile.c,v 1.3 2012/11/18 21:13:16 neal Exp $";

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

// private structure
typedef struct _tableDriverCtx_t
{
	int fd;
	char *pBuffer;

	const char *pParamFname;
	size_t paramColQty;
	char paramDelim;
	const char *pSessionId;
}tableDriverCtx_t;
#define TABLEDRIVERCTX_T

#include "tableDriverFlatFile.h"

// private prototypes
static int paramSetFn(tableDriverCtx_t *pDriverCtx, const char *pKey, va_list *vl);
static int openFn(tableDriverCtx_t *pDriverCtx);
static int closeFn(tableDriverCtx_t *pDriverCtx);
static int rowGetFn(tableDriverCtx_t *pDriverCtx, list_t **ppRow);
static int rewindFn(tableDriverCtx_t *pDriverCtx);
static int isOpenFn(tableDriverCtx_t *pDriverCtx);

#define BUFFERSIZE 8192

#include "misc.h"

tableDriver_t *tableDriverFlatFileCreate(const char *pSessionId)
{	tableDriver_t *pDriver = (tableDriver_t *)calloc(1,sizeof(tableDriver_t));

	if(pDriver != NULL)
	{
		//mlfi_debug(pSessionId, "tableDriverFlatFileCreate: 1\n");
		pDriver->paramSetFn = &paramSetFn;
		pDriver->openFn = &openFn;
		pDriver->closeFn = &closeFn;
		pDriver->rowGetFn = &rowGetFn;
		pDriver->rewindFn = &rewindFn;
		pDriver->isOpenFn = &isOpenFn;
		pDriver->pDriverCtx = (tableDriverCtx_t *)calloc(1,sizeof(tableDriverCtx_t));
		if(pDriver->pDriverCtx != NULL)
		{
			//mlfi_debug(pSessionId, "tableDriverFlatFileCreate: 2\n");
			pDriver->pDriverCtx->fd = -1;
			pDriver->pDriverCtx->pBuffer = calloc(1,BUFFERSIZE);
			pDriver->pDriverCtx->paramDelim = '|';
			pDriver->pDriverCtx->pSessionId = pSessionId;
		}
	}
	else
		mlfi_debug(pSessionId, "tableDriverFlatFileCreate: fail - invalid driver\n");

	return pDriver;
}

int tableDriverFlatFileDestroy(tableDriver_t **ppDriver)
{	int rc = -1;

	if(ppDriver != NULL)
	{	tableDriver_t *pDriver = *ppDriver;

		if(pDriver != NULL)
		{
			if(pDriver->pDriverCtx != NULL)
			{
				closeFn(pDriver->pDriverCtx);
				if(pDriver->pDriverCtx->pBuffer != NULL)
					free(pDriver->pDriverCtx->pBuffer);
				free(pDriver->pDriverCtx);
			}
			free(pDriver);
			*ppDriver = NULL;
			rc = 0;
		}
	}

	return rc;
}

int paramSetFn(tableDriverCtx_t *pDriverCtx, const char *pKey, va_list *pvl)
{	int rc = -1;

	if(pDriverCtx != NULL && pKey != NULL && *pKey)
	{
		if(strcasecmp(pKey,"table") == 0) { pDriverCtx->pParamFname = (const char *)va_arg(*pvl,const char *); rc = 0; }
		if(strcasecmp(pKey,"colqty") == 0) { pDriverCtx->paramColQty = (size_t)va_arg(*pvl,size_t); rc = 0; }
		if(strcasecmp(pKey,"delim") == 0) { pDriverCtx->paramDelim = (char)va_arg(*pvl,int); rc = 0; }
	}

	return rc;
}

int openFn(tableDriverCtx_t *pDriverCtx)
{	int rc = -1;

	if(pDriverCtx != NULL)
	{
		pDriverCtx->fd = open(pDriverCtx->pParamFname,O_RDONLY);
		rc = (pDriverCtx->fd == -1 ? -1 : 0);
		//mlfi_debug(pDriverCtx->pSessionId, "tableDriverFlatFile:openFn '%s' %d %d\n",pDriverCtx->pParamFname,pDriverCtx->fd,rc);
	}
	else
		mlfi_debug(pDriverCtx->pSessionId, "tableDriverFlatFile:openFn fail - invalid context\n");

	return rc;
}

int isOpenFn(tableDriverCtx_t *pDriverCtx)
{
	return (pDriverCtx != NULL ? pDriverCtx->fd != -1 : 0);
}

int closeFn(tableDriverCtx_t *pDriverCtx)
{	int rc = -1;

	if(pDriverCtx != NULL && pDriverCtx->fd != -1)
	{
		close(pDriverCtx->fd);
		pDriverCtx->fd = -1;
		rc = 0;
	}

	return rc;
}

int rowGetFn(tableDriverCtx_t *pDriverCtx, list_t **ppRow)
{	int rc = -1;

	if(pDriverCtx != NULL && ppRow != NULL)
	{	char *buf = pDriverCtx->pBuffer;

		if(mlfi_fdgets(pDriverCtx->fd,buf,BUFFERSIZE) >= 0)
		{	char * str = strchr(buf,'#');

			if(str != NULL)
				*(str--) = '\0';
			else
				str = buf+strlen(buf)-1;
			while(str >= buf && (*str ==' ' || *str == '\t' || *str == '\r' || *str == '\n'))
				*(str--) = '\0';

			str = buf;
			if(*ppRow != NULL)
			{
				listDestroy(*ppRow,NULL,NULL);
				*ppRow = NULL;
			}

			if(*str)
			{	char *pcol1;
				char *pcol2;

				*ppRow = listCreate();

				while(*str && listQty(*ppRow) < pDriverCtx->paramColQty)
				{
					pcol1 = mlfi_stradvtok(&str,pDriverCtx->paramDelim);
					pcol2 = pcol1+strlen(pcol1)-1;
					while(*pcol2 != pDriverCtx->paramDelim && (*pcol2 == ' ' || *pcol2 == '\t'))
						*(pcol2--) = '\0';
					listAdd(*ppRow,pcol1);
				}
			}
			rc=0;
		}
	}

	return rc;
}

int rewindFn(tableDriverCtx_t *pDriverCtx)
{	int rc = -1;

	if(pDriverCtx != NULL)
	{
		lseek(pDriverCtx->fd,0l,SEEK_SET);
		rc =  0;
	}

	return rc;
}

//#define UNIT_TEST
#ifdef UNIT_TEST_DRIVERFLATFILE
#include <pthread.h>

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

void test1(const char *fname, size_t cols, char delim)
{	char sessionStr[10];
	tableDriver_t *pTd;
	
	sprintf(sessionStr,"%04X",(unsigned int)pthread_self());
	pTd = tableDriverFlatFileCreate(sessionStr);

	if(pTd != NULL)
	{	
		tableOpen(pTd,"table, colqty, delim",fname,cols,delim);

		if(tableIsOpen(pTd))
		{
			tableForEachRow(pTd,&test1CallbackRow,NULL);
			tableClose(pTd);
		}
		else
			printf("Unable to open '%s'\n",fname);
		tableDriverFlatFileDestroy(&pTd);
	}
}

int main(int argc, char **argv)
{	char opt;
	const char *optflags = "f:c:d:";
	const char *fname = NULL;
	size_t cols = 0;
	char delim = '|';

	while((opt = getopt(argc,argv,optflags)) != -1)
	{
		switch(opt)
		{
			case 'f':
				if(optarg != NULL && *optarg)
					fname = optarg;
				break;
			case 'c':
				if(optarg != NULL && *optarg)
					cols = (short)atoi(optarg);
				break;
			case 'd':
				if(optarg != NULL && *optarg)
					delim = *optarg;
				break;
			default:
				break;
		}
	}

	argc -= optind;
	argv += optind;

	test1(fname,cols,delim);
}
#endif
