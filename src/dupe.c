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
 *	CVSID:  $Id: dupe.c,v 1.3 2011/07/29 21:23:16 neal Exp $
 *
 * DESCRIPTION:
 *	application:	Spamilter
 *	module:		dupe.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: dupe.c,v 1.3 2011/07/29 21:23:16 neal Exp $";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <string.h>

#include "spamilter.h"
#include "dupe.h"

void dupe_init(mlfiPriv *priv)
{
	if(priv != NULL)
	{
		priv->pdupercpt = NULL;
	}
}

void dupe_query(mlfiPriv *priv, char *mailstr, int stage)
{
	if(priv != NULL && mailstr != NULL)
	{
		switch(stage)
		{
			case DUPE_RCPT:
			case DUPE_FROM:
#include "dupelocal.inc"
				break;
		}
	}
}

void dupe_action(SMFICTX *ctx, mlfiPriv *priv)
{
	if(ctx != NULL && priv != NULL && priv->pdupercpt != NULL)
	{
		smfi_addrcpt(ctx, priv->pdupercpt);
		free(priv->pdupercpt);
		priv->pdupercpt = NULL;
	}
}
