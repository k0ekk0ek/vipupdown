#!/bin/sh
# vip-down.sh should replace /usr/share/ucarp/vip-down
IFACE=`vipif $1 $2`

if [ -n "$IFACE" ]; then
  /sbin/ifdown $IFACE
fi

