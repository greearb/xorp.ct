#!/bin/sh

CONFIG="$(dirname $0)/config"
. $CONFIG

TIMEOUT=3600

timeout()
{
    echo "-----------------------------------------------------------"
    echo "Process appears wedged. Timed out after ${TIMEOUT} seconds."
    echo "-----------------------------------------------------------"
    kill 0
}

# A test "process" that can be used to appear wedged.
funkster()
{
    iter=0
    while [ 1 ] ; do
	printf "."
	sleep 1
	iter=`expr $iter + 1`
	if [ $iter -gt 30 ] ; then
	    break
	fi
    done
}

###############################################################################

XORPDIR=${ROOTDIR}/xorp
WAKEFILE=".wake-$$"

cd ${XORPDIR}
rm -f ${WAKEFILE} 2>/dev/null

#
# This little nasty run these commands in the background.  We use
# ${WAKEFILE} both to detect the completion of these commands and to
# propagate the return value.
#
( ( ./configure 2>&1 && gmake -k $@ 2>&1 ) ; echo $? > ${WAKEFILE} ) &

#( ( funkster ) ; cat /dev/null > ${WAKEFILE} ) &

expiry=`date "+%s"`
expiry=`expr $expiry + ${TIMEOUT}`
while [ ! -f ${WAKEFILE} ] ; do
    now=`date "+%s"`
    if [ "$now" -gt "$expiry" ] ; then
	timeout
	break
    fi
    sleep 10
done

if [ -f ${WAKEFILE} ] ; then
    err=`cat ${WAKEFILE}`
    rm -f ${WAKEFILE}
    exit $err
fi

exit 100