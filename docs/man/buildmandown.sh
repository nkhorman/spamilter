#!/bin/sh

if [ "$1" != "" ]; then
	MANDOWNDIR=$1
	shift

	PWD="`pwd`"

	if [ -d ${MANDOWN} ]; then
		cd ${MANDOWNDIR}
		builddir=build

		if [ ! -f .git ]; then
			git submodule init; git submodule update
		fi

		if [ ! -d ${builddir} ]; then
			mkdir -p ${builddir}
		fi

		cd ${builddir}
		if [ ! -e Makefile ]; then 
			if [ ! -e `which cmake` ]; then
				echo "Ummm... cmake not found. unable to create make file to build hoedown/mandown."
				echo "Either install cmake, or add it to the PATH, and re-envoke."
			else
				cmake ..
			fi
		fi

		if [ -f Makefile ]; then
			make $@
		fi
	fi

	cd ${PWD}
fi
