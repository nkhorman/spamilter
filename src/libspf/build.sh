#!/bin/sh

distver="1.0.0-p3"
#distver="1.0.0-p5"	# server autoconf bustage
distname="libspf-${distver}"
distfile="${distname}.tar.gz"
disturl="http://wanlink.com/spamilter/download"
builddir="build"

#patchsetfile="patchset3.diff"

#fetch
getfile()
{
	url="$1"; shift
	file="$1"; shift

	fetch="`which fetch`"
	if [ ! -z "${fetch}" ]; then # freebsd
		${fetch} ${url}/${file}
	else
		curl="`which curl`" # everything else
		if [ ! -z "$curl" ]; then
			${curl} -L ${url} -o ${file}
		else
			echo "Unable to find \'fetch\' or \'curl\'"
		fi
	fi

	if [ ! -e ${file} ]; then
		echo "Unable to fetch ${url}/${file}"
		echo "Please manually obtain the file, and the re-run the script"
		exit 1
	fi
}

# get source
if [ ! -e ${distfile} ]; then
	getfile ${disturl} ${distfile}
fi

#extract
if [ ! -d ${builddir} ]; then
	mkdir ${builddir} && tar -C ${builddir} --strip-components 1 -xvzf ${distfile} || echo "Unable to extract ${distfile}"
fi

#patch
if [ -e ${patchsetfile} -a -d ${builddir} -a ! -f ${builddir}/.patched ]; then
	curdir="`pwd`"
	cd ${builddir} && patch < ../${patchsetfile} && touch .patched || echo "Unable to patch"
	cd ${curdir}
fi

#configure
#if [ -d ${builddir} -a -f ${builddir}/.patched -a ! -f ${builddir}/.configured ]; then
if [ -d ${builddir} -a ! -f ${builddir}/.configured ]; then
	curdir="`pwd`"
	cd ${builddir} && ./configure && touch .configured || echo "Unable to configure"
	cd ${curdir}
fi

#build
if [ -d ${builddir} -a -f ${builddir}/.configured -a ! -f .built ]; then
	curdir="`pwd`"
	cd ${builddir} && make && touch ../.built || echo "unable to build"
	cd ${curdir}
fi
