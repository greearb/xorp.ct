#!/usr/bin/env bash

#
# Test basic BGP routing reflection.
#
# This script started with no arguments will start all required process and
# terminate them at the end of the tests.
#
# Preconditons
# 1) Run a finder process
# 2) Run "../fea/xorp_fea_dummy"
# 3) Run xorp "../rib/xorp_rib"
# 4) Run xorp "../xorp_bgp"
# 5) Run "./test_peer -s peer1"
# 6) Run "./test_peer -s peer2"
# 7) Run "./test_peer -s peer3"
# 8) Run "./coord"
#
# The BGP process is configured as a route reflector.
# Peers 1,2 and 3 are all configured as I-BGP peers.
# Peers 2 and 3 are RR clients

set -e
. ./setup_paths.sh

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

if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi
. ${srcdir}/xrl_shell_funcs.sh ""
. $BGP_FUNCS ""
. $RIB_FUNCS ""

HOST=127.0.0.1
PORT1=10001
PORT2=10002
PORT3=10003
PORT1_IPV6=10004
PORT2_IPV6=10005
PORT3_IPV6=10006
PEER_PORT1=20001
PEER_PORT2=20002
PEER_PORT3=20003
PEER_PORT1_IPV6=20004
PEER_PORT2_IPV6=20005
PEER_PORT3_IPV6=20006
AS=65008
USE4BYTEAS=false
PEER1_AS=$AS
PEER2_AS=$AS
PEER3_AS=$AS
PEER1_AS_IPV6=$AS
PEER2_AS_IPV6=$AS
PEER3_AS_IPV6=$AS

HOLDTIME=0

# Next Hops
NH1_IPv4=172.16.1.2
NH2_IPV4=172.16.2.2
NH3_IPv4=172.16.3.2
NH1_IPV6=40:40:40:40:40:40:40:42
NH2_IPV6=50:50:50:50:50:50:50:52
NH3_IPV6=60:60:60:60:60:60:60:62

NEXT_HOP=192.150.187.78
ID=192.150.187.78
CLUSTER_ID=0.0.0.11

configure_bgp()
{
    LOCALHOST=$HOST
    AS=65008
    local_config $AS $ID $USE4BYTEAS
    route_reflector $CLUSTER_ID false

    # Don't try and talk to the rib.
    register_rib ""

    # IBGP - IPV4
    PEER=$HOST
    PORT=$PORT1;PEER_PORT=$PEER_PORT1;PEER_AS=$PEER1_AS
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer lo $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv4.Unicast true
    enable_peer $IPTUPLE

    # IBGP - CLIENT - IPV4
    PEER=$HOST
    PORT=$PORT2;PEER_PORT=$PEER_PORT2;PEER_AS=$PEER2_AS
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer lo $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv4.Unicast true
    route_reflector_client $IPTUPLE true
    enable_peer $IPTUPLE

    # IBGP - CLIENT - IPV4
    PEER=$HOST
    PORT=$PORT3;PEER_PORT=$PEER_PORT3;PEER_AS=$PEER3_AS
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer lo $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv4.Unicast true
    route_reflector_client $IPTUPLE true
    enable_peer $IPTUPLE

    # IBGP - IPV6
    PEER=$HOST
    PORT=$PORT1_IPV6;PEER_PORT=$PEER_PORT1_IPV6;PEER_AS=$PEER1_AS_IPV6
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer lo $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv6.Unicast true
    enable_peer $IPTUPLE

    # IBGP - CLIENT - IPV6
    PEER=$HOST
    PORT=$PORT2_IPV6;PEER_PORT=$PEER_PORT2_IPV6;PEER_AS=$PEER2_AS_IPV6
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer lo $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv6.Unicast true
    route_reflector_client $IPTUPLE true
    enable_peer $IPTUPLE

    # IBGP - CLIENT - IPV6
    PEER=$HOST
    PORT=$PORT3_IPV6;PEER_PORT=$PEER_PORT3_IPV6;PEER_AS=$PEER3_AS_IPV6
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer lo $IPTUPLE  $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv6.Unicast true
    route_reflector_client $IPTUPLE true
    enable_peer $IPTUPLE
}

config_peers_ipv4()
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
    echo "TEST1 - Establish three peerings"

    config_peers_ipv4
}

test2()
{
    echo "TEST2 (UNH - BGP_CONF.4.4 PART A)"
    echo "	1) Send an update packet on peer1"
    echo "	2) Verify that the packet arrives at peer2 and peer3"

    config_peers_ipv4

    PACKET="packet update
	origin 2
	aspath 1
	nexthop $NEXT_HOP
	nlri 10.10.10.0/24
	localpref 10
	clusterlist 1.2.3.4"

    RR_PACKET="$PACKET originatorid 10.10.10.1 clusterlist $CLUSTER_ID"

    coord peer1 expect $PACKET	
    coord peer2 expect $RR_PACKET	
    coord peer3 expect $RR_PACKET	

    coord peer1 send $PACKET
    sleep 2
    
    coord peer1 assert queue 1
    coord peer2 assert queue 0
    coord peer3 assert queue 0

# At the end of the test we expect all the peerings to still be established.
    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established
}

test3()
{
    echo "TEST3 (UNH - BGP_CONF.4.4 PART B)"
    echo "	1) Send an update packet on peer2"
    echo "	2) Verify that the packet arrives at peer1 and peer3"

    config_peers_ipv4

    PACKET="packet update
	origin 2
	aspath 1
	nexthop $NEXT_HOP
	nlri 10.10.10.0/24
	localpref 10
	clusterlist 1.2.3.4"

    RR_PACKET="$PACKET originatorid 10.10.10.2 clusterlist $CLUSTER_ID"

    coord peer1 expect $RR_PACKET	
    coord peer2 expect $RR_PACKET	
    coord peer3 expect $RR_PACKET	

    coord peer2 send $PACKET
    sleep 2
    
    coord peer1 assert queue 0
    coord peer2 assert queue 1
    coord peer3 assert queue 0

# At the end of the test we expect all the peerings to still be established.
    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established
}

test4()
{
    echo "TEST4"
    echo "	1) Send an update packet with the routers originator id"
    echo "	2) This packet should be supressed by the router"

    config_peers_ipv4

    BASE_PACKET="packet update
	origin 2
	aspath 1
	nexthop $NEXT_HOP
	nlri 10.10.10.0/24
	localpref 10"

    PACKET="$BASE_PACKET originatorid $ID clusterlist 1.2.3.4"

    RR_PACKET="$BASE_PACKET originatorid $ID clusterlist $CLUSTER_ID"

    coord peer1 expect $RR_PACKET	
    coord peer2 expect $RR_PACKET	
    coord peer3 expect $RR_PACKET	

    coord peer1 send $PACKET
    sleep 2
    
    coord peer1 assert queue 1
    coord peer2 assert queue 1
    coord peer3 assert queue 1

# At the end of the test we expect all the peerings to still be established.
    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established
}

test5()
{
    echo "TEST5"
    echo "	1) Send an update packet with the same cluster id"
    echo "	2) This packet should be supressed by the router"

    config_peers_ipv4

    PACKET="packet update
	origin 2
	aspath 1
	nexthop $NEXT_HOP
	nlri 10.10.10.0/24
	localpref 10
	clusterlist $CLUSTER_ID"

    RR_PACKET="$PACKET originatorid $ID"

    coord peer1 expect $RR_PACKET	
    coord peer2 expect $RR_PACKET	
    coord peer3 expect $RR_PACKET	

    coord peer1 send $PACKET
    sleep 2
    
    coord peer1 assert queue 1
    coord peer2 assert queue 1
    coord peer3 assert queue 1

# At the end of the test we expect all the peerings to still be established.
    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established
}

TESTS_NOT_FIXED=''
TESTS='test1 test2 test3 test4 test5'

# Include command line
. ${srcdir}/args.sh

#START_PROGRAMS="no"
if [ $START_PROGRAMS = "yes" ]
then
    CXRL="$CALLXRL -r 10"
    runit $QUIET $VERBOSE -c "$0 -s -c $*" <<EOF
    $XORP_FINDER
    $XORP_FEA_DUMMY           = $CXRL finder://fea/common/0.1/get_target_name
    $XORP_RIB                 = $CXRL finder://rib/common/0.1/get_target_name
    $XORP_BGP                 = $CXRL finder://bgp/common/0.1/get_target_name
    ./test_peer -s peer1      = $CXRL finder://peer1/common/0.1/get_target_name
    ./test_peer -s peer2      = $CXRL finder://peer2/common/0.1/get_target_name
    ./test_peer -s peer3      = $CXRL finder://peer3/common/0.1/get_target_name
    ./coord                   = $CXRL finder://coord/common/0.1/get_target_name
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
# Temporary fix to let TCP sockets created by call_xrl pass through TIME_WAIT
    TIME_WAIT=`time_wait_seconds`
    echo "Waiting $TIME_WAIT seconds for TCP TIME_WAIT state timeout"
    sleep $TIME_WAIT
    $i
done

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
