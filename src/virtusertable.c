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
 *	CVSID:  $Id: virtusertable.c,v 1.4 2012/11/18 21:13:16 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		virtusertable.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: virtusertable.c,v 1.4 2012/11/18 21:13:16 neal Exp $";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

#ifdef OS_Linux
#include <db_185.h>
#else
#include <db.h>
#endif

#include <fcntl.h>
#include <limits.h>

#include "misc.h"
#include "virtusertable.h"

#ifdef _UNIT_TEST
char	*gVirtUserTableChk		= "/etc/mail/virtusertable.db";
#endif

int virtusertable_validate_key(char *rcpt, DB *pdb)
{	int rc = 0;

	if(rcpt != NULL && pdb != NULL)
	{	DBT	key;
		DBT	value;

		memset(&key,0,sizeof(key));
		memset(&value,0,sizeof(value));

		key.data = rcpt;
		key.size = strlen(rcpt);
		rc = (pdb->get(pdb,
#if DB_VERSION_MAJOR >= 2
			NULL,
#endif
			&key,&value,0) == 0);
	}

	return(rc);
}

int virtusertable_validate(const char *pSessionId, char *rcpt, char *dbpath)
{	int	rc  = 0;

	if(rcpt != NULL && dbpath != NULL)
	{	DB	*pdb = NULL;
#if DB_VERSION_MAJOR < 2
		pdb = dbopen(dbpath,O_RDONLY,0x444,DB_HASH,NULL);
#else
		db_open(dbpath,DB_HASH,O_RDONLY,0x444,NULL,NULL,&pdb);
#endif

		if(pdb != NULL)
		{	char *pat = strchr(rcpt,'@');

			rc = ((pat != NULL && pat != rcpt ? virtusertable_validate_key(pat,pdb) : 0) || virtusertable_validate_key(rcpt,pdb));
			pdb->close(pdb
#if DB_VERSION_MAJOR >= 2
				,0
#endif
				);
		}
	}

#ifndef _UNIT_TEST
	mlfi_debug(pSessionId,"virtusertable_validate: '%s' %d/%s\n",rcpt,rc,(rc ? "Found" : "Not Found"));
#endif

	return(rc);
}

int virtusertable_validate_rcptdom(const char *pSessionId, char *rcpt, char *dom, char *dbpath)
{	int ok = 0;

	if(rcpt != NULL && dom != NULL && dbpath != NULL && strlen(rcpt) + strlen(dom) + 1 < 1024)
	{	char buf[1024];

		sprintf(buf,"%s@%s",rcpt,dom);
		ok = virtusertable_validate(pSessionId,buf,dbpath);
	}

	return(ok);
}

#ifdef _UNIT_TEST
#define SetKeyValStr(k,v,n,d) { if(strlen(k) && strlen(v) && strcasecmp((k),(n)) == 0) (d) = strdup((v)); }
#define SetKeyValInt(k,v,n,d) { if(strlen(k) && strlen(v) && strcasecmp((k),(n)) == 0) (d) = atoi((v)); }

#define ShowKeyValStr(k,v) { printf("%s = '%s'\n",(k),(v)); }
#define ShowKeyValInt(k,v) { printf("%s = %u\n",(k),(v)); }

void getconf(char *confpath)
{	FILE	*fin = fopen(confpath,"r");
	char	*str;
	char	buf[1024];
	char	key[1024];
	char	val[1024];

	if(fin != NULL)
	{
		while(!feof(fin))
		{
			fgets(buf,sizeof(buf),fin);
			
			str = strchr(buf,'#');
			if(str != NULL)
			{
				*(str--) = '\0';
				while(str >= buf && (*str ==' ' || *str == '\t'))
					*(str--) = '\0';
			}

			if(strlen(buf))
			{
				str = mlfi_strcpyadv(key,sizeof(key),buf,'=');
				str = mlfi_strcpyadv(val,sizeof(val),str,'=');

				SetKeyValStr(key,val,"VirtUserTableChk",gVirtUserTableChk);
			}
		}
		fclose(fin);
	}
	else
		printf("Warning! Unable to open config file '%s', using compiled defaults.\n",confpath);
}

int gDebug=1;
int main(int argc, char *argv[])
{	int i,ok;

	getconf("/etc/spamilter.rc");

	ShowKeyValStr("VirtUserTableChk",gVirtUserTableChk);

	for(i=1; gVirtUserTableChk != NULL && i<argc; i++)
	{
		ok = virtusertable_validate(argv[i],gVirtUserTableChk);

		printf("%s - %d/%s\n",argv[i],ok,(ok ? "Ok" : "Reject"));
	}

	return(0);
}
#endif
