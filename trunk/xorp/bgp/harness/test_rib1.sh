#!/usr/bin/env bash

#
# $XORP: xorp/bgp/harness/test_rib1.sh,v 1.11 2003/09/21 00:35:32 atanu Exp $
#

#
# Test basic BGP and RIB interaction. Note that all the tests are run twice
# once with the RIB unconfigured and once with the RIB configured.
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
# 6) Run "./coord"
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

# IBGP - IPV4
PORT1=10000
PEER1_PORT=20001
PEER1_AS=$AS

# EBGP - IPV4
PORT2=10001
PEER2_PORT=20001
PEER2_AS=65009

# IBGP - IPV6
PORT1_IPV6=10002
PEER1_PORT_IPV6=20002
PEER1_AS_IPV6=$AS

# EBGP - IPV6
PORT2_IPV6=10003
PEER2_PORT_IPV6=20003
PEER2_AS_IPV6=65009

HOLDTIME=5

configure_bgp()
{
    LOCALHOST=$HOST
    ID=192.150.187.78
    local_config $AS $ID

    # IBGP - IPV4
    PEER=$HOST
    NEXT_HOP=192.150.187.78
    PORT=$PORT1;PEER_PORT=$PEER1_PORT;PEER_AS=$PEER1_AS
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    enable_peer $IPTUPLE

    # EBGP - IPV4
    PEER=$HOST
    NEXT_HOP=192.150.187.78
    PORT=$PORT2;PEER_PORT=$PEER2_PORT;PEER_AS=$PEER2_AS
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    enable_peer $IPTUPLE

    # IBGP - IPV6
    PEER=$HOST
    NEXT_HOP=192.150.187.78
    PORT=$PORT1_IPV6;PEER_PORT=$PEER1_PORT_IPV6;PEER_AS=$PEER1_AS_IPV6
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocolIPv6
    enable_peer $IPTUPLE

    # EBGP - IPV6
    PEER=$HOST
    NEXT_HOP=192.150.187.78
    PORT=$PORT2_IPV6;PEER_PORT=$PEER2_PORT_IPV6;PEER_AS=$PEER2_AS_IPV6
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    add_peer $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocolIPv6
    enable_peer $IPTUPLE
}

# Next Hops
#NH1=10.10.10.10
#NH2=20.20.20.20
NH1=172.16.1.1
NH2=172.16.2.1
NH1_IPV6=40:40:40:40:40:40:40:40
NH2_IPV6=50:50:50:50:50:50:50:50

configure_rib()
{
    no_fea
    make_rib_errors_fatal

    VIF0="vif0"
    VIF1="vif1"
    VIF0_IPV6="vif2"
    VIF1_IPV6="vif3"

    new_vif $VIF0
    new_vif $VIF1
    new_vif $VIF0_IPV6
    new_vif $VIF1_IPV6

    add_vif_addr4 $VIF0 $NH1 $NH1/24
    add_vif_addr4 $VIF1 $NH2 $NH2/24

    add_vif_addr6 $VIF0_IPV6 $NH1_IPV6 $NH1_IPV6/24
    add_vif_addr6 $VIF1_IPV6 $NH2_IPV6 $NH2_IPV6/24
}

reset_ibgp()
{
    coord reset
    coord target $HOST $PORT1
    coord initialise attach peer1
    # Allow the initialization to take place
    sleep 1
}

reset_ibgp_ipv6()
{
    coord reset
    coord target $HOST $PORT1_IPV6
    coord initialise attach peer1
    # Allow the initialization to take place
    sleep 1
}

reset_ebgp()
{
    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1
    # Allow the initialization to take place
    sleep 1
}

reset_ebgp_ipv6()
{
    coord reset
    coord target $HOST $PORT2_IPV6
    coord initialise attach peer1
    # Allow the initialization to take place
    sleep 1
}

test1()
{
    echo "TEST1 - Send the same route to IBGP hence RIB twice"
    reset_ibgp

    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.100
    sleep 2

    PACKET="packet update
	origin 2
	aspath 1
	nexthop $NH1
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    coord peer1 send $PACKET
    coord peer1 send $PACKET
    
    coord peer1 assert established
}

test1_ipv6()
{
    echo "TEST1 IPV6 - Send the same route to IBGP hence RIB twice"
    reset_ibgp_ipv6

    coord peer1 establish AS $PEER1_AS_IPV6 holdtime 0 id 192.150.187.100
    sleep 2

    NLRI1=10:10:10:10:10:00:00:00/80
    NLRI2=20:20:20:20:20:00:00:00/80

    PACKET="packet update
	origin 2
	aspath 15
	nexthop6 $NH1_IPV6
	nlri6 $NLRI1
	nlri6 $NLRI2"

    coord peer1 send $PACKET
    coord peer1 send $PACKET
    
    coord peer1 assert established
}

test2()
{
    echo "TEST2 - Send the same route to EBGP hence RIB twice"
    reset_ebgp

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100
    sleep 2

    PACKET="packet update
	origin 2
	aspath $PEER2_AS
	nexthop $NH1
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    coord peer1 send $PACKET
    coord peer1 send $PACKET
    
    coord peer1 assert established
}

test3()
{
    echo "TEST3 - Send the same route to BGP hence RIB twice"
    reset_ebgp

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100
    sleep 2

    PACKET="packet update
	origin 2
	aspath 65009
	nexthop 192.150.187.2
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    coord peer1 send $PACKET
    sleep 2
    coord peer1 send $PACKET
    sleep 2

    coord peer1 assert established
}

test4()
{
    if [ ${RIB_CONFIGURED:-""} == "" ]
    then
	return
    fi

    echo "TEST4 - Exercise the next hop resolver"
    reset_ebgp

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100
    sleep 2

    PACKET1="packet update
	origin 2
	aspath 65009
	nexthop 128.16.0.1
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    PACKET2="packet update
	origin 2
	aspath 65009
	nexthop 128.16.1.1
	nlri 30.30.30.30/24
	nlri 40.40.40.40/24"

    # XXX This next hop should not resolve.

    coord peer1 send $PACKET1
    coord peer1 send $PACKET2
    sleep 2

    # Lets get it to resolve.
    add_route4 connected true false 128.16.0.0/16 172.16.1.2 1

    # Add a different route.
    add_route4 connected true false 128.16.0.0/24 172.16.2.2 1
    
    # Delete the better route.
    delete_route4 connected true false 128.16.0.0/24

    sleep 2
    coord peer1 assert established
}

test5()
{
    if [ ${RIB_CONFIGURED:-""} == "" ]
    then
	return
    fi

    echo "TEST5 - Exercise the next hop resolver"
    reset_ebgp

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100
    sleep 2

    PACKET1="packet update
	origin 2
	aspath 65009
	nexthop 128.16.0.1
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    PACKET2="packet update
	origin 2
	aspath 65009
	nexthop 128.16.1.1
	nlri 30.30.30.30/24
	nlri 40.40.40.40/24"

    # Lets get it to resolve.
    add_route4 connected true false 128.16.0.0/16 172.16.1.2 1

    # Add a different route.
    add_route4 connected true false 128.16.0.0/24 172.16.2.2 1
    
    coord peer1 send $PACKET1
    coord peer1 send $PACKET2
    sleep 2

    coord peer1 assert established
}

test6()
{
    ITER=5
    echo "TEST6 - Send an update packet on an EBGP $ITER times"
    PACKET='packet update
	origin 1
	aspath 65009
	nexthop 20.20.20.20 
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24'

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1
    # Allow the initialization to take place
    sleep 1
    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    sleep 2
    coord peer1 assert established

    set +e
    let i=0
    while ((i++ < $ITER))
    do
	# Using the (()) syntax seems to have invoked a new shell.
	# Any errors have to be explicitly propagated.
	trap 'exit $?' 0 2
	set -e
	echo "Iteration: $i"

	coord peer1 send $PACKET
	coord peer1 trie sent lookup 10.10.10.0/24 aspath 65009
	coord peer1 send packet update withdraw 10.10.10.0/24
	coord peer1 send packet update withdraw 20.20.20.0/24

	coord peer1 assert established

	# Every IPC call uses up more ports. Stop sending IPC's when
	# there are too few ports left.
	ports=$(netstat | wc -l)
	if (($ports > 900))
	then
	    echo "$ports sockets in use bailing"
	    break
	fi
    done
    set -e

    sleep 2
    coord peer1 assert established
}

test7()
{
    if [ ${RIB_CONFIGURED:-""} == "" ]
    then
	return
    fi

    echo "TEST7 - Try and force BGP to deregister interest from the RIB"
    reset_ebgp

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    PACKET_ADD1="packet update
	origin 2
	aspath 65009
	nexthop 128.16.0.1
	nlri 10.10.10.0/24"

    PACKET_ADD2="packet update
	origin 2
	aspath 65009
	nexthop 128.16.1.1
	nlri 20.20.20.20/24"

    PACKET_WITHDRAW="packet update
	withdraw 10.10.10.0/24
	withdraw 20.20.20.20/24"

    # Lets get it to resolve.
    add_route4 connected true false 128.16.0.0/16 172.16.1.2 1
    sleep 2

    # Add a different route.
    add_route4 connected true false 128.16.0.0/24 172.16.2.2 1
    sleep 2

    coord peer1 send $PACKET_ADD1
    coord peer1 send $PACKET_ADD2
    sleep 2

    # delete the routes 
    delete_route4 connected true false 128.16.0.0/24
    delete_route4 connected true false 128.16.0.0/16

    coord peer1 send $PACKET_WITHDRAW
    sleep 2

    coord peer1 assert established
}

test8()
{
    if [ ${RIB_CONFIGURED:-""} == "" ]
    then
	return
    fi

    echo "TEST8 - Exercise the next hop resolver"
    reset_ebgp

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100
    sleep 2

    PACKET1="packet update
	origin 2
	aspath 65009
	nexthop 128.16.0.1
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    PACKET2="packet update
	origin 2
	aspath 65009
	nexthop 128.16.1.1
	nlri 30.30.30.30/24
	nlri 40.40.40.40/24"

    # XXX This next hop should not resolve.

    coord peer1 send $PACKET1
    coord peer1 send $PACKET2
    sleep 2

    # Lets get it to resolve.
    add_route4 connected true false 128.16.0.0/16 172.16.1.2 1

    # Add a different route.
    add_route4 connected true false 128.16.0.0/24 172.16.2.2 2
    
    # Delete the better route.
    delete_route4 connected true false 128.16.0.0/24

    sleep 2
    coord peer1 assert established
}

TESTS_NOT_FIXED=''
TESTS='test1 test1_ipv6 test2 test3 test4 test5 test6 test7 test8'

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
    ./coord                   = $CXRL finder://coord/common/0.1/get_target_name
EOF
    trap '' 0
    exit $?
fi

# We run each test twice once with the RIB configured and once without.

if [ $CONFIGURE = "yes" ]
then
    configure_bgp
fi

for i in $TESTS
do
    $i
done

if [ $CONFIGURE = "yes" ]
then
    configure_rib
    RIB_CONFIGURED=true
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
