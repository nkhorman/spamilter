#!/bin/sh

# Purge the crap and the aged
cat<<EOF| /usr/local/psql/bin/psql -Uspamilter
vacuum greylist;
delete from greylist where count = 1 and now()-createtime >= interval'3 days';
delete from greylist where now()-lastaccesstime >= interval'30 days';
vacuum greylist;
EOF
