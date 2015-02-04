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
DMpage(ipfwmtad, `8', `0.60', ``May 30, 2012'', `Neal Horman <spamilter@wanlink.com>', `ipfw firewall rule injection daemon')
MANsection(`synopsis')
ipfwmtad [-d] [[-p port] [-n fname] [-u rule number]] [[-i fname] | [-r ipaddress] | [-q ipaddress] | [-a ipaddress] | [-q ipaddress]] [-?]
MANsection(`options')
DMoption(`d',`debug mode')
DMoption(`p',`server mode - tcp port number - [4739]')
DMoption(`n',`server mode - ip database file name - [/tmp/ipfwmtad.db]')
DMoption(`u',`server mode - ipfw rule number - [90]')
dnl DMoption(`b',`server mode - ipfw action (add/deny)')dnl what was this about ?
DMoption(`i',`imeadiate mode - ipfw resync using the ip database file name [ /tmp/ipfwmtad.db]')
DMoption(`r',`imeadiate mode - queue ipaddress for removal')
DMoption(`a',`imeadiate mode - queue ipaddress for addition')
DMoption(`q',`imeadiate mode - query ipaddress')
DMoption(`?',`show cli usage if more than one arg, or man page if no args')
MANsection(`description')
maintains and ages
MANbold(`ipfw')
firewall rules for a dynamic list of ip addresses as submitted in realtime by
MANbold(`spamilter')
for rate limiting and blocking.
MANlinebreak()
You must be root or euid root, to invoke
MANbold(`ipfwmtad')
for either server mode, or imediate mode.
By default,
MANbold(`ipfwmtad')
starts in server mode, unless one of the imediate mode arguments are provided.
MANsection(`files')
/tmp/ipfwmtad.db - The ip address database
MANsection(`see also')
spamilter, ipfwmtad
spamilter, greydbd
MANlinebreak()
MANunderline(`http://www.wanlink.com/spamilter')
MANsection(`author')
DMauthor()