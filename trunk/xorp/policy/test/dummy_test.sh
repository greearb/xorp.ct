#!/bin/sh

#
# $XORP: xorp/policy/test/dummy_test.sh,v 1.1 2004/09/17 13:49:00 abittau Exp $
#

if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi

test_policy() {
	${srcdir}/test_policy.sh $1 $2 $3

	if [ $? -ne 0 ]
	then
		exit 1
	fi	
}

test_accept() {
	test_policy $1 $2 1
}

test_reject() {
	test_policy $1 $2 0
}


test_accept "${srcdir}/policy1.src" "${srcdir}/policy1.var"
test_reject "policy2.src" "policy2.var"

exit 0
