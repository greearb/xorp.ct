#/usr/bin/env bash

#
# $XORP: xorp/bgp/harness/soak.sh,v 1.2 2002/12/18 03:17:51 atanu Exp $
#

#
# Soak test
#
set -e

TESTS="test_peering1.sh test_peering2.sh test_routing1.sh test_rib1.sh 
    test_rib_fea1.sh
    test_terminate.sh"

while :
do
    for i in $TESTS
    do
	./$i
	./$i -l
    done
done

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
