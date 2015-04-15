#!/bin/bash

audio=$(zenity  --list --title "RPiTC2 Audio output chooser"  --text "RPiTC2 Audio output chooser" --radiolist  --column "Pick" --column "Options" TRUE "HDMI" FALSE "Jack" FALSE "Auto" --width=400 --height=220)
if [ $audio = "HDMI" ] ; then
	amixer cset numid=3 2
	amixer sset PCM 90%
	zenity --info --text "Audio output changed to: $audio"
fi

if [ $audio = "Jack" ] ; then
	amixer cset numid=3 1
	amixer sset PCM 100%
	zenity --info --text "Audio output changed to: $audio"
fi

if [ $audio = "Auto" ] ; then
        amixer cset numid=3 0
	amixer sset PCM 90%
        zenity --info --text "Audio output changed to: $audio"
fi
