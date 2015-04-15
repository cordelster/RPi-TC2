# -*- mode: shell-script; coding: utf-8 -*-
#
# Copyright 2002-2014 Cendio AB.
# For more information, see http://www.cendio.com

if (! $?PATH) then
    setenv PATH ""
endif

set new="/opt/thinlinc/bin"
switch ($PATH)
    case "${new}:*":
    case "*:${new}:*":
    case "*:${new}":
    case "${new}":
        breaksw
    case "":
        setenv PATH "${new}"
        breaksw
    case "*":
        setenv PATH "${PATH}:${new}"
        breaksw
endsw

unset new
