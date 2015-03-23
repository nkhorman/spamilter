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
 *	CVSID:  $Id: mx.h,v 1.8 2012/12/09 18:19:42 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		mx.h
 *--------------------------------------------------------------------*/

#ifndef _SPAMILTER_MX_H_
#define _SPAMILTER_MX_H_

 	#include "config.h"

	#include <sys/types.h>
	#include <netinet/in.h>
	#include <arpa/nameser.h>
	#include <resolv.h>
	#include <netdb.h>

	#include "dns.h"

	#define  MAX_MXHOSTS 50

	typedef struct _mx_host
	{
		int	nsType;
		union
		{
			unsigned long ipv4;
			struct in6_addr ipv6;
		};
	} mx_host;

	typedef struct _mx_rr
	{
		char	name[NS_MAXDNAME];
		int	pref;
		int	visited;
		mx_host	host[MAX_MXHOSTS];
		int	qty;
	} mx_rr;

	typedef struct _mx_rr_list
	{
		int nsType;
		union
		{
			unsigned long ipv4;
			struct in6_addr ipv6;
		};
		char	domain[NS_MAXDNAME];
		int	match;
		mx_rr	mx[MAX_MXHOSTS];
		int	qty;
	} mx_rr_list;

	mx_rr *mx_get_rr_hosts(ds_t const * pDs, mx_rr *mxrr);
	mx_rr_list *mx_get_rr_bydomain(ds_t const *pDs, mx_rr_list *rrl, const char *name);

#endif
