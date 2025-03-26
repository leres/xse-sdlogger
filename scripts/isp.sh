#!/bin/sh
# @(#) $Id: isp.sh 131 2021-05-29 21:04:22Z leres $ (XSE)

f="$*"

if [ ! -r "${f}" ]; then
	echo "missing file ${f}" 2>&1
	exit 1
fi

avrdude \
    -B 1 \
    -V \
    -p atmega1284p  \
    -c usbtiny  \
    -U flash:w:${f}:i
