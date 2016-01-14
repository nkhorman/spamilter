spamilter(8)
===

## NAME
**Spamilter** A libmilter based spam filter

## SYNOPISIS
spamilter [-d level] [-c config file path filename] [-f] [-?]

## OPTIONS
- -d [numeric] - debug level, 1 or 2. NB. This implies -f
- -c [string] - config file path filename - [/etc/spamilter.rc]
- -f [boolean] - run spamilter in the foreground
- -? show cli usage if more than one arg, or man page if no args

## DESCRIPTION
Spamilter is a Sendmail milter written entirely in C, and therefore is faster and less cpu intensive than other interperative based solutions.
It blocks spam using the following methods;

- Configurable Realtime DNS Blocklists
- Sender/Reply-To Address devilvery verification
- Configurable Sender/Recipient Black and White lists
- Invalid MTA hostname verification
- Basic Virus/Worm file attachment rejection for files ending in .pif, .scr, etc.. via MsExtChk filter
- SPF via libspf
- GeoIP Country Code block listing

Also:
- Realtime firewall blocking of MTA hosts with invalid host names via **ipfwmtad**
- Realtime rate limited connection blocking, via **ipfwmtad**
- Greylisting via **greydbd**

All actions are logged via syslog with both the sender and the recipient.
From this, report generation and notification to recipients showing activity becomes extremely simple.

## OPERATION
Spamilter works by testing various parameters and content of any given email during the delivery attempt.
There are 8 points in the delivery process, at which information can be collected or evaluated.

Information is collected during the following stages;
- Connect - The MTA IP address. Databases are opened, MTA IP address is evaluated as being on local a local network or not. If the IP address is not a localnethost, and MtaHostIpfwNominate is true, then using IpfwMtad, nominate (inculpate) the IP address for rate limit blocking.

- Helo - (This is not a typo) - The MTA self proclaimed hostname.
- Envelope From - The email address of the sender.
- Envelope Reckipient - The email address of the intended recipient.
- Header - Information, including; previously transited MTAs, Subject, ReplyTo, etc...

Information collected from the preceding stages is evaluated, and acted upon in the following stages;
- End Of Header - This is a transisition point from header information to body information. The majority of Spamilter's "filter" evaluation is done here.
- Body - This is called potentially multiple times, with chunks of the email body content. Evaluate URLs found in the body against the DBL database. MsExtChk - Collect information about attachment filename extensions.
- End Of Message - The entierty of the email has been transfered. MsExtChk - Evaluate the filename extensions and react appropiately based on MsExtChkAction.
- Close - close the Databases, and if the MTA IP address was inculpated, and the email was not rejected, then the MTA IP address is IpfwMtad exculpated.

## FILTERS
Following is a list of filters, in basic order of proccessing during the End Of Header stage, and which config options control them.

### Policy Enforcement
- RcptFwdHostChk - Can we successfully forward to a MTA host based on the recipient's address ?
- Black / White list the Sender's email address

### Technical Enforcement
- MtaHostChk or MtaHostChkAsIp - The HELO MTA hostname should not be an ip address
- MtaHostChk - The HELO MTA hostname should not be that of the recipent's domain
- MtaHostChk - The HELO MTA hostname should resolve to an ip address

### Policy Enforcement
- MtaHostChk - DBL check the HELO MTA hostname

### Technical Enforcement
- MtaHostIpChk - The HELO MTA hostname should match the connecting ip address. This one would be effective if used, however, practically nobody bothers to properly configure reverse DNS to match the A record anymore, and many adminstrators simply improperly configure the MTA hostname anyway, so this should probably left off.

### Policy Enforcement
- MtaSpfChk - The MTA should pas SPF tests
- DNS RBL checking
- SmtpSndrChk - Validate the sender return path
- GeoIpChk - Do "Recieved by/from" header ip address resolution to GeoIP CountryCode checks
- Local compiled in content filter checks
- ReplyTo header checks
- Greylist checking

## SEE ALSO
spamilter.conf, ipfwmtad, greydbd
_http://www.wanlink.com/spamilter_

## AUTHOR
Neal Horman - spamilter@wanlink.com
