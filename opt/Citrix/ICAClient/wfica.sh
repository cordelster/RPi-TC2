#!/bin/sh
ICAROOT=/opt/Citrix/ICAClient 
export ICAROOT
$ICAROOT/wfica -file $1
