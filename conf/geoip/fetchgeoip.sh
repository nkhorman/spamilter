#!/bin/sh

geoip="GeoIP.dat"
geolitecity="GeoLiteCity.dat"
geoipcity="GeoIPCity.dat"
geoipdir="/var/db/spamilter/geoip"

# force to ipv4, because of reports of -6 bustage
# maybe curl would be better for this
fetch -4 http://geolite.maxmind.com/download/geoip/database/GeoLiteCountry/${geoip}.gz
fetch -4 http://geolite.maxmind.com/download/geoip/database/${geolitecity}.gz

if [ -e ${geoip}.gz ]; then

	if [ -e ${geoipdir}/${geoip} ]; then
		# rename old db
		t="`ls -lD%Y%m%d%H%M%S ${geoipdir}/${geoip}|awk '{print $6}'`"
		mv ${geoipdir}/${geoip} ${geoipdir}/${geoip}.${t}
		gzip ${geoipdir}/${geoip}.${t}
	fi

	# install new db
	gzcat ${geoip}.gz > ${geoipdir}/${geoip}
	rm -f ${geoip}.gz
fi

if [ -e ${geolitecity}.gz ]; then

	if [ -e ${geoipdir}/${geoipcity} ]; then
		# rename old db
		t="`ls -lD%Y%m%d%H%M%S ${geoipdir}/${geoipcity}|awk '{print $6}'`"
		mv ${geoipdir}/${geoipcity} ${geoipdir}/${geoipcity}.${t}
		gzip ${geoipdir}/${geoipcity}.${t}
	fi

	# install new db
	gzcat ${geolitecity}.gz > ${geoipdir}/${geoipcity}
	rm -f ${geolitecity}.gz
fi
