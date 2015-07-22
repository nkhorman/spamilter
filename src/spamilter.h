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
 *	CVSID:  $Id: spamilter.h,v 1.60 2014/09/01 18:25:28 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		spamilter.h
 *--------------------------------------------------------------------*/

#ifndef _SPAMILTER_H_
#define _SPAMILTER_H_

	#include "libmilter/mfapi.h"

	#define SYSLOGF LOG_LOCAL6
	#define LOGFAIL 1
	#define LOGREJ 1

	#include "config.h"

	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
#ifdef OS_FreeBSD
	#include <uuid.h>
#endif
	#include "dnsbl.h"

#ifdef SUPPORT_GEOIP
	#include "geoip.h" // our library api wrapper and support
#endif

	#include "list.h"
	#include "bwlist.h"
	#include "dbl.h"

	typedef struct _mlfiPriv
	{
		struct sockaddr	*pip;
		char		*ipstr;
		char		*iphostname;
		char		*header_xstatus;
		int		dnsblcount;
		char		*helo;
		char		*sndr;
		int		sndraction;
		int		sndrreject;
		char		*rcpt;
		int		rcptaction;
		char		*header_subject;
		bwlistCtx_t	*pbwlistctx;
		RBLLISTHOSTS	*pdnsrblhosts;
		RBLLISTMATCH	*pdnsrblmatch;
		int		dnsrblmatchqty;
		int		reject;
		char		rcptactionexec[1024];
		char		sndractionexec[1024];
		char		*body;
		int		bodylen;
		int		MsExtFound;
		int		SmtpSndrChkFail;
		int		islocalnethost;
		int		isAuthEn;
		res_state	presstate;
#if defined(SUPPORT_LIBSPF) || defined(SUPPORT_LIBSPF2)
		int		spf_rc;
		char		*spf_rs;
		char		*spf_error;
		char		*spf_explain;
#endif
		char		*badextlist;
		int		badextqty;
		char		*pdupercpt;
		int		fwdhostlistfd;
		int		heloaction;
		char		heloactionexec[1024];
		char		*header_replyto;
		char		*replyto;
		int		replytoaction;
		char		replytoactionexec[1024];
#ifdef SUPPORT_GEOIP
		const GeoIPDB	*pGeoipdb;
		GeoIP		*pGeoipCC;
		GeoIP		*pGeoipCity;
		int		fdGeoipBWL;
		list_t		*pGeoipList;
#endif
		list_t		*pListBodyHosts;
		char		*pSessionUuidStr;
		int		bodyTransferEncoding;
		dblCtx_t	*pDblCtx;
	} mlfiPriv;

	#define MLFIPRIV ((mlfiPriv *) smfi_getpriv(ctx))

	struct smfiDesc mlfi;		// forward declaration

	extern int gDebug;
	extern const char *gConfpath;		// /etc/spamilter.rc
	extern const char *gHostname;		// local hostname

	#define OPT_USERNAME			"UserName"			// Str - user name to run as
	#define OPT_POLICYURL			"PolicyUrl"			// Str - policy url
	#define OPT_DBPATH			"Dbpath"			// Str - /var/db/spamiter
	#define OPT_CONN			"Conn"				// Str - inet:7726@localhost
	#define OPT_DNSBLCHK			"DnsBlChk"			// Bool - dns blacklist checking
	#define OPT_SMTPSNDRCHK			"SmtpSndrChk"			// Bool - do smtp sender is deliverable checks
	#define OPT_SMTPSNDRCHKACTION		"SmtpSndrChkAction"		// Str - action to take when sender deliverable checks fail
	#define OPT_MTAHOSTCHK			"MtaHostChk"			// Bool - do MTA hostname checking
	#define OPT_MTAHOSTCHKASIP		"MtaHostChkAsIp"		// Bool - do MTA hostname as IP Address checking
	#define OPT_MTAHOSTIPFW			"MtaHostIpfw"			// Bool - do Ipfw inserts if MTA hostname check fails
	#define OPT_MTAHOSTIPFWNOMINATE		"MtaHostIpfwNominate"		// Bool - do Ipfw inserts based on connects per minute
	#define OPT_MTAHOSTIPCHK		"MtaHostIpChk"			// Bool - do MTA hostname to connect ip address resolution checking
	#define OPT_MTASPFCHK			"MtaSpfChk"			// Bool - do SPF MTA host checking using libspf
	#define OPT_MTASPFCHKSOFTFAILASHARD	"MtaSpfChkSoftFailAsHard"	// Bool - Reject on SPF SoftFail conditions
	#define OPT_MSEXTCHK			"MsExtChk"			// Int - do Microsoft File Extension vulnerablity checks with extension sub-set
	#define OPT_MSEXTCHKACTION		"MsExtChkAction"		// Str - action to take when vulnerable Microsoft attachemnts are found
	#define OPT_POPAUTHCHK			"PopAuthChk"			// Str - path/filename of pop-before-smtp berkely db
	#define OPT_VIRTUSERTABLECHK		"VirtUserTableChk"		// Bool - do virtusertable reject checking
	#define OPT_ALIASTABLECHK		"AliasTableChk"			// Bool - do aliastable reject checking
	#define OPT_LOCALUSERTABLECHK		"LocalUserTableChk"		// Bool - do localusertable reject checking
	#define OPT_GREYLISTCHK			"GreyListChk"			// Bool - do connecting mta ip address greylisting
	#define OPT_HEADERREPLYTOCHK		"HeaderReplyToChk"		// Bool - do header ReplyTo sender validation
	#define OPT_HEADERRECEIVEDCHK		"HeaderReceivedChk"		// Bool - do header Received ip checking
	#define OPT_RCPTFWDHOSTCHK		"RcptFwdHostChk"		// Bool - do check with an interior mta to validate the recipient
	#define OPT_GEOIPDBPATH			"GeoIPDBPath"			// Str - where to find the GeoIP address data base
	#define OPT_GEOIPCHK			"GeoIPChk"			// Bool - do GeoIP Country Code List action checking

	enum { MSE_A_TAG, MSE_A_REJECT };

	enum { BODYENCODING_NONE, BODYENCODING_UNKNOWN, BODYENCODING_BASE64 };

#endif
