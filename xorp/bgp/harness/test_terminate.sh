#!/usr/bin/env bash

#
# $XORP: xorp/bgp/harness/test_terminate.sh,v 1.8 2006/08/16 22:10:14 atanu Exp $
#

#
# Test BGP termination
#
# This script started with no arguments will start all required process and
# terminate them at the end of the tests.
#
# Preconditons
# 1) Run a finder process
# 2) Run "../fea/xorp_fea_dummy"
# 3) Run "../rib/xorp_rib"
# 4) Run "../xorp_bgp"

set -e
. ./setup_paths.sh

# srcdir is set by make for check target
if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi
. ${srcdir}/xrl_shell_funcs.sh ""
. $BGP_FUNCS ""

onexit()
{
    last=$?
    if [ $last = "0" ]
    then
	echo "$0: Tests Succeeded (BGP: $TESTS)"
    else
	echo "$0: Tests Failed (BGP: $TESTS)"
    fi

    trap '' 0 2
}

trap onexit 0 2

HOST=127.0.0.1
AS=65008
USE4BYTEAS=false

configure_bgp()
{
    LOCALHOST=$HOST
    ID=192.150.187.78
    local_config $AS $ID $USE4BYTEAS

    register_rib ${RIB:-""}
}

test1()
{
    echo "TEST1 - Verify that BGP shuts down cleanly"

    CALLXRL=$CALLXRL $BGP_FUNCS shutdown

    sleep 5
}

test2()
{
    echo "TEST2 - Verify that BGP shuts down after enabling three peerings"

    LOCALHOST=$HOST
    HOLDTIME=60
    NEXT_HOP="127.0.0.1"
    PEER=$HOST

    PORT=10001;PEER_PORT=20001;PEER_AS=6401
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer lo $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    enable_peer $IPTUPLE

    PORT=10002;PEER_PORT=20002;PEER_AS=6402
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer lo $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    enable_peer $IPTUPLE

    PORT=10003;PEER_PORT=20003;PEER_AS=6403
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer lo $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    enable_peer $IPTUPLE

    CALLXRL=$CALLXRL $BGP_FUNCS shutdown

    sleep 5
}
    
TESTS_NOT_FIXED=''
TESTS='test1 test2'

# Include command line
. ${srcdir}/args.sh

if [ $START_PROGRAMS = "yes" ]
then
    CXRL="$CALLXRL -r 10"
    runit $QUIET $VERBOSE -c "$0 -s -c $*" <<EOF
    $XORP_FINDER
    $XORP_FEA_DUMMY  = $CXRL finder://fea/common/0.1/get_target_name
    $XORP_RIB        = $CXRL finder://rib/common/0.1/get_target_name
    $XORP_BGP        = $CXRL finder://bgp/common/0.1/get_target_name
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
