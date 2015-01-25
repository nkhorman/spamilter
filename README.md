Spamilter
---------

This is a Sendmail milter written entirely in C, and therefore is faster and less cpu intensive than other interperative based solutions.

It blocks spam using the following methods;

- Configurable Realtime DNS Blacklists
- Sender Address verification
- Configurable Black and White lists
- Invalid MTA hostname verification
- Basic Virus/Worm file attachment rejection for files ending in .pif, .scr, etc.. via MsExtChk filter
- SPF via libspf

Also;

- Realtime firewall blocking of MTA hosts with invalid host names via MtaHostIpfw filter
- Realtime rate limited connection blocking via firewall rule injection

All actions are logged via syslog with both the sender and the recipient. From this, report generation and notification to recipients showing activity becomes extremely simple.
