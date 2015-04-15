#! /bin/sh
### BEGIN INIT INFO
# Provides:          h264
# Required-Start:
# Required-Stop:
# Should-Start:      glibc
# Default-Start:     S
# Default-Stop:
# Short-Description: Start intro video
# Description:       Start intro video based on displaz resolution/aspect ratio
### END INIT INFO

PATH=/sbin:/bin

case "$1" in
  start|"")
	VIDEOOUT=$(/usr/bin/tvservice -s)
	case "$VIDEOOUT" in 
		*16:9*)
			/usr/bin/h264.bin /opt/graphics/intro169.m4v
		;;
		*8:5*)
			/usr/bin/h264.bin /opt/graphics/intro85.m4v
		;;
		*4:3*)
			/usr/bin/h264.bin /opt/graphics/intro43.m4v
		;;
		*)
			/usr/bin/h264.bin /opt/graphics/intro169.m4v
		;;
	esac
	;;
  stop)
	# No-op
	;;
  *)
	echo "Usage: h264" >&2
	exit 3
	;;
esac

:

