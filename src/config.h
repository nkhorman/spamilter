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
 *	CVSID:  $Id: config.h,v 1.18 2014/12/31 00:51:49 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		config.h
 *--------------------------------------------------------------------*/

#ifndef _SPAMILTER_CONFIG_H_
#define _SPAMILTER_CONFIG_H_

#if defined( __FreeBSD__) && defined(BIGSETSIZE)
	#define FD_SETSIZE 2048
#endif

#if defined(__OpenBSD__)
	#include "ns_compat.h"
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#ifndef res_ninit
	#define RES_NALLOC(s)			(s)
#else
	#define RES_NALLOC(s)			calloc(1,sizeof(*s))
#endif

#if !defined(HAVE_RES_N) && __RES < 20030124
#if !defined(res_state) && !defined(__res_state_defined)
	typedef void *res_state;
#endif
#ifndef res_ninit
	#define res_ninit(s)                    res_init()
#endif
#ifndef res_nclose
	#define res_nclose(s)			res_close()
#endif
#ifndef res_nquery
	#define res_nquery(s,d,c,t,a,l)         res_query(d,c,t,a,l)
#endif
#ifndef res_nsearch
	#define res_nsearch(s,d,c,t,a,l)        res_search(d,c,t,a,l)
#endif
#ifndef res_nupdate
	#define res_nupdate(s,d,t)		res_update(d)
#endif
#else
	#include <res_update.h>
#endif

	#include <string.h>

#if !defined(OS_FreeBSD) && !defined(__FreeBSD_version) && !defined(OS_Darwin)
	#include "nstring.h"
#endif

#endif

#include "config.defs"
