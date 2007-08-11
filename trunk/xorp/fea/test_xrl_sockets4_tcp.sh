#!/usr/bin/env bash

#
# $XORP: xorp/fea/test_xrl_sockets4_tcp.sh,v 1.5 2007/08/10 01:59:33 pavlin Exp $
#

#
# Test FEA TCP support.
#
# This script started with no arguments will start all required process and
# terminate them at the end of the tests.
#

set -e

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

# Conditionally set ${srcdir} if it wasn't assigned (e.g., by `gmake check`)
if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi

. ${srcdir}/../utils/xrl_shell_lib.sh

configure_fea()
{
    echo "Configuring fea"

    export CALLXRL
    FEA_FUNCS=${srcdir}/xrl_shell_funcs.sh
    local tid=$($FEA_FUNCS start_fea_transaction)

    $FEA_FUNCS configure_all_interfaces_from_system $tid

    $FEA_FUNCS commit_fea_transaction $tid
}

test_xrl_sockets4_tcp()
{
    ./test_xrl_sockets4_tcp
}

TESTS_NOT_FIXED=""
TESTS="test_xrl_sockets4_tcp"

# Include command line
RUNITDIR="../utils"
. ${srcdir}/../utils/args.sh

if [ $START_PROGRAMS = "yes" ] ; then
    CXRL="$CALLXRL -r 10"
    runit $QUIET $VERBOSE -c "$0 -s -c $*" <<EOF
    ../libxipc/xorp_finder = $CXRL finder://finder/finder/0.2/get_xrl_targets
    ../fea/xorp_fea        = $CXRL finder://fea/common/0.1/get_target_name
EOF
    trap '' 0
    exit $?
fi

if [ $CONFIGURE = "yes" ] ; then
    configure_fea
    set -e
fi

for i in $TESTS ; do
    $i
done

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
