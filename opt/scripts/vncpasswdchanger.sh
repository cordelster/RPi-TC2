#!/bin/bash

password=$(zenity --title "RPiTC2 VNCServer password chooser" --password --width=400 --height=300)
if [ ! -z $password ] ; then
	x11vnc -storepasswd $password /home/rpitc/.vnc/passwd
	zenity --info --text "Password successfully changed"
	notify-send "VNCServer Password changed. New password is: $password"
else
	zenity --error --text "No Password given"
	notify-send "VNCServer Passworn NOT changed"
fi
