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
 *	CVSID:  $Id: pgsql.h,v 1.7 2012/05/04 00:14:06 neal Exp $
 *
 * DESCRIPTION:
 *	application:	cidsu
 *	source module:	pgsql.h
 *
 *--------------------------------------------------------------------*/

#ifndef _SPAMILTER_PGSQL_H_
#define _SPAMILTER_PGSQL_H_

	#include <libpq-fe.h>

	typedef struct _pgsql_link {
		PGconn		*conn;
		int		init,open;
		int		ok;
		int		debug;
		} pgsql_link, *ppgsql_link;

	typedef struct _pgsql_srvr
	{
		char const	*host;
		char const	*port;
		char const	*device;
		char const	*user;
		char const	*pass;
		ppgsql_link dbl;
		} pgsql_srvr, *ppgsql_srvr;

	typedef struct _pgsql_query {
		ppgsql_link	dbl;
		PGresult	*result;
		int		ok;
		int		lastOid;
		int		numRows,numFields;
		char		*query;
		int		stat;
		} pgsql_query, *ppgsql_query;

	typedef struct _pgsql_fieldInfo {
		int	findex;
		char	*fname;
		int	ftype;
		int	fsize;
		} pgsql_fieldInfo, *ppgsql_fieldInfo;


	/* constructor destructor */
	ppgsql_srvr pgsql_srvr_init(char const *host, char const *port, char const *device, char const *user, char const *pass);
	ppgsql_srvr pgsql_srvr_destroy(ppgsql_srvr srvr, int freeIt);

	ppgsql_link pgsql_dbl_init(ppgsql_link dbl);
	ppgsql_link pgsql_dbl_destroy(ppgsql_link dbl, int freeIt);


	/* link state */
	int pgsql_IsInit(ppgsql_link dbl);
	int pgsql_IsOpen(ppgsql_link dbl);
	int pgsql_verifyConn(ppgsql_link dbl);


	/* link operations */
	int pgsql_open(ppgsql_srvr srv);
	int pgsql_close(ppgsql_link dbl);


	/* link query */
	ppgsql_query pgsql_exec(ppgsql_link dbl, char *query);
	ppgsql_query pgsql_exec_printf(ppgsql_link dbl, char *fmt, ...);


	/* link query result set operations */
	char *pgsql_result(ppgsql_query dblq, int row_ind, char *field);
	int pgsql_getFieldBuf(ppgsql_query dblq, int row_ind, int field_ind, char *buf, int bufLenMax);
	int pgsql_getFieldInt(ppgsql_query dblq, int row_ind, int field_ind, int *val);

	ppgsql_fieldInfo pgsql_getFieldInfo(ppgsql_query dblq, int field_ind);
	void pgsql_freeFieldInfo(ppgsql_fieldInfo fi);

	void pgsql_freeResult(ppgsql_query dblq);

	int pgsql_numRows(ppgsql_query dblq);
	int pgsql_numFields(ppgsql_query dblq);

	int pgsql_getlastoid(ppgsql_query dblq);


	/* misc */
	char *pgsql_errorMessage(ppgsql_link dbl);

	void pgsql_StripDollarSlashes(char *string);

	char *pgsql_reg_eprint(int err);
	char *pgsql__RegReplace(char *pattern, char *replace, char *string);

	char *pgsql_AddSlashes(char *string, int freeit);
#endif
