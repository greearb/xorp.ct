#!/usr/bin/env bash

#
# $XORP: xorp/bgp/harness/test_route_flap_damping1.sh,v 1.3 2006/08/16 22:10:14 atanu Exp $
#

#
# Test BGP Route Flap Damping (RFC 2439)
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
# Peer 1 E-BGP
# Peer 2 E-BGP
# Peer 3 I-BGP

set -e
. ./setup_paths.sh

onexit()
{
    last=$?
    if [ $last = "0" ]
    then
	echo "$0: Tests Succeeded (BGP route flapping: $TESTS)"
    else
	echo "$0: Tests Failed (BGP route flapping: $TESTS)"
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
PEER1_AS=65001
PEER2_AS=65002
PEER3_AS=$AS
PEER1_AS_IPV6=65001
PEER2_AS_IPV6=65002
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

configure_bgp()
{
    LOCALHOST=$HOST
    AS=65008
    local_config $AS $ID $USE4BYTEAS

    # Don't try and talk to the rib.
    register_rib ""

    # EBGP - IPV4
    PEER=$HOST
    PORT=$PORT1;PEER_PORT=$PEER_PORT1;PEER_AS=$PEER1_AS
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer lo $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv4.Unicast true
    enable_peer $IPTUPLE

    # EBGP - IPV4
    PEER=$HOST
    PORT=$PORT2;PEER_PORT=$PEER_PORT2;PEER_AS=$PEER2_AS
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer lo $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv4.Unicast true
    enable_peer $IPTUPLE

    # IEBGP - IPV4
    PEER=$HOST
    PORT=$PORT3;PEER_PORT=$PEER_PORT3;PEER_AS=$PEER3_AS
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer lo $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv4.Unicast true
    enable_peer $IPTUPLE

    # IBGP - IPV6
    PEER=$HOST
    PORT=$PORT1_IPV6;PEER_PORT=$PEER_PORT1_IPV6;PEER_AS=$PEER1_AS_IPV6
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer lo $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv6.Unicast true
    enable_peer $IPTUPLE

    # IBGP - IPV6
    PEER=$HOST
    PORT=$PORT2_IPV6;PEER_PORT=$PEER_PORT2_IPV6;PEER_AS=$PEER2_AS_IPV6
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer lo $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv6.Unicast true
    enable_peer $IPTUPLE

    # IBGP - IPV6
    PEER=$HOST
    PORT=$PORT3_IPV6;PEER_PORT=$PEER_PORT3_IPV6;PEER_AS=$PEER3_AS_IPV6
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer lo $IPTUPLE  $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv6.Unicast true
    enable_peer $IPTUPLE
}

configure_bgp_damping_default()
{
    half_life=15
    max_suppress=60
    reuse=750
    suppress=3000

    set_damping $half_life $max_suppress $reuse $suppress false
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
    echo "TEST2 - Bugzilla BUG #471"
    echo "	1) Introduce a route on an E-BGP peering."
    echo "	2) Enable route flap damping on the BGP process."
    echo "	3) Withdraw orignal route. (used to cause a BGP assert)"

    config_peers_ipv4

    PACKET="packet update
	origin 2
	aspath $PEER1_AS
	nexthop $NEXT_HOP
	nlri 10.10.10.0/24"

    PACKET_IBGP_PEER="packet update
	origin 2
	aspath $PEER1_AS
	nexthop $NEXT_HOP
	localpref 100
	nlri 10.10.10.0/24"

    PACKET_EBGP_PEER="packet update
	origin 2
	aspath $AS,$PEER1_AS
	nexthop $NEXT_HOP
	med 1
	nlri 10.10.10.0/24"
    
    coord peer1 expect $PACKET
    coord peer2 expect $PACKET_EBGP_PEER	
    coord peer3 expect $PACKET_IBGP_PEER

    coord peer1 send $PACKET
    sleep 2
    
    coord peer1 assert queue 1
    coord peer2 assert queue 0
    coord peer3 assert queue 0

    configure_bgp_damping_default

    WITHDRAW="packet update withdraw 10.10.10.0/24"

    coord peer1 expect $WITHDRAW
    coord peer2 expect $WITHDRAW
    coord peer3 expect $WITHDRAW

    coord peer1 send $WITHDRAW
    sleep 2

    coord peer1 assert queue 2
    coord peer2 assert queue 0
    coord peer3 assert queue 0

# At the end of the test we expect all the peerings to still be established.
    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established
}

test3()
{
    echo "TEST3 - Variation on test2"
    echo "	1) Introduce a route on an E-BGP peering."
    echo "	2) Enable route flap damping on the BGP process."
    echo "	3) Introduce same route (used to cause a BGP assert)"

    config_peers_ipv4

    PACKET="packet update
	origin 2
	aspath $PEER1_AS
	nexthop $NEXT_HOP
	nlri 10.10.10.0/24"

    PACKET_IBGP_PEER="packet update
	origin 2
	aspath $PEER1_AS
	nexthop $NEXT_HOP
	localpref 100
	nlri 10.10.10.0/24"

    PACKET_EBGP_PEER="packet update
	origin 2
	aspath $AS,$PEER1_AS
	nexthop $NEXT_HOP
	med 1
	nlri 10.10.10.0/24"
    
    coord peer1 expect $PACKET
    coord peer2 expect $PACKET_EBGP_PEER	
    coord peer3 expect $PACKET_IBGP_PEER

    coord peer1 send $PACKET
    sleep 2
    
    coord peer1 assert queue 1
    coord peer2 assert queue 0
    coord peer3 assert queue 0

    configure_bgp_damping_default

    coord peer1 expect $PACKET
    coord peer2 expect $PACKET_EBGP_PEER	
    coord peer3 expect $PACKET_IBGP_PEER

    coord peer1 send $PACKET
    sleep 2

    coord peer1 assert queue 2
    coord peer2 assert queue 0
    coord peer3 assert queue 0

# At the end of the test we expect all the peerings to still be established.
    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established
}

TESTS_NOT_FIXED=''
TESTS='test1 test2 test3'

# Include command line
. ${srcdir}/args.sh

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
