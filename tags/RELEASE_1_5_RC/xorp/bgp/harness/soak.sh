#!/usr/bin/env bash

#
# $XORP: xorp/bgp/harness/soak.sh,v 1.3 2003/01/29 07:03:58 atanu Exp $
#

#
# Soak test
#
set -e

# srcdir is set by make for check target
if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi

TESTS="test_peering1.sh test_peering2.sh test_routing1.sh test_rib1.sh 
    test_rib_fea1.sh
    test_terminate.sh"

while :
do
    for i in $TESTS
    do
	${srcdir}/$i
	${srcdir}/$i -l
    done
done

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
