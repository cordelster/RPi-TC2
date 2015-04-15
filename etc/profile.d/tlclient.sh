# -*- mode: shell-script; coding: utf-8 -*-
#
# Copyright 2002-2014 Cendio AB.
# For more information, see http://www.cendio.com

new="/opt/thinlinc/bin"
case "${PATH}" in
    ${new}:*|*:${new}:*|*:${new}|${new}) ;;
    "") PATH="${new}" ;;
    *) PATH="${PATH}:${new}" ;;
esac

unset new
export PATH
