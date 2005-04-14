#!/bin/sh

#
# $XORP: xorp/policy/test/dummy_test.sh,v 1.2 2004/09/22 21:14:04 pavlin Exp $
#

if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi

test_policy() {
	${srcdir}/test_policy.sh $1 $2 $3 $4

	if [ $? -ne 0 ]
	then
		exit 1
	fi	
}

test_accept() {
	test_policy $1 $2 $3 1
}

test_reject() {
	test_policy $1 $2 $3 0
}


test_accept "${srcdir}/policy1.src" "${srcdir}/policyvarmap.conf" "${srcdir}/policy1.var"
test_reject "${srcdir}/policy2.src" "${srcdir}/policyvarmap.conf" "${srcdir}/policy2.var"

exit 0
