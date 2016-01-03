greydbd(8)
=

## NAME
**greydbd** - spamilter greylist support database service

## SYNOPSIS
greydbd [-v] [-d debug level] [-p port number] [-h psql hostname] [-i psql host port] [-j psql database device name] [-k psql database user name] [-l psql database user password] [-?]

## OPTIONS
- -v display version
- -d debug mode, forces foreground operation - must be non-zero debug level
- -p service port number [7892]
- -h psql hostname [localhost]
- -i psql host port [5432]
- -j psql database device name [spamilter]
- -k psql database user name [spamilter]
- -l psql database user password []
- -? show cli usage if more than one arg, or man page if no args

## DESCRIPTION
Provides a database storage and aging mechanisim to track ip address and email tuples.

## SEE ALSO
spamilter, ipfwmtad  
_http://www.wanlink.com/spamilter_

## AUTHOR
Neal Horman - spamilter@wanlink.com
