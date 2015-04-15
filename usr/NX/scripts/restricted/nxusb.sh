#!/bin/bash
######################################################################
#                                                                    #
#  Copyright (c) 2002, 2014 NoMachine, http://www.nomachine.com.     #
#                                                                    #
#  All rights reserved.                                              #
#                                                                    #
######################################################################

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

if test -e "/etc/NX/player/localhost/client.cfg"; then
  CLIENT_ROOT=$($COMMAND_AWK -F'"' '/^ClientRoot/ {print $2}' /etc/NX/player/localhost/client.cfg)
else
  if test -e "/etc/NX/server/localhost/client.cfg"; then
    CLIENT_ROOT=$($COMMAND_AWK -F'"' '/^ClientRoot/ {print $2}' /etc/NX/server/localhost/client.cfg)
  else
    if test -e "/etc/NX/node/localhost/client.cfg"; then
      CLIENT_ROOT=$($COMMAND_AWK -F'"' '/^ClientRoot/ {print $2}' /etc/NX/node/localhost/client.cfg)
    else
      if test -d "/Applications/NoMachine Service.app"; then
        CLIENT_ROOT="/Applications/NoMachine Service.app/Contents/Frameworks"
      else
        if test -d "/Applications/NoMachine.app"; then
          CLIENT_ROOT="/Applications/NoMachine.app/Contents/Frameworks"
        else
          if test -d "/Applications/NoMachine Node.app"; then
            CLIENT_ROOT="/Applications/NoMachine Node.app/Contents/Frameworks"
          else
            echo "ERROR: NX path not found"
            exit 1
          fi
        fi
      fi
    fi
  fi
fi

if test -d "$CLIENT_ROOT/../MacOS"; then
  HOST_OS="MacOS"
else
  HOST_OS="Linux"
fi

if [ -e "/bin/grep" ];
then
  COMMAND_GREP="/bin/grep"
else
  if [ -e "/usr/bin/grep" ];
  then
    COMMAND_GREP="/usr/bin/grep"
  else
    echo "ERROR: grep command not found"
    exit 1
  fi
fi

if [ -e "/bin/sed" ];
then
  COMMAND_SED="/bin/sed"
else
  if [ -e "/usr/bin/sed" ];
  then
    COMMAND_SED="/usr/bin/sed"
  else
    echo "ERROR: sed command not found"
    exit 1
  fi
fi

if [ -e "/bin/rm" ];
then
  COMMAND_RM="/bin/rm"
else
  echo "ERROR: rm command not found"
  exit 1
fi

if [ -e "/bin/ps" ];
then
  COMMAND_PS="/bin/ps"
else
  echo "ERROR: ps command not found"
  exit 1
fi

if [ "$HOST_OS" = "Linux" ]; then

  if [ -e "/sbin/lsmod" ];
  then
    COMMAND_LSMOD="/sbin/lsmod"
  elif [ -e "/bin/lsmod" ];
  then
    COMMAND_LSMOD="/bin/lsmod"
  else
    echo "ERROR: lsmod command not found"
    exit 1
  fi

  if [ -e "/sbin/rmmod" ];
  then
    COMMAND_RMMOD="/sbin/rmmod"
  else
    echo "ERROR: rmmod command not found"
    exit 1
  fi

  if [ -e "/sbin/insmod" ];
  then
    COMMAND_INSMOD="/sbin/insmod"
  else
    echo "ERROR: insmod command not found"
    exit 1
  fi

fi

if [ "$HOST_OS" = "MacOS" ]; then

  if [ -e "/sbin/kextload" ];
  then
    COMMAND_KEXTLOAD="/sbin/kextload"
  elif [ -e "/bin/kextload" ];
  then
    COMMAND_KEXTLOAD="/bin/kextload"
  else
    echo "ERROR: kextload command not found"
    exit 1
  fi

  if [ -e "/sbin/kextunload" ];
  then
    COMMAND_KEXTUNLOAD="/sbin/kextunload"
  elif [ -e "/bin/kextunload" ];
  then
    COMMAND_KEXTUNLOAD="/bin/kextunload"
  else
    echo "ERROR: kextunload command not found"
    exit 1
  fi

  if [ -e "/bin/launchctl" ];
  then
    COMMAND_LAUNCHCTL="/bin/launchctl"
  elif [ -e "/usr/bin/launchctl" ];
  then
    COMMAND_LAUNCHCTL="/usr/bin/launchctl"
  else
    echo "ERROR: launchctl command not found"
    exit 1
  fi

  if [ -e "/usr/sbin/system_profiler" ];
  then
    COMMAND_SYSPROFILE="/usr/sbin/system_profiler"
  elif [ -e "/usr/bin/system_profiler" ];
  then
    COMMAND_SYSPROFILE="/usr/bin/system_profiler"
  elif [ -e "/bin/system_profiler" ];
  then
    COMMAND_SYSPROFILE="/bin/system_profiler"
  else
    echo "ERROR: system_profiler command not found"
    exit 1
  fi

  if [ -e "/usr/sbin/sw_vers" ];
  then
    COMMAND_SWVERS="/usr/sbin/sw_vers"
  elif [ -e "/usr/bin/sw_vers" ];
  then
    COMMAND_SWVERS="/usr/bin/sw_vers"
  elif [ -e "/bin/sw_vers" ];
  then
    COMMAND_SWVERS="/bin/sw_vers"
  else
    echo "ERROR: sw_vers command not found"
    exit 1
  fi
fi

COMMAND_CAT="/bin/cat"

NXUSB_PID_FILE="/var/run/nxusb.pid"

NX_BIN="/bin/"

NXUSB_NAME="nxusbd"

NXUSB_MODULE_NAME="nxusb"
NXUSB_CONTROL_SOCKET_PATH="/tmp/nxusb"

if [ "$HOST_OS" = "Linux" ];
then

  NXUSB_NAME_PATH="$CLIENT_ROOT$NX_BIN$NXUSB_NAME"
  NXUSB_MODULE="$NXUSB_MODULE_NAME.ko"
  NXUSB_MODULE_NAME_PATH="$CLIENT_ROOT/bin/drivers/$NXUSB_MODULE"

else
  
  if [ -e "/usr/sbin/kextstat" ];
  then
    COMMAND_KEXTSTAT="/usr/sbin/kextstat"
  elif [ -e "/sbin/kextstat" ];
  then
    COMMAND_KEXTSTAT="/bin/kextstat"
  else
    echo "ERROR: kextstat command not found"
    exit 1
  fi

  NXUSB_NAME_PATH="/Applications/NoMachine.app/Contents/Resources/com.nomachine.nxusb.plist"
  NXUSB_HELPER_KEXT="nxusb_helper.kext"
  NXUSB_IO_KEXT="nxusb_io2.kext"

  if [ "`$COMMAND_KEXTSTAT | $COMMAND_GREP com.apple.iokit.IOUSBFamily | $COMMAND_SED -e 's/.*(//g' | $COMMAND_SED -e 's/).*//g' | $COMMAND_SED -e 's/\.//g'`" -lt "500" ];
  then
    NXUSB_IO_KEXT="nxusb_io1.kext"
  fi

  if [ -d "/Library/Extensions/$NXUSB_HELPER_KEXT" ];
  then
    NXUSB_HELPER_KEXT_NAME_PATH="/Library/Extensions/$NXUSB_HELPER_KEXT"
  else
    NXUSB_HELPER_KEXT_NAME_PATH="/System/Library/Extensions/$NXUSB_HELPER_KEXT"
  fi

  if [ -d "/Library/Extensions/$NXUSB_IO_KEXT" ];
  then
    NXUSB_IO_KEXT_NAME_PATH="/Library/Extensions/$NXUSB_IO_KEXT"
  else
    NXUSB_IO_KEXT_NAME_PATH="/System/Library/Extensions/$NXUSB_IO_KEXT"
  fi

  NXUSB_HELPER_KEXT_BUNDLE_NAME="com.nomachine.driver.nxusb.helper"
  NXUSB_IO_KEXT_BUNDLE_NAME="com.nomachine.driver.nxusb.io"

fi


removeSocket()
{
  while [ -e $NXUSB_CONTROL_SOCKET_PATH ];
  do
    $COMMAND_RM $NXUSB_CONTROL_SOCKET_PATH
  done

}

isRunning()
{
  if [ "$HOST_OS" = "Linux" ];
  then
    if [ -f "$NXUSB_PID_FILE" ];
    then
      NXUSB_PID=`$COMMAND_CAT "$NXUSB_PID_FILE"`

      kill -0 $NXUSB_PID 2>/dev/null
      RUN=$?
      if [ "$RUN" -eq 0 ];
      then
        return 1;
      else
        return 0;
      fi
    else
      return 0;
    fi 
  else
    RES=`$COMMAND_LAUNCHCTL list | $COMMAND_GREP nxusb | $COMMAND_AWK '{print $1}'`

    if [ "x$RES" = "x-" ];
    then
      $COMMAND_LAUNCHCTL unload $NXUSB_NAME_PATH
    fi

    RES=`$COMMAND_LAUNCHCTL list | $COMMAND_GREP nxusb`

    if [ "x$RES" != "x" ];
    then
      return 1;
    else
      return 0;
    fi
  fi
}

startDaemon()
{
  if [ ! -f "$NXUSB_NAME_PATH" ];
  then
    echo "Please install nxusb package first."
    return 1
  fi

  isRunning
  RUNNING=$?
  if [ "$RUNNING" -ne 0 ];
  then
    echo "The daemon is already launched."
    return 1
  fi

  if [ "$HOST_OS" = "Linux" ];
  then

    NX_VAR_PATH="/usr/NX/var" "$NXUSB_NAME_PATH" 1> /dev/null

  else

    systemVersion=`$COMMAND_SWVERS | $COMMAND_GREP "ProductVersion:" | $COMMAND_GREP -o "[0-9]*\..*"`

    if [[ "$systemVersion" == 10.6* || "$systemVersion" == 10.7* || "$systemVersion" == "10.8.0" \
       || "$systemVersion" == "10.8.1" || "$systemVersion" == "10.8.2"|| "$systemVersion" == "10.8.3" \
       || "$systemVersion" == "10.8.4" || "$systemVersion" == "10.8.5" || "$systemVersion" == "10.8" \
       || "$systemVersion" == "10.9" || "$systemVersion" == "10.9.1" || "$systemVersion" == "10.9.2" \
       || "$systemVersion" == "10.9.3" || "$systemVersion" == "10.9.4" || "$systemVersion" == "10.9.5" \
       || "$systemVersion" == "10.10" ]];
    then

      $COMMAND_LAUNCHCTL load $NXUSB_NAME_PATH

    else
      echo "ERROR: Nxusb disabled on Mac OS X $systemVersion"
      exit 1
    fi

  fi

  return 0

}

stopDaemon()
{
  isRunning
  RUNNING=$?
  if [ "$RUNNING" -ne 0 ];
  then

    if [ "$HOST_OS" = "Linux" ];
    then

      $NXUSB_NAME_PATH "--kill"

    else
    
      $COMMAND_LAUNCHCTL unload $NXUSB_NAME_PATH

    fi

  else
    echo "The daemon is not launched."
    return 1
  fi
}

forceKillDaemon()
{
  if [ -f "$NXUSB_PID_FILE" ];
  then
    NXUSB_PID=`$COMMAND_CAT "$NXUSB_PID_FILE"`

    kill -0 $NXUSB_PID 2>/dev/null
    RUN=$?
    if [ "$RUN" -eq 0 ];
    then
      kill -9 $NXUSB_PID 2>/dev/null
      isRunning
      return $?;
    else
      return 0;
    fi
  else
    return 0;
  fi 
}

restartDaemon()
{
  stopDaemon
  isRunning
  RUNNING=$?
  if [ "$RUNNING" -ne 0 ];
  then
    forceKillDaemon;
    RUNNING=$?
    removeSocket
    if [ "$RUNNING" -eq 0 ];
    then
      return 1;
    fi
  fi
  removeSocket
  startDaemon
}

loadModule()
{
  if [ "$HOST_OS" = "Linux" ];
  then

    if [ ! -f $NXUSB_MODULE_NAME_PATH ];
    then
      echo "Please install nxusb package first."
      return 1
    fi

    module=`$COMMAND_LSMOD | $COMMAND_AWK '{print $1}' | $COMMAND_GREP $NXUSB_MODULE_NAME`

    if [ ${#module} -ne 0 ];
    then
      echo "The module is already loaded."
      return 1
    fi

    $COMMAND_INSMOD $NXUSB_MODULE_NAME_PATH

#  else 
#
#    if [ ! -d "$NXUSB_HELPER_KEXT_NAME_PATH" ];
#    then
#      echo "Nxusb helper kext is not present."
#      return 1
#    fi
#
#    if [ ! -d "$NXUSB_IO_KEXT_NAME_PATH" ];
#    then
#      echo "Nxusb io kext is not present."
#      return 1
#    fi
#
#    kext=`$COMMAND_KEXTSTAT -b $NXUSB_HELPER_KEXT_BUNDLE_NAME -l`
#      
#    if [ "x$kext" != "x" ];
#    then
#      echo "The helper kext is already loaded."
#    else
#      $COMMAND_KEXTLOAD "$NXUSB_HELPER_KEXT_NAME_PATH"
#    fi
#
#    kext=`$COMMAND_KEXTSTAT -b $NXUSB_IO_KEXT_BUNDLE_NAME -l`
#      
#    if [ "x$kext" != "x" ];
#    then
#      echo "The io kext is already loaded."
#    else
#      $COMMAND_KEXTLOAD "$NXUSB_IO_KEXT_NAME_PATH"
#    fi

  fi

  return 0
}

unloadModule()
{
  if [ "$HOST_OS" = "Linux" ];
  then
    module=`$COMMAND_LSMOD | $COMMAND_AWK '{print $1}' | $COMMAND_GREP $NXUSB_MODULE_NAME`

    if [ ${#module} == 0 ];
    then

      echo "The module is not loaded."

      return 1

    else

      $COMMAND_RMMOD $NXUSB_MODULE_NAME > /dev/null 2>&1 &
      return 0

    fi
  else

    kext=`$COMMAND_KEXTSTAT -b $NXUSB_HELPER_KEXT_BUNDLE_NAME -l`
      
    if [ "x$kext" = "x" ];
    then

      echo "The helper kext is not loaded."

    else
      $COMMAND_KEXTUNLOAD "$NXUSB_HELPER_KEXT_NAME_PATH"
    fi
    
    kext=`$COMMAND_KEXTSTAT -b $NXUSB_IO_KEXT_BUNDLE_NAME -l`

    if [ "x$kext" = "x" ];
    then

      echo "The io kext is not loaded."

    else
      $COMMAND_KEXTUNLOAD "$NXUSB_IO_KEXT_NAME_PATH"
    fi


  fi
}


case "$3" in
'--start')      startDaemon     ;;
'--stop')       stopDaemon      ;;
'--restart')    restartDaemon   ;;
'--load')       loadModule      ;;
'--unload')     unloadModule    ;;
'--cleanup')    removeSocket    ;;
esac


