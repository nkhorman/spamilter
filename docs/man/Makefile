 #--------------------------------------------------------------------*
 #
 # Developed by;
 #      Neal Horman - http://www.wanlink.com
 #
 # DESCRIPTION:
 #      Makefile - for man pages
 #
 #--------------------------------------------------------------------*

MANPAGES = spamilter.8 ipfwmtad.8 greydbd.8 dnsblupd.8 dnsblchk.8 spamilter.conf.5

MANMODE= 0644
MANOWN= root
MANGRP=	wheel
MANDIR= /usr/local/man/man8

MANDOWNDIR=./hoedown.git
MANDOWN=${MANDOWNDIR}/build/mandown
MANDOWNFLAGS=--markdown --man-th-e1 ${SPAMILTER_RELEASE} --man-th-e2 ${SPAMILTER_DATE}

SPAMILTER_RELEASE="0.60"
SPAMILTER_DATE="May 30, 2012"

#.SUFFIXES: .mm .8 .5 .md
.SUFFIXES: .8 .5 .md

all: ${MANDOWN} ${MANPAGES}

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

superclean: clean
	git submodule deinit ${MANDOWNDIR}

${MANDOWN}:
	./buildmandown.sh ${MANDOWNDIR}

#.mm.8: Makefile m4man.m4
#	m4 m4man.m4 $< > $@

.md.8: Makefile ${MANDOWN}
	${MANDOWN} --man-th-t `echo $< | sed -e's/\.md//'` --man-th-s 8 ${MANDOWNFLAGS} $< > $@

.md.5: Makefile ${MANDOWN}
	${MANDOWN} --man-th-t `echo $< | sed -e's/\.md//'` --man-th-s 5 ${MANDOWNFLAGS} $< > $@

.PHONY: all install uninstall clean
