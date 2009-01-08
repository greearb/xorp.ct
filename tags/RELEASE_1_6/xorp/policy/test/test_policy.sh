#!/bin/sh

#
# $XORP: xorp/policy/test/test_policy.sh,v 1.2 2004/10/05 22:42:27 pavlin Exp $
#

TMPFILE=/tmp/xorp_policy_test.txt

cleanup() {
	rm -f ${TMPFILE}
}

if [ $# -ne "4" ]
then
	echo "Usage: `basename $0` policyfile policy_var_map_file varfile exitcode"
        exit 1
fi


echo Will run policy $1 with policy_var_map_file $2 with variables $3 checking for exit code $4

./compilepolicy -s $1 -m $2 -o ${TMPFILE}
if [ $? -ne 0 ] ; then 
exit 1
fi

cat ${TMPFILE}


./execpolicy ${TMPFILE} $3
EXITCODE=$?
if [ ${EXITCODE} -ne $4 ]
then
cleanup
echo Test FAILED exit code ${EXITCODE}
exit 1
fi

cleanup

echo Test was SUCCESSFULL
exit 0
