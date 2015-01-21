/*--------------------------------------------------------------------*
 *
 * Developed by;
 *	Ramsus Lerdorf - http://www.php.net
 *	Copyright (C) 1997 by Ramsus Lerdorf. All Rights Reserved
 *
 *	This code was taken from PHP/FI 2.0B13 and the modified heavily.
 *
 * Modified by;
 *	Neal Horman - http://www.wanlink.com
 *	Portions Copyright (c) 1997-2003 by Neal Horman. All Rights Reserved
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
 *	CVSID:  $Id: pgsql.c,v 1.10 2012/06/25 01:44:21 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		pgsql.c
 *
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: pgsql.c,v 1.10 2012/06/25 01:44:21 neal Exp $";

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <regex.h>
#include <libpq-fe.h>
#include "pgsql.h"

ppgsql_srvr pgsql_srvr_init(char const *host, char const *port, char const *device, char const *user, char const *pass)
{	ppgsql_srvr	srvr = calloc(1,sizeof(pgsql_srvr));

	if(srvr != NULL)
	{
		srvr->host	= host;
		srvr->port	= port;
		srvr->device	= device;
		srvr->user	= user;
		srvr->pass	= pass;

		if(srvr->dbl == NULL)
			srvr->dbl= pgsql_dbl_init(srvr->dbl);

		//pgsql_open(srvr);
	}

	return(srvr);
}

ppgsql_srvr pgsql_srvr_destroy(ppgsql_srvr srvr, int freeIt)
{
	if(srvr != NULL)
	{
		pgsql_dbl_destroy(srvr->dbl,freeIt);

		if(freeIt)
			free(srvr);
	}

	return(freeIt ? NULL : srvr);
}

ppgsql_link pgsql_dbl_init(ppgsql_link dbl)
{
	if(dbl == NULL)
		dbl = (ppgsql_link)calloc(1,sizeof(pgsql_link));

	if(dbl != NULL && !dbl->init)
	{
		dbl->init	= 0x4321;
		dbl->open	= 0;
		dbl->conn	= NULL;
		dbl->ok		= 1;
		dbl->debug	= 0;
	}

	return(dbl);
}

ppgsql_link pgsql_dbl_destroy(ppgsql_link dbl, int freeIt)
{
	if(dbl != NULL)
	{
		if(dbl->debug>1)
			printf("pgsql_destroy: enter\n");

		pgsql_close(dbl);

		if(freeIt)
			free(dbl);

		if(dbl->debug>1)
			printf("pgsql_destroy: exit - ok: %u\n",dbl->ok);
	}

	return(freeIt ? NULL : dbl);
}

int pgsql_IsInit(ppgsql_link dbl)
{
	return((dbl != NULL && dbl->init == 0x4321) ? 1 : 0);
}

int pgsql_IsOpen(ppgsql_link dbl)
{
	return((pgsql_IsInit(dbl) && dbl->conn != NULL && dbl->open) ? 1 : 0);
}

int pgsql_open(ppgsql_srvr srv)
{	ppgsql_link	dbl = srv->dbl;
	char		*buf = NULL;

	if(dbl->debug>1)
		printf("pgsql_open: enter\n");
	if(pgsql_IsInit(dbl) && !dbl->open)
	{
		asprintf(&buf,"host='%s' port='%s' dbname='%s' user='%s' password='%s'",
			srv->host, srv->port, srv->device, srv->user,srv->pass);
		dbl->conn = PQconnectdb(buf);

		dbl->open = dbl->ok = (dbl->conn != NULL && (PQstatus(dbl->conn) == CONNECTION_OK));
		if(!dbl->ok)
		{
			if(dbl->debug)
				printf("pgsql_connect: Could not connect to database (%s)\n",
					PQerrorMessage(dbl->conn));
			pgsql_close(dbl);
			dbl->ok = 0;	/* mask the PGfinish result */
		}
	}
	else
		dbl->ok = 1;	/* already open! */

	if(dbl->debug>1)
		printf("pgsql_open: exit - ok: %u\n",dbl->ok);

	return(dbl->ok);
}

int pgsql_close(ppgsql_link dbl)
{
	if(dbl->debug>1)
		printf("pgsql_close: enter\n");

	dbl->ok = 0;
	if(pgsql_IsOpen(dbl))
	{
		PQfinish(dbl->conn);
		dbl->ok = 1;
	}
	dbl->conn = NULL;
	dbl->open = 0;

	if(dbl->debug>1)
		printf("pgsql_close: exit - ok: %u\n",dbl->ok);

	return(dbl->ok);
}

int pgsql_verifyConn(ppgsql_link dbl)
{
	if(pgsql_IsOpen(dbl))
	{
		dbl->ok = (PQstatus(dbl->conn) == CONNECTION_OK);
		if(!dbl->ok)
		{
			/* try to reset the connection */
			PQreset(dbl->conn);

			dbl->ok = (dbl->ok && PQstatus(dbl->conn) == CONNECTION_OK);
			if(!dbl->ok)
			{
				if(dbl->debug) 
					printf("pgsql_verifyConn: Pgsql database connection error (%s)\n",
						PQerrorMessage(dbl->conn));
				pgsql_close(dbl);
				dbl->ok = 0;	/* mask the PGfinish result */
			}
		}
	}

	return(dbl->ok);
}

ppgsql_query pgsql_exec_printf(ppgsql_link dbl, char *fmt, ...)
{	va_list	vl;
	char	*str;
	ppgsql_query pq = NULL;
	int	rc = -1;
	
	va_start(vl,fmt);
	if(dbl != NULL && fmt != NULL && (rc = vasprintf(&str,fmt,vl)) && str != NULL && rc != -1)
	{
		pq = pgsql_exec(dbl,str);
		free(str);
	}

	va_end(vl);

	return(pq);
}

ppgsql_query pgsql_exec(ppgsql_link dbl, char *query)
{	char		*str;
	ppgsql_query	dblq = (ppgsql_query)calloc(1,sizeof(pgsql_query));

	dblq->query	= strdup(query);
	dblq->dbl	= dbl;
	dblq->lastOid	= -1;

	if(dbl->debug>1)
		printf("pgsql_exec: enter\n");

	if(pgsql_IsOpen(dbl) && pgsql_verifyConn(dbl))
	{
		pgsql_StripDollarSlashes(query);  /* Postgres doesn't recognize \$ */
		if(dbl->debug>1)
			printf("pgsql_exec: Sending query: %s\n",dblq->query);

		dblq->result = PQexec(dblq->dbl->conn, dblq->query);
		if(dbl->debug>1)
			printf("pgsql_exec: dblq->result %s NULL\n", dblq->result == NULL ? "==" : "!=");
		if (dblq->result == NULL)
			dblq->stat = (ExecStatusType) PQstatus(dblq->dbl->conn);
		else
			dblq->stat = PQresultStatus(dblq->result);

		switch(dblq->stat)
		{
			default:
				if(dblq->dbl->debug)
					printf("pgsql_exec: Unknown result status %d\n",dblq->stat);
			case PGRES_EMPTY_QUERY:
			case PGRES_BAD_RESPONSE:
			case PGRES_NONFATAL_ERROR:
				if(dblq->dbl->debug)
					printf("pgsql_exec: status: '%s'\n\t message: '%s'\n",
						PQresStatus(dblq->stat),
						PQerrorMessage(dblq->dbl->conn));
				pgsql_freeResult(dblq);
				dblq = NULL;
				break;
			case PGRES_FATAL_ERROR:
				if(dblq->dbl->debug)
					printf("pgsql_exec: status: '%s'\n\t message: '%s'\n\tShutting down connection.\n",
						PQresStatus(dblq->stat),
						PQerrorMessage(dblq->dbl->conn));
				pgsql_freeResult(dblq);
				pgsql_close(dblq->dbl);
				dblq = NULL;
				break;
			case PGRES_COMMAND_OK:
				if(dblq->dbl->debug)
					printf("pgsql_exec: command_ok\n");
				/* set last oid if this was an insert */
				if((dblq->lastOid = PQoidValue(dblq->result)) == InvalidOid)
					dblq->lastOid = 0;
				if((str = PQcmdTuples(dblq->result)) != NULL)
					sscanf(str,"%d",&dblq->numRows);
				dblq->ok = 1;
				break;
			case PGRES_TUPLES_OK:
				if(dblq->dbl->debug)
					printf("pgsql_exec: tuples_ok\n");
				dblq->numRows = PQntuples(dblq->result);
				dblq->numFields = PQnfields(dblq->result);
				dblq->ok = 1;
				break;
		}
	}
	else if(dbl->debug>1)
	{
		printf("pgsql_exec: open %u verify %u\n",pgsql_IsOpen(dbl),pgsql_verifyConn(dbl));
	}

	if(dbl->debug>1)
	{
		if(dblq != NULL)
			printf("pgsql_exec: exit - ok: %u, rows: %u, fields: %u, oid: %d\n",
				dblq->ok,dblq->numRows,dblq->numFields,dblq->lastOid);
		else
			printf("pgsql_exec: exit dblq == NULL\n");
	}

	return(dblq);
}

char *pgsql_result(ppgsql_query dblq, int row_ind, char *field)
{	int		field_ind = -1;
	char*		tmp = NULL;
	char*		ret = NULL;

	if(dblq->dbl->debug>1)
		printf("pgsql_result: enter\n");

	dblq->ok = 0;
	if(pgsql_IsOpen(dblq->dbl) && dblq->result != NULL && field != NULL)
	{
		/* get field index */
		field_ind = PQfnumber(dblq->result, field);
		dblq->ok = (field_ind >= 0);
		if (!dblq->ok && dblq->dbl->debug)
			printf("pgsql_result: Field %s not found\n", field);

		/* get ASCII value of the field */
		tmp = PQgetvalue(dblq->result, row_ind, field_ind);
		if(tmp != NULL)
		{
			ret = strdup(tmp);
			if (dblq->dbl->debug && ret == NULL)
				printf("pgsql_result: Out of memory\n");
		}
		else if(dblq->dbl->debug)
			printf("pgsql_result: error getting data value\n");
	}
	dblq->ok = (tmp != NULL && ret != NULL);

	if(dblq->dbl->debug>1)
		printf("pgsql_result: ok: %u\n",dblq->ok);

	return(ret);
}

int pgsql_getFieldInt(ppgsql_query dblq, int row_ind, int field_ind, int *val)
{	char	*str = NULL;
	int	ok = dblq->ok;

	if(dblq->dbl->debug>1)
		printf("pgsql_getFieldInt: enter\n");

	dblq->ok = 0;
	if(ok && val != NULL && pgsql_IsOpen(dblq->dbl) && dblq->result != NULL)
	{
		if (field_ind >= dblq->numFields)
		{
			if(dblq->dbl->debug)
				printf("pgsql_getFieldInt: Field index larger than number of fields\n");
		}
		else
		{
			str = PQgetvalue(dblq->result, row_ind, field_ind);
			if(str != NULL)
			{
				*val = atoi(str);
				dblq->ok = 1;
			}
		}
	}

	if(dblq->dbl->debug>1)
		printf("pgsql_getFieldInt: exit - ok: %u\n",dblq->ok);

	return(dblq->ok);
}

int pgsql_getFieldBuf(ppgsql_query dblq, int row_ind, int field_ind, char *buf, int bufLenMax)
{	char	*tmp = NULL;
	int	ok = dblq->ok;

	if(dblq->dbl->debug>1)
		printf("pgsql_getFieldBuf: enter\n");

	dblq->ok = 0;
	if(ok && pgsql_IsOpen(dblq->dbl) && dblq->result != NULL)
	{
		if (field_ind >= dblq->numFields)
		{
			if(dblq->dbl->debug)
				printf("pgsql_getFieldBuf: Field index (%d) larger than number of fields (%d)\n",field_ind,dblq->numFields);
		}
		else
		{
			tmp = PQgetvalue(dblq->result, row_ind, field_ind);
			if(tmp != NULL)
			{
				if(bufLenMax)
				{
					memset(buf,0,bufLenMax);
					strncpy(buf,tmp,bufLenMax);
				}
				else
					strcpy(buf,tmp);
				dblq->ok = 1;
			}
		}
	}

	if(dblq->dbl->debug>1)
		printf("pgsql_getFieldBuf: exit - ok: %u\n",dblq->ok);

	return(dblq->ok);
}

void pgsql_freeResult(ppgsql_query dblq)
{	int	debug = 0;

	if(dblq != NULL)
	{
		debug = dblq->dbl->debug;

		if(debug>1)
			printf("pgsql_freeResult: enter\n");

		if(dblq->result != NULL)
			PQclear(dblq->result);
		if(dblq->query != NULL)
			free(dblq->query);
		free(dblq);

		if(debug>1)
			printf("pgsql_freeResult: exit - ok: %u\n",dblq->ok);
	}
}

int pgsql_numRows(ppgsql_query dblq)
{
	return(dblq->numRows);
}


int pgsql_numFields(ppgsql_query dblq)
{
	return(dblq->numFields);
}

void pgsql_freeFieldInfo(ppgsql_fieldInfo fi)
{
	if(fi != NULL) {
		if(fi->fname != NULL)
			free(fi->fname);
		free(fi);
		}
}

ppgsql_fieldInfo pgsql_getFieldInfo(ppgsql_query dblq, int field_ind)
{	ppgsql_fieldInfo	ret = NULL;

	dblq->ok = 0;
	if(pgsql_IsOpen(dblq->dbl) && dblq->result != NULL) {
		if (field_ind >= dblq->numFields) {
			if(dblq->dbl->debug)
				printf("pgsql_fieldName: Field index larger than number of fields\n");
			}
		else {
			ret = (ppgsql_fieldInfo)calloc(1,sizeof(pgsql_fieldInfo));
			if(ret != NULL) {
				ret->findex = field_ind;
				ret->fname = strdup(PQfname(dblq->result, field_ind));
				ret->ftype = PQftype(dblq->result, field_ind);
				ret->fsize = PQfsize(dblq->result, field_ind);
				dblq->ok = (ret->fname != NULL);
				}
			if (!dblq->ok && dblq->dbl->debug) 
				printf("pgsql_field: Out of memory\n");
			}
		}

	return(ret);
}

int pgsql_getlastoid(ppgsql_query dblq)
{
	return(dblq->lastOid);
}

char *pgsql_errorMessage(ppgsql_link dbl)
{	char		*tmp = NULL;

	if(pgsql_IsOpen(dbl) && pgsql_verifyConn(dbl)) {
		tmp = strdup(PQerrorMessage(dbl->conn));
		if (dbl->debug && tmp == NULL)
			printf("Out of memory\n");
		}

	return(tmp);
}

void pgsql_StripDollarSlashes(char *string)
{	char *s,*t;
	int l;

	l = strlen(string); 
	s = string;
	t = string;
	while(*t && l>0) {
		if(*t=='\\' && *(t+1)=='$') {
			t++;
			*s++=*t++;
			l-=2;
		} else if(*t=='\\' && *(t+1)=='\\') {
			if(s!=t) *s++=*t++;
			else { s++; t++; }
			l--;
			if(s!=t) *s++=*t++;
			else { s++; t++; }
			l--;
		} else {
			if(s!=t) *s++=*t++;
			else { s++; t++; }
			l--;
		}
	}
	if(s!=t) *s='\0';
}

/* regular expression poo pooo.... :) */

/*
 * reg_eprint - convert error number to name
 */
char *pgsql_reg_eprint(int err)
{	static char epbuf[150];
	size_t len;

#ifdef REG_ITOA
	len = regerror(REG_ITOA|err, (regex_t *)NULL, epbuf, sizeof(epbuf));
#else
	len = regerror(err, (regex_t *)NULL, epbuf, sizeof(epbuf));
#endif
	if(len > sizeof(epbuf)) {
		epbuf[sizeof(epbuf)-1]='\0';
	}
	return(epbuf);
}

#define	NS	10
char *pgsql_RegReplace(char *pattern, char *replace, char *string)
{	char *buf, *nbuf;
	char o;
	int i,l,ll,new_l,allo;
	regex_t re;
	regmatch_t subs[NS];
	char erbuf[150];
	int err, len;
#ifdef HAVE_REGCOMP
	char oo;
#endif

	l = strlen(string);
	if(!l) return(string);

	err = regcomp(&re, pattern, 0);
	if(err) {
		len = regerror(err, &re, erbuf, sizeof(erbuf));
		printf("Regex error %s, %d/%d `%s'\n", pgsql_reg_eprint(err), len, sizeof(erbuf), erbuf);
		return((char *)-1);
	}	

	buf = malloc(l*2*sizeof(char)+1);
	if(!buf) {
		printf("Unable to allocate memory in pgsql_RegReplace\n");
		regfree(&re);
		return((char *)-1);
	}
	err = 0;
	i = 0;
	allo = 2*l+1;
	buf[0] = '\0';	
	ll = strlen(replace);
	while(!err) {
#ifndef HAVE_REGCOMP
		subs[0].rm_so = i;
		subs[0].rm_eo = l;
		err = regexec(&re, string, (size_t)NS, subs, REG_STARTEND);
#else
		oo = string[l];
		string[l] = '\0';
		err = regexec(&re, &string[i], (size_t)NS, subs, 0);
		string[l] = oo;				
		subs[0].rm_so += i;
		subs[0].rm_eo += i;
#endif
		if(err && err!=REG_NOMATCH) {
			len = regerror(err, &re, erbuf, sizeof(erbuf));
			printf("Regex error %s, %d/%d `%s'\n", pgsql_reg_eprint(err), len, sizeof(erbuf), erbuf);
			regfree(&re);
			return((char *)-1);
		}
		if(!err) {
			o = string[subs[0].rm_so];
			string[subs[0].rm_so]='\0';
			new_l = strlen(buf)+strlen(replace)+strlen(&string[i]);
			if(new_l > allo) {
				nbuf = malloc(1+allo+2*new_l);
				allo = 1+allo + 2*new_l;
				strcpy(nbuf,buf);
				buf=nbuf;
			}	
			strcat(buf,&string[i]);
			strcat(buf,replace);

			string[subs[0].rm_so] = o;
			i = subs[0].rm_eo;
		} else {
			new_l = strlen(buf)+strlen(&string[i]);
			if(new_l > allo) {
				nbuf = malloc(1+allo+2*new_l);
				allo = 1+allo + 2*new_l;
				strcpy(nbuf,buf);
				buf=nbuf;
			}	
			strcat(buf,&string[i]);
		}	
		if(subs[0].rm_so==0 && subs[0].rm_eo==0) break;
	}
	regfree(&re);
	return(buf);
}

/*    
 * If freeit is non-zero, then this function is allowed to free the
 * argument string.  If zero, it cannot free it.
 */     
char *pgsql_AddSlashes(char *string, int freeit)
{	static char *temp=NULL;
                
        if(strchr(string,'\\')) {
                temp = pgsql_RegReplace("\\\\","\\\\",string);
                if(freeit) {
			if(temp!=string) string=temp;
			}
		else {
                        if(temp!=string) strcpy(string,temp);
			}
		}

        if(strchr(string,'$')) {
                temp = pgsql_RegReplace("\\$","\\$",string);
                if(freeit) {
                        if(temp!=string) string=temp;
			}
		else {
                        if(temp!=string) strcpy(string,temp);
			}
		}

#if MAGIC_QUOTES
        if(strchr(string,'\'')) {
                temp = pgsql_RegReplace("'","\\'",string);
                if(freeit) {
                        if(temp!=string) string=temp;
			}
		else {
                        if(temp!=string) strcpy(string,temp);
			}
		}

        if(strchr(string,'\"')) {
                temp = pgsql_RegReplace("\"","\\\"",string);
                if(freeit) {
                        if(temp!=string) string=temp;
			}
		else {
                        if(temp!=string) strcpy(string,temp);
			}
		}
#endif
        return(string);
}
