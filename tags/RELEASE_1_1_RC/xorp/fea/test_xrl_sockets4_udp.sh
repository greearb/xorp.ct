#!/usr/bin/env bash

set -e

# srcdir is set by make for check target

# srcdir is set by make for check target
if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi
. ${srcdir}/../utils/xrl_shell_lib.sh

test_xrl_sockets4_udp() 
{
    ./test_xrl_sockets4_udp
}

TESTS="test_xrl_sockets4_udp"

# Include command line
. ${srcdir}/../utils/args.sh

if [ $START_PROGRAMS = "yes" ] ; then
    CXRL="../libxipc/call_xrl -w 10 -r 10"
    ../utils/runit $QUIET $VERBOSE -c "$0 -s -c $*" <<EOF
    ../libxipc/xorp_finder = $CXRL finder://finder/finder/0.2/get_xrl_targets
EOF
    exit $?
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
