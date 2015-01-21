#!/bin/sh

geoip="GeoIP.dat"
geolitecity="GeoLiteCity.dat"
geoipcity="GeoIPCity.dat"
geoipdir="/var/db/spamilter/geoip"

fetch http://geolite.maxmind.com/download/geoip/database/GeoLiteCountry/${geoip}.gz
fetch http://geolite.maxmind.com/download/geoip/database/${geolitecity}.gz

if [ -e ${geoip}.gz ]; then

	# rename old db
	t="`ls -lD%Y%m%d%H%M%S ${geoipdir}/${geoip}|awk '{print $6}'`"
	mv ${geoipdir}/${geoip} ${geoipdir}/${geoip}.${t}
	gzip ${geoipdir}/${geoip}.${t}

	# install new db
	gzcat ${geoip}.gz > ${geoipdir}/${geoip}
	rm -f ${geoip}.gz
fi

if [ -e ${geolitecity}.gz ]; then

	# rename old db
	t="`ls -lD%Y%m%d%H%M%S ${geoipdir}/${geoipcity}|awk '{print $6}'`"
	mv ${geoipdir}/${geoipcity} ${geoipdir}/${geoipcity}.${t}
	gzip ${geoipdir}/${geoipcity}.${t}

	# install new db
	gzcat ${geolitecity}.gz > ${geoipdir}/${geoipcity}
	rm -f ${geolitecity}.gz
fi
