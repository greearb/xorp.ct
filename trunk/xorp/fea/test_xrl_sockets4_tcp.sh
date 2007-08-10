#!/usr/bin/env bash

#
# $XORP: xorp/fea/test_xrl_sockets4_tcp.sh,v 1.4 2007/08/09 00:46:57 pavlin Exp $
#

#
# Test FEA TCP support.
#
# This script started with no arguments will start all required process and
# terminate them at the end of the tests.
#

set -e

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

TESTS="test_xrl_sockets4_tcp"

# Include command line
. ${srcdir}/../utils/args.sh

if [ $START_PROGRAMS = "yes" ] ; then
    CXRL="../libxipc/call_xrl -w 10 -r 10"
    ../utils/runit $QUIET $VERBOSE -c "$0 -s -c $*" <<EOF
    ../libxipc/xorp_finder = $CXRL finder://finder/finder/0.2/get_xrl_targets
    ../fea/xorp_fea = $CXRL finder://fea/common/0.1/get_target_name
EOF
    exit $?
fi

if [ $CONFIGURE = "yes" ]
then
    configure_fea
    set +e
fi

for t in ${TESTS} ; do
    $t
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
        echo
        echo "$0: Tests Failed"
        exit ${_ret_value}
    fi
done

echo
echo "$0: Tests Succeeded"
exit 0
