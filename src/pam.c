/*--------------------------------------------------------------------*
 *
 * Developed by;
 *	Neal Horman - http://www.wanlink.com
 *	Copyright (c) 2002 Neal Horman. All Rights Reserved
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
 *	CVSID:  $Id: pam.c,v 1.4 2005/12/15 06:13:52 neal Exp $
 *
 * DESCRIPTION:
 *	application:	ipfwmtad
 *	module:		pam.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: pam.c,v 1.4 2005/12/15 06:13:52 neal Exp $";

#include <security/pam_appl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "pam.h"

int pam_chat(int n, const struct pam_message **msg, struct pam_response **resp, void *data)
{	struct pam_response *aresp = (struct pam_response *)calloc(n,sizeof *aresp);
	int	rc = PAM_CONV_ERR;

	if(n == 1 && aresp != NULL && msg[0]->msg_style == PAM_PROMPT_ECHO_OFF && data != NULL)
	{
		aresp[0].resp = strdup(data);
		rc = PAM_SUCCESS;
	}

	if(rc == PAM_SUCCESS)
		*resp = aresp;
	else if(aresp != NULL)
		free(aresp);

	return(rc);
}

int pam_authuserpass(char *user, char *pass)
{	int retval = 0;
	pam_handle_t *pamh = NULL;
	struct pam_conv pamc = {&pam_chat,pass};
	int ok = (
		(retval = pam_start("check user",user,&pamc,&pamh)) == PAM_SUCCESS &&
		(retval = pam_authenticate(pamh, 0)) == PAM_SUCCESS &&
		(retval = pam_acct_mgmt(pamh, 0)) == PAM_SUCCESS
		);

	pam_end(pamh,retval);

	return(ok);
}

#ifdef _UNIT_TEST
main(int argc, char *argv[])
{
	if(argv[1] != NULL && argv[2] != NULL)
		printf("authenticate: %s = %s\n",argv[1],(pam_authuserpass(argv[1],argv[2]) ? "OK" : "Fail"));
	else
		printf("usage: user password\n");
}
#endif
