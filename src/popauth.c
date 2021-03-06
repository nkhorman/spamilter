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
 *	CVSID:  $Id: popauth.c,v 1.4 2011/07/29 21:23:17 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		popauth.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: popauth.c,v 1.4 2011/07/29 21:23:17 neal Exp $";

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

#include "spamilter.h"
#include "popauth.h"

int popauth_validate(mlfiPriv *priv, char *dbpath)
{	int	rc  = 0;

	if(priv != NULL)
	{
		if(dbpath != NULL)
		{	DB	*db = NULL;
#if DB_VERSION_MAJOR < 2
			db = dbopen(dbpath,O_RDONLY,0x444,DB_HASH,NULL);
#else
			db_open(dbpath,DB_HASH,O_RDONLY,0x444,NULL,NULL,&db);
#endif

			if(db != NULL)
			{	DBT	key;
				DBT	value;

				memset(&key,0,sizeof(key));
				memset(&value,0,sizeof(value));

				/* this assumes that the db file format is;
					xxx.xxx.xxx.xxx		some junk
				*/
				key.data = priv->ipstr;
				key.size = strlen(priv->ipstr);
				rc = (db->get(db,
#if DB_VERSION_MAJOR >= 2
					NULL,
#endif
					&key,&value,0) == 0);
				db->close(db
#if DB_VERSION_MAJOR >= 2
					,0
#endif
					);
			}
			else
				mlfi_debug(priv->pSessionUuidStr, "popauth_validate: unable to open '%s'\n", dbpath);
		}
		else
			mlfi_debug(priv->pSessionUuidStr, "popauth_validate: not popauth pathname specified\n");
	}

	return(rc);
}
