## Copyright

This Web UI code base is;
- Copyright by it's respective Authors.
- Portions Copyright (c) 2011-2015 by Neal Horman, All rights reserved.


## License
Anything written originally by me (Neal Horman) or contributors, for this Web UI that wasn't originally licensed GPL V3;
- Is hereby dual licensed under the basic 3 Clause BSD License, see http://opensource.org/licenses/BSD-3-Clause
- and under the GPL V3 as required by pChart2, see http://opensource.org/licenses/GPL-3.0

See the individual files for additional details.

This Web UI, is considered, by me (Neal Horman) to be a stand alone application, separate, but related to the Spamilter applications,
and therefore does not force any of the other Spamilter applications, to be GPV V3 licensed.


## Disclaimer

If the language / html usage and techniques, are crappy, well, sorry, but this was initially thrown together, just to give my a simple
UI to view log files, and like all things Q&D, was never ment to have any kind of real shelf life.
Unfortunately, it has survived several years without being "fixed" (re-written properly).

I've HACKED in some graph foo, just to be able to see some usage stats.
So if you find bugs, create a patch.
If you want to critisize anything else, well, then "fix" it, please!

Seriously, Web Dev is not my day job, and I'm sure it shows.

1995 called, they want their &lt;table&gt;s back!


## Testing
This Web UI has been tested, as of mid 2015, with current versions of apache22/apache24, php55, chrome, and firefox.


## Installation
This directory structure reflects how I layout apache applications in my installations.

Adding "Include /usr/local/www/apache24/apps/spamilter/httpd.conf" to any given vhost makes adding
the application trivial. Take the layout and the httpd.conf for whatever it's worth.

Put this folder in an adjacent folder to your main htdocs folder, tweak the httpd.conf,
and add the "Include" statement to your main httpd.conf infrastructure.
ie. something like;

- /usr/local/www/apache24/bla/bla
- /usr/local/www/apache24/htdocs/bla/bla
- /usr/local/www/apache24/apps/bla/bla
- /usr/local/www/apache24/apps/spamilter <--- Put me here.

A default user "changeme" with a password "pass" has been created in the htpasswd.db file.
It is only meant to protect the UI from non-admin's.
There is no fine grained permissions support, it's all or nothing.
All, means that you can add/remove white/black listed senders, dig into email transaction
history, and detail.

You'll need to add a periodic cron job, likely to your www user, something like

```
10 0 * * * www /usr/local/bin/php /usr/local/www/apache24/apps/spamilter/htdocs/totals.php
```

And, you'll probably need to run it once, manually after installation, to see the charts.


## Files and Permissions
This app expects to be able to write to the /var/db/spamilter/db.sndr file,
as well as read the other /var/db/spamilter/db.xxxx files. 

It also expects to be able to read the syslog generated files, which are
presumed to be /var/log/spam.log* and /var/log/spam.info*

If the files are elsewhere, or named elsewise, change index.php

Both today's log files, and past days log files are read.
Past days files are expected to be compressed as .gz files.

The app expects to be able to write to the /usr/local/www/apche24/apps/spamilter/htdocs folder and children.
Specifically;

- htdocs/totalscache.ser
- htdocs/totalsstatus.ser
- htdocs/chart/totals/*
- htdocs/chart/cache/*
- htdocs/chart/imgmaps/*

 
## Depenancies
PPH must provide GD, png, freetype, json and gzip/zlib support.


## Note
I always try to make the applications without hard coding any URL roots, so if you find any, then it's probably a mistake.

