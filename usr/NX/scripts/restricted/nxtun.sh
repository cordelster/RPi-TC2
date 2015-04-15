#!/bin/bash
######################################################################
#                                                                    #
#  Copyright (c) 2002, 2014 NoMachine, http://www.nomachine.com.     #
#                                                                    #
#  All rights reserved.                                              #
#                                                                    #
######################################################################

if test $# -ne 5; then
  echo "ERROR: parameters error"
  exit 1
fi

if [ -e "/bin/awk" ];
then
  COMMAND_AWK="/bin/awk"
elif [ -e "/usr/bin/awk" ];
then
  COMMAND_AWK="/usr/bin/awk"
else
  echo "ERROR: awk command not found"
  exit 1
fi

if test -x "/bin/echo"; then
  COMMAND_ECHO="/bin/echo"
else
  echo "NX> 500 Error: echo command not found."
  exit 1
fi

if test -x "/sbin/ifconfig"; then
  COMMAND_IFCONFIG="/sbin/ifconfig"
else
  echo "NX> 500 Error: ifconfig command not found."
  exit 1
fi

errorMsg ()
{
  ${COMMAND_ECHO} "NX> 500 Error: $1"
  if test "x$2" = "x1"; then
    exit 1
  fi
}

if test -e "/etc/NX/player/localhost/player.cfg"; then
  BIN_DIR=$($COMMAND_AWK -F'"' '/^PlayerRoot/ {print $2}' /etc/NX/player/localhost/player.cfg)
else
  if test -e "/etc/NX/server/localhost/node.cfg"; then
    BIN_DIR=$($COMMAND_AWK -F'"' '/^NodeRoot/ {print $2}' /etc/NX/server/localhost/node.cfg)
  else
    errorMsg "Cannot stat files: /etc/NX/player/localhost/player.cfg and /etc/NX/server/localhost/node.cfg." "1"
  fi
fi

NX_BIN="/bin/"

NXTUNCTL_NAME="nxtunctl"
NXTUNCTL_NAME_PATH="$BIN_DIR$NX_BIN$NXTUNCTL_NAME"

addInterface()
{
  STATUS=`$NXTUNCTL_NAME_PATH -u $1 -b`

  if test "x$STATUS" == "x"; then
    errorMsg "nxtunctl running error." "1"
  else
    ${COMMAND_IFCONFIG} $STATUS $2 netmask 255.255.255.252 up

    ${COMMAND_ECHO} $STATUS
  fi

  return 0;
}

case "$3" in
'--add')       addInterface "$4" "$5"       ;;
esac

