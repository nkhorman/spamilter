#!/bin/sh

if [ ! -x usr.sbin/sendmail/Makefile.orig ]; then
echo "Patching Sendmail Makefile"
cat <<EOF > usr.sbin/sendmail/Makefile.patch
*** Makefile.orig	Fri Aug  3 15:51:50 2001
--- Makefile	Mon Mar 17 19:06:39 2003
***************
*** 42,47 ****
--- 42,60 ----
  DPADD+=	${LIBSMUTIL}
  LDADD+=	${LIBSMUTIL}
  
+ CFLAGS+= -D_FFR_MILTER
+ 
+ .if exists(${.OBJDIR}/../../lib/libmilter)
+ LIBMILTERDIR:=	${.OBJDIR}/../../lib/libmilter
+ .else
+ LIBMILTERDIR!=	cd ${.CURDIR}/../../lib/libmilter; make -V .OBJDIR
+ .endif
+ LIBMILTER:=	${LIBMILTERDIR}/libmilter.a
+ 
+ DPADD+=	${LIBMILTER}
+ LDADD+=	${LIBMILTER}
+ 
+ 
  .if exists(${.CURDIR}/../../secure) && !defined(NOCRYPT) && \
  	!defined(NOSECURE) && !defined(NO_OPENSSL) && \
  	!defined(RELEASE_CRUNCH)
EOF
fi

if [ ! -x lib/libmilter ]; then mkdir lib/libmilter; fi
echo "Creating lib/libmilter directory"
if [ ! -x lib/libmilter/Makefile; then
echo "Creating lib/libmilter/Makefile";
cat <<EOF > lib/libmilter/Makfile
SENDMAIL_DIR=${.CURDIR}/../../contrib/sendmail
.PATH:	${SENDMAIL_DIR}/libmilter

CFLAGS+=-I${SENDMAIL_DIR}/src -I${SENDMAIL_DIR}/include
CFLAGS+=-DNEWDB -DNIS -DMAP_REGEX -DNOT_SENDMAIL -D_FFR_MILTER

# User customizations to the sendmail build environment
CFLAGS+=${SENDMAIL_CFLAGS}

LIB=	milter

SRCS+=	comm.c engine.c handler.c listener.c main.c signal.c sm_gethost.c smfi.c

#INTERNALLIB=		true
#NOPIC=			true
#INTERNALSTATICLIB=	true

.include <bsd.lib.mk>
EOF
fi

srcdirs="./lib/libmilter ./lib/libsmutil ./lib/libsmdb ./usr.sbin/sendmail ./libexec/mail.local ./usr.sbin/mailstats ./usr.sbin/makemap ./usr.sbin/praliases ./bin/rmail ./libexec/smrsh ./usr.bin/vacation"

for action in "all install clean"; do
	for srcdir in ${srcdirs}; do
		cd ${srcdir}
		make ${action} 
		cd ../..
	done
done
