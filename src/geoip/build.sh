#!/bin/sh

distver="1.4.8"
distname="GeoIP-${distver}"
distfile="${distname}.tar.gz"
disturl="http://www.maxmind.com/download/geoip/api/c"

patchsetfile="patchset3.diff"

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
if [ ! -d ${distname}.mod ]; then
	mkdir ${distname}.mod && tar -C ${distname}.mod --strip-components 2 -xvzf ${distfile} || echo "Unable to extract ${distfile}"
fi

#patch
if [ -d ${distname}.mod -a ! -f ${distname}.mod/.patched ]; then
	curdir="`pwd`"
	cd ${distname}.mod && mv libGeoIP/md5.c libGeoIP/md5_local.c && mv libGeoIP/md5.h libGeoIP/md5_local.h && patch < ../${patchsetfile} && touch .patched || echo "Unable to patch"
	cd ${curdir}
fi

#configure
if [ -d ${distname}.mod -a -f ${distname}.mod/.patched -a ! -f ${distname}.mod/.configured ]; then
	curdir="`pwd`"
	cd ${distname}.mod && ./configure && touch .configured || echo "Unable to configure"
	cd ${curdir}
fi

#build
if [ -d ${distname}.mod -a -f ${distname}.mod/.configured -a ! -f .built ]; then
	curdir="`pwd`"
	cd ${distname}.mod && make && make && touch ../.built || echo "unable to build"
	cd ${curdir}
fi
