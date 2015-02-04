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
DMpage(dnsblupd, `8', `1.0', ``May 30, 2012'', `Neal Horman <spamilter@wanlink.com>', `DNS BlackList update utility')
MANsection(`synopsis')
dnsblupd [-d] [-z RDNSBL zone name] [[-a IP address] [-i IP address]] [-r IP address] [-l IP adress] [-?]
MANsection(`options')
DMoption(`d',`debug mode')
DMoption(`z',`RDNSBL zone name. must preceed -i, -a, or -l ')
DMoption(`i',`inserts an ip address into the RDNSBL zone ')
DMoption(`a',`is the A or AAAA record address used on Insert, 127.0.0.1 or ::1 otherwise ')
DMoption(`r',`removes an ip address from the RDNSBL zone ')
DMoption(`l',`lookup ip address in RDNSBL zone ')
DMoption(`?',`show cli usage if more than one arg, or man page if no args')
MANsection(`description')
a utility specifically built for DNS zone updates in RBLs.
MANsection(`see also')
spamilter
MANlinebreak()
MANunderline(`http://www.wanlink.com/spamilter')
MANsection(`author')
DMauthor()
