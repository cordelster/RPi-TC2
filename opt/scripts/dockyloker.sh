#!/bin/bash

docky=$(zenity  --list  --text "Lock/Unlock Doky menu..." --radiolist  --column "Pick" --column "Opinion" TRUE "Lock Docky" FALSE "Unlock Docky")
if [ "$docky" = "Lock Docky" ] ; then
	gconftool-2 --set --type boolean /apps/docky-2/Docky/Items/DockyItem/ShowSettings false
	gconftool-2 --set --type boolean /apps/docky-2/Docky/Items/DockyItem/ShowQuit false
	gconftool-2 --set --type boolean /apps/docky-2/Docky/Items/FileApplicationProvider/AllowPinToDock false
	gconftool-2 --set --type boolean /apps/docky-2/Docky/Interface/DockDragTracker/ProvidersAcceptDrops false
	gconftool-2 --set --type boolean /apps/docky-2/Docky/Interface/DockDragTracker/LockDrags true
	gconftool-2 --set --type boolean /apps/docky-2/Docky/Items/DockyItem/ShowDockyItem false
	zenity --info --text "Docky menus loked!"
fi

if [ "$docky" = "Unlock Docky" ] ; then
	gconftool-2 --set --type boolean /apps/docky-2/Docky/Items/DockyItem/ShowSettings true
	gconftool-2 --set --type boolean /apps/docky-2/Docky/Items/DockyItem/ShowQuit true
	gconftool-2 --set --type boolean /apps/docky-2/Docky/Items/FileApplicationProvider/AllowPinToDock true
	gconftool-2 --set --type boolean /apps/docky-2/Docky/Interface/DockDragTracker/ProvidersAcceptDrops true
	gconftool-2 --set --type boolean /apps/docky-2/Docky/Interface/DockDragTracker/LockDrags false
	gconftool-2 --set --type boolean /apps/docky-2/Docky/Items/DockyItem/ShowDockyItem true
	zenity --info --text "Docky menus UNloked!"
fi
