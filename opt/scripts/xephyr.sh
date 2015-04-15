#!/bin/sh

config=$(cat /opt/config/xephyr.config|grep -v '#'|tr -d '\n')
if [ -z $config ] ; then
	#there is no parameter file specified or is empty, using default:
	config=" -ac +bs +iglx -reset -terminate -fullscreen +xinerama "
fi

server=$(cat /opt/config/xephyr.server|grep -v '#'|tr -d '\n')
if [ -z $server ] ; then
	#there is no server configured, quering user:
	server=$(zenity --entry --title="Server address" --text="Enter the Xephyr Server name or IP" --entry-text="$(cat /opt/config/xephyr.server|grep -v '#'|tr -d '\n')")
	#set the server ip/name to last entered as default:
	echo $server > /opt/config/xephyr.server
fi

#check if server ip/name is passed in cmd line
if [ ! -z $1 ] ; then
	server=$1
fi

###################Starting Xephyr
Xephyr \
$config \
-query $server :1
