#!/bin/sh

gcc -o vipup -Iifupdown \
	ifupdown/*.c utils.c vipupdown.c

ln -f -s vipup vipdown
