 #--------------------------------------------------------------------*
 #
 # Developed by;
 #      Neal Horman - http://www.wanlink.com
 #
 # DESCRIPTION:
 #      Makefile - for man pages
 #
 #--------------------------------------------------------------------*

MANPAGES = spamilter.8 ipfwmtad.8 greydbd.8 dnsblupd.8

MANMODE= 0644
MANOWN= root
MANGRP=	wheel
MANDIR= /usr/local/man/man8

.SUFFIXES: .mm .8

all: ${MANPAGES}

install: ${MANPAGES}
	for i in ${MANPAGES}; do \
		rm -f ${MANDIR}/$${i} ${MANDIR}/$${i}.gz; \
		install -C -m ${MANMODE} -g ${MANGRP} -o ${MANOWN} $${i} ${MANDIR}; \
		gzip ${MANDIR}/$${i}; \
	done

uninstall:
	for i in ${MANPAGES}; do rm -f ${MANDIR}/$${i} ${MANDIR}/$${i}.gz; done

clean:
	for i in ${MANPAGES}; do rm -f $${i}; done

.mm.8: Makefile m4man.m4
	m4 m4man.m4 $< > $@

.PHONY: all install uninstall clean
