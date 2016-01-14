dnsblupd
=

## NAME
**dnsblupd** - DNS BlackList update utility

## SYNOPSIS
dnsblupd [-d] [-z RDNSBL zone name] [-a IP address] [-i IP address] [-r IP address] [-l IP adress] [-?]

## OPTIONS
- -d debug mode
- -z RDNSBL zone name. must preceed -i, -a, or -l 
- -a is the A or AAAA record address used on Insert, 127.0.0.1 or ::1 otherwise 
- -i inserts an ip address into the RDNSBL zone
- -r removes an ip address from the RDNSBL zone 
- -l lookup ip address in RDNSBL zone 
- -? show cli usage if more than one arg, or man page if no args

## DESCRIPTION
A utility specifically built for DNS zone updates in RBLs.

## SEE ALSO
spamilter  
_http://www.wanlink.com/spamilter_

## AUTHOR
Neal Horman - spamilter@wanlink.com
