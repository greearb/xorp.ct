#!/bin/sh

# This script runs some of the libxipc tests through Erwin Andreason
# and Henner Zeller's LeakTracer in an attempt to spot memory leaks.
# It is little more than glue to automate both elements of the leak
# detection method - ie generating leak output and correlating it with
# the source code.

#
# Synopsis:
# 	spot_leaks <cmd> <args> 
#
# Description:
# 	Attempt to run command and look for leaks.  The supplied command 
# must be a dynamically linked C++ program since LeakTracer uses dynamic
# link loading to override the new and delete operators in libstdc++.XX.so.
#
spot_leaks()
{
    local cxxlibs

    if [ $# -eq 0 ] ; then
	echo "No command supplied to spots leak."
	return 1;
    fi

    if [ ! -x ${1} ] ; then
	echo "\"$1\" either does not exist or is not executable"
	return 1
    fi

    cxxlibs=`ldd $1 2>/dev/null | grep c++`
    if [ $? -ne 0 ] ; then
	echo "$1 does not appear to be dynamically linked."
	return 1
    fi

    if [ -z "${cxxlibs}" ] ; then
	echo "$1 does not appear to be a C++ generated executable (ignoring)."
	return 0
    fi

    status=">>> Running LeakTracer on \"$*\""
    echo "$status"

    ${LT_HOME}/LeakCheck $* 1>/dev/null 2>&1
    ${LT_HOME}/leak-analyze $1 1>leak-log 2>warn-log

    grep -q 'Gathered' warn-log
    if [ $? -ne 0 ] ; then
	echo "Did not appear to generate data points"
	cat warn-log leak-log
	return 1
    fi
    
    # Get number of unique points, ie leaks
    leaks=`cat warn-log | sed -n '1s/[^(]*(\([0-9][0-9]*\).*/\1/p'`
    ret=0

    if [ "${leaks}" -eq 0 ] ; then
	echo "   ==> No leaks detected."
    else
	echo "   ==> Leaks detected."
	cat warn-log leak-log
	ret=1
    fi
    return ${ret}
}

tidy_up()
{
    rm -rf warn-log leak-log
}

#
# Find LeakTracer installation directory.
#
LEAK_CHECK=`which LeakCheck`
if [ $? -ne 0 ] ; then
    echo "LeakCheck binary not found skipping test."
    exit 0
fi
LT_HOME=`dirname ${LEAK_CHECK}`

#
# These are the "simple" libxipc test programs, i.e. they take no
# arguments
#
SIMPLE_TESTS="./test_xrl_error ./test_xrl ./test_xrl_atom ./test_xrl_args"
SIMPLE_TESTS="${SIMPLE_TESTS} ./test_inproc ./test_stcp ./test_sudp "
SIMPLE_TESTS="${SIMPLE_TESTS} ./test_lemming ./test_xrl_router"
SIMPLE_TESTS="${SIMPLE_TESTS} ./test_finder ./test_finder_msgs"
SIMPLE_TESTS="${SIMPLE_TESTS} ./test_finder_messenger"

failures=0
for t in ${SIMPLE_TESTS} ; do
    spot_leaks $t
    [ $? -eq 0 ] || failures=1
done

#
# There are the "harder" libxipc test programs, ie those that take arguments
# or have test file inputs
#
spot_leaks ./test_finder_events -b 2 -r 4
[ $? -eq 0 ] || failures=1

if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi
test_parser_data=${srcdir}/test_xrl_parser.data
spot_leaks ./test_xrl_parser ${test_parser_data}
[ $? -eq 0 ] || failures=1

tidy_up
exit ${failures}