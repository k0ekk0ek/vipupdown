#!/bin/sh
# vip-up.sh should replace /usr/share/ucarp/vip-up
IFACE=`vipif $1 $2`

if [ -n "$IFACE" ]; then
  /sbin/ifup $IFACE
fi

