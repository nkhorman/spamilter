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
 *	CVSID:  $Id: dbl.c,v 1.3 2014/09/01 16:09:47 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		dsbl.c
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

#include "dbl.h"
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
	TBL_COL_DBL,
	TBL_COL_QTY
};
typedef struct _dtc_t
{
	char *pCols[TBL_COL_QTY];
	int colIndex;

	const char *pSessionId;
	res_state statp;
	const dblq_t *pDblq;
	const char *pDbl;
}dtc_t; // Dbl Table Context Type

dblCtx_t *dbl_Create(const char *pSessionId)
{	dblCtx_t *pDblCtx = (dblCtx_t *)calloc(1,sizeof(dblCtx_t));

	if(pDblCtx != NULL)
	{
		// setup the table drivers
#ifdef SUPPORT_TABLE_FLATFILE
		pDblCtx->pTableDriver = tableDriverFlatFileCreate(pSessionId);
#endif
#ifdef SUPPORT_TABLE_PGSQL
		pDblCtx->pTableDriver = tableDriverPsqlCreate(pSessionId);
#endif
		pDblCtx->pSessionId = pSessionId;
	}

	return pDblCtx;
}

void dbl_Destroy(dblCtx_t **ppDblCtx)
{
	if(ppDblCtx != NULL && *ppDblCtx != NULL)
	{
#ifdef SUPPORT_TABLE_FLATFILE
		tableDriverFlatFileDestroy(&(*ppDblCtx)->pTableDriver);
#endif
#ifdef SUPPORT_TABLE_PGSQL
		tableDriverPsqlDestroy(&(*ppDblCtx)->pTableDriver);
#endif
		free(*ppDblCtx);
		*ppDblCtx = NULL;
	}
}

int dbl_Open(dblCtx_t *pDblCtx, const char *dbpath)
{
	if(pDblCtx != NULL && dbpath != NULL)
	{
#ifdef SUPPORT_TABLE_FLATFILE
		char	*fn;
#endif

		if(!tableIsOpen(pDblCtx->pTableDriver))
		{
#ifdef SUPPORT_TABLE_FLATFILE
			asprintf(&fn,"%s/db.dbl",dbpath); tableOpen(pDblCtx->pTableDriver,"table, colqty",fn,TBL_COL_QTY);
			if(!tableIsOpen(pDblCtx->pTableDriver))
				mlfi_debug(pDblCtx->pSessionId,"dbl: Unable to open file '%s'\n",fn);
			free(fn);
#endif
#ifdef SUPPORT_TABLE_PGSQL
			tableOpen(pDblCtx->pTableDriver
				"table, colqty"
				",tableCols"
				",host" ",hostPort"
				",Device" ",userName" ",userPass"

				,"dbl",TBL_COL_QTY
				,"dbl"
				,"localhost" ,"5432"
				,"spamilter" ,"spamilter" ,""
				);
			if(!tableIsOpen(pDblCtx->pTableDriver))
				mlfi_debug(pDblCtx->pSessionId,"dbl: Unable to open\n");
#endif
		}
	}

	return (pDblCtx != NULL && tableIsOpen(pDblCtx->pTableDriver));
}

void dbl_Close(dblCtx_t *pDblCtx)
{
	if(pDblCtx != NULL)
	{
		if(tableIsOpen(pDblCtx->pTableDriver))
			tableClose(pDblCtx->pTableDriver);
	}
}

// double your insanity
typedef struct _dbldns_t
{
	dblcb_t dblcb;
	int again;
} dbldns_t;

// The dns response RR iterator calls us, so we
// in turn call the consumers callback abstracting the dns
// iterator's callback function protyping from the consumer
// as well as calling the policy function callback, which
// might seem silly on the surface of it, but by using a policy
// callback, you an maintain the DBL return result processing in
// one place instead of dealing with it in a billion callback
// functions that only care about when the DBL failed the queried
// Domain.
// (That is one long runon sentence...)
static int dbl_check_callback(dqrr_t *pDqrr, void *pData)
{	dbldns_t *pDbldns = (dbldns_t *)pData;

	// convience pointers
	dblcb_t *pDblcb = &pDbldns->dblcb;
	const dblq_t *pDblq = pDblcb->pDblq;

	pDblcb->pDqrr = pDqrr;

	switch(ns_rr_type(pDblcb->pDqrr->rr))
	{
		// valid DBL answer types
		case ns_t_a: // 0x7f0000xx
		case ns_t_aaaa: // ::FFFF:7F00:0
		case ns_t_txt:
			// It's our job to make sure this is empty, and free it afterwards
			pDblcb->pDblResult = NULL;

			if(
				// the consumer knows how to deal wth the ns_rr
				pDblq->pCallbackPolicyFn == NULL
				// let the policy callback deal with ns_rr testing
				|| (pDblq->pCallbackPolicyFn != NULL && pDblq->pCallbackPolicyFn(pDblcb))
			)
			{
				pDbldns->again = pDblq->pCallbackFn(pDblcb);
			}

			// Because the policy callback filled this out, we are supposed to free it
			if(pDblcb->pDblResult != NULL)
			{
				// We're in trouble f this is not heap!
				free((void *)pDblcb->pDblResult);
					pDblcb->pDblResult = NULL;
			}
			break;

		// all other answers
		default:
			break;
	}

	return 1; // again
}

// Our consumer, gives us a context to pass to the callback for every RR in the dns response.
// So, since we ourselves, use an iterator for the RR's, we have to stuff the consumer foo, in
// a context struct, to pass through the iterator to use when calling the consumer's callback.
// Double your pleasure, double your fun... :)
int dbl_check(const res_state statp, const char *pDbl, const dblq_t *pDblq)
{	dqrr_t *pDqrr = dns_query_rr_init(statp, ns_t_a);
	int rc = dns_query_rr_resp_printf(pDqrr, "%s.%s", pDblq->pDomain, pDbl);
	int again = 1;

	if(rc > 0)
	{
		dbldns_t dbldns;

		memset(&dbldns, 0, sizeof(dbldns));
		dbldns.dblcb.pDblq = pDblq;
		dbldns.dblcb.pDbl = pDbl;
		dbldns.again = 1;

		dns_parse_response_answer(pDqrr, &dbl_check_callback, &dbldns);

		again = dbldns.again;
	}

	dns_query_rr_free(pDqrr);

	return again;
}

// collect the column pointers
static int dblCallbackCol(void *pData, void *pCallbackData)
{	dtc_t *pDtc = (dtc_t *)pCallbackData;
	int again = 1; // iterate for more columns

	if(pDtc->colIndex < TBL_COL_QTY)
		pDtc->pCols[pDtc->colIndex++] = (char *)pData;
	else
		again = 0; // reached max column count, stop

	return again;
}

// Iterate a list of DBLs using a callback
static int dblCallbackRow(void *pCallbackCtx, list_t *pRow)
{	int again = 1;

	if(pRow != NULL)
	{	dtc_t *pDtc = (dtc_t *)pCallbackCtx;

		pDtc->colIndex = 0;
		listForEach(pRow, &dblCallbackCol, pDtc);

		if(pDtc->colIndex >= TBL_COL_QTY-1)
			again = dbl_check(pDtc->statp, pDtc->pCols[TBL_COL_DBL], pDtc->pDblq);
	}

	return again;
}

// check the list of DBLs
void dbl_check_all(dblCtx_t *pCtx, const res_state statp, const dblq_t *pDblq)
{	dtc_t dtc;

	dtc.pSessionId = pCtx->pSessionId;
	dtc.statp = statp;
	dtc.pDblq = pDblq;
	dtc.pDbl = NULL;

	tableForEachRow(pCtx->pTableDriver, &dblCallbackRow, &dtc);
}

int dbl_callback_policy_std(dblcb_t *pDblcb)
{	int bPolicyMatch = 0;
	dqrr_t *pDqrr = pDblcb->pDqrr;
	int nsType = ns_rr_type(pDqrr->rr);
	int afType = AF_UNSPEC;
	int respVal = 0;

	// http://www.spamhaus.org/faq/section/Spamhaus%20DBL#277
	// http://www.rfc-editor.org/rfc/rfc5782.txt
	//
	// http://www.spamhaus.org/faq/section/Spamhaus%20DBL#291
	// Return Codes	Data Source
	// 127.0.0.2	spam domain
	// 127.0.0.4	phish domain
	// 127.0.0.5	malware domain
	// 127.0.0.6	botnet C&C domain
	// http://www.spamhaus.org/faq/section/Spamhaus%20DBL#411
	// 127.0.0.102	abused legit spam
	// 127.0.0.103	abused spammed redirector domain
	// 127.0.0.104	abused legit phish
	// 127.0.0.105	abused legit malware
	// 127.0.0.106	abused legit botnet C&C
	// 127.0.0.255	IP queries prohibited!

	switch(nsType)
	{
		case ns_t_a: // 0x7f0000xx
			if(ns_rr_rdlen(pDqrr->rr) == NS_INADDRSZ)
			{
				// since ns_get32() advances the pointer that we want
				// to use again below for mlfi_inet_ntopAF, we
				// create a temp copy the rrdata pointer, and use it
				const unsigned char *pRrData = ns_rr_rdata(pDqrr->rr);
				unsigned long ip = ns_get32(pRrData);

				if((ip&0xffffff00) == 0x7f000000)
				{
					respVal = (ip & 0x000000ff);
					afType = AF_INET;
				}
			}
			break;

		case ns_t_aaaa: // ::FFFF:7F00:0
			if(ns_rr_rdlen(pDqrr->rr) == NS_IN6ADDRSZ)
			{
				struct in6_addr *pIp6 = (struct in6_addr *)ns_rr_rdata(pDqrr->rr);

				// 0x0000_0000 0000_0000 0000_ffff 7f00_0002
				if(
					pIp6->s6_addr32[0] == 0
					&& pIp6->s6_addr32[1] == 0
					&& pIp6->s6_addr32[2] == 0x0000ffff
					&& (pIp6->s6_addr32[3] & 0xffffff00) == 0x7f000000
					)
				{
					respVal = (pIp6->s6_addr32[3] & 0x000000ff);
					afType = AF_INET6;
				}
			}
			break;
	}

	if(afType != AF_UNSPEC)
	{
		pDblcb->abused = (respVal >= 100); // an abused domain
		if(respVal > 0 && respVal < 100)
		{
			pDblcb->pDblResult = mlfi_inet_ntopAF(AF_INET, (char *)ns_rr_rdata(pDqrr->rr));
			bPolicyMatch = 1;
		}
	}

	return bPolicyMatch;
}

#ifdef _UNIT_TEST
#include <syslog.h>

static int callback(const dblcb_t *pDblcb)
{
	printf("DBL '%s' query '%s' returned '%s'%s\n"
		, pDblcb->pDbl
		, pDblcb->pDblq->pDomain
		, pDblcb->pDblResult
		, (pDblcb->abused ? " - abused" : "")
		);

	return 1; // again
}

int main(int argc, char **argv)
{	char *pDbl = NULL;//"dbl.spamhaus.org";
	char *pDomain = NULL;
	int c;

	while ((c = getopt(argc, argv, "h:l:")) != -1)
	{
		switch (c)
		{
			case 'l':
				if(optarg != NULL && *optarg)
					pDbl = optarg;
			case 'h':
				if(optarg != NULL && *optarg)
					pDomain = optarg;
				break;
		}
	}
	argc -= optind;
	argv += optind;


	openlog("dbl", LOG_PERROR|LOG_NDELAY|LOG_PID, LOG_DAEMON);
	if(pDomain != NULL)
	{
		if(pDbl != NULL)
			printf("using DBL '%s' for lookup of '%s'\n",pDbl,pDomain);

		res_state presstate = NULL;

		presstate = RES_NALLOC(presstate);

		if(presstate != NULL)
		{	dblq_t dblq;

			res_ninit(presstate);

			dblq.pDomain = pDomain;
			dblq.pCallbackFn = &callback;
			dblq.pCallbackData = NULL;
			dblq.pCallbackPolicyFn = &dbl_callback_policy_std;

			if(pDbl != NULL)
				dbl_check(presstate, pDbl, &dblq);
			else
			{	dblCtx_t *pDblCtx = dbl_Create("");

				dbl_Open(pDblCtx, "/var/db/spamilter");

				dbl_check_all(pDblCtx, presstate, &dblq);

				dbl_Close(pDblCtx);
				dbl_Destroy(&pDblCtx);
			}

			res_nclose(presstate);
			free(presstate);
		}
		else
			printf("enable to init resolver\n");
	}
	else
		printf("no domain specified - use -h [host/domain name]\n");

	return 0;
}
#endif
