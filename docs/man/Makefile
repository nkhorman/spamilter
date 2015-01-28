 #--------------------------------------------------------------------*
 #
 # Developed by;
 #      Neal Horman - http://www.wanlink.com
 #
 # DESCRIPTION:
 #      Makefile - for man pages
 #
 #--------------------------------------------------------------------*

MANPAGES = spamilter.8 ipfwmtad.8 greydbd.8

MANMODE= 0644
MANOWN= root
MANGRP=	wheel
MANDIR= /usr/local/man/man8

all:

install: ${MANPAGES}
	for i in ${MANPAGES}; do \
		install -c -m ${MANMODE} -g ${MANGRP} -o ${MANOWN} $${i} ${MANDIR}; \
		gzip ${MANDIR}/$${i}; \
	done

uninstall:
	for i in ${MANPAGES}; do rm -f ${MANDIR}/$${i} ${MANDIR}/$${i}.gz; done

.PHONY: all install uninstall
