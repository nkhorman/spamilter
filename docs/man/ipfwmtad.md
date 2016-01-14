ipfwmtad(8)
=

## NAME
**ipfwmtad** - An IP firewall dynamic rule injection managment daemon used by the **sendmail** MTA filter daemon **spamilter**

## SYNOPISIS
ipfwmtad [-d] [[-A client ACL config file] [-I server ip address] [-p port] [-n fname] [-u rule number]] [[-U auth user name] [-P auth user password] [-i fname] | [-r ipaddress] | [-q ipaddress] | [-a ipaddress] | [-q ipaddress]] [-?]

## OPTIONS
- -d debug mode
- -A client ACL config file - [/usr/local/etc/spamilter/ipfwmtad.acl]
- -I server ip address - [127.0.0.1]
- -p server mode - tcp port number - [4739]
- -n server mode - ip database file name - [/tmp/ipfwmtad.db]
- -u server mode - ipfw rule number - [90]
- -i imeadiate mode - ipfw resync using the ip database file name [ /tmp/ipfwmtad.db]
- -r imeadiate mode - queue ipaddress for removal
- -a imeadiate mode - queue ipaddress for addition
- -q imeadiate mode - query ipaddress
- -U imeadiate mode - authentication user name
- -P imeadiate mode - authentication user password
- -? show cli usage if more than one arg, or man page if no args

## DESCRIPTION
**ipfwmtad** maintains and ages **ipfw** firewall rules for a dynamic list of ip addresses as submitted in
realtime by **spamilter** for rate limiting and blocking. \
You must be root or euid root, to invoke **ipfwmtad** for either server mode, or imediate mode.  
By default, **ipfwmtad** starts in server mode, unless one of the imediate mode arguments are provided. \
Note: For installations where spamilter is not connecting to a localhost instance of ipfwmtad, a local
account should be configured for the purposes of authentication on the ipfwmtad host. It should be a
restricted account with no login shell, a strong password, and would not require an associated directory.
The remote instance(s) of spamilter, will need to be configured with the account name and password.

## FILES
_/tmp/ipfwmtad.db_ - The ip address database cache  
_/usr/local/etc/spamilter/ipfwmtad.acl_ - Client ACL config file

## SEE ALSO
spamilter, spamilter.conf  
_http://www.wanlink.com/spamilter_

## AUTHOR
Neal Horman - spamilter@wanlink.com
