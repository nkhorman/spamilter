#!/bin/sh

# http://www.libspf2.org/spf/libspf2-1.2.10.tar.gz
distver="1.2.10"
distname="libspf2-${distver}"
distfile="${distname}.tar.gz"
disturl="http://www.libspf2.org/spf"
builddir="build2"

# git clone https://github.com/shevek/libspf2.git build2
# git checkout d57d79fde2753cdbaaec9a15969253a323474205
# d57d79-fde2753cdbaaec9a15969253a323474205

patchsetfile="${distname}.patch"

#fetch
getfile()
{
	url="$1"; shift
	file="$1"; shift

	fetch="`which fetch 2>/dev/null`"
	if [ ! -z "${fetch}" ]; then # freebsd
		${fetch} ${url}/${file}
	else
		curl="`which curl 2>/dev/null`" # everything else
		if [ ! -z "$curl" ]; then
			${curl} -L ${url}/${file} -o ${file}
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
	cd ${builddir} && patch -p0 < ../${patchsetfile} && touch .patched || echo "Unable to patch"
	cd ${curdir}
fi

#configure
if [ -d ${builddir} -a -f ${builddir}/.patched -a ! -f ${builddir}/.configured ]; then
	curdir="`pwd`"
	cd ${builddir} && ./configure --disable-shared && touch .configured || echo "Unable to configure"
	cd ${curdir}
fi

#build
if [ -d ${builddir} -a -f ${builddir}/.configured -a ! -f ${builddir}/.built ]; then
	curdir="`pwd`"
	cd ${builddir} && make && touch .built || echo "unable to build"
	cd ${curdir}
fi
