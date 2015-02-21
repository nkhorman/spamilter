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
 *	CVSID:  $Id: regexmisc.c,v 1.14 2012/12/12 02:38:35 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		regexmisc.c
 *--------------------------------------------------------------------*/

static char const cvsid[] = "@(#)$Id: regexmisc.c,v 1.14 2012/12/12 02:38:35 neal Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "misc.h"
#include "regexapi.h"
#include "list.h"


/*
http://regexlib.com/Search.aspx?k=ipv4&AspxAutoDetectCookieSupport=1
http://regexlib.com/REDetails.aspx?regexp_id=2690
	(^ \d{20} $)
	| (^ ((:[a-fA-F0-9]{1,4}){6}|::) ffff:(25[0-5] | 2[0-4][0-9] | 1[0-9][0-9] | [0-9]{1,2})(\.(25[0-5] | 2[0-4][0-9] | 1[0-9][0-9] | [0-9]{1,2})){3} $)
	| (^ ((:[a-fA-F0-9]{1,4}){6} | ::) ffff (:[a-fA-F0-9]{1,4}){2} $)
	| (^ ([a-fA-F0-9]{1,4}) (:[a-fA-F0-9]{1,4}){7} $)
	| (^ :(:[a-fA-F0-9]{1,4}(::)?){1,6} $)
	| (^ ((::)?[a-fA-F0-9]{1,4}:){1,6}: $)
	| (^ :: $)

http://stackoverflow.com/questions/53497/regular-expression-that-matches-valid-ipv6-addresses

*/

unsigned long mlfi_regex_ipv4(const char *pstr)
{	unsigned long ip = 0;

	if(ip == 0)
	{
		#define PAT_IPV4A "([0-9]{1,3})[.]([0-9]{1,3})[.]([0-9]{1,3})[.]([0-9]{1,3})"
		const char *regstrs[] = // must be in most specific to least specific order
		{
			// bla [ipv4] (bla [ipv4]) bla
			".{1,}[(].{1,}[ ][[]"PAT_IPV4A"[]][ ]{0,}[)]",
			// bla [ipv4]
			".{1,}[[]"PAT_IPV4A"[]]",
			// bla (ipv4)
			".{1,}[(]"PAT_IPV4A"[)]",
			// ipv4
			"[ ]{0,}"PAT_IPV4A"[ ]{0,}",
		};
		unsigned int i,match;

		for(i=0,match=0; !match && i<sizeof(regstrs)/sizeof(regstrs[0]); i++)
		{	regexapi_t *prat = regexapi_exec(pstr,regstrs[i],REGEX_DEFAULT_CFLAGS,1);
			int j,q = regexapi_nsubs(prat,0);

			match = (regexapi_matches(prat) && q == 4);
			for(j=0; match && j<q; j++)
			{	const char *p = regexapi_sub(prat,0,j);

				//printf("sub %d: '%s'\n",i+1,p);
				ip = ip << 8;
				ip |= (unsigned char)atoi(p);
			}
			regexapi_free(prat);
		}
	}

	if(ip == 0)
	{
		#define PAT_HOSTNAME "[a-zA-Z0-9][-a-zA-z0-9.]{1,}[.][a-zA-Z]{2,3}"
		const char *regstrs[] = // must be in most specific to least specific order
		{
			// from|by [hostname] bla
			"(from|by)[ \t]{1,}("PAT_HOSTNAME")[ \t]{1,}",
		};
		unsigned int i;

		for(i=0; ip == 0 && i<sizeof(regstrs)/sizeof(regstrs[0]); i++)
		{	regexapi_t *prat = regexapi_exec(pstr,regstrs[i],REGEX_DEFAULT_CFLAGS,1);
			const char *psub = (regexapi_nsubs(prat,0) > 1 ? regexapi_sub(prat,0,1) : NULL);
			struct hostent *phostent = (psub != NULL ? gethostbyname(psub) : NULL);

			ip = (phostent != NULL ? ntohl(*(unsigned long *)phostent->h_addr) : 0);

			regexapi_free(prat);
		}

	}

	return ip;
}

// The concept of this is similar to printf()
// You pass it an arbitrary string with the following substitute specifiers;
//	%m - replace with a regex suitable for the Mbox portion of an email address
//	%d - replace with a regex suitable for the Domain portion of an email address
//	%% - replace with a literal instance of %
// Unknown substitue specifiers are passed through, untouched.
// All other characters in the formatting string are copied without substitution.
//
// A special behavior of this routine is note the order in which it found the
// specific specifiers. ie. The first one found will have an index of 1, and the
// second one will have and index of 2, etc...
//
// This is convienent for use in regex_sub(.., index) for retriving the content
// of the sub expression relative to the occurance of the specifier in the
// formatting string.
//
// Returns;
//	a calloc'd string that the consumer must free after use.
//	pMboxIndex - the +1 index of the regex sub expression that matches the Mbox
//	pDomainIndex - the + 1 index of the regex sub expression that matches the Domain
//
static char *mboxRegexPrintf(const char *pFmt, int *pMboxIndex, int *pDomainIndex)
{	size_t l = (pFmt != NULL ? strlen(pFmt) : 0);
	char *pStr = (l > 0 ? calloc(1,l+1) : NULL);

	if(pStr != NULL)
	{	char *pTmp = pStr;

		#define PAT_MBOX "[a-zA-Z0-9][a-zA-Z0-9!#$%&'*+-/=?^_`{|}~.]"
		#define PAT_DOMAIN "[a-zA-Z0-9][a-zA-Z0-9._-]{0,}[.][a-zA-Z]{2,}"

		const char *pPatMbox = PAT_MBOX;
		const char *pPatDomain = PAT_DOMAIN;
		int index = 1;

		while(*pFmt)
		{
			switch(*pFmt)
			{
				case '%':
					switch(*(++pFmt))
					{
						case '%':
							*(pTmp++) = *pFmt;
							break;
						case 'm':
							l += strlen(pPatMbox)+2;
							pStr = realloc(pStr,l);
							if(pStr != NULL)
							{
								strcat(pStr,"(");
								strcat(pStr,pPatMbox);
								strcat(pStr,")");
								pTmp = (pStr + strlen(pStr));
								if(pMboxIndex != NULL)
									*pMboxIndex = (index++);
							}
							break;
						case 'd':
							l += strlen(pPatDomain)+2;
							pStr = realloc(pStr,l);
							if(pStr != NULL)
							{
								strcat(pStr,"(");
								strcat(pStr,pPatDomain);
								strcat(pStr,")");
								pTmp = (pStr + strlen(pStr));
								if(pDomainIndex != NULL)
									*pDomainIndex = (index++);
							}
							break;
						default: // pass all unknown substitute specifiers through
							*(pTmp++) = '%';
							*(pTmp++) = *pFmt;
							break;
					}
					break;
				default:
					*(pTmp++) = *pFmt;
					break;
			}
			pFmt++;
			*pTmp = 0;
		}
	}

	return pStr;
}

// Try to match pMboxDomain against pRegex, if so, split Mbox from Domain
// and strdup() them into ppMbox and ppDomain for the consumer.
int mlfi_regex_mboxtwist(const char *pMboxDomain, const char *pRegex, char **ppMbox, char **ppDomain)
{	int match = 0;

	if(pMboxDomain != NULL && pRegex != NULL)
	{
		int mboxIndex = 0;
		int domainIndex = 0;
		char *regstr = mboxRegexPrintf(pRegex,&mboxIndex,&domainIndex); // build a regex
		regexapi_t *prat = (regstr != NULL ? regexapi_exec(pMboxDomain,regstr,REGEX_DEFAULT_CFLAGS,1) : NULL); // test the regex

		// validate the regex formatting, and the results of the regex test
		match = (mboxIndex > 0 && domainIndex > 0 && prat != NULL && regexapi_matches(prat) && regexapi_nsubs(prat,0) == 2);

		// strdup() the split Mbox and Domain compoments of the email address
		// NB. we always return a valid pointer to a string, even if it is empty (as long as strdup() succeeds)
		if(ppMbox != NULL)
			*ppMbox = strdup(match ? regexapi_sub(prat,0,mboxIndex-1) : "");
		if(ppDomain != NULL)
			*ppDomain = strdup(match ? regexapi_sub(prat,0,domainIndex-1) : "");

		// overhead clean-up
		if(regstr != NULL)
			free(regstr);
		if(prat != NULL)
			regexapi_free(prat);
	}

	return match;
}

// Iterate over pMboxDomain with each of the regex patterns to find a valid
// email address, and when found, split the Mbox from the Domain and strdup()
// them into ppMbox and ppDomain for the consumer.
int mlfi_regex_mboxsplit(const char *pMboxDomain, char **ppMbox, char **ppDomain)
{	int match = 0;

	if(pMboxDomain != NULL && ppMbox != NULL && ppDomain != NULL)
	{
		const char *regstrs[] = // must be in most specific to least specific order
		{
			".*<%m@%d>.*", // "bla<Mbox@Domain>bla"
			".*[ ]{1,}%m@%d[ ]{1,}.*", // "bla Mbox@Domain bla"
			"%m@%d", // "Mbox@Domain"
		};
		unsigned int i;

		// help to clean-up (not leak) previous uses
		// of the pointers that are passed into us
		if(*ppMbox != NULL)
		{
			free(*ppMbox);
			*ppMbox = NULL;
		}
		if(*ppDomain != NULL)
		{
			free(*ppDomain);
			*ppDomain = NULL;
		}

		// test all of our regex patterns for a match, and return the split-up
		// duplicates of the Mbox and Domain portions of the regex to the caller
		for(i=0,match=0; !match && i<sizeof(regstrs)/sizeof(regstrs[0]); i++)
			match = mlfi_regex_mboxtwist(pMboxDomain,regstrs[i],ppMbox,ppDomain);
	}

	return match;
}

typedef struct _listSearch_t
{
	int found;
	const char *pNeedle;
	const char *pSessionId;
}listSearch_t;

static int listCallbackSearch(void *pData, void *pCallbackData)
{	int again = 1;
	const char *pHaystack = (const char *)pData;
	listSearch_t *pSearch = (listSearch_t *)pCallbackData;

	if(pHaystack != NULL && pSearch != NULL)
	{
		pSearch->found = (strcasecmp(pHaystack,pSearch->pNeedle) == 0);
		//mlfi_debug(pSearch->pSessionId,"listCallbackSearch: found %u '%s' %c= '%s'",pSearch->found,pHaystack,(pSearch->found ? '=' : '!'),pSearch->pNeedle);
		again = (pSearch->found == 0);
	}

	return again;
}

void mlfi_regex_line_http(const char *pSessionId, const char *pbuf, list_t *pListHosts)
{
	#define PAT_IPV4B "[0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3}[.][0-9]{1,3}"
	//#define PAT_HOSTNAME "[a-zA-Z0-9][-a-zA-z0-9.]{1,}[.][a-zA-Z]{2,3}"

	if(pbuf != NULL)
	{	char *pat1 = "(http|ftp|https)://("PAT_HOSTNAME"|"PAT_IPV4B")(:[0-9]{1,}/|/|[ \t\r\n]|$)";
		regexapi_t *prat = regexapi_exec(pbuf,pat1,REGEX_DEFAULT_CFLAGS,REGEX_FIND_ALL);
			
		if(prat != NULL)
		{	int i,iq = regexapi_matches(prat);

			//mlfi_debug(pSessionId,"mlfi_regex_line_http: %u matches found in body '%s'",iq,pbuf);
			// iterate matches
			for(i=0; i<iq; i++)
			{
				// if match has enough sub-compoments
				if(regexapi_nsubs(prat,i) >= 2)
				{	const char *p = (prat != NULL ? regexapi_sub(prat,i,1) : NULL);

					// don't add the needle to the list of hosts that we've already collected
					if(p != NULL)
					{	listSearch_t ls = {0,p,pSessionId};

						// find needle in list
						listForEach(pListHosts,&listCallbackSearch,&ls);

						// add needle to list ?
						if(!ls.found)
						{	int ok = listAdd(pListHosts,(void *)strdup(p));

							mlfi_debug(pSessionId,"mlfi_regex_line_http: adding '%s' to body list %d\n",p,ok);
						}
					}
				}
			}
			regexapi_free(prat);
		}
	}
}

#ifdef _UNIT_TEST_REGEXMISC
void usage(void)
{
	printf(
		"-i [ip address] - test and ip addres to see if it passes an ipv4 regex for use with \"Recived by ...\" headers\n"
		"-e [email address] - test and email address to see if it passes an email regex\n"
	);
}

int main(int argc, char **argv)
{	int c;

	if(argc < 2)
	{
		usage();
		exit(0);
	}

	while ((c = getopt(argc, argv, "i:e:")) != -1)
	{
		switch (c)
		{
			case 'i':
				if(optarg != NULL && *optarg)
				{	unsigned long ip = mlfi_regex_ipv4(optarg);

					printf("'%s' = %lu.%lu.%lu.%lu\n"
						,optarg
						,(ip&0xff000000)>>24 ,(ip&0x00ff0000)>>16 ,(ip&0x0000ff00)>>8 ,(ip&0x000000ff)
						);
				}
				else
					printf("Bogus/Missing command line arguement\n");
				break;
			case 'e':
				if(optarg != NULL && *optarg)
				{	char	*frm = NULL;
					char	*dom = NULL;

					if(mlfi_regex_mboxsplit(optarg,&frm,&dom))
					{
						printf("split: '%s'@'%s'\n",frm,dom);
						if(frm != NULL)
							free(frm);
						if(dom != NULL)
							free(dom);
					}
					else
						printf("regex fail\n");
				}
				else
					printf("Bogus/Missing command line arguement\n");
				break;
		}
	}

	argc -= optind;
	argv += optind;

	return 0;
}
#endif
