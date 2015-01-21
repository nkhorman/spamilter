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
 *	CVSID:  $Id: smisc.c,v 1.3 2011/07/29 21:23:17 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		smisc.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: smisc.c,v 1.3 2011/07/29 21:23:17 neal Exp $";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/time.h>

#include "spamilter.h"
#include "smisc.h"

int mlfi_setreply(SMFICTX *ctx, int replycode, char *replysect, char *fmt, ...)
{	char	*str = NULL;
	char	replycodestr[15];
	int	rc = 0;
	va_list	vl;

	va_start(vl,fmt);
	rc = vasprintf(&str,fmt,vl);
	if(str != NULL && rc != -1)
	{
		sprintf(replycodestr,"%d",replycode);
		smfi_setreply(ctx,replycodestr,replysect,str);
	}

	if(str != NULL)
		free(str);

	va_end(vl);

	return(rc);
}

int mlfi_addhdr_printf(SMFICTX *ctx, char *hdrname, char *hdrfmt, ...)
{	char	*hdrbdy = NULL;
	int	rc = 0;
	va_list	vl;

	va_start(vl,hdrfmt);
	rc = vasprintf(&hdrbdy,hdrfmt,vl);
	if(hdrbdy != NULL && rc != -1)
		smfi_addheader(ctx,hdrname,hdrbdy);

	if(hdrbdy != NULL)
		free(hdrbdy);

	va_end(vl);

	return(rc);
}
