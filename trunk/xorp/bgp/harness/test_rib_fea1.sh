#!/usr/bin/env bash

#
# $XORP: xorp/bgp/harness/test_rib_fea1.sh,v 1.1.1.1 2002/12/11 23:55:51 hodson Exp $
#

#
# Test BGP, RIB and FEA interaction.
#
# This script started with no arguments will start all required process and
# terminate them at the end of the tests.
#
# Preconditons
# 1) Run a finder process
# 2) Run a FEA process.
# 3) Run a RIB process.
# 4) Run xorp "../bgp"
# 5) Run "./test_peer -s peer1"
# 6) Run "./test_peer -s peer2"
# 7) Run "./test_peer -s peer3"
# 8) Run "./coord"
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

. ../xrl_shell_funcs.sh ""
. ./xrl_shell_funcs.sh ""
. ../../rib/xrl_shell_funcs.sh ""

HOST=localhost
PORT1=10001
PORT2=10002
PORT3=10003
PEER_PORT1=20001
PEER_PORT2=20002
PEER_PORT3=20003
AS=65008
PEER1_AS=64001
PEER2_AS=64002
PEER3_AS=$AS

HOLDTIME=0

# Next Hops
#NH1=10.10.10.10
#NH2=20.20.20.20
NH1=172.16.1.1
NH2=172.16.2.1
VIF0="vif0"
VIF1="vif1"

configure_bgp()
{
    LOCALHOST=$HOST
    ID=192.150.187.78
    local_config $AS $ID

    PEER=$HOST
    NEXT_HOP=192.150.187.78
    PORT=$PORT1;PEER_PORT=$PEER_PORT1;PEER_AS=$PEER1_AS
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    enable_peer $IPTUPLE

    PEER=$HOST
    NEXT_HOP=192.150.187.78
    PORT=$PORT2;PEER_PORT=$PEER_PORT2;PEER_AS=$PEER2_AS
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    enable_peer $IPTUPLE

    PEER=$HOST
    NEXT_HOP=192.150.187.78
    PORT=$PORT3;PEER_PORT=$PEER_PORT3;PEER_AS=$PEER3_AS
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE  $PEER_AS $NEXT_HOP $HOLDTIME
    enable_peer $IPTUPLE
}

configure_fea()
{
    echo "Configuring fea"

    export CALLXRL
    FEA_FUNCS=../../fea/xrl_shell_funcs.sh
    local tid=$($FEA_FUNCS start_fea_transaction)

    $FEA_FUNCS create_interface $tid $VIF0
    $FEA_FUNCS create_interface $tid $VIF1

    $FEA_FUNCS create_vif $tid $VIF0 $VIF0
    $FEA_FUNCS create_vif $tid $VIF1 $VIF1
    
    $FEA_FUNCS enable_vif $tid $VIF0 $VIF0
    $FEA_FUNCS enable_vif $tid $VIF1 $VIF1

    $FEA_FUNCS create_address4 $tid $VIF0 $VIF0 $NH1
    $FEA_FUNCS create_address4 $tid $VIF1 $VIF1 $NH2

    $FEA_FUNCS set_prefix4 $tid $VIF0 $VIF0 $NH1 24
    $FEA_FUNCS set_prefix4 $tid $VIF1 $VIF1 $NH2 24

    $FEA_FUNCS commit_fea_transaction $tid
}

unconfigure_fea()
{
    echo "Unconfigure fea"
}

config_peers()
{
    coord reset

    coord target $HOST $PORT1
    coord initialise attach peer1

    coord peer1 establish AS $PEER1_AS \
	holdtime $HOLDTIME \
	id 10.10.10.1 \
	keepalive false

    coord peer1 assert established

    coord target $HOST $PORT2
    coord initialise attach peer2

    coord peer2 establish AS $PEER2_AS \
	holdtime $HOLDTIME \
	id 10.10.10.2 \
	keepalive false

    coord peer2 assert established

    coord target $HOST $PORT3
    coord initialise attach peer3

    coord peer3 establish AS $PEER3_AS \
	holdtime $HOLDTIME \
	id 10.10.10.3 \
	keepalive false

    coord peer3 assert established
}

test1()
{
    echo "TEST1 - Exercise the next hop resolver"

    config_peers

    PACKET1="packet update
	origin 2
	aspath $PEER1_AS
	nexthop 128.16.0.1
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    PACKET2="packet update
	origin 2
	aspath $PEER1_AS
	nexthop 128.16.1.1
	nlri 30.30.30.30/24
	nlri 40.40.40.40/24"

    # XXX This next hop should not resolve.

    coord peer1 send $PACKET1
    coord peer1 send $PACKET2
    sleep 2

    coord peer2 trie recv lookup 10.10.10.0/24 not

    # Lets get it to resolve.
    add_route4 connected true false 128.16.0.0/16 172.16.1.2 1

    # Add a different route.
    add_route4 connected true false 128.16.0.0/24 172.16.2.2 1
    
    # Delete the better route.
    delete_route4 connected true false 128.16.0.0/24

    # Try and verify that the correct route has popped out at peer2.
    sleep 2
    coord peer2 trie recv lookup 10.10.10.0/24 aspath "$AS,$PEER1_AS"
    
    # Make sure that the connections still exist.

    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established

    # Tidy up. Remove any routes that we may have added to the RIB.
    delete_route4 connected true false 128.16.0.0/16

    # At this point the RIB can no longer resolve the nexthop associated
    # with this net so the net should no longer be present.
    sleep 2
    coord peer2 trie recv lookup 10.10.10.0/24 not
}

test2()
{
    echo "TEST2 - Run test1 twice with the same process"

    test1
    test1
}

test3()
{
    echo "TEST3 - Try and force a deregistration from the RIB"

    config_peers

    PACKET1="packet update
	origin 2
	aspath $PEER1_AS
	nexthop 128.16.0.1
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    # XXX This next hop should not resolve.

    coord peer1 send $PACKET1
    sleep 2

    # Lets get it to resolve.
    add_route4 connected true false 128.16.0.0/16 172.16.1.2 1

    # Add a different route.
    add_route4 connected true false 128.16.0.0/24 172.16.2.2 1
    
    # Delete the better route.
    delete_route4 connected true false 128.16.0.0/24

    # Try and verify that the correct route has popped out at peer2.

    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established

    # Tidy up. Remove any routes that we may have added to the RIB.
    delete_route4 connected true false 128.16.0.0/16

    config_peers
}

test4()
{
    # 1) Send an update packet on peer1 with a nexthop that is not resolvable.
    # 2) Make the next hop resolvable by adding a route to the RIB.
    # 3) Verify that an update packet now pops out of peer2 and peer3.

    echo "TEST4 - Metrics changing for a nexthop"

    config_peers

    PACKET="packet update
	origin 2
	aspath $PEER1_AS
	nexthop 128.16.0.1
	nlri 10.10.10.0/24"

    # At this point the nexthop should not resolve.
    coord peer1 send $PACKET
    
    # Make sure that update packet has popped out even though the nexthops
    # are not resolvable.
    sleep 2
    coord peer2 trie recv lookup 10.10.10.0/24 not
    coord peer3 trie recv lookup 10.10.10.0/24 not

    # Lets get it to resolve.
    add_route4 connected true false 128.16.0.0/16 172.16.1.2 1

    # Try and verify that the correct route has popped out at peer2.
    sleep 2
    coord peer2 trie recv lookup 10.10.10.0/24 aspath "$AS,$PEER1_AS"
    coord peer3 trie recv lookup 10.10.10.0/24 aspath "$PEER1_AS"
    
    # Make sure that the connections still exist.
    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established
}

TESTS_NOT_FIXED=''
TESTS='test1 test2 test3 test4'

# Include command line
. ./args.sh

if [ $START_PROGRAMS = "yes" ]
then
CXRL="$CALLXRL -r 10"
    ../../utils/runit $QUIET $VERBOSE -c "$0 -s -c $*" <<EOF
    ../../libxipc/finder
    ../../fea/fea_dummy   = $CXRL finder://fea/common/0.1/get_target_name
    ../../rib/rib         = $CXRL finder://rib/common/0.1/get_target_name
    ../bgp                = $CXRL finder://bgp/common/0.1/get_target_name
    ./test_peer -s peer1  = $CXRL finder://peer1/common/0.1/get_target_name
    ./test_peer -s peer2  = $CXRL finder://peer2/common/0.1/get_target_name
    ./test_peer -s peer3  = $CXRL finder://peer3/common/0.1/get_target_name
    ./coord               = $CXRL finder://coord/common/0.1/get_target_name
EOF
    trap '' 0
    exit $?
fi

if [ $CONFIGURE = "yes" ]
then
    configure_fea
    set +e
    configure_bgp
    set -e
fi

# Temporary fix to let TCP sockets created by call_xrl pass through TIME_WAIT
TIME_WAIT=`time_wait_seconds`

for i in $TESTS
do
    $i
    echo "Waiting $TIME_WAIT seconds for TCP TIME_WAIT state timeout"
    sleep $TIME_WAIT
done

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
