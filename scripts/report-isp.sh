#!/bin/sh
# @(#) $Id: report-isp.sh 131 2021-05-29 21:04:22Z leres $ (XSE)
#
# Report the fuse settings
#

prog="`basename $0`"

cd `dirname $0` || exit 1

flfuse=/tmp/${prog}.lfuse.$$
fhfuse=/tmp/${prog}.hfuse.$$
fefuse=/tmp/${prog}.efuse.$$
flock=/tmp/${prog}.lock.$$
fsig=/tmp/${prog}.signature.$$

temps="
    ${flfuse}
    ${fhfuse}
    ${fefuse}
    ${flock}
    ${fsig}
    "

cleanup()
{
	rm -f ${temps}
	exit 1
}

trap cleanup 1 2 3 15 EXIT

(set -x
avrdude \
    -p atmega1284p \
    -c usbtiny \
    -V \
    -qq \
    -U lfuse:r:${flfuse}:h \
    -U hfuse:r:${fhfuse}:h \
    -U efuse:r:${fefuse}:h \
    -U lock:r:${flock}:h \
    -U signature:r:${fsig}:h) || exit 1

for fn in ${files}; do
	if [ ! -r ${fn} ]; then
		echo "${prog}: missing ${fn}" 1>& 2
		cleanup
	fi
	if [ ! -s ${fn} ]; then
		echo "${prog}: zero length ${fn}" 1>& 2
		cleanup
	fi
done

sed="sed -e s/^0x// -e s/,0x/,/g"
tr="tr a-z A-Z"

lfuse="`${sed} ${flfuse} | ${tr}`"
hfuse="`${sed} ${fhfuse} | ${tr}`"
efuse="`${sed} ${fefuse} | ${tr}`"
lock="`${sed} ${flock} | ${tr}`"
sig="`${sed} ${fsig} | ${tr}`"

echo "lfuse:	${lfuse}"
echo "hfuse:	${hfuse}"
echo "efuse:	${efuse}"
echo "lock:	${lock}"
echo "sig:	${sig}"

case "${sig}" in

1E,95,F)
	part="atmega328p"
	break
	;;

1E,96,A)
	part="atmega644p"
	break
	;;

1E,97,5)
	part="atmega1284p"
	break
	;;

*)
	echo "${prog}: unknown device (${sig})" 1>&2
	cleanup
	;;
esac

echo "http://eleccelerator.com/fusecalc/fusecalc.php?chip=${part}&LOW=${lfuse}&HIGH=${hfuse}&EXTENDED=${efuse}&LOCKBIT=${lock}"
