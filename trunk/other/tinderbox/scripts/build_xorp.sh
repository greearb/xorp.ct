#!/bin/sh

CONFIG="$(dirname $0)/config"
. $CONFIG

#
# Handler for SIGTERM.  Exits with supplied exit code.
#
sigterm_handler()
{
    exit ${1}
}

#
# Send SIGTERM to all processes in group and exit with errorcode
# supplied in argument 1.
#
tidy_exit()
{
    errcode=$1
    trap "sigterm_handler $errcode" TERM
    kill 0
}

timeout()
{
    local err bar
    err="Process appears wedged. Timed out after ${TIMEOUT} seconds."
    bar=`echo $err | sed 's/./=/g'`
    printf "\n${bar}\n${err}\n${bar}\n"
    tidy_exit 1
}

# A test "process" that can be used to appear wedged.
funkster()
{
    local iter
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

CMD="./configure && gmake -k $@"
#CMD=funkster
( ( eval $CMD ) 2>&1 ; echo $? > ${WAKEFILE} ) &

expiry=`date "+%s"`
expiry=`expr $expiry + ${TIMEOUT}`
while [ ! -f ${WAKEFILE} ] ; do
    now=`date "+%s"`
    if [ "$now" -gt "$expiry" ] ; then
	timeout
	break
    fi
    sleep ${TIMEOUT_CHECK}
done

if [ -f ${WAKEFILE} ] ; then
    err=`cat ${WAKEFILE}`
    rm -f ${WAKEFILE}
    tidy_exit $err
fi

tidy_exit 100