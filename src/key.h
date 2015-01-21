/*--------------------------------------------------------------------*
 *
 * Developed by;
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
 *	This code was derived from oppenssl's crypto/openssh/key.c module
 *
 *	CVSID:  $Id: key.h,v 1.4 2011/10/27 17:31:24 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		key.h
 *--------------------------------------------------------------------*/

#ifndef _SPAMILTER_KEY_H_
#define _SPAMILTER_KEY_H_

	#include <openssl/rsa.h>

	RSA *key_new();
	RSA *key_demote(RSA *privkey);
	void key_free(RSA *rsa);
	RSA *key_generate(int bits);

	int key_size(RSA *rsa);

	int key_read(RSA *rsa, unsigned char **cpp);
	int key_write(RSA *rsa, FILE *f);

	int key_encrypt(RSA *rsa, unsigned char *src, int srclen, unsigned char **dst, int *dstlen);
	int key_decrypt(RSA *rsa, unsigned char *src, int srclen, unsigned char **dst);

	unsigned char *key_encrypttoasc(RSA *rsa, unsigned char *pktsrc, int pktsrclen);
	unsigned char *key_decrypttoasc(RSA *rsa, unsigned char *pktsrc);

	int key_bn_encrypt(RSA *rsa, BIGNUM *in, BIGNUM *out);
	int key_bn_decrypt(RSA *rsa, BIGNUM *in, BIGNUM *out);

	unsigned char *key_bintoasc(unsigned char *src, int srclen);
	unsigned char *key_asctobin(unsigned char *src, int *srclen);

	unsigned char *key_pubkeytoasc(RSA *rsa);
#endif
