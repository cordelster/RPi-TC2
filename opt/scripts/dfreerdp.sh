#!/bin/bash

#Removing Conky to avoid screen glitch
sudo killall -9 conky

config=$(cat /opt/config/dfreerdp.config|grep -v '#'|tr -d '\n')
if [ -z $config ] ; then
	#there is no parameter file specified or is empty, using default:
	####config=" --fonts --ignore-certificate --no-tls --no-nla -f -g 1920x1080 -x l --nsc --composition --gdi hw --plugin rdpsnd --data latency:50 "
	config=" --fonts --ignore-certificate --no-tls --no-nla -f -g 1920x1080 -x l --nsc --composition --gdi hw --plugin rdpsnd --data latency:100 --plugin drdynvc --data tsmf:decoder:gstreamer "
fi

server=$(cat /opt/config/dfreerdp.server|grep -v '#'|tr -d '\n')
if [ -z $server ] ; then
	#there is no server configured, quering user:
	server=$(zenity --entry --title="Server address" --text="Enter the RDP Server name or IP" --entry-text="$(cat /opt/config/dfreerdp.server|grep -v '#'|tr -d '\n')")
	#set the server ip/name to last entered as default:
	echo $server > /opt/config/dfreerdp.server
fi

#check if server ip/name is passed in cmd line
if [ ! -z $1 ] ; then
	server=$1
fi

###################Starting dFreeRDP
LD_LIBRARY_PATH=/opt/dfreerdp/lib/ \
/opt/dfreerdp/bin/dfreerdp \
$config \
-- $server

#Restart Conky and exit shell, this will refresh also the desktop
conky -d -c /home/rpitc/.conkyrc
exit
