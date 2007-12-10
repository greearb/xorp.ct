#!/usr/bin/env bash

#
# $XORP: xorp/bgp/harness/test_peering2.sh,v 1.60 2007/12/07 20:50:16 atanu Exp $
#

#
# Feed saved data to our BGP process.
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

# srcdir is set by make for check target
if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi
. ${srcdir}/xrl_shell_funcs.sh ""
. ${srcdir}/../xrl_shell_funcs.sh ""
. ${srcdir}/../../rib/xrl_shell_funcs.sh ""
. ${srcdir}/notification_codes.sh

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

HOST=127.0.0.1
LOCALHOST=$HOST
ID=192.150.187.78
AS=65008
USE4BYTEAS=false

# IBGP
PORT1=10001
PEER1_PORT=20001
PEER1_AS=$AS

# EBGP
PORT2=10002
PEER2_PORT=20002
PEER2_AS=65000

# EBGP
PORT3=10003
PEER3_PORT=20003
PEER3_AS=65003

HOLDTIME=5

TRAFFIC_DIR="${srcdir}/../../../data/bgp"
TRAFFIC_FILES="${TRAFFIC_DIR}/icsi1.mrtd"

# NOTE: The Win32 versions of coord and peer will perform
# path conversion and expansion of /tmp internally.
TMPDIR=${TMPDIR:-/tmp}
EXT=${LOGNAME:-unknown}

configure_bgp()
{
    local_config $AS $ID $USE4BYTEAS

    # Don't try and talk to the rib.
    register_rib ""

    PEER=$HOST
    NEXT_HOP=192.150.187.78
    add_peer $LOCALHOST $PORT1 $PEER $PEER1_PORT $PEER1_AS $NEXT_HOP $HOLDTIME
    set_parameter $LOCALHOST $PORT1 $PEER $PEER1_PORT MultiProtocol.IPv4.Unicast true
    enable_peer $LOCALHOST $PORT1 $PEER $PEER1_PORT

    PEER=$HOST
    NEXT_HOP=192.150.187.78
    add_peer $LOCALHOST $PORT2 $PEER $PEER2_PORT $PEER2_AS $NEXT_HOP $HOLDTIME
    set_parameter $LOCALHOST $PORT2 $PEER $PEER2_PORT MultiProtocol.IPv4.Unicast true
    enable_peer $LOCALHOST $PORT2 $PEER $PEER2_PORT

    PEER=$HOST
    NEXT_HOP=192.150.187.78
    add_peer $LOCALHOST $PORT3 $PEER $PEER3_PORT $PEER3_AS $NEXT_HOP $HOLDTIME
    set_parameter $LOCALHOST $PORT3 $PEER $PEER3_PORT MultiProtocol.IPv4.Unicast true
    enable_peer $LOCALHOST $PORT3 $PEER $PEER3_PORT
}

wait_for_peerdown()
{
    # Interact with the BGP process itself to find out when the peerings have
    # gone down. If we are not testing the XORP bgp then replace with the
    # sleep.

    while ../tools/print_peers -v | grep 'Peer State' | grep ESTABLISHED
    do
	sleep 2
    done

    # SLEEPY=30
    # echo "sleeping for $SLEEPY seconds"
    # sleep $SLEEPY
}

reset()
{
    coord reset

    coord target $HOST $PORT1
    coord initialise attach peer1

    coord target $HOST $PORT2
    coord initialise attach peer2

    coord target $HOST $PORT3
    coord initialise attach peer3

    bgp_not_established

    while pending | grep true
    do
	sleep 2
    done

    # The test_peer has closed its connection to the BGP process.
    # If the test_peer was injecting a lot of data to the BGP process only
    # when all the data has been read and processed by the BGP process will it
    # see the end of the stream. So add an arbitary delay until the BGP process
    # sees the end of the stream. If we don't do this connection attempts will
    # be rejected by the the BGP process, causing the tests to fail.

    wait_for_peerdown
}

bgp_not_established()
{
    for i in peer1 peer2 peer3
    do
	status $i
	while status $i | grep established
	do
	    sleep 2
	done
    done
}

bgp_peer_unchanged()
{
    while :
    do
	# debug
	status $1

	a=$(status $1)
	sleep 10
	b=$(status $1)
	if [ "$a" = "$b" ]
	then
	    break
	fi
    done
}

delay()
{
    # XXX - decrementing the counter to zero generates an error?
    # So count down to one
    echo "Sleeping for $1 seconds"
    let counter=$1+1
    while [ $counter != 1 ]
    do
	sleep 1
	let counter=counter-1
	echo -e " $counter      \r\c"
    done

    return 0
}

test1()
{
    TFILE=$1

    echo "TEST1 - Inject a saved feed then drop peering - $TFILE"

    # Reset the peers
    reset

    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.102
    coord peer2 assert established

    NOBLOCK=true coord peer2 send dump mrtd update $TFILE

    # Wait for the file to be transmitted by the test peer.
    bgp_peer_unchanged peer2

    coord peer2 assert established

    # Reset the connection
    reset

    if [ x"$OSTYPE" != xmsys ]; then
	uptime
	echo "NOTE: Occasionally, we fail to make a connection. See the comment in the reset function."
    fi

    # Establish the new connection.
    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.102
    coord peer2 assert established
}

test2()
{
    TFILE=$1

    echo "TEST2 - Inject a saved feed then toggle another peering - $TFILE"

    # Reset the peers
    reset
    
    # Establish the EBGP peering.
    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.102
    coord peer2 assert established

    # send in the saved file
    NOBLOCK=true coord peer2 send dump mrtd update $TFILE

    # Wait for the file to be transmitted by the test peer.
    bgp_peer_unchanged peer2

    # Bring up peer1 and wait for it to receive all the updates
    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.101
    bgp_peer_unchanged peer1

    # Bring up another peering to test the dump code.
    for i in 1 2
    do
        coord peer1 disconnect

	# Wait for the peering to be dropped
	while status peer1 | grep established
	do
	    sleep 2
	done

	sleep 2

	coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.101
	sleep 2
	coord peer1 assert established

	# Wait for the BGP process to send all the updates to peer1
	bgp_peer_unchanged peer1
    done

    coord peer2 disconnect

    while status peer2 | grep established
    do
	sleep 2
    done

    sleep 2

    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.102
    coord peer2 assert established

    # Tearing out peer2 will cause all the routes to be withdrawn, wait
    bgp_peer_unchanged peer1

    # Make sure that the connections are still established.
    coord peer1 assert established
    coord peer2 assert established
}

test3()
{
    TFILE=$1

    echo "TEST3:"
    echo "      1) Start injecting a saved feed (peer2) - $TFILE" 
    echo "      2) Bring in a second peering (peer1) "
    echo "      3) Drop the injecting feed (peer2) "

    # Reset the peers
    reset
    
    # Establish the EBGP peering.
    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.102
    coord peer2 assert established

    # send in the saved file
    NOBLOCK=true coord peer2 send dump mrtd update $TFILE

    # Bring up another peering
    NOBLOCK=true coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.101

    # Allow some routes to be loaded
    sleep 5

    # Drop the injecting feed
    NOBLOCK=true coord peer2 disconnect

    # Wait for the BGP to stabilise
    bgp_peer_unchanged peer1

    # Add a delay so if the BGP process core dumps we detect it.
    sleep 5

    # Make sure that the peer1 connection is still established
    coord peer1 assert established
}

test4()
{
    TFILE=$1

    echo "TEST4:"
    echo "      1) Start injecting a saved feed (peer2) - $TFILE" 
    echo "      2) Immediately bring up a second peering (peer1) "

    # Reset the peers
    reset

    result=$(status peer1)
    echo "$result"

    # Establish the EBGP peering.
    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.102
    coord peer2 assert established

    # send in the saved file
    NOBLOCK=true coord peer2 send dump mrtd update $TFILE

    # Bring up a second peering and wait for all the updates to arrive
    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.101

    bgp_peer_unchanged peer1

    # Make sure that the peer1 connection is still established
    coord peer1 assert established
    coord peer2 assert established
}

test5()
{
    TFILE=$1

    echo "TEST5:"
    echo "      1) Start injecting a saved feed (peer2) - $TFILE" 
    echo "      2) Immediately bring up a second peering (peer1) "
    echo "      3) Wait for all the updates to arrive at (peer1) "
    echo "      4) Drop both peerings "
    echo "      5) Bring up (peer1) "
    echo "      6) Peer1 should not receive any update traffic "

    # Reset the peers
    reset

    result=$(status peer1)
    echo "$result"

    # Establish the EBGP peering.
    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.102
    coord peer2 assert established

    # send in the saved file
    NOBLOCK=true coord peer2 send dump mrtd update $TFILE

    # Bring up a second peering and wait for all the updates to arrive
    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.101

    bgp_peer_unchanged peer1

    coord reset

    # Debugging to demonstrate that the BGP process believes that both peerings
    # have been taken down.
    ../tools/print_peers -v

    # debug
    status peer1
    status peer2

    # Establish peer1
    coord target $HOST $PORT1
    coord initialise attach peer1
    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.101

    # If peer1 receives any updates this is an error
    a=$(status peer1)
    sleep 2
    b=$(status peer1)
    if [ "$a" = "$b" ]
    then
	    :
    else
	echo "Peer1 received updates, but this is the only peering?"
	echo $a
	echo $b
	return -1
    fi
}

test6()
{
    TFILE=$1
    UPDATE_COUNT=10

    echo "TEST6 (testing sending only $UPDATE_COUNT updates):"
    echo "      1) Start injecting a saved feed (peer2) - $TFILE" 
    echo "      2) Immediately bring up a second peering (peer1) "
    echo "      3) Wait for all the updates to arrive at (peer1) "
    echo "      4) Verify that both peering still exist."

    # Reset the peers
    reset

    status peer1
    status peer1

    # Establish the EBGP peering.
    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.102
    coord peer2 assert established

    # send in the saved file
    NOBLOCK=true coord peer2 send dump mrtd update $TFILE $UPDATE_COUNT

    # Bring up a second peering and wait for all the updates to arrive
    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.101

    bgp_peer_unchanged peer1

    status peer1
    status peer2

    coord peer2 assert established
    coord peer2 assert established
}

test7()
{
    echo "TEST7 (Simple route propogation test - searching for memory leak):"
    echo "	1) Bring up peering (peer2) and introduce one route"
    echo "	2) Bring up a second peering (peer1) check route"
    echo "	3) Tear down both peerings"

    reset

    # Establish an EBGP peering.
    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.102
    coord peer2 assert established

    # Inject a route
    coord peer2 send packet update \
	origin 2 \
	aspath "$PEER2_AS" \
	nexthop 10.10.10.10 \
	nlri 10.10.10.0/24

    sleep 1

    # Bring up a second peering
    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.101

    sleep 1

    coord peer1 assert established
    coord peer2 assert established

    reset
}

test8()
{
    echo "TEST8 (Simple route propogation test - searching for memory leak):"
    echo "	1) Bring up peering (peer1)"
    echo "	2) Bring up a second peering (peer2) and introduce a route"
    echo "	3) Tear down both peerings"

    reset

    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.101
    coord peer1 assert established

    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.102
    coord peer2 assert established

    # Inject a route
    coord peer2 send packet update \
	origin 2 \
	aspath "$PEER2_AS" \
	nexthop 10.10.10.10 \
	nlri 10.10.10.0/24


    sleep 1

    coord peer1 assert established
    coord peer2 assert established

    reset
}

test9()
{
    echo "TEST9 (Simple route propogation test - searching for memory leak):"
    echo "	1) Bring up peering (peer1)"
    echo "	2) Bring up a second peering (peer2) and introduce a route"
    echo "	3) Tear down both peerings"

    reset

    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.102
    coord peer2 assert established

    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.101
    coord peer1 assert established

    # Inject a route
    coord peer2 send packet update \
	origin 2 \
	aspath "$PEER2_AS" \
	nexthop 10.10.10.10 \
	nlri 10.10.10.0/24


    sleep 1

    coord peer1 assert established
    coord peer2 assert established

    reset
}

test10()
{
    echo "TEST10 (Verify that a router ID of 0.0.0.0 doesn't cause a problem):"
    echo "	1) Bring up peering (peer1)"

    reset

    coord peer2 connect

    sleep 2

    coord peer2 expect packet notify $OPENMSGERROR $BADBGPIDENT

    coord peer2 send packet \
	open \
	asnum $PEER2_AS \
	bgpid 0.0.0.0 \
	holdtime 0

    sleep 2

    coord peer2 assert queue 0

    reset
}

test11()
{
    echo "TEST11 (Behave sanely when two peers use the same router ID):"
    echo "	1) Bring up peering (peer1)"
    echo "	2) Bring up a second peering (peer2)"
    echo "	3) Tear down both peerings"

    reset

    coord peer2 establish AS $PEER2_AS holdtime 0 id 1.1.1.1
    coord peer2 assert established

    coord peer1 establish AS $PEER1_AS holdtime 0 id 1.1.1.1
    coord peer1 assert established

    sleep 1

    coord peer1 assert established
    coord peer2 assert established

    reset
}

test12()
{
    echo "TEST12 (Behave sanely when a peer reuses a router ID):"
    echo "	1) Bring up peering (peer1)"
    echo "	2) Tear down the peering (peer1)"
    echo "	2) Bring up a second peering (peer2) with the same router ID"
    echo "	3) Tear down peering (peer2)"

    reset

    coord peer2 establish AS $PEER2_AS holdtime 0 id 1.1.1.1
    coord peer2 assert established

    # Reset the connection and reuse the ID on the other peer,should be legal. 
    reset

    coord peer1 establish AS $PEER1_AS holdtime 0 id 1.1.1.1
    coord peer1 assert established

    sleep 1

    coord peer1 assert established

    reset
}

test13()
{
    TFILE=$1

    echo "TEST13"
    echo "         1) Inject a saved peering - $TFILE"
    echo "         2) Change next-hop resolution"
    echo "         3) Drop peering"
    echo "         4) Used to cause BGP to fail"

    # Reset the peers
    reset

    VIF0="vif0"
    IF1=172.16.1.1
    GW1=172.16.1.2
    NH1=192.150.187.2

    register_rib "rib"
    new_vif $VIF0
    add_vif_addr4 $VIF0 $IF1 $IF1/24

    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.102
    coord peer2 assert established

    sleep 2

    NOBLOCK=true coord peer2 send dump mrtd update $TFILE

    bgp_peer_unchanged peer2

    coord peer2 assert established

    add_igp_table4 is-is isis isis true false
    add_route4 is-is true false $NH1/24 $GW1 10

    # Reset the connection
    reset
    
    # Establish the new connection.
    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.102
    coord peer2 assert established
}

test14()
{
    echo "TEST14 (Simple route propogation test with RIB resolving next hop):"
    echo "	1) Bring up peering (peer1)"
    echo "	2) Bring up a second peering (peer2)"
    echo "	3) Introduce a route on (peer1) the IBGP peering"
    echo "	4) Verify that the route appears on (peer2) the EBGP peering"
    echo "	5) Tear down both peerings"

    reset

    VIF0="vif0"
    IF1=172.16.1.1
    GW1=172.16.1.2
    NH1=192.150.187.2

    register_rib "rib"
    new_vif $VIF0
    add_vif_addr4 $VIF0 $IF1 $IF1/24

    IGP_METRIC=10

    add_igp_table4 is-is isis isis true false
    add_route4 is-is true false $NH1/24 $GW1 $IGP_METRIC

    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.101
    coord peer1 assert established

    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.102
    coord peer2 assert established

    coord peer2 expect packet update \
	origin 2 \
	aspath 65008,1000 \
	med $IGP_METRIC \
	nexthop $NEXT_HOP \
	nlri 10.10.10.0/24

    # Inject a route
    coord peer1 send packet update \
	origin 2 \
	aspath 1000 \
	localpref 100 \
	nexthop $NH1 \
	nlri 10.10.10.0/24

    sleep 2

    coord peer2 assert queue 0

    coord peer1 assert established
    coord peer2 assert established

    reset
}

test15()
{
    TFILE=$1

    echo "TEST15:"
    echo "      1) Start injecting a saved feed (peer1 I-BGP) - $TFILE" 
    echo "      2) Wait until the whole feed has been sent" 
    echo "      3) Bring up a second peering (peer3 E-BGP)"
    echo "      4) Verify all sent packets arrived"
    echo "      5) Verify all received packets were sent"

    # Reset the peers
    reset

    # Establish the EBGP peering.
    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.101
    sleep 2
    coord peer1 assert established

    # Save the traffic that we send on peer1
    SENT=${TMPDIR}/peer1_sent.text.$EXT
    rm -f $SENT
    coord peer1 dump sent text ipv4 traffic $SENT

    # send in the saved file
    UPDATES=
    NOBLOCK=true coord peer1 send dump mrtd update $TFILE $UPDATES

    # Wait for the whole feed to be sent it may not all have arrived at BGP
    bgp_peer_unchanged peer1

    # XXXX
    # Put a big delay here if you want all the packets to have been processed
    # by BGP.

    # Save the traffic that we receive on peer3 useful for debugging
    RECV=${TMPDIR}/peer3_recv.text.$EXT
    rm -f $RECV
    coord peer3 dump recv text ipv4 traffic $RECV

    # Bring up a second peering and wait for all the updates to arrive
    coord peer3 establish AS $PEER3_AS holdtime 0 id 192.150.187.103

    # Wait for the whole feed to be received
    bgp_peer_unchanged peer3

    # Check all the sent packets and verify that they arrived at peer3
    XRLS=${TMPDIR}/xrls.$EXT
    echo "Building lookup list in $XRLS"
    cat $SENT | $PYTHON $srcdir/lookup.py --peer peer3 \
					 --trie recv \
					 --add 65008 > $XRLS
    echo "Performing lookup"					 
    $CALLXRL -f $XRLS

    # Check all the received packets and verify that they were sent by peer1
    XRLS=${TMPDIR}/xrls.$EXT
    echo "Building lookup list in $XRLS"
    cat $RECV | $PYTHON $srcdir/lookup.py --peer peer1 \
					 --trie sent \
					 --remove 65008 > $XRLS
    echo "Performing lookup"					 
    $CALLXRL -f $XRLS

    # Make sure that the peer1 connection is still established
    coord peer1 assert established
    coord peer3 assert established

    rm -f $SENT $RECV $XRLS
}

test16()
{
    TFILE=$1

    echo "TEST16:"
    echo "      1) Start injecting a saved feed (peer2 E-BGP) - $TFILE" 
    echo "      2) Wait until the whole feed has been sent" 
    echo "      3) Bring up a second peering (peer1 I-BGP) "
    echo "      4) Verify all sent packets arrived"
    echo "      5) Verify all received packets were sent"

    # Reset the peers
    reset

    # Establish the EBGP peering.
    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.102
    coord peer2 assert established

    # Save the traffic that we send on peer2
    SENT=${TMPDIR}/peer2_sent.text.$EXT
    rm -f $SENT
    coord peer2 dump sent text ipv4 traffic $SENT

    # send in the saved file
    UPDATES=
    NOBLOCK=true coord peer2 send dump mrtd update $TFILE $UPDATES

    # Wait for the whole feed to be sent it may not all have arrived at BGP
    bgp_peer_unchanged peer2

    # XXXX
    # Put a big delay here if you want all the packets to have been processed
    # by BGP.

    # Save the traffic that we receive on peer1 useful for debugging
    RECV=${TMPDIR}/peer1_recv.text.$EXT
    rm -f $RECV
    coord peer1 dump recv text ipv4 traffic $RECV

    # Bring up a second peering and wait for all the updates to arrive
    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.101

    # Wait for the whole feed to arrive
    bgp_peer_unchanged peer1

    # Check all the sent packets and verify that they arrived at peer1
    XRLS=${TMPDIR}/xrls.$EXT
    echo "Building lookup list in $XRLS"
    cat $SENT | $PYTHON $srcdir/lookup.py --peer peer1 \
					 --trie recv > $XRLS
    echo "Performing lookup"					 
    $CALLXRL -f $XRLS


    # Check all the received packets and verify that they were sent by peer2
    XRLS=${TMPDIR}/xrls.$EXT
    echo "Building lookup list in $XRLS"
    cat $RECV | $PYTHON $srcdir/lookup.py --peer peer2 \
					 --trie sent \
					 --remove 65008 > $XRLS
    echo "Performing lookup"					 
    $CALLXRL -f $XRLS

    # Make sure that the peer1 connection is still established
    coord peer1 assert established
    coord peer2 assert established

    rm -f $SENT $RECV $XRLS
}

test17()
{
    echo "TEST17:"
    echo "      1) Run test15 thrice" 

    test15 $*
    test15 $*
    test15 $*
}

test_work_in_progress()
{
    TFILE=$1

    echo "TEST:"
    echo "      1) Start injecting a saved feed (peer1 I-BGP) - $TFILE" 
    echo "      2) Wait until the whole feed has been sent" 
    echo "      3) Bring up a second peering (peer3 E-BGP)"
    echo "      4) Verify all sent packets arrived"
    echo "      5) Verify all received packets were sent"

    # Reset the peers
    reset

    # Establish the EBGP peering.
    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.101
    coord peer1 assert established

    # Save the traffic that we send on peer1
    SENT=${TMPDIR}/peer1_sent.text.$EXT
    rm -f $SENT
    coord peer1 dump sent text ipv4 traffic $SENT

    # send in the saved file
    UPDATES=
    NOBLOCK=true coord peer1 send dump mrtd update $TFILE $UPDATES

    # Wait for the whole feed to be sent it may not all have arrived at BGP
    bgp_peer_unchanged peer1

    # XXXX
    # Put a big delay here if you want all the packets to have been processed
    # by BGP.

    for i in 1 2
    do
	# Drop receive peering the first time round we are not connected
	coord peer3 disconnect

	# Save the traffic that we receive on peer3 useful for debugging
	RECV=${TMPDIR}/peer3_recv.text.$EXT
	rm -f $RECV
	coord peer3 dump recv text ipv4 traffic $RECV

	# Bring up a second peering and wait for all the updates to arrive
	coord peer3 establish AS $PEER3_AS holdtime 0 id 192.150.187.103

	# Wait for the whole feed to be received
	bgp_peer_unchanged peer3

	# Check all the sent packets and verify that they arrived at peer3
	XRLS=${TMPDIR}/xrls.$EXT
	cat $SENT | $PYTHON $srcdir/lookup.py --peer peer3 \
					 --trie recv \
					 --add 65008 > $XRLS
	$CALLXRL -f $XRLS

	# Check all the received packets and verify that they were sent by peer1
	XRLS=${TMPDIR}/xrls.$EXT
	cat $RECV | $PYTHON $srcdir/lookup.py --peer peer1 \
					 --trie sent \
					 --remove 65008 > $XRLS
	$CALLXRL -f $XRLS

	# Make sure that the peer1 connection is still established
	coord peer1 assert established
	coord peer3 assert established
    done

    # Stop saving received packets
    coord peer3 dump recv text ipv4 traffic

    for i in 1 2 3 4 5 6 7 8 9 10
    do
	# drop feed peering
	coord peer1 disconnect

	# Establish the EBGP peering.
	coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.101
	coord peer1 assert established

	# Save the traffic that we send on peer1
	SENT=${TMPDIR}/peer1_sent.text.$EXT
	rm -f $SENT
	coord peer1 dump sent text ipv4 traffic $SENT

	# send in the saved file
	UPDATES=
	NOBLOCK=true coord peer1 send dump mrtd update $TFILE $UPDATES

	# Wait for the whole feed to be sent it may not all have arrived at BGP
	bgp_peer_unchanged peer1

	# Wait for the whole feed to be received
	bgp_peer_unchanged peer3

	# Check all the sent packets and verify that they arrived at peer3
	XRLS=${TMPDIR}/xrls.$EXT
	cat $SENT | $PYTHON $srcdir/lookup.py --peer peer3 \
					 --trie recv \
					 --add 65008 > $XRLS
	$CALLXRL -f $XRLS

	# Make sure that the peer1 connection is still established
	coord peer1 assert established
	coord peer3 assert established
    done

    rm -f $SENT $RECV $XRLS
}

TESTS_NOT_FIXED=''
TESTS='test1 test2 test3 test4 test5 test6 test7 test8 test9 test10 test11
    test12 test13 test14 test15 test16 test17'

# Sanity check that python is present on the host system.
PYTHON=${PYTHON:=python}
python_version=$(${PYTHON} -V 2>&1)
if [ "X${python_version}" = "X" ]; then
    exit 2
else
    export PYTHON
fi

# Include command line
. ${srcdir}/args.sh

if [ $START_PROGRAMS = "yes" ]
then
    CXRL="$CALLXRL -r 10"
    runit $QUIET $VERBOSE -c "$0 -s -c $*" <<EOF
    ../../libxipc/xorp_finder
    ../../fea/xorp_fea_dummy  = $CXRL finder://fea/common/0.1/get_target_name
    ../../rib/xorp_rib        = $CXRL finder://rib/common/0.1/get_target_name
    ../xorp_bgp               = $CXRL finder://bgp/common/0.1/get_target_name
    ./test_peer -s peer1      = $CXRL finder://peer1/common/0.1/get_target_name
    ./test_peer -s peer2      = $CXRL finder://peer1/common/0.1/get_target_name
    ./test_peer -s peer3      = $CXRL finder://peer1/common/0.1/get_target_name
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
    for t in ${TFILE:-$TRAFFIC_FILES}
    do 
	if [ -f $t ]
	then
# Temporary fix to let TCP sockets created by call_xrl pass through TIME_WAIT
	    TIME_WAIT=`time_wait_seconds`
	    echo "Waiting $TIME_WAIT seconds for TCP TIME_WAIT state timeout"
	    sleep $TIME_WAIT
	    $i $t
	else
	    echo "Traffic file $t missing."
	fi
    done
done

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
