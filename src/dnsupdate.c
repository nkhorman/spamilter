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
 *	CVSID:  $Id: dnsupdate.c,v 1.4 2011/07/29 21:23:16 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		dnsupdate.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: dnsupdate.c,v 1.4 2011/07/29 21:23:16 neal Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "config.h"
#include "dnsupdate.h"

int dns_update_rr_a(int debug, int r_opcode, char *r_dname, u_int32_t r_ttl, char *r_addr, int afType)
{	int	rc = -1;
	int	r_size = 0;
	ns_updrec *rrecp;
	res_state res = RES_NALLOC(res);

	if(r_dname != NULL)
		r_size = strlen(r_dname);

	switch(r_opcode)
	{
		case 0:
			r_opcode = DELETE;
			break;
		case 1:
			r_opcode = ADD;
			break;
	}

	rrecp = res_mkupdrec(S_UPDATE, r_dname, C_IN, (afType == AF_INET6 ? T_AAAA : T_A), r_ttl);
	if(rrecp != NULL)
	{
		rrecp->r_opcode = r_opcode;
		rrecp->r_size = r_size;
		if(r_size)
			rrecp->r_data = (u_char *)malloc(r_size);
		strncpy((char *)rrecp->r_data, r_addr, r_size);

		res_ninit(res);
		if(debug)
			res->options |= RES_DEBUG;
		else
			res->options &= ~RES_DEBUG;
		rc = res_nupdate(res, rrecp, NULL);
	}
	if(rrecp->r_data != NULL)
		free(rrecp->r_data);
	res_freeupdrec(rrecp);

	return(rc);
}
