/*--------------------------------------------------------------------*
 *
 * Contributed by;
 *	kerbawy@gmail.com - Initial OpenBSD support
 *	John Kerbawy says;
 *	"... nameser_compat.h was compiled by me, but mostly from FreeBSD. ..."
 *	The obligatory headers from from my version of 
 *	/usr/include/arpa/nameser.h have been added below.
 *
 *--------------------------------------------------------------------*/
 
/*
 * Copyright (c) 1983, 1989, 1993
 *    The Regents of the University of California.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 * 	This product includes software developed by the University of
 * 	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1996 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
 *	From: Id: nameser.h,v 8.16 1998/02/06 00:35:58 halley Exp
 * $FreeBSD: src/include/arpa/nameser.h,v 1.14.2.1 2001/06/15 22:08:27 ume Exp $
 */

/*--------------------------------------------------------------------*
 *
 * Mildly modified after submission from kerbawy@gmail.com by;
 *	Neal Horman - http://www.wanlink.com
 *	Copyright (c) 2004 Neal Horman. All Rights Reserved
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
 *	CVSID:  $Id: ns_compat.h,v 1.3 2012/05/04 00:14:06 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		ns_compat.h
 *--------------------------------------------------------------------*/

#ifndef _SPAMILTER_NS_COMPAT_H_
#define _SPAMILTER_NS_COMPAT_H_

#include <arpa/nameser.h>

#define NS_CMPRSFLGS    INDIR_MASK
#define NS_GET16        GETSHORT
#define NS_GET32        GETLONG
#define NS_INT16SZ      INT16SZ
#define NS_INT32SZ      INT32SZ
#define NS_MAXDNAME     MAXDNAME
#define NS_MAXCDNAME    MAXCDNAME
#define NS_PACKETSZ     PACKETSZ
#define NS_PUT16        PUTSHORT
#define NS_PUT32        PUTLONG

#define ns_c_in         C_IN
#define ns_t_a          T_A
#define ns_t_mx         T_MX
#define ns_t_ptr        T_PTR

#define ns_msg_base(handle)		((handle)._msg + 0)
#define ns_msg_count(handle, section)	((handle)._counts[section] + 0)
#define ns_msg_end(handle)		((handle)._eom + 0)
#define ns_rr_rdata(rr)			((rr).rdata + 0)
#define ns_rr_type(rr)			((rr).type + 0)

struct _ns_flagdata { int mask, shift; };

/*
 * These can be expanded with synonyms, just keep ns_parse.c:ns_parserecord()
 * in synch with it.
 */
typedef enum __ns_sect {
	ns_s_qd = 0,		/* Query: Question. */
	ns_s_zn = 0,		/* Update: Zone. */
	ns_s_an = 1,		/* Query: Answer. */
	ns_s_pr = 1,		/* Update: Prerequisites. */
	ns_s_ns = 2,		/* Query: Name servers. */
	ns_s_ud = 2,		/* Update: Update. */
	ns_s_ar = 3,		/* Query|Update: Additional records. */
	ns_s_max = 4
} ns_sect;

/*
 * This is a message handle.  It is caller allocated and has no dynamic data.
 * This structure is intended to be opaque to all but ns_parse.c, thus the
 * leading _'s on the member names.  Use the accessor functions, not the _'s.
 */
typedef struct __ns_msg {
	const u_char	*_msg, *_eom;
	u_int16_t	_id, _flags, _counts[ns_s_max];
	const u_char	*_sections[ns_s_max];
	ns_sect		_sect;
	int		_rrnum;
	const u_char	*_ptr;
} ns_msg;

/*
 * This is a parsed record.  It is caller allocated and has no dynamic data.
 */
typedef struct __ns_rr {
	char		name[MAXDNAME];	/* XXX need to malloc */
	u_int16_t	type;
	u_int16_t	rr_class;
	u_int32_t	ttl;
	u_int16_t	rdlength;
	const u_char	*rdata;
} ns_rr;

#endif /* _SPAMILTER_NS_COMPAT_H_ */
