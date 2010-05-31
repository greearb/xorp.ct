#!/usr/bin/env bash

#
# $XORP: xorp/bgp/harness/test_routing2.sh,v 1.21 2006/08/16 22:10:14 atanu Exp $
#

#
# Test basic BGP routing with three peers.
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
# Peers 1,2 and 3 are all I-BGP peerings.
#

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
PEER1_AS_IPV6=64004
PEER2_AS_IPV6=64005
PEER3_AS_IPV6=$AS

HOLDTIME=20

# Next Hops
IF1=172.16.1.1
IF2=172.16.2.1
IF3=172.16.3.1
GW1=172.16.1.2
GW2=172.16.2.2
GW3=172.16.3.2
IF1_IPV6=40:40:40:40:40:40:40:41
IF2_IPV6=50:50:50:50:50:50:50:51
IF3_IPV6=60:60:60:60:60:60:60:61
GW1_IPV6=40:40:40:40:40:40:40:42
GW2_IPV6=50:50:50:50:50:50:50:52
GW3_IPV6=60:60:60:60:60:60:60:62

# The next hops are not directly connected. Its necessary to add a route to
# reach them.
NH1=162.16.1.2
NH2=162.16.2.2
NH3=162.16.3.2
NH1_IPV6=70:40:40:40:40:40:40:42
NH2_IPV6=80:50:50:50:50:50:50:52
NH3_IPV6=90:60:60:60:60:60:60:62

NEXT_HOP=192.150.187.78

configure_bgp()
{
    LOCALHOST=$HOST
    AS=65008
    ID=192.150.187.78
    local_config $AS $ID $USE4BYTEAS

    # IBGP - IPV4
    PEER=$HOST
    PORT=$PORT1;PEER_PORT=$PEER_PORT1;PEER_AS=$PEER1_AS
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer lo $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv4.Unicast true
    enable_peer $IPTUPLE

    # IBGP - IPV4
    PEER=$HOST
    PORT=$PORT2;PEER_PORT=$PEER_PORT2;PEER_AS=$PEER2_AS
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer lo $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv4.Unicast true
    enable_peer $IPTUPLE

    # IBGP - IPV4
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

configure_rib()
{
    make_rib_errors_fatal

    VIF0="vif0"
    VIF1="vif1"
    VIF2="vif2"
    VIF0_IPV6="vif3"
    VIF1_IPV6="vif4"
    VIF2_IPV6="vif5"

    new_vif $VIF0
    new_vif $VIF1
    new_vif $VIF2
    new_vif $VIF0_IPV6
    new_vif $VIF1_IPV6
    new_vif $VIF2_IPV6

    add_vif_addr4 $VIF0 $IF1 $IF1/24
    add_vif_addr4 $VIF1 $IF2 $IF2/24
    add_vif_addr4 $VIF2 $IF3 $IF3/24

    add_vif_addr6 $VIF0_IPV6 $IF1_IPV6 $IF1_IPV6/24
    add_vif_addr6 $VIF1_IPV6 $IF2_IPV6 $IF2_IPV6/24
    add_vif_addr6 $VIF2_IPV6 $IF3_IPV6 $IF3_IPV6/24
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

config_peers_ipv6()
{
    coord reset

    coord target $HOST $PORT1_IPV6
    coord initialise attach peer1

    coord peer1 establish AS $PEER1_AS_IPV6 \
	holdtime $HOLDTIME \
	id 10.10.10.1 \
	keepalive false \
	ipv6 true

    coord peer1 assert established

    coord target $HOST $PORT2_IPV6
    coord initialise attach peer2

    coord peer2 establish AS $PEER2_AS_IPV6 \
	holdtime $HOLDTIME \
	id 10.10.10.2 \
	keepalive false \
	ipv6 true

    coord peer2 assert established

    coord target $HOST $PORT3_IPV6
    coord initialise attach peer3

    coord peer3 establish AS $PEER3_AS_IPV6 \
	holdtime $HOLDTIME \
	id 10.10.10.3 \
	keepalive false \
	ipv6 true

    coord peer3 assert established
}

test1()
{
    echo "TEST1 - Establish three peerings"

    config_peers
}

test1_ipv6()
{
    echo "TEST1 IPV6 - Establish three peerings"

    config_peers_ipv6
}

test2()
{
    echo "TEST2 - This test used to cause the RIB to core dump"
    # While writing a test for deterministic meds found a problem with the RIB.

    #  Route	ASPATH		MED 	IGP DISTANCE
    #  1	1,2,3		10	10
    #  2	1,2,3		5	30
    #  3	4,5,6		20	20

    # Route 3 should always be the winner regardless of the order of arrival
    # of the routes.

    config_peers

    # Configure IGP routes in the RIB as if IS-IS is running.
    # This allows us to test varying IGP distances.
    add_igp_table4 is-is isis isis true false

    add_route4 is-is true false $NH1/24 $GW1 10
    add_route4 is-is true false $NH2/24 $GW2 30
    add_route4 is-is true false $NH3/24 $GW3 20

    # Route 1
    coord peer1 send packet update \
	origin 2 \
	aspath "1,2,3" \
	med 10 \
	nexthop $NH1 \
	localpref 100 \
	nlri 10.10.10.10/24 \
	nlri 20.20.20.20/24

    # Route 2
    coord peer2 send packet update \
	origin 2 \
	aspath "1,2,3" \
	med 5 \
	nexthop $NH2 \
	localpref 100 \
	nlri 10.10.10.10/24 \
	nlri 20.20.20.20/24

    # Route 3
    coord peer3 send packet update \
	origin 2 \
	aspath "4,5,6" \
	med 20 \
	nexthop $NH2 \
	localpref 100 \
	nlri 10.10.10.10/24 \
	nlri 20.20.20.20/24
	
    # Remove IGP state from the RIB
    delete_route4 is-is true false $NH1/24
    delete_route4 is-is true false $NH2/24
    delete_route4 is-is true false $NH3/24

    delete_igp_table4 is-is isis isis true false
}

test3()
{
    echo "TEST3 - Test for deterministic MEDs"

    #  Route	ASPATH		MED 	IGP DISTANCE
    #  1	1,2,3		10	10
    #  2	1,2,3		5	30
    #  3	4,5,6		20	20

    # Route 3 should always be the winner regardless of the order of arrival
    # of the routes.

    config_peers

    # Configure IGP routes in the RIB as if IS-IS is running.
    # This allows us to test varying IGP distances.
    add_igp_table4 is-is isis isis true false

    add_route4 is-is true false $NH1/24 $GW1 10
    add_route4 is-is true false $NH2/24 $GW2 30
    add_route4 is-is true false $NH3/24 $GW3 20

    # Route 1
    coord peer1 send packet update \
	origin 2 \
	aspath "1,2,3" \
	med 10 \
	nexthop $NH1 \
	localpref 100 \
	nlri 10.10.10.10/24 \
	nlri 20.20.20.20/24

    # Route 2
    coord peer2 send packet update \
	origin 2 \
	aspath "1,2,3" \
	med 5 \
	nexthop $NH2 \
	localpref 100 \
	nlri 10.10.10.10/24 \
	nlri 20.20.20.20/24

    # Route 3
    coord peer3 send packet update \
	origin 2 \
	aspath "4,5,6" \
	med 20 \
	nexthop $NH3 \
	localpref 100 \
	nlri 10.10.10.10/24 \
	nlri 20.20.20.20/24
	
    # Wait for the route changes to propogate.
    sleep 2
    result=$(lookup_route_by_dest4 10.10.10.10 true false)
    echo $result

    OIFS=$IFS
    IFS='='
    set $result
    result=$2
    IFS=$OIFS

    if [ $result != $GW3 ]
    then
	echo "Gateway is $result should be $GW3"
	return 1
    fi

    # Remove IGP state from the RIB
    delete_route4 is-is true false $NH1/24
    delete_route4 is-is true false $NH2/24
    delete_route4 is-is true false $NH3/24

    delete_igp_table4 is-is isis isis true false
}

test4()
{
    echo "TEST4 - Test for deterministic MEDs same as test3 except for the order of route arrival R1,R3,R2"

    #  Route	ASPATH		MED 	IGP DISTANCE
    #  1	1,2,3		10	10
    #  2	1,2,3		5	30
    #  3	4,5,6		20	20

    # Route 3 should always be the winner regardless of the order of arrival
    # of the routes.

    config_peers

    # Configure IGP routes in the RIB as if IS-IS is running.
    # This allows us to test varying IGP distances.
    add_igp_table4 is-is isis isis true false

    add_route4 is-is true false $NH1/24 $GW1 10
    add_route4 is-is true false $NH2/24 $GW2 30
    add_route4 is-is true false $NH3/24 $GW3 20

    # Route 1
    coord peer1 send packet update \
	origin 2 \
	aspath "1,2,3" \
	med 10 \
	nexthop $NH1 \
	localpref 100 \
	nlri 10.10.10.10/24 \
	nlri 20.20.20.20/24

    # Route 3
    coord peer3 send packet update \
	origin 2 \
	aspath "4,5,6" \
	med 20 \
	nexthop $NH3 \
	localpref 100 \
	nlri 10.10.10.10/24 \
	nlri 20.20.20.20/24

    # Route 2
    coord peer2 send packet update \
	origin 2 \
	aspath "1,2,3" \
	med 5 \
	nexthop $NH2 \
	localpref 100 \
	nlri 10.10.10.10/24 \
	nlri 20.20.20.20/24

    # Wait for the route changes to propogate.
    sleep 2
    result=$(lookup_route_by_dest4 10.10.10.10 true false)
    echo $result

    OIFS=$IFS
    IFS='='
    set $result
    result=$2
    IFS=$OIFS

    if [ $result != $GW3 ]
    then
	echo "Gateway is $result should be $GW3"
	return 1
    fi

    # Remove IGP state from the RIB
    delete_route4 is-is true false $NH1/24
    delete_route4 is-is true false $NH2/24
    delete_route4 is-is true false $NH3/24

    delete_igp_table4 is-is isis isis true false
}

test5()
{
    echo "TEST5 - Test for deterministic MEDs same as test3 but test all arrival permutations"

    #  Route	ASPATH		MED 	IGP DISTANCE
    #  1	1,2,3		10	10
    #  2	1,2,3		5	30
    #  3	4,5,6		20	20

    # Route 3 should always be the winner regardless of the order of arrival
    # of the routes.

    config_peers

    # Configure IGP routes in the RIB as if IS-IS is running.
    # This allows us to test varying IGP distances.
    add_igp_table4 is-is isis isis true false

    add_route4 is-is true false $NH1/24 $GW1 10
    add_route4 is-is true false $NH2/24 $GW2 30
    add_route4 is-is true false $NH3/24 $GW3 20

    R1="peer1 send packet update origin 2 \
	aspath 1,2,3 \
	med 10 \
	nexthop $NH1 \
	localpref 100 \
	nlri 10.10.10.10/24 \
	nlri 20.20.20.20/24"

    R2="peer2 send packet update \
	origin 2 \
	aspath 1,2,3 \
	med 5 \
	nexthop $NH2 \
	localpref 100 \
	nlri 10.10.10.10/24 \
	nlri 20.20.20.20/24"

    R3="peer3 send packet update \
	origin 2 \
	aspath 4,5,6 \
	med 20 \
	nexthop $NH3 \
	localpref 100 \
	nlri 10.10.10.10/24 \
	nlri 20.20.20.20/24"

    delete_routes() {
	WITHDRAW="packet update\
			withdraw 10.10.10.10/24 \
			withdraw 20.20.20.20/24"
	coord peer1 send $WITHDRAW
	coord peer2 send $WITHDRAW
	coord peer3 send $WITHDRAW
    }

    # Try all the permutations of sending in these routes
    for i in    "$R1:$R2:$R3" \
		"$R1:$R3:$R2" \
		"$R2:$R1:$R3" \
		"$R2:$R3:$R1" \
		"$R3:$R1:$R2" \
		"$R3:$R2:$R1"
    do
	OIFS=$IFS
	IFS=':'
	set $i
	IFS=$OIFS
	
	coord $1
	coord $2
	coord $3

	# Wait for the route changes to propogate.
        sleep 2
	result=$(lookup_route_by_dest4 10.10.10.10 true false)
	echo $result

	OIFS=$IFS
	IFS='='
	set $result
	result=$2
	IFS=$OIFS

	if [ $result != $GW3 ]
	then
	    echo "Gateway is $result should be $GW3"
	    return 1
	fi

	delete_routes
    done

    # Remove IGP state from the RIB
    delete_route4 is-is true false $NH1/24
    delete_route4 is-is true false $NH2/24
    delete_route4 is-is true false $NH3/24

    delete_igp_table4 is-is isis isis true false
}

test6()
{
    echo "TEST6 - Verify that it is possible add and remove tables in the RIB"

    add_igp_table4 is-is isis isis true false
    delete_igp_table4 is-is isis isis true false
    add_igp_table4 is-is isis isis true false
    delete_igp_table4 is-is isis isis true false
}

test7()
{
    echo "TEST7 - Next hop resolvability on multiple peerings"
    # Used to trigger a bug in the RIB-OUT

    config_peers

    # Configure IGP routes in the RIB as if IS-IS is running.
    add_igp_table4 is-is isis isis true false

    # Route 1
    coord peer1 send packet update \
	origin 2 \
	aspath "1,2,3" \
	nexthop $NH1 \
	localpref 100 \
	med 1 \
	nlri 10.10.10.10/24

    # Route 2
    coord peer2 send packet update \
	origin 2 \
	aspath "1,2,3" \
	nexthop $NH1 \
	localpref 100 \
	med 2 \
	nlri 10.10.10.10/24

    # Route 3
    coord peer3 send packet update \
	origin 2 \
	aspath "1,2,3" \
	nexthop $NH1 \
	localpref 100 \
	med 3 \
	nlri 10.10.10.10/24

    add_route4 is-is true false $NH1/24 $GW1 10

    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established
}

test8()
{
    echo "TEST8 - Next hop resolvability on multiple peerings"
    # Used to trigger a bug in the RIB-OUT

    config_peers

    # Configure IGP routes in the RIB as if IS-IS is running.
    add_igp_table4 is-is isis isis true false

    # Route 1
    coord peer1 send packet update \
	origin 2 \
	aspath "1,2,3" \
	nexthop $NH1 \
	localpref 100 \
	nlri 10.10.10.10/24

    # Route 2
    coord peer2 send packet update \
	origin 2 \
	aspath "1,2,3" \
	nexthop $NH1 \
	localpref 100 \
	nlri 10.10.10.10/24

    # Route 3
    coord peer3 send packet update \
	origin 2 \
	aspath "1,2,3" \
	nexthop $NH1 \
	localpref 100 \
	nlri 10.10.10.10/24

    add_route4 is-is true false $NH1/24 $GW1 10
    delete_route4 is-is true false $NH1/24

    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established
}

test9()
{
    echo "TEST9 - Next hop resolvability on multiple peerings"
    # Used to trigger a bug in the RIB-OUT
    # Same as previous test use the meds to change the priority of the routes.

    config_peers

    # Configure IGP routes in the RIB as if IS-IS is running.
    add_igp_table4 is-is isis isis true false

    sleep 2

    # Route 1
    coord peer1 send packet update \
	origin 2 \
	aspath "1,2,3" \
	nexthop $NH1 \
	localpref 100 \
	med 3 \
	nlri 10.10.10.10/24

    # Route 2
    coord peer2 send packet update \
	origin 2 \
	aspath "1,2,3" \
	nexthop $NH1 \
	localpref 100 \
	med 2 \
	nlri 10.10.10.10/24

    # Route 3
    coord peer3 send packet update \
	origin 2 \
	aspath "1,2,3" \
	nexthop $NH1 \
	localpref 100 \
	med 1 \
	nlri 10.10.10.10/24

    add_route4 is-is true false $NH1/24 $GW1 10

    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established
}

test10()
{
    echo "TEST10 - Next hop resolvability on multiple peerings"
    # Used to trigger a bug in the RIB-OUT

    config_peers

    # Configure IGP routes in the RIB as if IS-IS is running.
    add_igp_table4 is-is isis isis true false
    add_route4 is-is true false $NH1/24 $GW1 10

    # Route 1
    coord peer1 send packet update \
	origin 2 \
	aspath "1,2,3" \
	nexthop $NH1 \
	localpref 100 \
	med 3 \
	nlri 10.10.10.10/24

    # Route 2
    coord peer2 send packet update \
	origin 2 \
	aspath "1,2,3" \
	nexthop $NH1 \
	localpref 100 \
	med 2 \
	nlri 10.10.10.10/24

    # Route 3
    coord peer3 send packet update \
	origin 2 \
	aspath "1,2,3" \
	nexthop $NH1 \
	localpref 100 \
	med 1 \
	nlri 10.10.10.10/24


    delete_route4 is-is true false $NH1/24

    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established
}

test11()
{
    echo "TEST11 - Bugzilla BUG #139"
    echo "	1) On two I-BGP peerings send an update with an empty aspath"
    echo "	2) Used to cause a the BGP decision process to fail."

    coord reset

    coord target $HOST $PORT1
    coord initialise attach peer1

    coord peer1 establish AS $PEER1_AS \
	holdtime 0 \
	id 10.10.10.1 \
	keepalive false

    coord peer1 assert established

    coord target $HOST $PORT2
    coord initialise attach peer2

    coord peer2 establish AS $PEER2_AS \
	holdtime 0 \
	id 10.10.10.2 \
	keepalive false

    coord peer2 assert established

    coord target $HOST $PORT3
    coord initialise attach peer3

    coord peer3 establish AS $PEER3_AS \
	holdtime 0 \
	id 10.10.10.3 \
	keepalive false

    coord peer3 assert established

    PACKET1="packet update
	origin 1
	aspath empty
	nexthop $IF1
	med 1
	nlri 10.10.10.0/24"
 
    PACKET2="packet update
	origin 1
	aspath empty
	nexthop $IF2
	med 1
	nlri 10.10.10.0/24"
   
    coord peer1 send $PACKET1
    coord peer2 send $PACKET1

    sleep 2

    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established
}

# http://www.xorp.org/bugzilla/show_bug.cgi?id=296
test12()
{
    echo "TEST12 - match a BGP route with a static route"

    config_peers

    PACKET="packet update
	origin 2
	aspath 1
	nexthop $GW1
	nlri 10.10.10.0/24"

    coord peer1 send $PACKET

    add_igp_table4 static bgp bgp true false
    add_route4 static true false 10.10.10.0/24 $GW2 1
    
    sleep 5

    coord peer1 assert established
}

TESTS_NOT_FIXED='test12'
TESTS='test1 test1_ipv6 test2 test3 test4 test5 test6 test7 test8 test9 test10
    test11'

# Include command line
. ${srcdir}/args.sh
. ./setup_paths.sh

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
    configure_rib
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
