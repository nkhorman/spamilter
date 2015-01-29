dnl --------------------------------------------------------------------*
dnl 
dnl  Developed by;
dnl       Neal Horman - http://www.wanlink.com
dnl 
dnl  DESCRIPTION:
dnl       manpage content using m4 macros - don't ask why
dnl 
dnl --------------------------------------------------------------------*
dnl
DMpage(greydbd, `8', `1.0', ``May 30, 2012'', `Neal Horman <spamilter@wanlink.com>', `spamilter greylist support database service')
MANsection(`synopsis')
greydbd [-v] [-d debug level] [-p port number] [-h psql hostname] [-i psql host port] [-j psql database device name] [-k psql database user name] [-l psql database user password]
MANsection(`options')
DMoption(`v',`display version')
DMoption(`d',`debug mode, forces foreground operation - must be non-zero debug level')
DMoption(`p',`service port number [7892]')
DMoption(`h',`psql hostname [localhost]')
DMoption(`i',`psql host port [5432]')
DMoption(`j',`psql database device name [spamilter]')
DMoption(`k',`psql database user name [spamilter]')
DMoption(`l',`psql database user password []')
MANsection(`description')
provides a database storage and aging mechanisim to track ipaddress and email tuples.
MANsection(`see also')
spamilter, ipfwmtad
MANlinebreak()
MANunderline(`http://www.wanlink.com/spamilter')
MANsection(`author')
DMauthor()
