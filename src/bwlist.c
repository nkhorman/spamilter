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
 *	CVSID:  $Id: bwlist.c,v 1.29 2014/02/28 05:37:57 neal Exp $
 *
 * DESCRIPTION:
 *	application:	Spamilter
 *	module:		bwlist.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: bwlist.c,v 1.29 2014/02/28 05:37:57 neal Exp $";

#ifndef UNIT_TEST
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>

#define BWLIST_API
#include "spamilter.h"
#include "misc.h"
#ifdef SUPPORT_TABLE_FLATFILE
#include "tableDriverFlatFile.h"
#endif
#ifdef SUPPORT_TABLE_PGSQL
#include "tableDriverPsql.h"
#endif

#include "regexapi.h"

#ifdef UNIT_TEST
#include <syslog.h>
#endif
/*
#ifdef UNIT_TEST
//#define mlfi_debug printf
void mlfi_debug(const char *pSessionId, const char *pfmt, ...)
{
	if(pfmt != NULL)
	{	va_list	vl;

		va_start(vl,pfmt);
		vprintf(pfmt,vl);
		va_end(vl);
	}
}
#endif
*/

char *gpBwlStrs[] =
{
	"None",
	"Accept",
	"Reject",
	"Discard",
	"TempFail",
	"TarPit",
	"Exec",
	"IPFW"
};

enum
{
	BWLIST_COL_DOM,
	BWLIST_COL_MBOX,
	BWLIST_COL_ACTION,
	BWLIST_COL_EXEC,
	BWLIST_COL_QTY
};
typedef struct _bga_t
{
	char *pCols[BWLIST_COL_QTY];
	int colIndex;
	int action;
	const char *mbox;
	const char *dom;
	char *exec;
	const char *pSessionId;
}bga_t;

// manage the bwlist context
bwlistCtx_t *bwlistCreate(const char *pSessionId)
{	bwlistCtx_t *pBwlistCtx = (bwlistCtx_t *)calloc(1,sizeof(bwlistCtx_t));

	if(pBwlistCtx != NULL)
	{
		// setup the table drivers
#ifdef SUPPORT_TABLE_FLATFILE
		pBwlistCtx->pTdSndr = tableDriverFlatFileCreate(pSessionId);
#ifdef SUPPORT_AUTO_WHITELIST
		pBwlistCtx->pTdSndrAuto = tableDriverFlatFileCreate(pSessionId);
#endif
		pBwlistCtx->pTdRcpt = tableDriverFlatFileCreate(pSessionId);
#endif
#ifdef SUPPORT_TABLE_PGSQL
		pBwlistCtx->pTdSndr = tableDriverPsqlCreate(pSessionId);
#ifdef SUPPORT_AUTO_WHITELIST
		pBwlistCtx->pTdSndrAuto = NULL;
#endif
		pBwlistCtx->pTdRcpt = tableDriverPsqlCreate(pSessionId);
#endif
		pBwlistCtx->pSessionId = pSessionId;
	}

	return pBwlistCtx;
}

void bwlistDestroy(bwlistCtx_t **ppBwlistCtx)
{
	if(ppBwlistCtx != NULL && *ppBwlistCtx != NULL)
	{
#ifdef SUPPORT_TABLE_FLATFILE
		tableDriverFlatFileDestroy(&(*ppBwlistCtx)->pTdSndr);
#ifdef SUPPORT_AUTO_WHITELIST
		tableDriverFlatFileDestroy(&(*ppBwlistCtx)->pTdSndrAuto);
#endif
		tableDriverFlatFileDestroy(&(*ppBwlistCtx)->pTdRcpt);
#endif
#ifdef SUPPORT_TABLE_PGSQL
		tableDriverPsqlDestroy(&(*ppBwlistCtx)->pTdSndr);
		tableDriverPsqlDestroy(&(*ppBwlistCtx)->pTdRcpt);
#endif
		free(*ppBwlistCtx);
		*ppBwlistCtx = NULL;
	}
}

// open the data sources
int bwlistOpen(bwlistCtx_t *pBwlistCtx, const char *dbpath)
{
	if(pBwlistCtx != NULL && dbpath != NULL)
	{
#ifdef SUPPORT_TABLE_FLATFILE
		char	*fn;
#endif

		if(!tableIsOpen(pBwlistCtx->pTdSndr))
		{
#ifdef SUPPORT_TABLE_FLATFILE
			asprintf(&fn,"%s/db.sndr",dbpath); tableOpen(pBwlistCtx->pTdSndr,"table, colqty",fn,BWLIST_COL_QTY);
			if(!tableIsOpen(pBwlistCtx->pTdSndr))
				mlfi_debug(pBwlistCtx->pSessionId,"bwlist: Unable to open Sender file '%s'\n",fn);
			free(fn);
#endif
#ifdef SUPPORT_TABLE_PGSQL
			tableOpen(pBwlistCtx->pTdSndr,
				"table, colqty"
				",tableCols"
				",host" ",hostPort"
				",Device" ",userName" ",userPass"

				,"bwlist_sndr",4
				,"domain,mbox,action,exec"
				,"localhost" ,"5432"
				,"spamilter" ,"spamilter" ,""
				);

			if(!tableIsOpen(pBwlistCtx->pTdSndr))
				mlfi_debug(pBwlistCtx->pSessionId,"bwlist: Unable to open Senders\n");
#endif
		}

#ifdef SUPPORT_AUTO_WHITELIST
		if(!tableIsOpen(pBwlistCtx->pTdSndrAuto))
		{
#ifdef SUPPORT_TABLE_FLATFILE
			asprintf(&fn,"%s/db.sent",dbpath); tableOpen(pBwlistCtx->pTdSndrAuto,"table, colqty",fn,BWLIST_COL_QTY);
			if(!tableIsOpen(pBwlistCtx->pTdSndrAuto))
				mlfi_debug(pBwlistCtx->pSessionId,"bwlist: Unable to open Recipient file '%s'\n",fn);
			free(fn);
#endif
		}
#endif

		if(!tableIsOpen(pBwlistCtx->pTdRcpt))
		{
#ifdef SUPPORT_TABLE_FLATFILE
			asprintf(&fn,"%s/db.rcpt",dbpath); tableOpen(pBwlistCtx->pTdRcpt,"table, colqty",fn,BWLIST_COL_QTY);
			if(!tableIsOpen(pBwlistCtx->pTdRcpt))
				mlfi_debug(pBwlistCtx->pSessionId,"bwlist: Unable to open Recipient file '%s'\n",fn);
			free(fn);
#endif
#ifdef SUPPORT_TABLE_PGSQL
			tableOpen(pBwlistCtx->pTdRcpt,
				"table, colqty"
				",tableCols"
				",host" ",hostPort"
				",Device" ",userName" ",userPass"

				,"bwlist_rcpt",4
				,"domain,mbox,action,exec"
				,"localhost" ,"5432"
				,"spamilter" ,"spamilter" ,""
				);
			if(!tableIsOpen(pBwlistCtx->pTdRcpt))
				mlfi_debug(pBwlistCtx->pSessionId,"bwlist: Unable to open Recipients\n");
#endif
		}
	}

	return (pBwlistCtx != NULL && tableIsOpen(pBwlistCtx->pTdSndr) && tableIsOpen(pBwlistCtx->pTdRcpt));
}

// close the data sources
void bwlistClose(bwlistCtx_t *pBwlistCtx)
{
	if(pBwlistCtx != NULL)
	{
		if(tableIsOpen(pBwlistCtx->pTdSndr))
			tableClose(pBwlistCtx->pTdSndr);
#ifdef SUPPORT_AUTO_WHITELIST
		if(tableIsOpen(pBwlistCtx->pTdSndrAuto))
			tableClose(pBwlistCtx->pTdSndrAuto);
#endif
		if(tableIsOpen(pBwlistCtx->pTdRcpt))
			tableClose(pBwlistCtx->pTdRcpt);
	}
}

// collect the column pointers
static int bwlistCallbackCol(void *pData, void *pCallbackData)
{	bga_t *pBga = (bga_t *)pCallbackData;
	int again = 1; // iterate for more columns

	if(pBga->colIndex < BWLIST_COL_QTY)
		pBga->pCols[pBga->colIndex++] = (char *)pData;
	else
		again = 0; // reached max column count, stop

	return again;
}

// analyze the columns
static int bwlistCallbackRow(void *pCallbackCtx, list_t *pRow)
{
	if(pRow != NULL)
	{	bga_t *pbga = (bga_t *)pCallbackCtx;

		pbga->colIndex = 0;
		memset(&pbga->pCols, 0, sizeof(pbga->pCols));
		listForEach(pRow,&bwlistCallbackCol,pbga);

		if(pbga->colIndex >= BWLIST_COL_QTY-1)
		{	char *aaction = pbga->pCols[BWLIST_COL_ACTION];
			char *adom = pbga->pCols[BWLIST_COL_DOM];
			char *ambox = pbga->pCols[BWLIST_COL_MBOX];
			char *aexec = pbga->pCols[BWLIST_COL_EXEC];
			int action = (
				aaction == NULL				? BWL_A_NULL :
				strcasecmp(aaction,"accept") == 0	? BWL_A_ACCEPT :
				strcasecmp(aaction,"reject") == 0	? BWL_A_REJECT :
				strcasecmp(aaction,"discard") == 0	? BWL_A_DISCARD :
				strcasecmp(aaction,"fail") == 0		? BWL_A_TEMPFAIL :
				strcasecmp(aaction,"tarpit") == 0	? BWL_A_TARPIT :
				strcasecmp(aaction,"exec") == 0		? BWL_A_EXEC :
				strcasecmp(aaction,"ipfw") == 0		? BWL_A_IPFW :
				BWL_A_NULL
				);

			if(
				// match adom == dom
				strcasecmp(adom,pbga->dom) == 0
				// assuming amdom = '.bar.com', match for '...foo.bar.com' and 'bar.com'
				|| (adom[0] == '.' && (strcasecmp(adom,pbga->dom+(strlen(pbga->dom)-strlen(adom))) == 0 || strcasecmp(adom+1,pbga->dom) == 0))
				// match any dom
				|| strcmp(adom,".") == 0
				)
			{	char *pregpat = NULL;

				asprintf(&pregpat, "^%s$", ambox);
				//printf("mbox %d regex %d\n", strcasecmp(ambox,pbga->mbox) == 0, (pregpat != NULL ? regexapi(pbga->mbox, pregpat, REGEX_DEFAULT_CFLAGS) : 0) );
				if(strcasecmp(ambox,pbga->mbox) == 0 || (pregpat != NULL ? regexapi(pbga->mbox, pregpat, REGEX_DEFAULT_CFLAGS) : 0) )
				{
					pbga->action = action;
					if(pbga->action == BWL_A_EXEC && strlen(aexec) && pbga->exec != NULL)
						strcpy(pbga->exec,pbga->pCols[BWLIST_COL_EXEC]);
					mlfi_debug(((bga_t *)pCallbackCtx)->pSessionId,"bwlistActionGet match: '%s@%s' - action = '%s'\n",ambox,adom,aaction);
				}
				else if(strlen(ambox) == 0)
				{
					pbga->action = action;
					if(pbga->action == BWL_A_EXEC && strlen(aexec) && pbga->exec != NULL)
						strcpy(pbga->exec,aexec);
					mlfi_debug(((bga_t *)pCallbackCtx)->pSessionId,"bwlistActionGet match: '%s' - action = '%s'\n",adom,aaction);
				}
				if(pregpat != NULL)
					free(pregpat);
			}
		}
	}

	return 1; // again (iterate all rows)
}

// find the last row in the specified table that matches mbox@dom
static int bwlistActionGet(const char *pSessionId, tableDriver_t *pDriver, const char *dom, const char *mbox, char *exec)
{	int	rc = BWL_A_NULL;

	if(pDriver != NULL)
	{	bga_t bga;
	
		if(exec != NULL)
			*exec = '\0';

		memset(&bga,0,sizeof(bga));
		bga.mbox = mbox;
		bga.dom = dom;
		bga.exec = exec;
		bga.action = BWL_A_NULL;
		bga.pSessionId = pSessionId;
		tableForEachRow(pDriver,&bwlistCallbackRow,&bga);

		// if there are enough columns in the row, and an action has been parsed
		// out based on a match for "dom", then try to match the mbox
		if(bga.colIndex >= BWLIST_COL_MBOX && bga.action != BWL_A_NULL)
			rc = bga.action;

		if(rc != BWL_A_EXEC && exec != NULL)
			*exec = '\0';
	}

	mlfi_debug(pSessionId,"bwlistActionGet: %s@%s - action %s\n",mbox,dom,gpBwlStrs[rc]);

	return rc;
}

// use the specified data set, and attempt to find a matching row for mbox@domain
int bwlistActionQuery(bwlistCtx_t *pBwlistCtx, int list, const char *dom, const char *mbox, char *exec)
{	int	rc = BWL_A_NULL;

	if(pBwlistCtx != NULL && dom != NULL && mbox != NULL)
	{
		switch(list)
		{
			case BWL_L_SNDR:
				rc = bwlistActionGet(pBwlistCtx->pSessionId,pBwlistCtx->pTdSndr,dom,mbox,exec);
#ifdef SUPPORT_AUTO_WHITELIST
				if(rc == BWL_A_NULL)
					rc = bwlistActionGet(pBwlistCtx->pSessionId, pBwlistCtx->pTdSndrAuto, dom, mbox, exec);
#endif
				break;
			case BWL_L_RCPT:
				rc = bwlistActionGet(pBwlistCtx->pSessionId,pBwlistCtx->pTdRcpt,dom,mbox,exec);
				break;
		}
	}

	return rc;
}


#ifdef UNIT_TEST

#include <pthread.h>

void test1(const char *pPath, int list, const char *dom, const char *mbox)
{
	if(pPath != NULL && dom != NULL && mbox != NULL)
	{	char sessionStr[10];
		bwlistCtx_t *pbwl;

		sprintf(sessionStr,"%04X",(unsigned int)pthread_self());
		pbwl = bwlistCreate(sessionStr);

		if(pbwl != NULL)
		{
			if(bwlistOpen(pbwl,pPath))
			{	char exec[1024];
				int rc = bwlistActionQuery(pbwl, list, dom, mbox, (char *)&exec);

				printf("action query rc %d/'%s'\n", rc, gpBwlStrs[rc]);
				printf("\texec '%s'\n", exec);
				bwlistClose(pbwl);
			}
			else
				printf("Unable to open database.\n");
			bwlistDestroy(&pbwl);
		}
	}
	else
	{
		if(pPath == NULL)
			printf("Database path not specified\n");
		if(dom == NULL)
			printf("Domain not psecified\n");
		if(mbox == NULL)
			printf("Mbox not specified\n");
	}
}

void usage(void)
{
	printf("-p [/var/db/spamilter] -d [domain] -m [mbox] -l [sndr | rcpt]");
	exit(0);
}

int main(int argc, char **argv)
{	char opt;
	const char *optflags = "p:l:d:m:";
	const char *path = "/var/db/spamilter";//NULL;
	int list = 0;
	const char *dom = NULL;
	const char *mbox = NULL;

	if(argc < 3)
		usage();

	while((opt = getopt(argc,argv,optflags)) != -1)
	{
		switch(opt)
		{
			case 'p': if(optarg != NULL && *optarg) path = optarg; break;
			case 'd': if(optarg != NULL && *optarg) dom = optarg; break;
			case 'm': if(optarg != NULL && *optarg) mbox = optarg; break;
			case 'l':
				if(optarg != NULL && *optarg)
				{
					if(strcasecmp("sndr",optarg) == 0)
						list = BWL_L_SNDR;
					else if(strcasecmp("rcpt",optarg) == 0)
						list = BWL_L_RCPT;
				}
				break;
			default: break;
		}
	}

	argc -= optind;
	argv += optind;

	openlog("bwlist", LOG_PERROR|LOG_NDELAY|LOG_PID, LOG_DAEMON);
	test1(path,list,dom,mbox);

	return 0;
}
#endif
