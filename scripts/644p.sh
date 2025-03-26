#!/bin/sh
# @(#) $Id: 644p.sh 143 2023-08-27 21:34:57Z leres $ (XSE)
#
# Program fuses using Sparkfun Pocket Programmer
#

## stock
# http://eleccelerator.com/fusecalc/fusecalc.php?chip=atmega644p&LOW=ee&HIGH=dc&EXTENDED=ff&LOCKBIT=3f

## turn on eeprom save
# http://eleccelerator.com/fusecalc/fusecalc.php?chip=atmega644p&LOW=EE&HIGH=D4&EXTENDED=FF&LOCKBIT=3F

set -x

avrdude \
    -p atmega644p \
    -c usbtiny \
    -F \
    -V \
    -qq \
    -B 8 \
    -e \
    -U lfuse:w:0xD7:m \
    -U hfuse:w:0xD4:m \
    -U efuse:w:0xFE:m \
    -U lock:w:0x3F:m
