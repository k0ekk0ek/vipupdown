#!/bin/sh

gcc -g -O0 -o vipup -Iifupdown \
	ifupdown/*.c ifupdown.c utils.c vipupdown.c

gcc -g -O0 -o vipif -Iifupdown \
	ifupdown/*.c ifupdown.c utils.c vipif.c

ln -f -s vipup vipdown
