#!/usr/bin/env bash

#
# $XORP: xorp/bgp/harness/test_rib_fea1.sh,v 1.9 2003/10/30 04:37:45 atanu Exp $
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
# 4) Run xorp "../xorp_bgp"
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

# srcdir is set by make for check target
if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi
. ${srcdir}/xrl_shell_funcs.sh ""
. ${srcdir}/../xrl_shell_funcs.sh ""
. ${srcdir}/../../rib/xrl_shell_funcs.sh ""

HOST=localhost
AS=65008

# EBGP - IPV4
PORT1=10001
PEER_PORT1=20001
PEER1_AS=64001

# EBGP - IPV4
PORT2=10002
PEER_PORT2=20002
PEER2_AS=64002

# IBGP - IPV4
PORT3=10003
PEER_PORT3=20003
PEER3_AS=$AS

# EBGP - IPV6
PORT1_IPV6=10004
PEER_PORT1_IPV6=20004
PEER1_AS_IPV6=64004

# EBGP - IPV6
PORT2_IPV6=10005
PEER_PORT2_IPV6=20005
PEER2_AS_IPV6=64005

# IBGP - IPV6
PORT3_IPV6=10006
PEER_PORT3_IPV6=20006
PEER3_AS_IPV6=$AS

HOLDTIME=0

# Next Hops
#NH1=10.10.10.10
#NH2=20.20.20.20
NH1=172.16.1.1
NH2=172.16.2.1
NH1_IPV6=40:40:40:40:40:40:40:40
NH2_IPV6=50:50:50:50:50:50:50:50
VIF0="vif0"
VIF1="vif1"
VIF0_IPV6="vif2"
VIF1_IPV6="vif3"

configure_bgp()
{
    LOCALHOST=$HOST
    ID=192.150.187.78
    local_config $AS $ID

    # EBGP - IPV4
    PEER=$HOST
    NEXT_HOP=192.150.187.78
    PORT=$PORT1;PEER_PORT=$PEER_PORT1;PEER_AS=$PEER1_AS
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    enable_peer $IPTUPLE

    # EBGP - IPV4
    PEER=$HOST
    NEXT_HOP=192.150.187.78
    PORT=$PORT2;PEER_PORT=$PEER_PORT2;PEER_AS=$PEER2_AS
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    enable_peer $IPTUPLE

    # IBGP - IPV4
    PEER=$HOST
    NEXT_HOP=192.150.187.78
    PORT=$PORT3;PEER_PORT=$PEER_PORT3;PEER_AS=$PEER3_AS
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE  $PEER_AS $NEXT_HOP $HOLDTIME
    enable_peer $IPTUPLE

    # EBGP - IPV6
    PEER=$HOST
    NEXT_HOP=192.150.187.78
    PORT=$PORT1_IPV6;PEER_PORT=$PEER_PORT1_IPV6;PEER_AS=$PEER1_AS_IPV6
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv6.Unicast
    enable_peer $IPTUPLE

    # EBGP - IPV6
    PEER=$HOST
    NEXT_HOP=192.150.187.78
    PORT=$PORT2_IPV6;PEER_PORT=$PEER_PORT2_IPV6;PEER_AS=$PEER2_AS_IPV6
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv6.Unicast
    enable_peer $IPTUPLE

    # IBGP - IPV6
    PEER=$HOST
    NEXT_HOP=192.150.187.78
    PORT=$PORT3_IPV6;PEER_PORT=$PEER_PORT3_IPV6;PEER_AS=$PEER3_AS_IPV6
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE  $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv6.Unicast
    enable_peer $IPTUPLE
}

configure_rib()
{
    echo "Configuring rib"
    export CALLXRL
    RIB_FUNCS=${srcdir}/../../rib/xrl_shell_funcs.sh
    $RIB_FUNCS make_rib_errors_fatal
}

configure_fea()
{
    echo "Configuring fea"

    export CALLXRL
    FEA_FUNCS=${srcdir}/../../fea/xrl_shell_funcs.sh
    local tid=$($FEA_FUNCS start_fea_transaction)

    $FEA_FUNCS create_interface $tid $VIF0
    $FEA_FUNCS create_interface $tid $VIF1

    $FEA_FUNCS create_interface $tid $VIF0_IPV6
    $FEA_FUNCS create_interface $tid $VIF1_IPV6

    $FEA_FUNCS create_vif $tid $VIF0 $VIF0
    $FEA_FUNCS create_vif $tid $VIF1 $VIF1
    
    $FEA_FUNCS create_vif $tid $VIF0_IPV6 $VIF0_IPV6
    $FEA_FUNCS create_vif $tid $VIF1_IPV6 $VIF1_IPV6

    $FEA_FUNCS enable_vif $tid $VIF0 $VIF0
    $FEA_FUNCS enable_vif $tid $VIF1 $VIF1

    $FEA_FUNCS enable_vif $tid $VIF0_IPV6 $VIF0_IPV6
    $FEA_FUNCS enable_vif $tid $VIF1_IPV6 $VIF1_IPV6

    $FEA_FUNCS create_address4 $tid $VIF0 $VIF0 $NH1
    $FEA_FUNCS create_address4 $tid $VIF1 $VIF1 $NH2

    $FEA_FUNCS create_address6 $tid $VIF0_IPV6 $VIF0_IPV6 $NH1_IPV6
    $FEA_FUNCS create_address6 $tid $VIF1_IPV6 $VIF1_IPV6 $NH2_IPV6

    $FEA_FUNCS set_prefix4 $tid $VIF0 $VIF0 $NH1 24
    $FEA_FUNCS set_prefix4 $tid $VIF1 $VIF1 $NH2 24

    $FEA_FUNCS set_prefix6 $tid $VIF0_IPV6 $VIF0_IPV6 $NH1_IPV6 24
    $FEA_FUNCS set_prefix6 $tid $VIF1_IPV6 $VIF1_IPV6 $NH2_IPV6 24

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

config_peers_ipv6()
{
    coord reset

    coord target $HOST $PORT1_IPV6
    coord initialise attach peer1

    coord peer1 establish AS $PEER1_AS_IPV6 \
	holdtime $HOLDTIME \
	id 10.10.10.1 \
	keepalive false

    coord peer1 assert established

    coord target $HOST $PORT2_IPV6
    coord initialise attach peer2

    coord peer2 establish AS $PEER2_AS_IPV6 \
	holdtime $HOLDTIME \
	id 10.10.10.2 \
	keepalive false

    coord peer2 assert established

    coord target $HOST $PORT3_IPV6
    coord initialise attach peer3

    coord peer3 establish AS $PEER3_AS_IPV6 \
	holdtime $HOLDTIME \
	id 10.10.10.3 \
	keepalive false

    coord peer3 assert established
}

NLRI1=10.10.10.0/24
NLRI2=20.20.20.20/24
NLRI3=30.30.30.30/24
NLRI4=40.40.40.40/24

test1()
{
    echo "TEST1 - Exercise the next hop resolver"

    config_peers

    PACKET1="packet update
	origin 2
	aspath $PEER1_AS
	nexthop 128.16.0.1
	nlri $NLRI1
	nlri $NLRI2"

    PACKET2="packet update
	origin 2
	aspath $PEER1_AS
	nexthop 128.16.1.1
	nlri $NLRI3
	nlri $NLRI4"

    # XXX This next hop should not resolve.

    coord peer1 send $PACKET1
    coord peer1 send $PACKET2
    echo "Sent routes to BGP, waiting..."
    sleep 2

    coord peer2 trie recv lookup $NLRI1 not
    coord peer2 trie recv lookup $NLRI2 not
    coord peer2 trie recv lookup $NLRI3 not
    coord peer2 trie recv lookup $NLRI4 not

    # Lets get it to resolve.
    add_route4 connected true false 128.16.0.0/16 $NH1

    # Add a different route.
    add_route4 connected true false 128.16.0.0/24 $NH2
    
    # Delete the better route.
    delete_route4 connected true false 128.16.0.0/24

    # Try and verify that the correct route has popped out at peer2.
    sleep 2
    coord peer2 trie recv lookup $NLRI1 aspath "$AS,$PEER1_AS"
    coord peer2 trie recv lookup $NLRI2 aspath "$AS,$PEER1_AS"
    coord peer2 trie recv lookup $NLRI3 aspath "$AS,$PEER1_AS"
    coord peer2 trie recv lookup $NLRI4 aspath "$AS,$PEER1_AS"
    
    # Make sure that the connections still exist.

    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established

    # Tidy up. Remove any routes that we may have added to the RIB.
    delete_route4 connected true false 128.16.0.0/16

    # At this point the RIB can no longer resolve the nexthop associated
    # with this net so the net should no longer be present.
    sleep 2
    coord peer2 trie recv lookup $NLRI1 not
    coord peer2 trie recv lookup $NLRI2 not
    coord peer2 trie recv lookup $NLRI3 not
    coord peer2 trie recv lookup $NLRI4 not
}

NLRI1_IPV6=10:10:10:10:10:00:00:00/80
NLRI2_IPV6=20:20:20:20:20:00:00:00/80
NLRI3_IPV6=30:30:30:30:30:00:00:00/80
NLRI4_IPV6=40:40:40:40:40:00:00:00/80

test1_ipv6()
{
    echo "TEST1 IPV6 - Exercise the next hop resolver"

    config_peers_ipv6

    PACKET1="packet update
	origin 2
	aspath $PEER1_AS_IPV6
	nexthop6 128:16::1
	nlri6 $NLRI1_IPV6
	nlri6 $NLRI2_IPV6"

    PACKET2="packet update
	origin 2
	aspath $PEER1_AS_IPV6
	nexthop6 128:16:1::1
	nlri6 $NLRI3_IPV6
	nlri6 $NLRI4_IPV6"

    # XXX This next hop should not resolve.

    coord peer1 send $PACKET1
    coord peer1 send $PACKET2
    echo "Sent routes to BGP, waiting..."
    sleep 2

    coord peer2 trie recv lookup $NLRI1_IPV6 not
    coord peer2 trie recv lookup $NLRI2_IPV6 not
    coord peer2 trie recv lookup $NLRI3_IPV6 not
    coord peer2 trie recv lookup $NLRI4_IPV6 not

    # Lets get it to resolve.
    add_route6 connected true false 128:16::0/32 $NH1_IPV6

    # Add a different route.
    add_route6 connected true false 128:16::0/48 $NH2_IPV6
    
    # Delete the better route.
    delete_route6 connected true false 128:16::0/48

    # Try and verify that the correct route has popped out at peer2.
    sleep 2
    coord peer2 trie recv lookup $NLRI1_IPV6 aspath "$AS,$PEER1_AS_IPV6"
    coord peer2 trie recv lookup $NLRI2_IPV6 aspath "$AS,$PEER1_AS_IPV6"
    coord peer2 trie recv lookup $NLRI3_IPV6 aspath "$AS,$PEER1_AS_IPV6"
    coord peer2 trie recv lookup $NLRI4_IPV6 aspath "$AS,$PEER1_AS_IPV6"
    
    # Make sure that the connections still exist.

    coord peer1 assert established
    coord peer2 assert established
    coord peer3 assert established

    # Tidy up. Remove any routes that we may have added to the RIB.
    delete_route6 connected true false 128:16::0/32

    # At this point the RIB can no longer resolve the nexthop associated
    # with this net so the net should no longer be present.
    sleep 2
    coord peer2 trie recv lookup $NLRI1_IPV6 not
    coord peer2 trie recv lookup $NLRI2_IPV6 not
    coord peer2 trie recv lookup $NLRI3_IPV6 not
    coord peer2 trie recv lookup $NLRI4_IPV6 not
}

test2()
{
    echo "TEST2 - Run test1 twice with the same process"

    test1
    test1
}

test2_ipv6()
{
    echo "TEST2 IPV6 - Run test1 twice with the same process"

    test1_ipv6
    test1_ipv6
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
TESTS='test1 test1_ipv6 test2 test2_ipv6 test3 test4'

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
    configure_fea
    set +e
    configure_rib
    set +e
    configure_bgp
    set -e
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
