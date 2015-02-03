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
DMpage(Spamilter, `8', `0.60', ``May 30, 2012'', `Neal Horman <spamilter@wanlink.com>', `A libmilter based spamfilter')
MANsection(`synopsis')
spamilter [-d level] [-c config file path filename] [-f] [-?]
MANsection(`options')
DMoption(`d',`[numeric] - debug level, 1 or 2. NB. This implies -f')
DMoption(`c',`[string] - config file path filename - default = /etc/spamilter.rc')
DMoption(`f',`[boolean] - run spamilter in the foreground')
DMoption(`?',`show cli usage if more than one arg, or man page if no args')

MANsection(`description')
Spamilter is a Sendmail milter written entirely in C, and therefore is faster and less cpu intensive than other interperative based solutions.
MANlinebreak()
It blocks spam using the following methods;
MANrs()
MANip(`Configurable Realtime DNS Blocklists')
MANip(`Sender/Reply-To Address devilvery verification')
MANip(`Configurable Sender/Recipient Black and White lists')
MANip(`Invalid MTA hostname verification')
MANip(`Basic Virus/Worm file attachment rejection for files ending in .pif, .scr, etc.. via MsExtChk filter')
MANip(`SPF via libspf')
MANip(`GeoIP Country Code block listing')
MANre()
Also;
MANrs()
MANip(`Realtime firewall blocking of MTA hosts with invalid host names via
MANbold(`ipfwmtad')')
MANip(`And realtime rate limited connection blocking, also via
MANbold(`ipfwmtad')')
MANip(`Greylisting via
MANbold(`greydbd')')
MANre()
MANlinebreak()
All actions are logged via syslog with both the sender and the recipient.
From this, report generation and notification to recipients showing activity becomes extremely simple.
MANsection(`see also')
ipfwmtad, greydbd
MANlinebreak()
MANunderline(`http://www.wanlink.com/spamilter')
MANsection(`author')
DMauthor()
