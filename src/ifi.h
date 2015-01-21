/*--------------------------------------------------------------------*
 *
 * Developed by;
 *	W. Richard Stevens - http://www.kohala.com
 *	Copyright (c) 1998 W. Richard Stevens. All Rights Reserved
 *
 *	This code is published at
 *	ftp://ftp.kohala.com/pub/rstevens/unpv12e.tar.gz as a result
 * 	of Richard's exelent work documenting the BSD TCP/IP network stack.
 *	One of which is;
 *	"Unix Network Programming - Network APIs: Sockets and XTI Volume 1"
 *
 *	I took this portion of the library and modified it. Richard does
 *	not appear to have placed the source under a specific license,
 *	although he does disclaim damages;
 *
 *	"        LIMITS OF LIABILITY AND DISCLAIMER OF WARRANTY
 *	
 *	The author and publisher of the book "UNIX Network Programming" have
 *	used their best efforts in preparing this software.  These efforts
 *	include the development, research, and testing of the theories and
 *	programs to determine their effectiveness.  The author and publisher
 *	make no warranty of any kind, express or implied, with regard to
 *	these programs or the documentation contained in the book. The author
 *	and publisher shall not be liable in any event for incidental or
 *	consequential damages in connection with, or arising out of, the
 *	furnishing, performance, or use of these programs."
 *
 *	Sadly, we will see no more books published by him.
 *
 *	
 * Modified by;
 *	Neal Horman - http://www.wanlink.com
 *	Portions Copyright (c) 2003 Neal Horman. All Rights Reserved
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
 *	CVSID:  $Id: ifi.h,v 1.9 2012/05/04 00:14:06 neal Exp $
 *
 * DESCRIPTION:
 *	application:	spamilter
 *	module:		ifi.h
 *--------------------------------------------------------------------*/

#ifndef	_SPAMILTER_IFI_H_
#define	_SPAMILTER_IFI_H_

	#define	IFI_NAME	16			/* same as IFNAMSIZ in <net/if.h> */
	#define	IFI_HADDR	8			/* allow for 64-bit EUI-64 in future */
	#define	IFI_ALIAS	1			/* ifi_addr is an alias */

	struct ifi_info
	{
		char ifi_name[IFI_NAME];		/* interface name, null terminated */
		u_char ifi_haddr[IFI_HADDR];		/* hardware address */
		u_short ifi_hlen;			/* #bytes in hardware address: 0, 6, 8 */
		short ifi_flags;			/* IFF_xxx constants from <net/if.h> */
		short ifi_myflags;			/* our own IFI_xxx flags */
		struct sockaddr  *ifi_addr;		/* primary address */
		struct sockaddr  *ifi_brdaddr;		/* broadcast address */
		struct sockaddr  *ifi_dstaddr;		/* destination address */
#ifdef SIOCGIFNETMASK
		struct sockaddr  *ifi_netmask;		/* netmask */
#endif
		struct ifi_info  *ifi_next;		/* next of these structures */
	};


	/* function prototypes */
	struct ifi_info	*get_ifi_info(int, int);
	void free_ifi_info(struct ifi_info *);

	int ifi_islocalip(long ip);
	int ifi_islocalnet(long ip);

#endif
