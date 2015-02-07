#!/bin/sh

distver="1.0.0-p3"
#distver="1.0.0-p5"	# servere autoconf bustage
distname="libspf-${distver}"
distfile="${distname}.tar.gz"
disturl="http://wanlink.com/spamilter/download"
builddir="build"

# patches from http://cvs.schmorp.de/libspf/ and libspf-1.0.0-p3 > libspf-1.0.0-p5
#patchsetfile="${distname}.patch"

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

if [ -d ${builddir} ]; then
	curdir="`pwd`"
	cd ${builddir}

	#patch
	if [ -e "${patchsetfile}" -a ! -f .patched ]; then
		patch -p0 < ../${patchsetfile} && touch .patched || echo "Unable to patch"
	fi
echo `pwd`
	#configure
	if [ ! -f .configured ]; then
		./configure && touch .configured || echo "Unable to configure"
	fi

	#build
	if [ -f .configured -a ! -f .built ]; then
		make && touch .built || echo "unable to build"
	fi

	cd ${curdir}
fi
