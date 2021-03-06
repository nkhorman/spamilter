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
 *	CVSID:  $Id: uam.c,v 1.3 2005/12/15 06:14:15 neal Exp $
 *
 * DESCRIPTION:
 *	application:	ipfwmtad
 *	module:		uam.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: uam.c,v 1.3 2005/12/15 06:14:15 neal Exp $";

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef OS_Linux
#include <crypt.h>
#endif
#include <unistd.h>
#include <pwd.h>
#include <string.h>
#include "uam.h"

int uam_authuserpass(char *user, char *pass)
{	int		ok = 0;
	struct passwd	*pwd= NULL;

	if(user != NULL && pass != NULL)
	{
		setpwent();
		ok = ((pwd = getpwnam(user)) != NULL && strcmp((char *)crypt(pass,pwd->pw_passwd),pwd->pw_passwd) == 0);
		endpwent();
	}

	return(ok);
}

#ifdef _UNIT_TEST
main(int argc, char *argv[])
{
	if(argv[1] != NULL && argv[2] != NULL && getuid() == 0)
		printf("authenticate: %s = %s\n",argv[1],(uam_authuserpass(argv[1],argv[2]) ? "OK" : "Fail"));
	else
		printf("usaage: user password\n");
}
#endif
