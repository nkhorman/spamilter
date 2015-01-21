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
 *	CVSID:  $Id: hndlrs.h,v 1.7 2012/01/02 03:15:44 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		hndlrs.h
 *--------------------------------------------------------------------*/

#ifndef _SPAMILTER_HNDLRS_H_
#define _SPAMILTER_HNDLRS_H_

	#include "libmilter/mfapi.h"

	sfsistat mlfi_connect(SMFICTX *ctx, char *hostname, _SOCK_ADDR *hostaddr);
	sfsistat mlfi_helo(SMFICTX *ctx, char *helohost);
	sfsistat mlfi_envfrom(SMFICTX *ctx, char **envfrom);
	sfsistat mlfi_envrcpt(SMFICTX *ctx, char **argv);
	sfsistat mlfi_header(SMFICTX *ctx, char *headerf, char *headerv);
	sfsistat mlfi_eoh(SMFICTX *ctx);
	sfsistat mlfi_body(SMFICTX *ctx, u_char *bodyp, size_t len);
	sfsistat mlfi_eom(SMFICTX *ctx);
	sfsistat mlfi_abort(SMFICTX *ctx);
	sfsistat mlfi_close(SMFICTX *ctx);

	sfsistat mlfi_cleanup(SMFICTX *);

#endif
