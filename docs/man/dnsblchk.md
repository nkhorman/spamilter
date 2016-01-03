dnsblchk
=

## NAME
**dnsblchk** - DNS BlackList testing utility

## SYNOPSIS
dnsblchk [-d] [-p db.rdnsbl file pathname] [-i ip address] [-e mbox@domain] [domain] ...

## OPTIONS
- -d debug mode
- -p specify directory path to db.rdnsbl file
- -i test ip address to see if it is listed at any of the RBLs from db.rdnsbl
- -e test email delivery
- -? show cli usage if more than one arg, or man page if no args

## DESCRIPTION
Query RBLs from db.rdnsbl for;
- An ip address
- An ip address of any of the MX hosts for a domain

Also check if an email can be sent to an mbox@domain address.

## files
_/var/db/spamilter/db.rdnsbl_

## SEE ALSO
spamilter  
_http://www.wanlink.com/spamilter_

## AUTHOR
Neal Horman - spamilter@wanlink.com
