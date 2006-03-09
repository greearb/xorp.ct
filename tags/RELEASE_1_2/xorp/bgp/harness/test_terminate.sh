#!/usr/bin/env bash

#
# $XORP: xorp/bgp/harness/test_terminate.sh,v 1.5 2003/07/17 00:28:32 pavlin Exp $
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

# srcdir is set by make for check target
if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi
. ${srcdir}/xrl_shell_funcs.sh ""
. ${srcdir}/../xrl_shell_funcs.sh ""

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
    echo "TEST1 - Verify that BGP shuts down cleanly"

    CALLXRL=$CALLXRL ${srcdir}/../xrl_shell_funcs.sh shutdown

    sleep 5
}

test2()
{
    echo "TEST2 - Verify that BGP shuts down after enabling three peerings"

    LOCALHOST=localhost
    HOLDTIME=60
    NEXT_HOP="127.0.0.1"
    PEER=localhost

    PORT=10001;PEER_PORT=20001;PEER_AS=6401
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    enable_peer $IPTUPLE

    PORT=10002;PEER_PORT=20002;PEER_AS=6402
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    enable_peer $IPTUPLE

    PORT=10003;PEER_PORT=20003;PEER_AS=6403
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    enable_peer $IPTUPLE

    CALLXRL=$CALLXRL ${srcdir}/../xrl_shell_funcs.sh shutdown

    sleep 5
}
    
TESTS_NOT_FIXED=''
TESTS='test1 test2'

# Include command line
. ${srcdir}/args.sh

if [ $START_PROGRAMS = "yes" ]
then
CXRL="$CALLXRL -r 10"
    ../../utils/runit $QUIET $VERBOSE -c "$0 -s -c $*" <<EOF
    ../../libxipc/xorp_finder
    ../../fea/xorp_fea_dummy  = $CXRL finder://fea/common/0.1/get_target_name
    ../../rib/xorp_rib        = $CXRL finder://rib/common/0.1/get_target_name
    ../xorp_bgp               = $CXRL finder://bgp/common/0.1/get_target_name
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
