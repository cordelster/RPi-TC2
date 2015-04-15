######################################################################
#                                                                    #
#  Copyright (c) 2001, 2014 NoMachine, http://www.nomachine.com.     #
#                                                                    #
#  All rights reserved.                                              #
#                                                                    #
######################################################################

PARAMS=$#
PARENT_USER=$1
PARENT_APP=$2

if test -x "/bin/echo"; then
  COMMAND_ECHO="/bin/echo"
else
  echo "NX> 500 Error: echo command not found."
  exit 1
fi

errorMsg ()
{
  ${COMMAND_ECHO} "NX> 500 Error: $1"
  if test "x$2" = "x1"; then
    exit 1
  fi
}

errorCommandNotFound ()
{
  errorMsg "$1 command not found" $2
}

if test -x "/usr/bin/nawk"; then
  COMMAND_AWK="/usr/bin/nawk"
elif test -x "/usr/xpg4/bin/awk"; then
  COMMAND_AWK="/usr/xpg4/bin/awk"
elif test -x "/bin/awk"; then
  COMMAND_AWK="/bin/awk"
elif test -x "/usr/bin/awk"; then
  COMMAND_AWK="/usr/bin/awk"
else
  errorCommandNotFound "awk" "1"
fi

if test -x "/bin/grep"; then
  COMMAND_GREP="/bin/grep"
elif test -x "/usr/bin/grep"; then
  COMMAND_GREP="/usr/bin/grep"
else
  errorCommandNotFound "grep" "1"
fi

if test -x "/bin/sed"; then
  COMMAND_SED="/bin/sed"
elif test -x "/usr/bin/sed"; then
  COMMAND_SED="/usr/bin/sed"
else
  errorCommandNotFound "sed" "1"
fi

if test -x "/bin/ls"; then
  COMMAND_LS="/bin/ls"
elif test -x "/usr/bin/ls"; then
  COMMAND_LS="/usr/bin/ls"
else
  errorCommandNotFound "ls" "1"
fi

if test -x "/bin/cat"; then
  COMMAND_CAT="/bin/cat"
elif test -x "/usr/bin/cat"; then
  COMMAND_CAT="/usr/bin/cat"
else
  errorCommandNotFound "cat" "1"
fi

if test -x "/bin/su"; then
  COMMAND_SU="/bin/su"
elif test -x "/usr/bin/su"; then
  COMMAND_SU="/usr/bin/su"
else
  errorCommandNotFound "su" "1"
fi

if test -x "/bin/kill"; then
  COMMAND_KILL="/bin/kill"
elif test -x "/usr/bin/kill"; then
  COMMAND_KILL="/usr/bin/kill"
else
  errorCommandNotFound "kill" "1"
fi

if test -x "/bin/cut"; then
  COMMAND_CUT="/bin/cut"
elif test -x "/usr/bin/cut"; then
  COMMAND_CUT="/usr/bin/cut"
else
  errorCommandNotFound "cut" "1"
fi

if test -x "/bin/mkdir"; then
  COMMAND_MKDIR="/bin/mkdir"
elif test -x "/usr/bin/mkdir"; then
  COMMAND_MKDIR="/usr/bin/mkdir"
else
  errorCommandNotFound "mkdir" "1"
fi

if test -x "/bin/rm"; then
  COMMAND_RM="/bin/rm"
elif test -x "/usr/bin/rm"; then
  COMMAND_RM="/usr/bin/rm"
else
  errorCommandNotFound "rm" "1"
fi

if test -x "/bin/cp"; then
  COMMAND_CP="/bin/cp"
elif test -x "/usr/bin/cp"; then
  COMMAND_CP="/usr/bin/cp"
else
  errorCommandNotFound "cp" "1"
fi

if test -x "/bin/mv"; then
  COMMAND_MV="/bin/mv"
elif test -x "/usr/bin/mv"; then
  COMMAND_MV="/bin/bin/mv"
else
  errorCommandNotFound "mv" "1"
fi

if test -x "/bin/chmod"; then
  COMMAND_CHMOD="/bin/chmod"
elif test -x "/usr/bin/chmod"; then
  COMMAND_CHMOD="/usr/bin/chmod"
else
  errorCommandNotFound "chmod" "1"
fi

if test -x "/bin/chown"; then
  COMMAND_CHOWN="/bin/chown"
elif test -x "/usr/bin/chown"; then
  COMMAND_CHOWN="/usr/bin/chown"
elif test -x "/usr/sbin/chown"; then
  COMMAND_CHOWN="/usr/sbin/chown"
else
  errorCommandNotFound "chown" "1"
fi

if test -x "/usr/xpg4/bin/id"; then
  COMMAND_ID="/usr/xpg4/bin/id"
elif test -x "/bin/id"; then
  COMMAND_ID="/bin/id"
elif test -x "/usr/bin/id"; then
  COMMAND_ID="/usr/bin/id"
else
  errorCommandNotFound "id" "1"
fi

if test -x "/usr/bin/expr"; then
  COMMAND_EXPR="/usr/bin/expr"
elif test -x "/bin/expr"; then
  COMMAND_EXPR="/bin/expr"
elif test -x "/usr/local/bin/expr"; then
  COMMAND_EXPR="/usr/local/bin/expr"
else
  errorCommandNotFound "expr" "1"
fi

if test -x "/bin/sleep"; then
  COMMAND_SLEEP="/bin/sleep"
elif test -x "/usr/bin/sleep"; then
  COMMAND_SLEEP="/usr/bin/sleep"
else
  errorCommandNotFound "sleep" "1"
fi

if test -x "/usr/ucb/whoami"; then
  COMMAND_WHOAMI="/usr/ucb/whoami"
elif test -x "/usr/bin/whoami"; then
  COMMAND_WHOAMI="/usr/bin/whoami"
else
  errorCommandNotFound "whoami" "1"
fi

checkParametersCount()
{
  if test "x${PARAMS_COUNT}" != "x"; then
    if test "${PARAMS}" -ne "${PARAMS_COUNT}"; then
      errorMsg "Wrong parameters count." "1"
    fi
    return 0
  elif test "x${PARAMS_COUNT_MIN}" != "x" -a "x${PARAMS_COUNT_MAX}" != "x"; then
    if test ${PARAMS} -lt ${PARAMS_COUNT_MIN} -o ${PARAMS} -gt ${PARAMS_COUNT_MAX}; then
      errorMsg "Wrong parameters range." "1"
    fi
    return 0
  fi

  errorMsg "Not defined the number of allowed parameters." "1"
}

getNXRoot()
{
  if test "x${RestrictedDir}" = "x"; then
    errorMsg "Error with retrieving restricted directory." "1"
  fi

  NX_ROOT=$(cd "${RestrictedDir}/../.." && /bin/pwd)

  if test "x${NX_ROOT}" = "x"; then
    errorMsg "Error with retrieving NX installation folder." "1"
  fi

  if test ! -d "${NX_ROOT}"; then
    errorMsg "Cannot stat NX installation folder: ${NX_ROOT}." "1"
  fi
}

getNodeRoot()
{
  COMMAND_NXNODE="${NX_ROOT}/bin/nxnode"
  if test ! -x "${COMMAND_NXNODE}"; then
    errorCommandNotFound "${COMMAND_NXNODE}" "1"
  fi

  NODE_CONFIG_FILE="${NX_ROOT}/etc/node.cfg"
  if test ! -e "${NODE_CONFIG_FILE}"; then
    errorMsg "Config file ${NODE_CONFIG_FILE} doesn't exist." "1"
  fi
}

getServerRoot()
{

  COMMAND_NXSERVER="${NX_ROOT}/bin/nxserver"
  if test ! -x "${COMMAND_NXSERVER}"; then
    errorCommandNotFound "${COMMAND_NXSERVER}" "1"
  fi

  SERVER_CONFIG_FILE="${NX_ROOT}/etc/server.cfg"
  if test ! -e "${SERVER_CONFIG_FILE}"; then
    errorMsg "Config file ${SERVER_CONFIG_FILE} doesn't exist." "1"
  fi
}

checkParametersCount

getNXRoot

if test "x${PARENT_APP}" = "xnode" -o "x${PARENT_APP}" = "xserver"; then
  getNodeRoot

  if test "x${PARENT_APP}" = "xserver"; then
    getServerRoot
  fi
fi


getRootGName ()
{
  ROOTGNAME=`${COMMAND_ID} -gn root`
  if test "x${ROOTGNAME}" = "x"; then
    errorMsg "Cannot read group owner of root user." "1"
  fi
}

getCUPSSbin ()
{
  tmp=`${COMMAND_AWK} -F '"' '/^ *CUPSSbin/ {print $2}' "${NODE_CONFIG_FILE}"`
  if test "x$tmp" = "x"; then
    tmp=`${COMMAND_AWK} -F ' ' '/^ *CUPSSbin/ {print $2}' "${NODE_CONFIG_FILE}"`
  fi

  if test "x${tmp}" != "x"; then
    LPADMIN="${tmp}/lpadmin"
  else
    errorMsg "CUPS support not configured correctly." "1"
  fi

  if test ! -x ${LPADMIN}; then
    errorMsg "Wrong PATH for lpadmin binary." "1"
  fi
}

isEnabledCUPSSupport ()
{
  tmp=`${COMMAND_AWK} -F '"' '/^ *EnableCUPSSupport/ {print $2}' "${NODE_CONFIG_FILE}"`
  if [ "x$tmp" = "x" ]; then
    tmp=`${COMMAND_AWK} -F ' ' '/^ *EnableCUPSSupport/ {print $2}' "${NODE_CONFIG_FILE}"`
  fi

  if test "x${tmp}" = "x" -o "x${tmp}" = "x0"; then
    errorMsg "CUPS support is disabled." "1"
  fi
}

isRunByNode ()
{
  if test "x${PARENT_APP}" != "xnode"; then
    errorMsg "Only the nxnode can run this script." "1"
  fi
}

isRunByServer ()
{
  if test "x${PARENT_APP}" != "xserver"; then
    errorMsg "Only the nxserver can run this script." "1"
  fi
}

isRunByServerOrNode ()
{
  if test "x${PARENT_APP}" != "xserver"; then
    if test "x${PARENT_APP}" != "xnode"; then
      errorMsg "Only nxserver or nxnode can run this script." "1"
    fi
  fi
}

isRunByNodeOrPlayer ()
{
  if test "x${PARENT_APP}" != "xnode"; then
    if test "x${PARENT_APP}" != "xplayer"; then
      errorMsg "Only nxnode or nxplayer can run this script." "1"
    fi
  fi
}

isRunByNodeOrClient ()
{
  if test "x${PARENT_APP}" != "xnode"; then
    if test "x${PARENT_APP}" != "xclient"; then
      errorMsg "Only nxnode or nxclient can run this script." "1"
    fi
  fi
}

isRunByUserNX ()
{
  if test "x${PARENT_USER}" != "xnx"; then
    errorMsg "Only user nx can run this script." "1"
  fi
}
