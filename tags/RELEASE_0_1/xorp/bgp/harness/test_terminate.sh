#!/usr/bin/env bash

#
# $XORP: xorp/bgp/harness/test_terminate.sh,v 1.4 2002/12/09 10:59:38 pavlin Exp $
#

#
# Test BGP termination
#
# This script started with no arguments will start all required process and
# terminate them at the end of the tests.
#
# Preconditons
# 1) Run a finder process
# 2) Run xorp "../bgp"
#

set -e

. ../xrl_shell_funcs.sh ""
. ./xrl_shell_funcs.sh ""

onexit()
{
    last=$?
    if [ $last = "0" ]
    then
	echo "$0: Tests Succeeded"
    else
	echo "$0: Tests Failed"
    fi

    trap '' 0 2
}

trap onexit 0 2

HOST=localhost
AS=65008

configure_bgp()
{
    LOCALHOST=$HOST
    ID=192.150.187.78
    local_config $AS $ID

    register_rib ${RIB:-""}
}

test1()
{
    CALLXRL=$CALLXRL ../xrl_shell_funcs.sh terminate

    sleep 5
}
    
TESTS_NOT_FIXED=''
TESTS='test1'

# Include command line
. ./args.sh

if [ $START_PROGRAMS = "yes" ]
then
CXRL="$CALLXRL -r 10"
    ../../utils/runit $QUIET $VERBOSE -c "$0 -s -c $*" <<EOF
    ../../libxipc/finder
    ../bgp                = $CXRL finder://bgp/common/0.1/get_target_name
EOF
    trap '' 0
    exit $?
fi

if [ $CONFIGURE = "yes" ]
then
    configure_bgp
fi

for i in $TESTS
do
    $i
done

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
