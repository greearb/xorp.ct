#/usr/bin/env bash

#
# $XORP: xorp/bgp/harness/soak.sh,v 1.3 2002/12/09 10:59:37 pavlin Exp $
#

#
# Soak test
#
set -e

TESTS="test_peering1.sh test_routing1.sh test_rib1.sh"

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
