#!/bin/sh

config=$(cat /opt/config/xfreerdp.config|grep -v '#'|tr -d '\n')
if [ -z $config ] ; then
	#there is no parameter file specified or is empty, using default:
	config=" +fonts -sec-nla /rfx /sound:sys:alsa /cert-ignore /size:1280x720 "
fi

server=$(cat /opt/config/xfreerdp.server|grep -v '#'|tr -d '\n')
if [ -z $server ] ; then
	#there is no server configured, quering user:
	server=$(zenity --entry --title="Server address" --text="Enter the RDP Server name or IP" --entry-text="$(cat /opt/config/xfreerdp.server|grep -v '#'|tr -d '\n')")
	#set the server ip/name to last entered as default:
	echo $server > /opt/config/xfreerdp.server
fi

#check if server ip/name is passed in cmd line
if [ ! -z $1 ] ; then
	server=$1
fi

###################Starting xFreeRDP
xfreerdp \
$config \
/v:$server
