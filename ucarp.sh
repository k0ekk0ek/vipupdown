#!/bin/sh
# ucarp.sh should replace /etc/networks/if-up.d/ucarp
UCARP=/usr/sbin/ucarp

if [ ! -x $UCARP ]; then
	exit 0
fi

# Repeat steps for all UCARP instances configured in /etc/network/interfaces.
for vip in `printenv | sed -n 's/^\(IF_UCARP_VIP[0-9]*\)=.*$/\1/p'`; do
    i=`echo $vip | sed 's/IF_UCARP_VIP//'`

    # Set local variables to work with.
    EXTRA_PARAMS=""
    eval UCARP_VID=\$"IF_UCARP_VID$i"
    eval UCARP_VIP=\$"IF_UCARP_VIP$i"
    eval UCARP_PASSWORD=\$"IF_UCARP_PASSWORD$i"
    eval UCARP_UPSCRIPT=\$"IF_UCARP_UPSCRIPT$i"
    eval UCARP_DOWNSCRIPT=\$"IF_UCARP_DOWNSCRIPT$i"
    eval UCARP_MASTER=\$"IF_UCARP_MASTER$i"
    eval UCARP_ADVSKEW=\$"IF_UCARP_ADVSKEW$i"
    eval UCARP_ADVBASE=\$"IF_UCARP_ADVBASE$i"
    eval UCARP_DEADRATIO=\$"IF_UCARP_DEADRATIO$i"
    eval UCARP_NOMCAST=\$"IF_UCARP_NOMCAST$i"
    eval UCARP_XPARAM=\$"IF_UCARP_XPARAM$i"

    if [ -n "$UCARP_VID" -a -n "$UCARP_VIP" -a -n "$UCARP_PASSWORD" ]; then
        if [ -z "$UCARP_UPSCRIPT" ]; then
            UCARP_UPSCRIPT=/usr/share/ucarp/vip-up
        fi

        if [ -z "$UCARP_DOWNSCRIPT" ]; then
            UCARP_DOWNSCRIPT=/usr/share/ucarp/vip-down
        fi

        if [ -n "$UCARP_MASTER" ]; then
            if ! expr "$UCARP_MASTER" : "no\|off\|false\|0" > /dev/null; then
                EXTRA_PARAMS="-P"
            fi
        fi

        if [ -n "$UCARP_ADVSKEW" ]; then
            EXTRA_PARAMS="$EXTRA_PARAMS -k $UCARP_ADVSKEW"
        fi

        if [ -n "$UCARP_ADVBASE" ]; then
            EXTRA_PARAMS="$EXTRA_PARAMS -b $UCARP_ADVBASE"
        fi

        if [ -n "$UCARP_DEADRATIO" ]; then
            EXTRA_PARAMS="$EXTRA_PARAMS -r $UCARP_DEADRATIO"
        fi

        if [ -n "$UCARP_NOMCAST" ]; then
            if ! expr "$UCARP_NOMCAST" : "no\|off\|false\|0" > /dev/null; then
                EXTRA_PARAMS="$EXTRA_PARAMS -M"
            fi
        fi

        if [ -n "$UCARP_XPARAM" ]; then
            EXTRA_PARAMS="$EXTRA_PARAMS -x $UCARP_XPARAM"
        fi

        $UCARP -i $IFACE -s $IF_ADDRESS -B -z -v $UCARP_VID \
            -p $UCARP_PASSWORD -a $UCARP_VIP -u $UCARP_UPSCRIPT \
            -d $UCARP_DOWNSCRIPT $EXTRA_PARAMS
    fi
done

