#!/bin/sh

#
# $XORP$
#

test_policy() {
	./test_policy.sh $1 $2 $3

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


test_accept "policy1.src" "policy1.var"
test_reject "policy2.src" "policy2.var"

exit 0
