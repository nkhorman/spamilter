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
	extern char *gPolicyUrl;		// policy url
	extern char *gDbpath;			// /var/db/spamiter
	extern char *gConfpath;			// /etc/spamilter.rc
	extern char *gMlfiConn;			// inet:7726@localhost
	extern int gDnsBlChk;			// dns blacklist checking
	extern int gSmtpSndrChk;		// do smtp sender is deliverable checks
	extern char *gSmtpSndrChkAction;	// action to take when sender deliverable checks fail
	extern int gMtaHostChk;			// do MTA hostname checking
	extern int gMtaHostChkAsIp;		// do MTA hostname as IP Address checking
	extern int gMtaHostIpfw;		// do Ipfw inserts if MTA hostname check fails
	extern int gMtaHostIpfwNominate;	// do Ipfw inserts based on connects per minute
	extern int gMtaHostIpChk;		// do MTA hostname to connect ip address resolution checking
	extern int gMsExtChk;			// do Microsoft File Extension vulnerablity checks
	extern char *gMsExtChkAction;		// action to take when vulnerable Microsoft attachemnts are found
	extern char *gHostname;			// local hostname
#ifdef SUPPORT_POPAUTH
	extern char *gPopAuthChk;		// path/filename of pop-before-smtp berkely db
#endif
#if defined(SUPPORT_LIBSPF) || defined(SUPPORT_LIBSPF2)
	extern int gMtaSpfChk;			// do SPF MTA host checking using libspf
	extern int gMtaSpfChkSoftFailAsFail;	// Reject on SPF SoftFail conditions
#endif
#ifdef SUPPORT_VIRTUSER
	extern char *gVirtUserTableChk;		// do virtusertable reject checking
#endif
#ifdef SUPPORT_ALIASES
	extern char *gAliasTableChk;		// do aliastable reject checking
#endif
#ifdef SUPPORT_LOCALUSER
	extern int gLocalUserTableChk;		// do localusertable reject checking
#endif
#ifdef SUPPORT_GREYLIST
	extern int gGreyListChk;
#endif
#ifdef SUPPORT_FWDHOSTCHK
	extern int gRcptFwdHostChk;
#endif
	extern int gHeaderChkReplyTo;		// do header ReplyTo checking
	extern int gHeaderChkReceived;		// do header Received ip checking
#ifdef SUPPORT_GEOIP
	extern char *gpGeoipDbPath;		// path to the GeoIP data base dir
	extern int gGeoIpCcChk;			// do GeoIP Country Code List action checking
#endif

	enum { MSE_A_TAG, MSE_A_REJECT };

	enum { BODYENCODING_NONE, BODYENCODING_UNKNOWN, BODYENCODING_BASE64 };

#endif
