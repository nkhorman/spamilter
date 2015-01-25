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
 *	CVSID:  $Id: badext.c,v 1.6 2011/07/29 21:23:16 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		badext.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: badext.c,v 1.6 2011/07/29 21:23:16 neal Exp $";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>

/*
#include <db.h>
#include <fcntl.h>
#include <limits.h>
*/

#include "misc.h"
#include "spamilter.h"
#include "badext.h"

#ifndef strcasestr
/* This function was wholesale copied from FreeBSD's libc/string/strcasestr.c
   see the file strcasestr.lic for license and copyright information
*/ 
char *
strcasestr(s, find)
        register const char *s, *find;
{
        register char c, sc;
        register size_t len;

        if ((c = *find++) != 0) {
                c = tolower((unsigned char)c);
                len = strlen(find);
                do {
                        do {
                                if ((sc = *s++) == 0)
                                        return (NULL);
                        } while ((char)tolower((unsigned char)sc) != c);
                } while (strncasecmp(s, find, len) != 0);
                s--;
        }
        return ((char *)s);
}
#endif

void badext_init(mlfiPriv *priv, char *dbpath)
{
	if(gMsExtChk > 0 && priv != NULL && priv->badextlist == NULL && dbpath != NULL)
	{	FILE	*fin;
		char	*str,*fn;
		char	buf[8192];

		asprintf(&fn,"%s/db.extensions",dbpath);
		fin = fopen(fn,"r");
		if(fin != NULL)
		{	
			while(!feof(fin))
			{
				memset(buf,0,sizeof(buf));
				str = fgets(buf,sizeof(buf)-1,fin);
				if(str != NULL)
				{
					/* truncate comment ? */
					str = strchr(buf,'#');
					if(str != NULL)
						*(str--) = '\0';

					/* right trim */
					str = (buf+strlen(buf)-1);
					while(str >= buf && (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r'))
						*(str--) = '\0';

					/* left trim */
					str = buf;
					while(*str ==' ' || *str == '\t')
						str++;

					if(strlen(str))
					{	char *sep = strpbrk(str," \t");

						if(sep != NULL)
						{
							*sep = '\0';
							if(atoi(str) == gMsExtChk)
							{	char	*s,*d;

								str = sep+1;
								/* left trim */
								while(*str ==' ' || *str == '\t')
									str++;
								d=s=str;

								/* pack the string ie. remove white space */
								while(*s && s<buf+sizeof(buf))
								{
									if(*s!=' ' && *s!='\t')
										*(d++) =*s;
									s++;
								}
								if(*d != '|')
									*(d++)='|';
								*d='\0';

								/* allocate the storage space */
								d=priv->badextlist = malloc(d-str+1);

								/* copy string while changing '|' to '\0' and counting the substrings */
								s=str;
								priv->badextqty=0;
								while(*s)
								{
									*d = *(s++);
									if(*d == '|')
									{
										*d = '\0';
										priv->badextqty++;
									}
									d++;
								}
								*s='\0';
							}
						}
					}
				}
			}
			fclose(fin);
		}
		else
			mlfi_debug(priv->pSessionUuidStr, "badext_init: Unable to open Extenstions file '%s'\n", fn);
		free(fn);
	}
}

void badext_close(mlfiPriv *priv)
{
	if(priv != NULL && priv->badextlist != NULL)
	{
		free(priv->badextlist);
		priv->badextqty = 0;
	}
}

/* duplicate while removing all white space */
char *strdupnowhitespace(char *src)
{	char *dst,*dup = NULL;

	if(src != NULL && strlen(src))
	{
		dup = dst = malloc(strlen(src)+1);
		while(*src)
		{
			if(*src != ' ' && *src != '\t')
				*(dst++) = *src;
			src++;
		}
		*dst = '\0';
	}

	return(dup);
}

char *badext_find(char *str1, char*exts, int extqty)
{	char *str2 = NULL;
	int i;

	for(i=0; i<extqty && str2 == NULL; i++)
	{
		str2 = strcasestr(str1,exts);
		exts += strlen(exts)+1;
	}

	return(str2);
}

int mlfi_findBadExtHeader(char *badexts, int badextqty, char *headerf, char *headerftype, char *headerv, char *headervtype, char **attachfname)
{	int	found = 0;
	char	*fname = NULL;
	char	*str1,*str2;
	char	*hvt = strdupnowhitespace(headervtype);

	if(hvt != NULL)
	{
		if(headerf != NULL && headerftype != NULL && strcasecmp(headerf, headerftype) != 0 &&
			headerv != NULL && (fname = strcasestr(headerv,hvt)) != NULL)
		{
			fname += strlen(headervtype);
			str1 = strdup(fname);

			found = ((str2 = badext_find(str1,badexts,badextqty)) != NULL);
			if(found)
			{
				/* backup to begining of file name */
				while(isalnum(*str2) || *str2 == '.' || *str2 == ' ' || *str2 == '_' || *str2 == '-')
					str2--;

				/* trim right */
				while(!isalnum(*(str1+strlen(str1)-1)))
					*(str1+strlen(str1)-1) ='\0';

				*attachfname = strdup(str2+1);
			}

			free(str1);
		}
	}

	if(hvt != NULL)
		free(hvt);

	return(found);
}

/* this scribbles in the body, but should undo all changes */
char *badext_findLine(char *badexts, int badextqty, char *body, char *key, int *found, char **attachfname)
{	char	*eol1 = (body != NULL ? strchr(body,'\n') : NULL);
	char	*eol2 = (eol1 != NULL ? strchr(eol1+1,'\n') : NULL);
	char	*str1,*str2;

	if(!*found && body != NULL && eol1 != NULL && key != NULL)
	{	char	*cpy;

		*eol1 = '\0';
		if(eol2 != NULL)
			*eol2 = '\0';

		cpy = strdupnowhitespace(body);

		/* look for key in body ie. "filename=" */
		str1 = strcasestr(cpy,key);
		if(str1 == NULL)
		{
			*eol1 = '\n';
			free(cpy);
			cpy = strdupnowhitespace(body);
			str1 = strcasestr(cpy,key);
		}

		/* find an extension*/
		if(str1 != NULL)
			*found = ((str2 = badext_find(str1,badexts,badextqty)) != NULL);

		if(*found)
		{
			/* backup to begining of file name */
			while(str2 > cpy && (isalnum(*str2) || *str2 == '.' || *str2 == ' ' || *str2 == '_' || *str2 == '-'))
				str2--;

			/* trim right */
			str1 = str2+strlen(str2)-1;
			while(str1 > cpy && !isalnum(*str1))
				*(str1--) ='\0';

			*attachfname = strdup(str2+1);
		}

		free(cpy);

		*eol1 = '\n';
		body = eol1+1;
		if(eol2 != NULL)
		{
			*eol2 = '\n';
			body = eol2+1;
		}
	}

	return(body);
}

int mlfi_findBadExtBody(char *badexts, int badextqty, char *body, char **attachfname)
{	int	found = 0;
	char	*str1 = body;
	char	*str2 = body;

	while(!found && (str1 != NULL || str2 != NULL))
	{
		if(str1 != NULL)
			str1 = badext_findLine(badexts,badextqty,strcasestr(str1,"Content-Disposition:"),"filename=",&found,attachfname);
		if(str2 != NULL && *attachfname == NULL)
			str2 = badext_findLine(badexts,badextqty,strcasestr(str2,"Content-Type:"),"name=",&found,attachfname);
	}

	return(found);
}
