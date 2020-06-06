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
 *	CVSID:  $Id: key.c,v 1.4 2011/10/27 17:31:24 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		key.c
 *--------------------------------------------------------------------*/

/*
 * read_bignum():
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *
 * As far as I am concerned, the code I have written for this software
 * can be used freely for any purpose.  Any derived versions of this
 * software must be clearly marked as such, and if the derived work is
 * incompatible with the protocol description in the RFC file, it must be
 * called by a name other than "ssh" or "Secure Shell".
 *
 *
 * Copyright (c) 2000, 2001 Markus Friedl.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

static char const cvsid[] = "@(#)$Id: key.c,v 1.4 2011/10/27 17:31:24 neal Exp $";

#include <openssl/rsa.h>
#include <openssl/opensslv.h>
#include <openssl/rand.h>

#include <string.h>

#include "config.h"
#include "key.h"

#ifndef min
#define min(a,b) ((a) <= (b) ? (a) : (b))
#endif

// https://www.openssl.org/docs/man1.1.1/man3/RSA_get0_key.html
RSA *key_new()
{	RSA	*rsa = RSA_new();
#if OPENSSL_VERSION_NUMBER > 0x010100000L
	int	ok = (rsa != NULL && RSA_set0_key(rsa, BN_new(), BN_new(), NULL) == 1);
#else
	int	ok = 0;

	if(rsa != NULL)
	{
		rsa->n = BN_new();
		rsa->e = BN_new();
	}

	ok = (rsa != NULL && rsa->n != NULL && rsa->e != NULL);
#endif

	if(!ok)
		key_free(&rsa);

	return(ok ? rsa : NULL);
}

/*
RSA *key_demote(RSA *privkey)
{	RSA	*pubkey = key_new();

	if(pubkey != NULL)
	{
#if OPENSSL_VERSION_NUMBER > 0x010100000L
		BIGNUM *rsaN = NULL;
		BIGNUM *rsaE = NULL;

		RSA_get0_key(privkey, (const BIGNUM **)&rsaN, (const BIGNUM **)&rsaE, NULL);
		RSA_set0_key(pubkey, BN_dup(rsaN), BN_dup(rsaE), NULL);
#else
		pubkey->e = BN_dup(privkey->e);
		pubkey->n = BN_dup(privkey->n);
#endif
	}

	return(pubkey);
}
*/

void key_free(RSA **rsa)
{
	if(rsa != NULL)
	{
		if(*rsa != NULL)
			RSA_free(*rsa);
	}
}

/*
 * Reads a multiple-precision integer in decimal from the buffer, and advances
 * the pointer.  The integer must already be initialized.  This function is
 * permitted to modify the buffer.  This leaves *cpp to point just beyond the
 * last processed (and maybe modified) character.  Note that this may modify
 * the buffer containing the number.
 */
int read_bignum(unsigned char **cpp, BIGNUM * value)
{
	unsigned char *cp = *cpp;
	int old;

	/* Skip any leading whitespace. */
	for (; *cp == ' ' || *cp == '\t'; cp++)
		;

	/* Check that it begins with a decimal digit. */
	if (*cp < '0' || *cp > '9')
		return 0;

	/* Save starting position. */
	*cpp = cp;

	/* Move forward until all decimal digits skipped. */
	for (; *cp >= '0' && *cp <= '9'; cp++)
		;

	/* Save the old terminating character, and replace it by \0. */
	old = *cp;
	*cp = 0;

	/* Parse the number. */
	if (BN_dec2bn(&value, (char *)*cpp) == 0)
		return 0;

	/* Restore old terminating character. */
	*cp = old;

	/* Move beyond the number and return success. */
	*cpp = cp;
	return 1;
}

int write_bignum(FILE *f, BIGNUM *num)
{	char	*buf = BN_bn2dec(num);
	int	ok =(buf != NULL);

	if (buf != NULL)
	{
		fprintf(f, " %s", buf);
		OPENSSL_free(buf);
	}

	return(ok);
}

void key_getRsaNE(RSA *rsa, BIGNUM **rsaN, BIGNUM **rsaE)
{
	if(rsa != NULL)
	{
#if OPENSSL_VERSION_NUMBER > 0x010100000L
		RSA_get0_key(rsa, (const BIGNUM **)rsaN, (const BIGNUM **)rsaE, NULL);
#else
		if(rsaN != NULL)
			*rsaN = rsa->n;
		if(rsaE != NULL)
			*rsaE = rsa->e;
#endif
	}
}

BIGNUM *key_getRsaN(RSA *rsa)
{
	BIGNUM *bn = NULL;

	key_getRsaNE(rsa, &bn, NULL);

	return bn;
}

BIGNUM *key_getRsaE(RSA *rsa)
{
	BIGNUM *bn = NULL;

	key_getRsaNE(rsa, NULL, &bn);

	return bn;
}

int key_read(RSA *rsa, unsigned char **cpp)
{	int	success = 0;
	unsigned char	*cp = *cpp;
	int	bits;
	BIGNUM	*rsaN = NULL;
	BIGNUM	*rsaE = NULL;

	// Get number of bits.
	for (bits = 0; *cp >= '0' && *cp <= '9'; cp++)
		bits = 10 * bits + *cp - '0';
	*cpp = cp;

	// Get public exponent, public modulus.
	key_getRsaNE(rsa, &rsaN, &rsaE);
	success = (bits > 0 && rsaE != NULL && rsaN != NULL && read_bignum(cpp, rsaE) && read_bignum(cpp, rsaN));

	return(success);
}

int key_size(RSA *rsa)
{
	return BN_num_bits(key_getRsaN(rsa));
}

int key_write(RSA *rsa, FILE *f)
{
	BIGNUM	*rsaN = NULL;
	BIGNUM	*rsaE = NULL;

	key_getRsaNE(rsa, &rsaN, &rsaE);
	fprintf(f, "rsa %u", key_size(rsa));

	return(write_bignum(f, rsaE) && write_bignum(f, rsaN));
}

RSA *key_generate(int bits)
{
// https://www.openssl.org/docs/man1.1.1/man3/RSA_generate_key_ex.html
// https://www.dynamsoft.com/codepool/how-to-use-openssl-generate-rsa-keys-cc.html
#if OPENSSL_VERSION_NUMBER > 0x010100000L
	RSA *rsa = RSA_new();
	BIGNUM *rsaE = BN_new();
	int ok = 0;

	if(rsaE != NULL)
		ok = (BN_set_word(rsaE, RSA_F4) == 1);
	ok = (ok && rsa != NULL ? RSA_generate_key_ex(rsa, bits, rsaE, NULL) == 1 : 0);

	if(ok != 1)
	{
		key_free(&rsa);
		if(rsaE != NULL)
			BN_free(rsaE);
	}

	return rsa;
#else
	return RSA_generate_key(bits, 65537, NULL, NULL);
#endif
}

unsigned char *key_pubkeytoasc(RSA *rsa)
{	char	*dst = NULL;

	if(rsa != NULL)
	{
		BIGNUM	*rsaN = NULL;
		BIGNUM	*rsaE = NULL;
		char	*buf1 = NULL;
		char	*buf2 = NULL;

		key_getRsaNE(rsa, &rsaN, &rsaE);
		buf1 = BN_bn2dec(rsaE);
		buf2 = BN_bn2dec(rsaN);

		asprintf(&dst, "rsa %u %s %s", key_size(rsa), buf1, buf2);

		OPENSSL_free(buf1);
		OPENSSL_free(buf2);
	}

	return (unsigned char *)dst;
}

int key_encrypt(RSA *rsa, unsigned char *src, int srclen, unsigned char **dst, int *dstlen)
{ 	int	len = (src != NULL ? min(srclen,RSA_size(rsa)-42) : 0);
	int	sz = RSA_size(rsa);

	*dst = calloc(1,sz);
	if(*dst != NULL)
		*dstlen = RSA_public_encrypt(len,src,*dst,rsa,RSA_PKCS1_OAEP_PADDING);

	return(*dstlen > 0 ? len : 0);
}

int key_bn_encrypt(RSA *rsa, BIGNUM *in, BIGNUM *out)
{	int	len;
	int	ilen	= BN_num_bytes(in);
	int	olen	= BN_num_bytes(key_getRsaN(rsa));
	unsigned char	*ibuf	= calloc(1,ilen);
	unsigned char	*obuf	= calloc(1,olen);

	BN_bn2bin(in, ibuf);

	if((len = RSA_public_encrypt(ilen,ibuf,obuf,rsa,RSA_PKCS1_PADDING)) > 0)
		BN_bin2bn(obuf,len,out);

	memset(obuf,0,olen);
	memset(ibuf,0,ilen);
	free(obuf);
	free(ibuf);

	return(len);
}

int key_bn_decrypt(RSA *rsa, BIGNUM *in, BIGNUM *out)
{	int	len;
	int	ilen	= BN_num_bytes(in);
	int	olen	= BN_num_bytes(key_getRsaN(rsa));
	unsigned char	*ibuf	= calloc(1,ilen);
	unsigned char	*obuf	= calloc(1,olen);

	BN_bn2bin(in, ibuf);

	if((len = RSA_private_decrypt(ilen,ibuf,obuf,rsa,RSA_PKCS1_PADDING)) > 0)
		BN_bin2bn(obuf,len,out);

	memset(obuf,0,olen);
	memset(ibuf,0,ilen);
	free(obuf);
	free(ibuf);

	return(len);
}

int key_decrypt(RSA *rsa, unsigned char *src, int srclen, unsigned char **dst)
{	int	len = 0;

	*dst = calloc(1,RSA_size(rsa)-1);
	if(*dst != NULL)
		len = RSA_private_decrypt(srclen,src,*dst,rsa,RSA_PKCS1_OAEP_PADDING);

	if(len < 1)
	{
		free(*dst);
		*dst = NULL;
	}

	return(len);
}

unsigned char key_btoa(unsigned char x)
{	unsigned char	c = x + '0';

	if(x>9)
		c = x + 'A' - 10;

	return(c);
}

unsigned char key_atob(unsigned char x)
{	unsigned char	c = x - '0';

	if(x > '9')
		c = x - 'A' + 10;

	return(c);
}

unsigned char *key_bintoasc(unsigned char *src, int srclen)
{	unsigned char	*dst = calloc(1,(srclen*2) + 1);

	if(dst != NULL)
	{	int	i;
		unsigned char	*d = dst;

		for(i=0; i<srclen; i++)
		{	unsigned char	c;

			c = *(src++);
			*(d++) = key_btoa(c&0x0f);
			*(d++) = key_btoa((c&0xf0)>>4);
		}
		*d = '\0';
	}

	return(dst);
}

unsigned char *key_asctobin(unsigned char *src, int *srclen)
{	unsigned char	*dst = calloc(1,strlen((char *)src)/2);

	if(dst != NULL)
	{	int	i;
		unsigned char	*d = dst;

		*srclen = strlen((char *)src)/2;

		for(i=0; *src; i++)
		{
			*d = key_atob(*(src++));
			*d |= (key_atob(*(src++)) << 4);
			d++;
		}
	}

	return(dst);
}

unsigned char *key_encrypttoasc(RSA *rsa, unsigned char *pktsrc, int pktsrclen)
{	unsigned char	*dst = NULL;
	unsigned char	*pktdst = NULL;
	int	pktdstlen = 0;

	if(pktsrc != NULL && rsa != NULL && key_encrypt(rsa,pktsrc,strlen((char *)pktsrc),&pktdst,&pktdstlen))
		dst = key_bintoasc(pktdst,pktdstlen);

	if(pktdst != NULL)
		free(pktdst);

	return(dst);
}

unsigned char *key_decrypttoasc(RSA *rsa, unsigned char *pktsrc)
{	unsigned char	*dst = NULL;

	if(rsa != NULL && pktsrc != NULL && strlen((char *)pktsrc))
	{	int	encrndlen = 0;
		unsigned char	*pencrnd = key_asctobin(pktsrc,&encrndlen);
		unsigned char	*pdecrnd = NULL;
		int	decrndlen = 0;

		if((decrndlen = key_decrypt(rsa,pencrnd,encrndlen,&pdecrnd)) != -1)
			dst = key_bintoasc(pdecrnd,decrndlen);

		if(pencrnd != NULL)
			free(pencrnd);
		if(pdecrnd != NULL)
			free(pdecrnd);
	}

	return(dst);
}
