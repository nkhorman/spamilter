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
 *	CVSID:  $Id: nstring.c,v 1.7 2011/07/29 21:23:17 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		nstring.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: nstring.c,v 1.7 2011/07/29 21:23:17 neal Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nstring.h"

/* bit-wise */
#define	LONGINT	1
#define	HEXINT	2
#define	HAVEMINSIZE 4
#define	HAVEMAXSIZE 8
int vasprintf(char **ret, char *fmt, va_list ap1)
{	va_list	ap2;
	char	*p1,*s,c,*p2;
	int	d,size,convertFlagNeeded,flags,x;
	int	convertSsize;
	char	convertSizeSpec[11],itoabuf[32];
	long	l;
	int	rc = -1;

	if(ret == NULL || fmt == (char *)NULL || !*fmt)
		return(rc);

	ap2 = ap1;
	size= 0;
	convertFlagNeeded= 0;
	convertSsize= 0;
	flags= 0;
	p1= fmt;

	/* evaluate all of the ... arguments based on the fmt string
	 * and account for the space needed to create the final 
	 * formatted string to be generated.
	 *
	 * This routine takes a very simplistic view of the standard
	 * format string used in the printf and family of routines.
	 */
	while(*p1)
	{
		switch(*p1)
		{
			case '%':
				if(!convertFlagNeeded)
				{
					flags= 0;
					convertSsize= 0;
					convertFlagNeeded= 1;
					memset(&convertSizeSpec,0,sizeof(convertSizeSpec));
					p2= (char *)&convertSizeSpec;
				}
				else
				{
					convertFlagNeeded= 0;
					size++;
				}
				break;
			case '*':
				if(convertFlagNeeded)
				{
					convertSsize = (int)va_arg(ap2, int);
					if((flags & HAVEMINSIZE) == 0)
						flags |= HAVEMINSIZE;
					else if(flags & HAVEMINSIZE)
						flags |= HAVEMAXSIZE;
				}
				else
					size++;
				break;
			case 's':
				if(convertFlagNeeded)
				{
					s = va_arg(ap2, char *);
					x = (s != NULL ? strlen(s) : 0);
					if((flags & HAVEMINSIZE) && x<convertSsize)
						x = convertSsize;
					if((flags & HAVEMAXSIZE) && x>convertSsize)
						x = convertSsize;
					convertFlagNeeded= 0;
					size += x;
				}
				else
					size++;
				break;
			case 'x':
			case 'X':
				flags |= HEXINT;
				/* fall through */
			case 'd':
			case 'i':
			case 'o':
			case 'u':
				if(convertFlagNeeded)
				{
					if(flags & LONGINT)
					{
						l = (long)va_arg(ap2, long);
						if(flags & HEXINT) 
							sprintf(itoabuf,"%lX",l);
						else
							sprintf(itoabuf,"%ld",l);
					}
					else
					{
						d = (int)va_arg(ap2, int);
						if(flags & HEXINT)
							sprintf(itoabuf,"%X",d);
						else
							sprintf(itoabuf,"%d",d);
					}
					size += strlen(itoabuf);
					if(strlen(convertSizeSpec))
						size += atoi(convertSizeSpec);
					convertFlagNeeded= 0;
				}
				else size++;
				break;
			case 'c':
				if(convertFlagNeeded)
				{
					c = (char)va_arg(ap2, int);
					convertFlagNeeded= 0;
				}
				size ++;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				if(convertFlagNeeded && strlen(convertSizeSpec)<sizeof(convertSizeSpec)-1)
					*(p2++)= *p1;
				else
					size++;
				flags |= HAVEMINSIZE;
				break;
			case '.':
				/* throw away pervious size - is min size, now get max size */
				if(convertFlagNeeded)
				{
					memset(&convertSizeSpec,0,sizeof(convertSizeSpec));
					p2= (char *)&convertSizeSpec;
					flags |= HAVEMAXSIZE;
				}
				else
					size++;
			case 'l':
				if(convertFlagNeeded)
					flags |= LONGINT;
				else
					size++;
				break;
			case 'L':
				/* this is for a long double - no action taken */
			default:
				if(convertFlagNeeded)
				{
					/* add a some space for this unknown flag.
					 * We have no idea if this is enough space
					 * for this flag, but we'll give it a try
					 * anyway. This amount of space was arbitarily
					 * choosen as the smallest amount of space
					 * that might be applicable. One could take
					 * the opposite approach, and go for liberalizm,
					 * I choose minimalizm. KNH 971110
					 */
					size += 8;
				}
				else
					size++;
				break;
		}
		p1++;
	}

	/* create storage space */
	*ret= malloc(size+1);
	if(*ret != (char *)NULL)
		/* let a more robust routine handle the job of formatting. */
		rc = vsnprintf(*ret, size+1, fmt, ap1);

	return(rc);
}

int asprintf(char **ret, char *fmt, ...)
{	va_list	vlist;
	int	rc = -1;

	va_start(vlist,fmt);
	if(ret != NULL && fmt != NULL && *fmt)
		rc = vasprintf(ret,fmt,vlist);
	va_end(vlist);

	return(rc);
}
