#!/bin/sh

if [ "$1" != "" ]; then
	MANDOWNDIR=$1
	shift

	PWD="`pwd`"

	if [ ! -d ${MANDOWNDIR} ]; then
		mkdir -p ${MANDOWNDIR}
	fi

	cd ${MANDOWNDIR}
	if [ ! -e Makefile ]; then 
		cmake ..
	fi

	if [ -f Makefile ]; then
		make $@
	fi
	cd ${PWD}
fi
