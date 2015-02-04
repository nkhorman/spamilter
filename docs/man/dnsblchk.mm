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
DMpage(dnsblchk, `8', `1.0', ``May 30, 2012'', `Neal Horman <spamilter@wanlink.com>', `DNS BlackList testing utility')
MANsection(`synopsis')
dnsblchk [-d] [-p db.rdnsbl file pathname] [-i ip address] [-e mbox@domain] [domain] ...
MANsection(`options')
DMoption(`d',`debug mode')
DMoption(`p',`specify directory path to db.rdnsbl file')
DMoption(`i',`test ip address to see if it is listed at any of the RBLs from db.rdnsbl')
DMoption(`e',`test email delivery')
DMoption(`?',`show cli usage if more than one arg, or man page if no args')
MANsection(`description')
Query RBLs from db.rdnsbl for;
MANrs()
MANip(`an ip address')
MANip(`an ip address of any of the MX hosts for a domain')
MANre()
Also check if an email can be sent to an mbox@domain address.
MANsection(`files')
/var/db/spamilter/db.rdnsbl
MANsection(`see also')
spamilter
MANlinebreak()
MANunderline(`http://www.wanlink.com/spamilter')
MANsection(`author')
DMauthor()
