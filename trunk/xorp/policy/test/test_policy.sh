#!/bin/sh

#
# $XORP$
#

TMPFILE=/tmp/xorp_policy_test.txt

cleanup() {
	rm -f ${TMPFILE}
}

if [ $# -ne "3" ]
then
	echo "Usage: `basename $0` policyfile varfile exitcode"
        exit 1
fi


echo Will run policy $1 with variables $2 checking for exit code $3

if ! ./compilepolicy -s $1 -o ${TMPFILE}
then
exit 1
fi

cat ${TMPFILE}


./execpolicy ${TMPFILE} $2
EXITCODE=$?
if [ ${EXITCODE} -ne $3 ]
then
cleanup
echo Test FAILED exit code ${EXITCODE}
exit 1
fi

cleanup

echo Test was SUCCESSFULL
exit 0
