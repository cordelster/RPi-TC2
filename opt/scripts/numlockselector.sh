#!/bin/bash

numlockx=$(zenity  --list  --text "RPiTC2 numlock selector" --radiolist  --column "Pick" --column "Options" --column "Description" TRUE "on" "turns numlock on" FALSE "off" "turns numlock off" FALSE "toggle" "toggles numlock state" --width=400 --height=250)
if [ "$numlockx" = "on" ] ; then
	echo "NUMLOCK=on" > /etc/default/numlockx
	numlockx on
	zenity --info --text "Numlock enabled"
fi

if [ "$numlockx" = "off" ] ; then
	numlockx off
	echo "NUMLOCK=off" > /etc/default/numlockx
	zenity --info --text "Numlock disabled"
fi

if [ "$numlockx" = "toggle" ] ; then
	echo "NUMLOCK=toggle" > /etc/default/numlockx
	numlockx toggle
        zenity --info --text "Numlock switched"
fi
