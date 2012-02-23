#!/bin/sh

gcc -g -O0 -o getprocbyargs \
	../utils.c getprocbyargs.c

gcc -g -O0 -o getprocstarttime \
	../utils.c getprocstarttime.c

gcc -g -O0 -o killproc \
	../utils.c killproc.c
