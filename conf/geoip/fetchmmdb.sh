#!/bin/sh


dbType="GeoLite2-City"
tardir=""
spamilterconf=""

if [ -z "`which fetch`" ]; then

	curlbin="`which curl`"
	if [ -z "${curlbin}" ]; then
		echo "curl not found, exiting"
		exit 1
	fi

	fetch () { "${curlbin}" -# -R -L $@; }
fi

help()
{
	cat <<-EOF
		Sorry, you need a license key.
		Specify it after the --key argument, or with --file followed by the file that contains it.
		eg. $0 [--key licencekey] | [--file licensekeyfile.txt] [--tardir destination path name]
		See https://dev.maxmind.com/geoip/geoip2/geolite2/ for instruction on how to obtain a license key.
	EOF
}

while [ "${1}" != "" ]; do
	case "${1}" in
		--file)
			if [ -e "${2}" ]; then
				# grab the first line after removing comments and blank lines, as the license
				licensekey="`cat ${2} | sed -e's/[ 	]\{0,\}#.*\$//' -e's/^[ 	]\{1,\}//' -e's/[ 	]\{1,\}\$//' -e'/^[ 	]\{0,\}\$/d' | head -1 | tr -d '\r'`"
			else
				echo "file ${2} doesn't exist"
				exit 1
			fi
			shift
			;;
		--show)
			echo "...${licensekey}..."
			exit 1
			;;
		--key)
			licensekey="${2}"
			shift
			;;
		--dbtype)
			dbType="${2}"
			shift
			;;
		--tardir)
			tardir="${2}"
			shift
			;;
		--spamilterconf)
			spamilterconf="${2}"
			shift
			;;
		*)
			echo "unknown argument ${1}"
			help
			exit 1
			;;
	esac
	shift
done

if [ -z "${licensekey}" ]; then
	help
	exit 1
fi

url="https://download.maxmind.com/app/geoip_download?license_key=${licensekey}&suffix=tar.gz&edition_id="
urlDb="${url}${dbType}"

tardirconf="`grep -i GeoIPDBPath "${spamilterconf:=/usr/local/etc/spamilter.conf}" | head -1 | sed -e's;[^=]\{1,\}=[ 	]\{1,\};;' -e's;[ 	]\{0,\}\$;;' | tr -d '\r'`"
tardir="${tardir:=${tardirconf:=.}}"

updatetar()
{
	tartmp="${2}"
	tardir="${3}"
	tardate="`tar -tvzf ${tartmp} | head -1 | sed -e's;[^_]*_\([^/]*\)/.*\$;\1;'`"
	tarname="${1}_${tardate}.tgz"

	if [ ! -e "${tardir}/${tarname}" ]; then
		mv "${tartmp}" "${tardir}/${tarname}"
		tar -C "${tardir}" --strip-components 1 -xvzf "${tardir}/${tarname}" '*.mmdb'
	else
		rm -f "${tartmp}"
	fi
}

tartmp="/tmp/${dbType}_tmp.tgz"
fetch -o "${tartmp}" "${urlDb}" && updatetar "${dbType}" "${tartmp}" "${tardir}"
