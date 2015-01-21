#!/bin/sh

progdir="/usr/local/bin"
progname1="spamilter"
progname2="greydbd"
prog1="$progdir/$progname1"
prog2="$progdir/$progname2"

case $1 in
	start)
		$prog1;
		echo "$progname1"
		$prog2;
		echo "$progname2"
	;;
	stop)
		killall $progname1 $progname2;
	;;
esac
