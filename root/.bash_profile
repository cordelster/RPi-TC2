#! /bin/bash
# ediatable values:
TIMETOWAIT=1 #time to wait before entering UserMode (in seconds)
ADMINKEY="K" #shift-k the key to press to switch to AdminMode
ADMINPASSWORD="LOL" #password for entering AdminMode

### dont touch here below, thanks :)

echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo performance > /sys/devices/system/cpu/cpu1/cpufreq/scaling_governor
echo performance > /sys/devices/system/cpu/cpu2/cpufreq/scaling_governor
echo performance > /sys/devices/system/cpu/cpu3/cpufreq/scaling_governor

if [[ -z $DISPLAY ]] && [[ $(tty) = /dev/tty1 ]]; then
  read -n1 -s -t $TIMETOWAIT -p "Press secret key to enter admin mode:" ans
  case "$ans" in
    [$ADMINKEY]|[$ADMINKEY])
      /usr/bin/clear_console -q
      echo "Entering Admin mode!"
      touch /tmp/usermode_root
      ;;
    *)
      /usr/bin/clear_console -q
      echo "Entering user mode"
      touch /tmp/usermode_rpitc
      ;;
  esac
fi


trap '' 2
if [ -f /tmp/usermode_rpitc ]
then
  /usr/bin/clear_console -q
  rm -fr /tmp/usermode*
  su - rpitc
fi

if [ -f /tmp/usermode_browser ]
then
  /usr/bin/clear_console -q
  rm -fr /tmp/usermode*
  su - browser
fi

if [ -f /tmp/usermode_root ]
then
  /usr/bin/killall -9 h264
  /usr/bin/clear_console -q
  rm -fr /tmp/usermode*
  while read -s -p "Press secret key to enter admin mode:" ans
  do
     case "$ans" in
      $ADMINPASSWORD)
        echo "password ok!"
        if [[ -z $DISPLAY ]] && [[ $(tty) = /dev/tty1 ]]; then
          exec startx >/dev/null 2>&1
        fi
        break;;
      *)
        echo "wrong password!"
        ;;
    esac
  done
fi
trap 2
