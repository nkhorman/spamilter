spamilter.conf(5)
===

## NAME
**spamilter.conf** - Spamilter configuration file

## DESCRIPTION 
spamilter is configured based on compiled in defaults and the overrides supplied in this file.
Comments can be embeded in the file starting with the `#` character to the end of the exsting line.

## FILE FORMAT
Each line is used to alter a compiled in default configuration option, and should be in the form of

	variable = value

Where `variable` is one of the Switches and Knobs below.

Assuming the the compiled in defaults are great, except for two, an example of your spamilter.conf file could be as simple as

	# my url for reject policy explations
	PolicyUrl	= http://www.example.com/policy.html
	DnsBlChk	= Off	# turn off RBLs

## SWITCHES AND KNOBS
These options can be String, Boolean, or Integer types.
Quotations are not reqired by any of these options.
Booleans are `1`, `On`, `True`, and `Yes` for True, and anything else for False.
Integers are deicmal numbers.

### AliasTableChk
Validate the recipient against the alias.db file, otherwise Reject.
Location of alias.db file.
- Type - String
- Default - `/etc/mail/alias.db`

### Conn
The sendmail milter connection specification that Spamilter uses for communications with sendmail.
See [this page](http://www.sendmail.org/~gshapiro/8.10.Training/milterconfig.html "Sendmail's documentation")
- Type - String
- Default - `inet:7726@localhost`

### Dbpath
The filepath where the numerous db.* configuation files are located.
- Type - String
- Default - `/var/db/spamilter`

### DnsBlChk
Check the connecting MTA ip address against the RBLs specified in db.rdnsbl
- Type - Boolean
- Default - `True`

### GeoIPChk
Lookup the connecting MTA ip address from the GeoIP database, find the Country,
and check against the db.geocc database for Reject
- Type - Boolean
- Default - `True`
  
### GeoIPDBPath
The location of GeoIp Database directory
- Type - String
- Default - `/var/db/spamilter/geoip`
 
### GreyListChk
Consult GreyDbd GreyList qualification before accepting.
- Type - Boolean
- Default - `True`
  
### GreyListHost
The host address where GreyDbd resides.
- Type - String
- Default - `127.0.0.1`
   
### GreyListPort
The port number that GreyDbd listens to.
- Type - Integer
- Default - `7892`
  
### HeaderReceivedChk  
Check that all Received header's ip address are not on an RBL.
- Type - Boolean
- Default - `False`
  
### HeaderReplyToChk
Check the ReplyTo Address header, like the Envelope Sender Address (Smtp Sender Check), to validate that
the address will accept a reply from the recipient.
- Type - Boolean
- Default - `True`
 
### IpfwHost
The host address where IpfwMtad resides.
- Type - String
- Default - `127.0.0.1`
 
### IpfwPass
The IpfwUser's account password in clear text.
- Type - String
- Default - `` (empty)
 
### IpfwPort
The port number that IpfwMtad listens to.
- Type - Integer
- Default - `4739`
  
### IpfwUser
A system user account name used for authentication when connecting to IpfwMtad that is valid on the host where IpfwMtad resides.
Note that the user account should be a nologin account, and doesn't require an account directory
- Type - String
- Default - `` (empty)
  
### LocalUserTableChk
Validate the recipent against the local user account database, otherwise Reject.
- Type - Boolean
- Default - `True`

### MsExtChk
Check email body for attachements with certain extension names, and `Reject` or `Tag` based on the MsExtChkAction setting.
- Type - Integer
- Default - `0`
 
Valid Values;
  * 0 = Off
  * 1 = Small extension sub-set
  * 2 - Full extension set

### MsExtChkAction
How to handle attachment filename extension matches.
- Type - String
- Default - `Reject`

Valid Values;
  * `Reject`
  * `Tag`

### MtaHostChk
This filter enables several tests. It requires that the HELO MTA hostname;
  * NOT be an ip address.
  * resolves to an ip address.
  * is NOT that of the recipient's domain.  

   Also, the following are tested;
  * Enforces Black/White domain listing based on entries in db.sndr
  * Require that the HELO MTA hostname passes DomainBlockList RBL testing based on db.dbl entries.
- Type - Boolean
- Default - `True`

### MtaHostChkAsIp   
Require that the MTA hostname NOT be an ip address.
- Type - Boolean
- Default - `False`

### MtaHostIpChk
The HELO MTA hostname should match the connecting IP address.
- Type - Boolean
- Default - `False`

### MtaHostIpfw
As the result of a Reject operation, add the MTA host ip address to the firewall block list managed by IpfwMtad.
- Type - Boolean
- Default - `False`

### MtaHostIpfwNominate
Every connecting MTA ip address is nominated / tested for rate limited blocking as managed by IpfwMtad.
- Type - Boolean
- Default - `False`

### MtaSpfChk
Require the connecting MTA ip addres pass SPF checking.
- Type - Boolean
- Default - `True`

### MtaSpfChkSoftFailAsHard
Force SPF `Soft` failures to be treated as `Hard` failures.
- Type - Boolean
- Default - `False`

### PolicyUrl   
A base URL that is appened with anchor tags and provided in the error message when rejecting and email.
- Type - String
- Default - `http://www.somedomain.com/policy.html`

### PopAuthChk
Location of berkely db file.
If the MTA connect ip address is listed in the db file, then Pop-before-SMTP is valid, so treat the
connecing MTA ip address as a localnet host, which bypasses all filters.
- Type - String
- Default - `` (empty)

### SmtpSndrChk
Validate that an email can be returned to the sender.
- Type - Boolean
- Default - True

### SmtpSndrChkAction
What to do if the Smtp Sender Check fails.
- Type - String
- Default - `Reject`

Valid Values;
  * `Reject`
  * `Tag`

### UserName
The system user account name that is switched to for runtime privileges if started as root.
- Type - String
- Default - `nobody`

### VirtUserTableChk
Location of virtuser.db file.
Validate the recipent against the virtusertable.db file, otherwise Reject.
- Type - String
- Default - `/etc/mail/virtuser.db`

## SEE ALSO
spamilter

## AUTHOR
Neal Horman - spamilter@wanlink.com
