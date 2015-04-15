#!/bin/sh
#generic bash to keep up a process/command

#get all arguments from commandline
COMMAND=$@
#generally first argument is the daemon name: freerdp, vmware-view, firefox etc
DAEMON=$1

if [ ! -z "$COMMAND" ] ; then
	#if the daemon is already up ill kill it to avoid dead loop
	PIDOFDAEMON=`pidof $DAEMON`
	if [ ! -z "$PIDOFDAEMON" ] ; then
		killall -9 $DAEMON
	fi
	#then start the control loop
	(
		flock -x -n 9 || exit 1
		while true
		do
			$COMMAND
		done
	) 9>/tmp/lockfile_$RANDOM

fi
