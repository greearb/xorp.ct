#!/usr/bin/env bash

#
# $XORP: xorp/bgp/harness/test_routing1.sh,v 1.14 2003/10/30 04:55:40 atanu Exp $
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
# Peers 1 and 2 are E-BGP Peer 3 is I-BGP
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

if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi
. ${srcdir}/xrl_shell_funcs.sh ""
. ${srcdir}/../xrl_shell_funcs.sh ""
. ${srcdir}/../../rib/xrl_shell_funcs.sh ""

HOST=localhost
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
PEER1_AS=64001
PEER2_AS=64002
PEER3_AS=$AS
PEER1_AS_IPV6=64004
PEER2_AS_IPV6=64005
PEER3_AS_IPV6=$AS

HOLDTIME=20

# Next Hops
#NH1=10.10.10.10
#NH2=20.20.20.20
IF1=172.16.1.1
IF2=172.16.2.1
IF3=172.16.3.1
IF1_IPV6=40:40:40:40:40:40:40:41
IF2_IPV6=50:50:50:50:50:50:50:51
IF3_IPV6=60:60:60:60:60:60:60:61
NH1=172.16.1.2
NH2=172.16.2.2
NH3=172.16.3.2
NH1_IPV6=40:40:40:40:40:40:40:42
NH2_IPV6=50:50:50:50:50:50:50:52
NH3_IPV6=60:60:60:60:60:60:60:62

NEXT_HOP=192.150.187.78

configure_bgp()
{
    LOCALHOST=$HOST
    AS=65008
    ID=192.150.187.78
    local_config $AS $ID

    # EBGP - IPV4
    PEER=$HOST
    PORT=$PORT1;PEER_PORT=$PEER_PORT1;PEER_AS=$PEER1_AS
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    enable_peer $IPTUPLE

    # EBGP - IPV4
    PEER=$HOST
    PORT=$PORT2;PEER_PORT=$PEER_PORT2;PEER_AS=$PEER2_AS
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    enable_peer $IPTUPLE

    # IBGP - IPV4
    PEER=$HOST
    PORT=$PORT3;PEER_PORT=$PEER_PORT3;PEER_AS=$PEER3_AS
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    enable_peer $IPTUPLE

    # EBGP - IPV6
    PEER=$HOST
    PORT=$PORT1_IPV6;PEER_PORT=$PEER_PORT1_IPV6;PEER_AS=$PEER1_AS_IPV6
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv6.Unicast
    enable_peer $IPTUPLE

    # EBGP - IPV6
    PEER=$HOST
    PORT=$PORT2_IPV6;PEER_PORT=$PEER_PORT2_IPV6;PEER_AS=$PEER2_AS_IPV6
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv6.Unicast
    enable_peer $IPTUPLE

    # IBGP - IPV6
    PEER=$HOST
    PORT=$PORT3_IPV6;PEER_PORT=$PEER_PORT3_IPV6;PEER_AS=$PEER3_AS_IPV6
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE  $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv6.Unicast
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

test1()
{
    echo "TEST1 - Establish three peerings"

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

test1_ipv6()
{
    echo "TEST1 IPV6 - Establish three peerings"

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

test2()
{
# 1) Add a route (A) via peer1
# 2) Verify that route (A) appears at peer3	
# 3) Add a better route (B) via peer2
# 4) Verify that route (B) appears at peer3
# 5) Withdraw route (b) via peer2
# 6) Verify that route (A) appears at peer3
    echo "TEST2 - Adding and deleting routes"

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

    # Add a route from peer1.
    coord peer1 send packet update \
	origin 2 \
	aspath "$PEER1_AS,2,(3,4,5),6,(7,8),9" \
	nexthop $NH1 \
	nlri 10.10.10.0/24 \
	nlri 20.20.20.20/24

    sleep 2
    coord peer1 trie sent lookup 10.10.10.0/24 \
	aspath "$PEER1_AS,2,(3,4,5),6,(7,8),9"
    coord peer2 trie recv lookup 10.10.10.0/24 \
	aspath "65008,$PEER1_AS,2,(3,4,5),6,(7,8),9"
    coord peer3 trie recv lookup 10.10.10.0/24 \
	aspath "$PEER1_AS,2,(3,4,5),6,(7,8),9" 

    # Add a better route from peer2.
    coord peer2 send packet update \
	origin 2 \
	aspath "$PEER2_AS" \
	nexthop $NH2 \
	nlri 10.10.10.10/24 \
	nlri 20.20.20.20/24

    sleep 2
    coord peer1 trie recv lookup 10.10.10.0/24 aspath "65008,$PEER2_AS"
    coord peer2 trie sent lookup 10.10.10.0/24 aspath "$PEER2_AS"
    coord peer3 trie recv lookup 10.10.10.0/24 aspath "$PEER2_AS"

    # Withdraw the better route.
    coord peer2 send packet update \
	origin 2 \
	aspath "$PEER2_AS" \
	nexthop $NH2 \
	withdraw 10.10.10.10/24 \
	withdraw 20.20.20.20/24

    sleep 2
    coord peer1 trie sent lookup 10.10.10.0/24 \
	aspath "$PEER1_AS,2,(3,4,5),6,(7,8),9"
    coord peer2 trie recv lookup 10.10.10.0/24 \
	aspath "65008,$PEER1_AS,2,(3,4,5),6,(7,8),9"
    coord peer3 trie recv lookup 10.10.10.0/24 \
	aspath "$PEER1_AS,2,(3,4,5),6,(7,8),9"


# At the end of the test we expect all the peerings to still be established.
    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established
}

test2_ipv6()
{
# 1) Add a route (A) via peer1
# 2) Verify that route (A) appears at peer3	
# 3) Add a better route (B) via peer2
# 4) Verify that route (B) appears at peer3
# 5) Withdraw route (b) via peer2
# 6) Verify that route (A) appears at peer3
    echo "TEST2 IPV6 - Adding and deleting routes"

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

    NLRI1=10:10:10:10:10:00:00:00/80
    NLRI2=20:20:20:20:20:00:00:00/80

    # Add a route from peer1.
    coord peer1 send packet update \
	origin 2 \
	aspath "$PEER1_AS_IPV6,2,(3,4,5),6,(7,8),9" \
	nexthop6 $NH1_IPV6 \
	nlri6 $NLRI1 \
	nlri6 $NLRI2

    sleep 2
    coord peer1 trie sent lookup $NLRI1 \
	aspath "$PEER1_AS_IPV6,2,(3,4,5),6,(7,8),9"
    coord peer2 trie recv lookup $NLRI1 \
	aspath "65008,$PEER1_AS_IPV6,2,(3,4,5),6,(7,8),9"
    coord peer3 trie recv lookup $NLRI1 \
	aspath "$PEER1_AS_IPV6,2,(3,4,5),6,(7,8),9" 

    coord peer1 trie sent lookup $NLRI2 \
	aspath "$PEER1_AS_IPV6,2,(3,4,5),6,(7,8),9"
    coord peer2 trie recv lookup $NLRI2 \
	aspath "65008,$PEER1_AS_IPV6,2,(3,4,5),6,(7,8),9"
    coord peer3 trie recv lookup $NLRI2 \
	aspath "$PEER1_AS_IPV6,2,(3,4,5),6,(7,8),9" 

    # Add a better route from peer2.
    coord peer2 send packet update \
	origin 2 \
	aspath "$PEER2_AS_IPV6" \
	nexthop6 $NH2_IPV6 \
	nlri6 $NLRI1 \
	nlri6 $NLRI2

    sleep 2
    coord peer1 trie recv lookup $NLRI1 aspath "65008,$PEER2_AS_IPV6"
    coord peer2 trie sent lookup $NLRI1 aspath "$PEER2_AS_IPV6"
    coord peer3 trie recv lookup $NLRI1 aspath "$PEER2_AS_IPV6"

    coord peer1 trie recv lookup $NLRI2 aspath "65008,$PEER2_AS_IPV6"
    coord peer2 trie sent lookup $NLRI2 aspath "$PEER2_AS_IPV6"
    coord peer3 trie recv lookup $NLRI2 aspath "$PEER2_AS_IPV6"

    # Withdraw the better route.
    coord peer2 send packet update \
	origin 2 \
	aspath "$PEER2_AS_IPV6" \
	nexthop6 $NH2_IPV6 \
	withdraw6 $NLRI1 \
	withdraw6 $NLRI2

    sleep 2
    coord peer1 trie sent lookup $NLRI1 \
	aspath "$PEER1_AS_IPV6,2,(3,4,5),6,(7,8),9"
    coord peer2 trie recv lookup $NLRI1 \
	aspath "65008,$PEER1_AS_IPV6,2,(3,4,5),6,(7,8),9"
    coord peer3 trie recv lookup $NLRI1 \
	aspath "$PEER1_AS_IPV6,2,(3,4,5),6,(7,8),9"

    coord peer1 trie sent lookup $NLRI2 \
	aspath "$PEER1_AS_IPV6,2,(3,4,5),6,(7,8),9"
    coord peer2 trie recv lookup $NLRI2 \
	aspath "65008,$PEER1_AS_IPV6,2,(3,4,5),6,(7,8),9"
    coord peer3 trie recv lookup $NLRI2 \
	aspath "$PEER1_AS_IPV6,2,(3,4,5),6,(7,8),9"


# At the end of the test we expect all the peerings to still be established.
    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established
}

test3()
{
# Route A is better than route B which is better than route C.
# 1) Add a route (B) via peer1
# 2) Add a route (C) via peer2 
# 3) Add a route (A) via peer2 

    echo "TEST3 - Add routes on the same peer."

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

    PACKET_A="packet update
	origin 2
	aspath $PEER2_AS,1
	nexthop $NH1
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    PACKET_B="packet update
	origin 2
	aspath $PEER1_AS,1,2
	nexthop $NH2
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"
    
    PACKET_C="packet update
	origin 2
	aspath $PEER2_AS,1,2,3
	nexthop $NH3
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    coord peer1 send $PACKET_B

    sleep 2
    coord peer1 trie sent lookup 10.10.10.0/24 \
	aspath "$PEER1_AS,1,2"
    coord peer2 trie recv lookup 10.10.10.0/24 \
	aspath "65008,$PEER1_AS,1,2"
    coord peer3 trie recv lookup 10.10.10.0/24 \
	aspath "$PEER1_AS,1,2" 


    coord peer2 send $PACKET_C

    sleep 2
#    coord peer1 trie sent lookup 10.10.10.0/24 \
#	aspath "$PEER1_AS,1,2"
    coord peer2 trie sent lookup 10.10.10.0/24 \
	aspath "$PEER2_AS,1,2,3"
    coord peer3 trie recv lookup 10.10.10.0/24 \
	aspath "$PEER1_AS,1,2" 

    coord peer2 send $PACKET_A

    sleep 2
    coord peer1 trie recv lookup 10.10.10.0/24 \
	aspath "65008,$PEER2_AS,1"
    coord peer2 trie sent lookup 10.10.10.0/24 \
	aspath "$PEER2_AS,1"
    coord peer3 trie recv lookup 10.10.10.0/24 \
	aspath "$PEER2_AS,1" 

# At the end of the test we expect all the peerings to still be established.
    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established
}

test4()
{
# Route A is better than route B which is better than route C.
# 1) Add a route (B) via peer1
# 2) Add a route (A) via peer2 

    echo "TEST4 - Add a route on one peer add a better route on another peer."

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

    LONG_AS_PATH="$PEER1_AS,2,3,4,5"

    PACKET_A="packet update
	origin 2
	aspath $PEER2_AS
	nexthop $NH1
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    PACKET_B="packet update
	origin 2
	aspath $LONG_AS_PATH
	nexthop $NH2
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"
    
    coord peer1 send $PACKET_B

    sleep 2
    coord peer1 trie sent lookup 10.10.10.0/24 \
	aspath $LONG_AS_PATH
    coord peer2 trie recv lookup 10.10.10.0/24 \
	aspath "65008,$LONG_AS_PATH"
    coord peer3 trie recv lookup 10.10.10.0/24 \
	aspath $LONG_AS_PATH 

    coord peer2 send $PACKET_A

    sleep 2
    coord peer1 trie recv lookup 10.10.10.0/24 \
	aspath "65008,$PEER2_AS"
    coord peer2 trie sent lookup 10.10.10.0/24 \
	aspath "$PEER2_AS"
    coord peer3 trie recv lookup 10.10.10.0/24 \
	aspath "$PEER2_AS" 

# At the end of the test we expect all the peerings to still be established.
    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established
}

test5()
{
# Route A is better than route B which is better than route C.
# 1) Add a route (B) via peer1
# 2) Add a route (A) via peer1

    echo "TEST5 - Add a route on one peer then add a better route"

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

    LONG_AS_PATH="$PEER1_AS,1,2,3,4,5"

    PACKET_A="packet update
	origin 2
	aspath $PEER1_AS
	nexthop $NH1
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    PACKET_B="packet update
	origin 2
	aspath $LONG_AS_PATH
	nexthop $NH2
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"
    
    coord peer1 send $PACKET_B

    sleep 2
    coord peer1 trie sent lookup 10.10.10.0/24 \
	aspath $LONG_AS_PATH
    coord peer2 trie recv lookup 10.10.10.0/24 \
	aspath "$AS,$LONG_AS_PATH"
    coord peer3 trie recv lookup 10.10.10.0/24 \
	aspath $LONG_AS_PATH 

    coord peer1 send $PACKET_A

    sleep 2
    coord peer1 trie sent lookup 10.10.10.0/24 \
	aspath "$PEER1_AS"
    coord peer2 trie recv lookup 10.10.10.0/24 \
	aspath "$AS,$PEER1_AS"
    coord peer3 trie recv lookup 10.10.10.0/24 \
	aspath "$PEER1_AS" 

# At the end of the test we expect all the peerings to still be established.
    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established
}

test6()
{
    echo "TEST6 On an EBGP peering send an update with a local preference"

# Sending an update with a local preference is wrong, but it shouldn't cause
# any problems. We test that when the update is propogated that the bad local
# preference is removed and replaced with a local preference of 100. Which
# is the recommended default. Note we are also testing that peer2 does not
# receive the med from peer1.

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

    local LOCAL_NH=$NH1

    packet()
    {
	local PACKET="packet update
	    origin 2
	    aspath $1
	    nexthop $2
	    nlri 10.10.10.0/24
	    med $3"

	echo $PACKET
    }

    local MED=50

    coord peer1 expect $(packet $PEER1_AS $LOCAL_NH $MED)
    coord peer2 expect $(packet $AS,$PEER1_AS $NEXT_HOP 0)
    coord peer3 expect $(packet $PEER1_AS $LOCAL_NH $MED) localpref 100

    coord peer1 send $(packet $PEER1_AS $LOCAL_NH $MED) localpref 17

    sleep 2
    
    coord peer1 assert queue 1
    coord peer2 assert queue 0
    coord peer3 assert queue 0

    coord peer1 trie sent lookup 10.10.10.0/24 aspath $PEER1_AS
    coord peer2 trie recv lookup 10.10.10.0/24 aspath $AS,$PEER1_AS
    coord peer3 trie recv lookup 10.10.10.0/24 aspath $PEER1_AS

# At the end of the test we expect all the peerings to still be established.
    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established
}

TESTS_NOT_FIXED=''
TESTS='test1 test1_ipv6 test2 test2_ipv6 test3 test4 test5 test6'

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
