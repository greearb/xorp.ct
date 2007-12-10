#!/usr/bin/env bash

#
# $XORP: xorp/bgp/harness/test_peering1.sh,v 1.65 2006/08/16 22:10:14 atanu Exp $
#

#
# Test basic BGP error generation and connection creation.
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

# srcdir is set by make for check target
if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi
. ${srcdir}/xrl_shell_funcs.sh ""
. ${srcdir}/../xrl_shell_funcs.sh ""
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

# IBGP - IPV4
PEER1=$HOST
PORT1=10001
PEER1_PORT=20001
PEER1_AS=$AS

# EBGP - IPV4
PEER2=$HOST
PORT2=10002
PEER2_PORT=20002
PEER2_AS=65000

# IBGP - IPV6
PEER3=$HOST
PORT3=10003
PEER3_PORT=20003
PEER3_AS=$AS

# EBGP - IPV6
PEER4=$HOST
PORT4=10004
PEER4_PORT=20004
PEER4_AS=65000

HOLDTIME=30

configure_bgp()
{
    local_config $AS $ID $USE4BYTEAS

    # Don't try and talk to the rib.
    register_rib ""

    # IBGP - IPV4
    PEER=$PEER1
    NEXT_HOP=192.150.187.78
    add_peer $LOCALHOST $PORT1 $PEER $PEER1_PORT $PEER1_AS $NEXT_HOP $HOLDTIME
    set_parameter $LOCALHOST $PORT1 $PEER $PEER1_PORT MultiProtocol.IPv4.Unicast true
    enable_peer $LOCALHOST $PORT1 $PEER $PEER1_PORT

    # EBGP - IPV4
    PEER=$PEER2
    NEXT_HOP=192.150.187.78
    add_peer $LOCALHOST $PORT2 $PEER $PEER2_PORT $PEER2_AS $NEXT_HOP $HOLDTIME
    set_parameter $LOCALHOST $PORT2 $PEER $PEER2_PORT MultiProtocol.IPv4.Unicast true
    enable_peer $LOCALHOST $PORT2 $PEER $PEER2_PORT

    # IBGP - IPV6
    PEER=$PEER3
    NEXT_HOP=192.150.187.78
    add_peer $LOCALHOST $PORT3 $PEER $PEER3_PORT $PEER3_AS $NEXT_HOP $HOLDTIME
    set_parameter $LOCALHOST $PORT3 $PEER $PEER3_PORT MultiProtocol.IPv6.Unicast true
    enable_peer $LOCALHOST $PORT3 $PEER $PEER3_PORT

    # IBGP - IPV6
    PEER=$PEER4
    NEXT_HOP=192.150.187.78
    add_peer $LOCALHOST $PORT4 $PEER $PEER4_PORT $PEER4_AS $NEXT_HOP $HOLDTIME
    set_parameter $LOCALHOST $PORT4 $PEER $PEER4_PORT MultiProtocol.IPv6.Unicast true
    enable_peer $LOCALHOST $PORT4 $PEER $PEER4_PORT
}

reset()
{
    coord reset
    coord target $HOST $PORT1
    coord initialise attach peer1
}

test1()
{
    echo "TEST1 - Establish a peering with a holdtime of $HOLDTIME wait for expiration"

    reset
    # XXX
    # This test is a little suspect as the number of keepalives sent
    # by the peer is implementation dependent although it is
    # recommended that the keepalive interval is determined by
    # dividing the holdtime by three.  There should be a way of
    # soaking up keepalives.

    coord peer1 expect packet open asnum $AS bgpid $ID holdtime $HOLDTIME \
	afi 1 safi 1
    coord peer1 expect packet keepalive
    coord peer1 expect packet keepalive
    coord peer1 expect packet keepalive
    coord peer1 expect packet keepalive
    coord peer1 expect packet notify $HOLDTIMEEXP
    coord peer1 establish AS $PEER1_AS \
	holdtime $HOLDTIME \
	id 192.150.187.100 \
	keepalive false

    sleep $HOLDTIME
    sleep 2

    sleep 2
    coord peer1 assert queue 0
}

test2()
{
    echo "TEST2 - Make a connection and send an update packet before establishing a session"

    reset

    coord peer1 connect

    sleep 2
    coord peer1 expect packet notify $FSMERROR
    coord peer1 send packet update

    sleep 2
    coord peer1 assert queue 0
}

test3()
{
    echo "TEST3 - Try and send an illegal hold time of 1"

    reset

    coord peer1 expect packet open asnum $AS bgpid $ID holdtime $HOLDTIME \
	afi 1 safi 1
    coord peer1 expect packet notify $OPENMSGERROR $UNACCEPTHOLDTIME
    coord peer1 establish AS $PEER1_AS \
	holdtime 1 \
	id 192.150.187.100 \
	keepalive false

    sleep 2
    coord peer1 assert queue 0
}

test4()
{
    echo "TEST4 - Send an update packet and don't get it back"
    PACKET='packet update
	origin 2
	aspath 1,2,[3,4,5],6,[7,8],9
	nexthop 20.20.20.20 
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24
        localpref 100'

    reset
    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.100

    coord peer1 assert established
    coord peer1 expect $PACKET
    coord peer1 send $PACKET

    sleep 2
    coord peer1 assert queue 1
    coord peer1 assert established
}

test5()
{
    echo "TEST5 - Passively wait for a connection."

    coord reset
    coord target $HOST $PEER1_PORT
    coord initialise attach peer1

    coord peer1 establish \
	active false \
	AS $PEER1_AS \
	holdtime $HOLDTIME \
	id 192.150.187.100

    # Toggle the BGP peering to force a connection attempt.
    disable_peer $LOCALHOST $PORT1 $PEER1 $PEER1_PORT $PEER1_AS
    enable_peer $LOCALHOST $PORT1 $PEER1 $PEER1_PORT $PEER1_AS

    sleep 2
    coord peer1 assert established
}

test6()
{
    echo "TEST6 - Send an update packet without an origin"
    PACKET='packet update
	aspath 1,2,[3,4,5],6,[7,8],9
	nexthop 20.20.20.20 
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24'

    reset
    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $UPDATEMSGERR $MISSWATTR 1
    coord peer1 send $PACKET

    sleep 2
    coord peer1 assert queue 0
}

test7()
{
    echo "TEST7 - Send an update packet without an aspath"
    PACKET='packet update
	origin 1
	nexthop 20.20.20.20 
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24'

    reset
    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $UPDATEMSGERR $MISSWATTR 2
    coord peer1 send $PACKET

    sleep 2
    coord peer1 assert queue 0
}

test8()
{
    echo "TEST8 - Send an update packet without a nexthop"
    PACKET='packet update
	origin 1
	aspath 1
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24'

    reset
    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.100

    sleep 2
    coord peer1 expect packet notify $UPDATEMSGERR $MISSWATTR 3
    coord peer1 send $PACKET

    coord peer1 assert queue 0
}

test8_ipv6()
{
    echo "TEST8 IPv6"
    echo "	1) Establish an EBGP IPv6 peering"
    echo "	2) Send a multiprotocol nlri with no nexthop"

    coord reset
    coord target $HOST $PORT4
    coord initialise attach peer1

    coord peer1 establish AS $PEER4_AS holdtime 0 id 192.150.187.100 ipv6 true

    PACKET="packet update
	origin 1
	aspath $PEER4_AS
	nlri6 2000::/3"
    
    coord peer1 expect packet notify $UPDATEMSGERR $MISSWATTR 3
	
    coord peer1 send $PACKET

    sleep 2
    coord peer1 assert queue 0

    coord peer1 assert idle
}

test9()
{
    echo "TEST9:"
    echo "	1) Send an update packet as an IBGP peer with no local pref"
    echo "	2) This is not an error. Our BGP emits a warning"

    PACKET='packet update
	origin 1
	aspath 1
	nexthop 20.20.20.20 
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24'

    reset
    coord peer1 establish AS $AS holdtime 0 id 192.150.187.100

    coord peer1 assert established
    coord peer1 expect packet notify $UPDATEMSGERR $MISSWATTR 5
    coord peer1 send $PACKET

    sleep 2
    coord peer1 assert queue 1
    coord peer1 assert established
}

test10()
{
    echo "TEST10:"
    echo "	1) Send an update packet as an EBGP peer with local pref"
    echo "	2) This is not an error. Our BGP emits a warning"

    PACKET="packet update
	origin 1
	aspath $PEER2_AS
	nexthop 20.20.20.20
	localpref 100
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 assert established
#we dont expect an error - the line below is to trap one if it does occur
    coord peer1 expect packet notify $UPDATEMSGERR $MISSWATTR
    coord peer1 send $PACKET

    sleep 2
    coord peer1 assert queue 1
    coord peer1 assert established
}

test11()
{
    echo "TEST11:"
    echo "	1) Send an update packet with two NLRIs"
    echo "	2) Then send an update packet to withdraw both NLRIs"

    PACKET_1="packet update
	origin 1
	aspath $PEER2_AS
	nexthop 20.20.20.20
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    PACKET_2='packet update
	withdraw 10.10.10.0/24
	withdraw 20.20.20.20/24'

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 assert established
    coord peer1 expect packet notify $UPDATEMSGERR $MISSWATTR
    coord peer1 send $PACKET_1

    sleep 2
    coord peer1 assert queue 1
    coord peer1 assert established

    sleep 2
    coord peer1 assert established
    coord peer1 expect packet notify $UPDATEMSGERR $MISSWATTR
    coord peer1 send $PACKET_2

    sleep 2
    coord peer1 assert queue 2
    coord peer1 assert established
}

test12()
{
    echo "TEST12 - Send an update packet on an IBGP peer twice"
    PACKET='packet update
	origin 1
	aspath 1
	localpref 2
	nexthop 20.20.20.20 
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24'

    reset
    coord peer1 establish AS $AS holdtime 0 id 192.150.187.100

    coord peer1 assert established
    coord peer1 expect packet notify $UPDATEMSGERR $MISSWATTR
    coord peer1 send $PACKET
    coord peer1 send $PACKET

    sleep 2
    coord peer1 assert queue 1
    coord peer1 assert established
}

test12_ipv6()
{
    echo "TEST12 IPV6 - Send an update packet on an IBGP peer twice"
    PACKET="packet update
	origin 1
	aspath $PEER4_AS
	localpref 2
	nexthop6 20:20:20:20:20:20:20:20
	nlri6 1000::/3
	nlri6 2000::/3"

    coord reset
    coord target $HOST $PORT4
    coord initialise attach peer1

    coord peer1 establish AS $PEER4_AS holdtime 0 id 192.150.187.100 ipv6 true

    coord peer1 assert established
    coord peer1 expect packet notify $UPDATEMSGERR $MISSWATTR
    coord peer1 send $PACKET
    coord peer1 send $PACKET

    sleep 2
    coord peer1 assert queue 1
    coord peer1 assert established
}

test13()
{
    echo "TEST13:"
    echo "	1) Send an update packet on an IBGP peer with no local pref twice"
    echo "	2) This is not an error. Our BGP emits a warnings"

    PACKET='packet update
	origin 1
	aspath 1
	nexthop 20.20.20.20 
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24'

    reset
    coord peer1 establish AS $AS holdtime 0 id 192.150.187.100

    coord peer1 assert established
    coord peer1 expect packet notify $UPDATEMSGERR $MISSWATTR
    coord peer1 send $PACKET
    coord peer1 send $PACKET

    sleep 2
    coord peer1 assert queue 1
    coord peer1 assert established
}

test14()
{
    echo "TEST14 - Send an update packet on an EBGP peer twice"
    PACKET="packet update
	origin 1
	aspath $PEER2_AS
	nexthop 20.20.20.20 
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 assert established
    coord peer1 expect packet notify $UPDATEMSGERR $MISSWATTR
    coord peer1 send $PACKET
    coord peer1 send $PACKET

    sleep 2
    coord peer1 assert queue 1
    coord peer1 assert established
}

test15()
{
    echo "TEST15 - Establish a connection drop the peering and establish again"

    for i in 1 2
    do
	coord reset
	coord target $HOST $PORT2
	coord initialise attach peer1

	coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

	coord peer1 assert established
    done
}

test16()
{
    echo "TEST16 - Send two update packets on an EBGP peer with local pref set"
    # Local preference should not be set on an update packet from an EBGP peer.
    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 assert established

    PACKET_1="packet update
	origin 1
	aspath $PEER2_AS
	nexthop 20.20.20.20
	localpref 100
	nlri 10.10.10.0/16"

    PACKET_2="packet update
	origin 1
	aspath $PEER2_AS,2
	nexthop 20.20.20.20
	localpref 100
	nlri 30.30.30.0/24"

    coord peer1 send $PACKET_1
    coord peer1 send $PACKET_2

    # Make sure that the session still exists.
    sleep 2
    coord peer1 assert established

    # Reset the connection
    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1
}

test17()
{
    echo "TEST17 - Send an update packet with a repeated NLRI."
    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100
    
    coord peer1 assert established

    PACKET="packet update
	origin 1
	aspath $PEER2_AS
	nexthop 20.20.20.20
	nlri 30.30.30.0/24
	nlri 30.30.30.0/24"

    coord peer1 send $PACKET

    # Make sure that the session still exists.
    sleep 2
    coord peer1 assert established

    # Reset the connection
    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1
}

test18()
{
    echo "TEST18 - Send two update packets on an EBGP peer."
    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 assert established

    PACKET_1="packet update
	origin 1
	aspath $PEER2_AS
	nexthop 20.20.20.20
	nlri 10.10.10.0/16"

    PACKET_2="packet update
	origin 1
	aspath $PEER2_AS,2
	nexthop 20.20.20.20
	nlri 30.30.30.0/24"

    coord peer1 send $PACKET_1
    coord peer1 send $PACKET_2

    # Make sure that the session still exists.
    sleep 2
    coord peer1 assert established

    # Reset the connection
    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    # Make a new connection.
    sleep 2
    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 assert established
}

test19()
{
    echo "TEST19 - EBGP peer update packet then different withdraw"
    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 assert established

    PACKET_1='packet update
	origin 1
	aspath 1
	nexthop 20.20.20.20
	nlri 10.10.10.0/16'

    PACKET_2='packet update withdraw 30.30.30.0/24'

    #coord peer1 send $PACKET_1
    coord peer1 send $PACKET_2

    # Make sure that the session still exists.
    sleep 2
    coord peer1 assert established

    # Reset the connection
    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1
    sleep 2

    # Make a new connection.
    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100
    sleep 2
    coord peer1 assert established
}

test20()
{
    echo "TEST20 - EBGP peer single withdraw"
    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 assert established

    PACKET='packet update withdraw 30.30.30.0/24'

    coord peer1 send $PACKET

    # Make sure that the session still exists.
    sleep 2
    coord peer1 assert established

    # Reset the connection
    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1
    sleep 2

    # Make a new connection.
    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100
    sleep 2
    coord peer1 assert established
}

test20_ipv6()
{
    echo "TEST20 IPV6 - EBGP peer single withdraw"
    coord reset
    coord target $HOST $PORT4
    coord initialise attach peer1

    coord peer1 establish AS $PEER4_AS holdtime 0 id 192.150.187.100 ipv6 true

    coord peer1 assert established

    PACKET='packet update withdraw6 2000::/3'

    coord peer1 send $PACKET

    # Make sure that the session still exists.
    sleep 2
    coord peer1 assert established

    # Reset the connection
    coord reset
    coord target $HOST $PORT4
    coord initialise attach peer1
    sleep 2

    # Make a new connection.
    coord peer1 establish AS $PEER4_AS holdtime 0 id 192.150.187.100 ipv6 true

    sleep 2
    coord peer1 assert established
}

test21()
{
    echo "TEST21 - EBGP peer update packet then corresponding withdraw"
    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 assert established

    PACKET_1="packet update
	origin 1
	aspath $PEER2_AS
	nexthop 20.20.20.20
	nlri 10.10.10.0/16"

    PACKET_2='packet update withdraw 10.10.10.0/16'

    coord peer1 send $PACKET_1
    coord peer1 send $PACKET_2

    # Make sure that the session still exists.
    sleep 2
    coord peer1 assert established

    # Reset the connection
    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1
    sleep 2

    # Make a new connection.
    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100
    sleep 2
    coord peer1 assert established
}

test22()
{
    echo "TEST22 - EBGP Establish an IPV6 peering"

    coord reset
    coord target $HOST $PORT3
    coord initialise attach peer1

    coord peer1 establish AS $PEER3_AS holdtime 0 id 192.150.187.100 ipv6 true

    coord peer1 assert established
}

test23()
{
    echo "TEST23"
    echo "	1) Establish an IBGP IPV6 peering"
    echo "	2) Send an IPv6 only update packet"

    coord reset
    coord target $HOST $PORT3
    coord initialise attach peer1

    coord peer1 establish AS $PEER3_AS holdtime 0 id 192.150.187.100 ipv6 true

    PACKET="packet update
	origin 1
	aspath 1,2,[3,4,5],6,[7,8],9
	nexthop6 20:20:20:20:20:20:20:20
	nlri6 2000::/3
        localpref 100"
    
    coord peer1 send $PACKET

    coord peer1 assert established
}

test24()
{
    echo "TEST24"
    echo "	1) Establish an EBGP IPV6 peering"
    echo "	2) Send an IPv6 only update packet"

    coord reset
    coord target $HOST $PORT4
    coord initialise attach peer1

    coord peer1 establish AS $PEER4_AS holdtime 0 id 192.150.187.100 ipv6 true

    PACKET="packet update
	origin 1
	aspath $PEER4_AS
	nexthop6 20:20:20:20:20:20:20:20
	nlri6 2000::/3"
    
    coord peer1 send $PACKET

    coord peer1 assert established
}

test25()
{
    echo "TEST25"
    echo "	1) Establish an EBGP IPV6 peering"
    echo "	2) Send an IPv4 and IPv6 update packet"

    coord reset
    coord target $HOST $PORT4
    coord initialise attach peer1

    coord peer1 establish AS $PEER4_AS holdtime 0 id 192.150.187.100 ipv6 true

    PACKET="packet update
	origin 1
	aspath $PEER4_AS
	nexthop 20.20.20.20
	nlri 10.10.10.0/24
	nexthop6 20:20:20:20:20:20:20:20
	nlri6 2000::/3"
    
    coord peer1 send $PACKET

    coord peer1 assert established
}

test26()
{
    echo "TEST26"
    echo "	1) Establish an EBGP IPV6 peering"
    echo "	2) Send an IPv4 and IPv6 update packet repeat both nexthops"

    coord reset
    coord target $HOST $PORT4
    coord initialise attach peer1

    coord peer1 establish AS $PEER4_AS holdtime 0 id 192.150.187.100 ipv6 true

    PACKET="packet update
	origin 1
	aspath $PEER4_AS
	nexthop 20.20.20.20
	nexthop 20.20.20.20
	nlri 10.10.10.0/24
	nexthop6 20:20:20:20:20:20:20:20
	nexthop6 20:20:20:20:20:20:20:20
	nlri6 2000::/3"
    
    coord peer1 expect packet notify $UPDATEMSGERR $MALATTRLIST
	
    coord peer1 send $PACKET

    sleep 2
    coord peer1 assert queue 0

    coord peer1 assert idle
}

test27()
{
    echo "TEST27 - Verify that routes originated by BGP reach an IBGP peer"

    coord reset
    coord target $HOST $PORT1
    coord initialise attach peer1

    # Introduce a route
    originate_route4 10.10.10.0/24 20.20.20.20 true false

    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.100

    sleep 2
    coord peer1 trie recv lookup 10.10.10.0/24 aspath empty

    coord peer1 assert queue 0
    coord peer1 assert established
}

test27_ipv6()
{
    echo "TEST27 (IPV6) - Verify that routes originated by BGP reach an IBGP peer"

    coord reset
    coord target $HOST $PORT3
    coord initialise attach peer1

    # Introduce a route
    originate_route6 2000::/3 20:20:20:20:20:20:20:20 true false

    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.100 ipv6 true

    sleep 2
    coord peer1 trie recv lookup 2000::/3 aspath empty

    coord peer1 assert queue 0
    coord peer1 assert established
}

test28()
{
    echo "TEST28 - Verify that routes originated by BGP reach an EBGP peer"
    echo "Also verify that the nexthop is rewritten and the provided nexthop"
    echo "is ignored"

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 expect packet open asnum $AS bgpid $ID holdtime $HOLDTIME \
	afi 1 safi 1

    coord peer1 expect packet keepalive

    coord peer1 expect packet update \
	origin 0 \
	nexthop 192.150.187.78 \
	aspath 65008 \
	med 1 \
	nlri 10.10.10.0/24

    # Introduce a route
    originate_route4 10.10.10.0/24 20.20.20.20 true false

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    sleep 2
    coord peer1 trie recv lookup 10.10.10.0/24 aspath 65008

    coord peer1 assert queue 0
    coord peer1 assert established
}

test28_ipv6()
{
    echo "TEST28 (IPV6) - Verify that routes originated by BGP reach an EBGP peer"
    echo "Also verify that the nexthop is rewritten and the provided nexthop"
    echo "is ignored"

    coord reset
    coord target $HOST $PORT4
    coord initialise attach peer1

    coord peer1 expect packet open asnum $AS bgpid $ID holdtime $HOLDTIME \
	afi 2 safi 1

    coord peer1 expect packet keepalive

    coord peer1 expect packet update \
	origin 0 \
	aspath 65008 \
	med 1 \
	nlri6 2000::/3 \
	nexthop6 ::

    # Introduce a route
    originate_route6 2000::/3 20:20:20:20:20:20:20:20 true false

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100 ipv6 true

    sleep 2
    coord peer1 trie recv lookup 2000::/3 aspath 65008

    coord peer1 assert queue 0
    coord peer1 assert established
}

test29()
{
    echo "TEST29"
    echo "	1) Make a connection"
    echo "	2) Send a notify packet before establishing a session"
    echo "	3) Verify that we do not get a notify packet in response"
    echo "	4) Verify that the TCP session has been torn down"

    reset

    coord peer1 connect

    # Verify that we have a connection
    sleep 2
    coord peer1 assert connected

    sleep 2
    # We have to add something to the queue in order to make the
    # assertion later
    coord peer1 expect packet notify $FSMERROR
    coord peer1 send packet notify $UPDATEMSGERR $MALATTRLIST

    sleep 2
    coord peer1 assert queue 1

    sleep 2
    coord peer1 assert idle
}

test30()
{
    echo "TEST30 - Bugzilla BUG #32"
    echo "	1) The test peer performs a bind but no listen"
    echo "	2) A BGP connection attempt will leave it in connect state"
    echo "	3) Toggle the peering to ensure BGP can get out of this state"

    coord reset
    BASE=peer1 bind $HOST $PEER1_PORT

    # Toggle the BGP peering to force a connection attempt.
    disable_peer $LOCALHOST $PORT1 $PEER1 $PEER1_PORT $PEER1_AS
    enable_peer $LOCALHOST $PORT1 $PEER1 $PEER1_PORT $PEER1_AS

    # The BGP should be in the CONNECT state now.

    # Toggle the BGP peering again to verify that it can get out of the
    # CONNECT state.
    disable_peer $LOCALHOST $PORT1 $PEER1 $PEER1_PORT $PEER1_AS
    enable_peer $LOCALHOST $PORT1 $PEER1 $PEER1_PORT $PEER1_AS

    # Need to find out that the BGP process still exists so make a peering.
    BASE=peer1 ${srcdir}/xrl_shell_funcs.sh reset
    sleep 2

    coord reset
    coord target $HOST $PORT1
    coord initialise attach peer1

    coord peer1 establish \
	active true \
	AS $PEER1_AS \
	holdtime $HOLDTIME \
	id 192.150.187.100

    sleep 2
    coord peer1 assert established
}

test31()
{
    echo "TEST31 - On an I-BGP peering send an update with an empty aspath"

    PACKET='packet update
	origin 1
	aspath empty
	nexthop 20.20.20.20 
	nlri 10.10.10.0/24'

    reset
    coord peer1 establish AS $AS holdtime 0 id 192.150.187.100

    coord peer1 assert established
    coord peer1 send $PACKET

    sleep 2
    coord peer1 assert established
}

test32()
{
    echo "TEST32 - Bugzilla BUG #139"
    echo "	1) Originate a route on an I-BGP peering with an empty aspath."
    echo "	2) Send the same route to the BGP process."
    echo "	3) The comparison of two routes with empty aspath caused BGP"
    echo "	   BGP to fail"

    coord reset
    coord target $HOST $PORT1
    coord initialise attach peer1

    # Introduce a route
    originate_route4 10.10.10.0/24 20.20.20.20 true false

    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.100

    sleep 2
    coord peer1 trie recv lookup 10.10.10.0/24 aspath empty

    PACKET='packet update
	origin 2
	aspath empty
	nexthop 20.20.20.20 
	nlri 10.10.10.0/24'

    coord peer1 send $PACKET

    sleep 2

    coord peer1 assert queue 0
    coord peer1 assert established
}

test33()
{
    echo "TEST33 - DampPeerOscillations 10 sessions 11th should fail"

    for i in 1 2 3 4 5 6 7 8 9 10
    do
	coord reset
	coord target $HOST $PORT2
	coord initialise attach peer1

	coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

	sleep 2

	coord peer1 assert established
    done

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    sleep 2

    coord peer1 assert idle
}

test34()
{
    echo "TEST34 - Illicit a Message Header Error Connection Not Synchronized."
    echo "	1) Establish a connection"
    echo "	2) Send a keepalive with a corrupted marker field"
    echo "	3) Should return a notify Connection Not Synchronized."

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $MSGHEADERERR $CONNNOTSYNC

    coord peer1 send packet corrupt 0 0 keepalive

    sleep 2

    coord peer1 assert queue 0

    sleep 2
    
    coord peer1 assert idle
}

test35()
{
    echo "TEST35 - Illicit a Message Header Error Bad Message Length. 0"
    echo "	1) Establish a connection"
    echo "	2) Send a keepalive with a short message length"
    echo "	3) Should return a notify Bad Message Length.."

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $MSGHEADERERR $BADMESSLEN 0 0

    coord peer1 send packet corrupt 17 0 keepalive

    sleep 2

    coord peer1 assert queue 0

    sleep 2
    
    coord peer1 assert idle
}

test36()
{
    echo "TEST36 - Illicit a Message Header Error Bad Message Length. 20"
    echo "	1) Establish a connection"
    echo "	2) Send a keepalive with a long message length"
    echo "	3) Should return a notify Bad Message Length."

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $MSGHEADERERR $BADMESSLEN 0 20

    coord peer1 send packet corrupt 17 20 keepalive

    # send a second message to force the previous one to be delivered.
    coord peer1 send packet keepalive

    sleep 2

    coord peer1 assert queue 0

    sleep 2
    
    coord peer1 assert idle
}

test37()
{
    echo "TEST37 - Illicit a Message Header Error Bad Message Type."
    echo "	1) Establish a connection"
    echo "	2) Send a message with the type set to 10"
    echo "	3) Should return a notify Bad Message Type with the 10"

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $MSGHEADERERR $BADMESSTYPE 10

    coord peer1 send packet corrupt 18 10 keepalive

    sleep 2

    coord peer1 assert queue 0

    sleep 2
    
    coord peer1 assert idle
}

test38()
{
    echo "TEST38 - Illicit a UPDATE Message Error Malformed AS_PATH."
    echo "	1) Establish a connection"
    echo "	2) Send a malformed aspath"
    echo "	3) Should return a notify UPDATE Message Error."

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $UPDATEMSGERR $MALASPATH

    # 
    # Hit the check in AsPath::decode()
    # 
    coord peer1 send packet corrupt 38 0 \
	update \
	origin 2 \
	aspath "[$PEER2_AS],[$PEER2_AS]" \
	nlri 10.10.10.0/24 \
	nexthop 20.20.20.20

    sleep 2

    coord peer1 assert queue 0

    sleep 2
    
    coord peer1 assert idle
}

test39()
{
    echo "TEST39 - Send a short open packet. 20"
    echo "	1) Send an open packet with a length field of 20"
    echo "	2) Should return a notify Bad Message Length."
    reset

    coord peer1 connect

    sleep 2

    coord peer1 expect packet notify $MSGHEADERERR $BADMESSLEN 0 20

    coord peer1 send packet corrupt 17 20 \
	open \
	asnum $PEER1_AS \
	bgpid 192.150.187.100 \
	holdtime 0

    sleep 2

    coord peer1 assert queue 0
}    

test40()
{
    echo "TEST40 - Open message with illegal version number"
    echo "	1) Send an open message with a version number of 6"
    echo "	2) Should get a notify with unsupported version number 4"
    reset

    coord peer1 connect

    sleep 2

    coord peer1 expect packet notify $OPENMSGERROR $UNSUPVERNUM 0 4

    coord peer1 send packet corrupt 19 6 \
	open \
	asnum $PEER1_AS \
	bgpid 192.150.187.100 \
	holdtime 0

    sleep 2

    coord peer1 assert queue 0
}    

test41()
{
    echo "TEST41 - Open message with an illegal BGP ID"
    echo "	1) Send an open message with a BGP ID of 0.0.0.0"
    echo "	2) Should get a notify with Bad BGP Identifier"
    reset

    coord peer1 connect

    sleep 2

    coord peer1 expect packet notify $OPENMSGERROR $BADBGPIDENT

    coord peer1 send packet \
	open \
	asnum $PEER1_AS \
	bgpid 0.0.0.0 \
	holdtime 0

    sleep 2

    coord peer1 assert queue 0
}    

test42()
{
    echo "TEST42 - Send an update message that has a short length field."
    echo "	1) Establish a connection"
    echo "	2) Send an update with a short length 22"
    echo "	3) Should return a notify Bad Message Length"

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $MSGHEADERERR $BADMESSLEN 0 22

    coord peer1 send packet corrupt 17 22 \
	update \
	origin 2 \
	aspath "[$PEER2_AS],[$PEER2_AS]" \
	nlri 10.10.10.0/24 \
	nexthop 20.20.20.20

    sleep 2

    coord peer1 assert queue 0

    sleep 2
    
    coord peer1 assert idle
}

test43()
{
    echo "TEST43 - Send an notify message that has a short length field."
    echo "	1) Establish a connection"
    echo "	2) Send a notify with a short length 20"
    echo "	3) Should return a notify Bad Message Length"

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $MSGHEADERERR $BADMESSLEN 0 20

    coord peer1 send packet corrupt 17 20 notify $CEASE

    sleep 2

    coord peer1 assert queue 0

    sleep 2
    
    coord peer1 assert idle
}

test44()
{
    echo "TEST44 - Send an update message with an origin with bad flags."
    echo "	1) Establish a connection"
    echo "	2) Send an update with an origin with bad flags"
    echo "	3) Should return an UPDATE Message Error Attribute"

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $UPDATEMSGERR $ATTRFLAGS 0 1 1 1

    coord peer1 send packet \
	update \
	pathattr 0,1,1,1 \
	aspath "[$PEER2_AS],[$PEER2_AS]" \
	nexthop 20.20.20.20 \
	nlri 10.10.10.0/24

    sleep 2

    coord peer1 assert queue 0

    sleep 2
    
    coord peer1 assert idle
}

test45()
{
    echo "TEST45 - Send an update message with an aspath with bad flags."
    echo "	1) Establish a connection"
    echo "	2) Send an update with an aspath with bad flags"
    echo "	3) Should return an UPDATE Message Error Attribute"

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $UPDATEMSGERR $ATTRFLAGS 0 2 4 2 1 253 232

    # 0 - Attribute Flags (BAD)
    # 2 - Attribute Type Code (AS_PATH)
    # 4 - Attribute Length
    # 2 - Path Segment Type (AS_SEQUENCE)
    # 1 - Path Segment Length (One AS)
    # 253 - Path Segment Value (65000 / 256)
    # 232 - Path Segment Value (65000 % 256)

    coord peer1 send packet \
	update \
	origin 2 \
	pathattr 0,2,4,2,1,253,232 \
	nexthop 20.20.20.20 \
	nlri 10.10.10.0/24


    sleep 2

    coord peer1 assert queue 0

    sleep 2
    
    coord peer1 assert idle
}

test46()
{
    echo "TEST46 - Send an update message with a nexthop with bad flags."
    echo "	1) Establish a connection"
    echo "	2) Send an update with a nexthop with bad flags"
    echo "	3) Should return an UPDATE Message Error Attribute"

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $UPDATEMSGERR $ATTRFLAGS 0 3 4 20 20 20 20

    # 0 - Attribute Flags (BAD)
    # 3 - Attribute Type Code (NEXT_HOP)
    # 4 - Attribute Length
    # 20 - Value
    # 20 - Value
    # 20 - Value
    # 20 - Value

    coord peer1 send packet \
	update \
	origin 2 \
	pathattr 0,3,4,20,20,20,20 \
	nlri 10.10.10.0/24

    sleep 2

    coord peer1 assert queue 0

    sleep 2
    
    coord peer1 assert idle
}

test47()
{
    echo "TEST47 - Send an update message with a med with bad flags."
    echo "	1) Establish a connection"
    echo "	2) Send an update with a med with bad flags"
    echo "	3) Should return an UPDATE Message Error Attribute"

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $UPDATEMSGERR $ATTRFLAGS 0 4 4 20 20 20 20

    # 0 - Attribute Flags (BAD)
    # 4 - Attribute Type Code (MULTI_EXIT_DISC)
    # 4 - Attribute Length
    # 20 - Value
    # 20 - Value
    # 20 - Value
    # 20 - Value

    coord peer1 send packet \
	update \
	origin 2 \
	aspath "[$PEER2_AS],[$PEER2_AS]" \
	pathattr 0,4,4,20,20,20,20 \
	nlri 10.10.10.0/24

    sleep 2

    coord peer1 assert queue 0

    sleep 2
    
    coord peer1 assert idle
}

test48()
{
    echo "TEST48 - Send an update message with a LOCAL_PREF with bad flags."
    echo "	1) Establish a connection"
    echo "	2) Send an update with a LOCAL_PREF with bad flags"
    echo "	3) Should return an UPDATE Message Error Attribute"

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $UPDATEMSGERR $ATTRFLAGS 0 5 4 20 20 20 20

    # 0 - Attribute Flags (BAD)
    # 5 - Attribute Type Code (LOCAL_PREF)
    # 4 - Attribute Length
    # 20 - Value
    # 20 - Value
    # 20 - Value
    # 20 - Value

    coord peer1 send packet \
	update \
	origin 2 \
	aspath "[$PEER2_AS],[$PEER2_AS]" \
	pathattr 0,5,4,20,20,20,20 \
	nlri 10.10.10.0/24

    sleep 2

    coord peer1 assert queue 0

    sleep 2
    
    coord peer1 assert idle
}

test49()
{
    echo "TEST49 - An update message with an ATOMIC_AGGREGATE with bad flags."
    echo "	1) Establish a connection"
    echo "	2) Send an update with a ATOMIC_AGGREGATE with bad flags"
    echo "	3) Should return an UPDATE Message Error Attribute"

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $UPDATEMSGERR $ATTRFLAGS 0 6 0

    # 0 - Attribute Flags (BAD)
    # 6 - Attribute Type Code (ATOMIC_AGGREGATE)
    # 0 - Attribute Length

    coord peer1 send packet \
	update \
	origin 2 \
	aspath "[$PEER2_AS],[$PEER2_AS]" \
	pathattr 0,6,0 \
	nlri 10.10.10.0/24

    sleep 2

    coord peer1 assert queue 0

    sleep 2
    
    coord peer1 assert idle
}

test50()
{
    echo "TEST50 - An update message with an AGGREGATOR with bad flags."
    echo "	1) Establish a connection"
    echo "	2) Send an update with a AGGREGATOR with bad flags"
    echo "	3) Should return an UPDATE Message Error Attribute"

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $UPDATEMSGERR $ATTRFLAGS \
	0 7 6 1 1 20 20 20 20

    # 0 - Attribute Flags (BAD)
    # 7 - Attribute Type Code (ATOMIC_AGGREGATE)
    # 6 - Attribute Length
    # 1 - Last AS
    # 1 - Last AS
    # 20 - IP address
    # 20 - IP address
    # 20 - IP address
    # 20 - IP address
    
    coord peer1 send packet \
	update \
	origin 2 \
	aspath "[$PEER2_AS],[$PEER2_AS]" \
	pathattr 0,7,6,1,1,20,20,20,20 \
	nlri 10.10.10.0/24

    sleep 2

    coord peer1 assert queue 0

    sleep 2
    
    coord peer1 assert idle
}

test51()
{
    echo "TEST51 - An update message with an origin with an illegal value."
    echo "	1) Establish a connection"
    echo "	2) Send an update with an origin with an illegal value"
    echo "	3) Should return an UPDATE Message Error Attribute"

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $UPDATEMSGERR $INVALORGATTR 64 1 1 5

    # 0x40 - Attribute Flags
    # 1 - Attribute Type Code (ORIGIN)
    # 1 - Attribute Length
    # 5 - Attribute value (ILLEGAL)

    coord peer1 send packet \
	update \
	pathattr 64,1,1,5 \
	aspath "[$PEER2_AS],[$PEER2_AS]" \
	nexthop 20.20.20.20 \
	nlri 10.10.10.0/24

    sleep 2

    coord peer1 assert queue 0

    sleep 2
    
    coord peer1 assert idle
}

test52()
{
    echo "TEST52 - An update message with an invalid nexthop"
    echo "	1) Establish a connection"
    echo "	2) Send an update with a nexthop with an illegal value"
    echo "	3) Should return an UPDATE Message Error Attribute"

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $UPDATEMSGERR $INVALNHATTR \
	64 3 4 255 255 255 255

    # 64 - Attribute Flags
    # 3 - Attribute Type Code (NEXT_HOP)
    # 4 - Attribute Length
    # 255 - Value
    # 255 - Value
    # 255 - Value
    # 255 - Value

    coord peer1 send packet \
	update \
	origin 2 \
	pathattr 64,3,4,255,255,255,255 \
	nlri 10.10.10.0/24

    sleep 2

    coord peer1 assert queue 0

    sleep 2
    
    coord peer1 assert idle
}

test53()
{
    echo "TEST53 - A NLRI that is syntactically incorrect"
    echo "	1) Establish a connection"
    echo "	2) Should return an UPDATE Message Error Attribute"

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $UPDATEMSGERR $INVALNETFIELD

    # Make the number of bits 54
    coord peer1 send packet corrupt 45 54\
	update \
	origin 2 \
	aspath "[$PEER2_AS],[$PEER2_AS]" \
	nexthop 20.20.20.20 \
	nlri 10.10.10.0/24

    sleep 2

    coord peer1 assert queue 0

    sleep 2
    
    coord peer1 assert idle
}

test54()
{
    echo "TEST54 - A mandatory unknown path attribute."
    echo "	1) Establish a connection"
    echo "	2) Should get an Unrecognized Well-known Attribute"

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $UPDATEMSGERR $UNRECOGWATTR 0 255 1 0

    coord peer1 send packet \
	update \
	origin 2 \
	aspath "[$PEER2_AS],[$PEER2_AS]" \
	nexthop 20.20.20.20 \
	nlri 10.10.10.0/24 \
	pathattr 0,255,1,0

    sleep 2

    coord peer1 assert queue 0

    sleep 2
    
    coord peer1 assert idle
}

test55()
{
    echo "TEST55 - Send an AS_PATH with confederation segments."
    echo "	1) Establish a connection"
    echo "	2) Should get a Malformed AS_PATH"

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $UPDATEMSGERR $MALASPATH

    coord peer1 send packet \
	update \
	origin 2 \
	aspath "[$PEER2_AS],($PEER2_AS)" \
	nexthop 20.20.20.20 \
	nlri 10.10.10.0/24

    sleep 2

    coord peer1 assert queue 0

    sleep 2
    
    coord peer1 assert idle
}

test56()
{
    echo "TEST56 - Routes originated by BGP sent to an I-BGP peer:"
    echo "	1) Should have a local pref added."
    echo "	2) Should have an empty aspath."
    echo "	3) Should honour the next hop?"
    

    coord reset
    coord target $HOST $PORT1
    coord initialise attach peer1

    coord peer1 expect packet open asnum $AS bgpid $ID holdtime $HOLDTIME \
	afi 1 safi 1

    coord peer1 expect packet keepalive

    coord peer1 expect packet update \
	origin 0 \
	nexthop 20.20.20.20 \
	aspath empty \
	localpref 100 \
	nlri 10.10.10.0/24

    # Introduce a route
    originate_route4 10.10.10.0/24 20.20.20.20 true false

    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.100

    sleep 2
    coord peer1 trie recv lookup 10.10.10.0/24 aspath empty

    coord peer1 assert queue 0
    coord peer1 assert established
}

test56_ipv6()
{
    echo "TEST56 (IPv6) - Routes originated by BGP sent to an I-BGP peer:"
    echo "	1) Should have a local pref added."
    echo "	2) Should have an empty aspath."
    echo "	3) Should honour the next hop?"
    
    coord reset
    coord target $HOST $PORT3
    coord initialise attach peer1

    coord peer1 expect packet open asnum $AS bgpid $ID holdtime $HOLDTIME \
	afi 2 safi 1

    coord peer1 expect packet keepalive

    coord peer1 expect packet update \
	origin 0 \
	nexthop6 20:20:20:20:20:20:20:20 \
	aspath empty \
	localpref 100 \
	nlri6 2000::/3

    # Introduce a route
    originate_route6 2000::/3 20:20:20:20:20:20:20:20 true false

    coord peer1 establish AS $PEER3_AS holdtime 0 id 192.150.187.100 ipv6 true

    sleep 2
    coord peer1 trie recv lookup 2000::/3 aspath empty

    coord peer1 assert queue 0
    coord peer1 assert established
}

test57()
{
    echo "TEST57 - Send an ORIGINATOR_ID on an E-BGP peering."
    echo "	1) Establish a connection"
    echo "	2) Should get a Malformed AS_PATH"

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $UPDATEMSGERR $MALATTRLIST

    coord peer1 send packet \
	update \
	origin 2 \
	aspath "[$PEER2_AS],[$PEER2_AS]" \
	nexthop 20.20.20.20 \
	nlri 10.10.10.0/24 \
	originatorid 1.2.3.4

    sleep 2

    coord peer1 assert queue 0

    sleep 2
    
    coord peer1 assert idle
}

test58()
{
    echo "TEST58 - Send CLUSTER_LIST on an E-BGP peering."
    echo "	1) Establish a connection"
    echo "	2) Should get a Malformed AS_PATH"

    coord reset
    coord target $HOST $PORT2
    coord initialise attach peer1

    coord peer1 establish AS $PEER2_AS holdtime 0 id 192.150.187.100

    coord peer1 expect packet notify $UPDATEMSGERR $MALATTRLIST

    coord peer1 send packet \
	update \
	origin 2 \
	aspath "[$PEER2_AS],[$PEER2_AS]" \
	nexthop 20.20.20.20 \
	nlri 10.10.10.0/24 \
	clusterlist 1.2.3.4

    sleep 2

    coord peer1 assert queue 0

    sleep 2
    
    coord peer1 assert idle
}

TESTS_NOT_FIXED=''
TESTS='test1 test2 test3 test4 test5 test6 test7 test8 test8_ipv6
    test9 test10 test11 test12 test12_ipv6 test13 test14 test15 test16
    test17 test18 test19 test20 test20_ipv6 test21 test22 test23 test24
    test25 test26 test27 test27_ipv6 test28 test28_ipv6 test29 test30 test31
    test32 test33 test34 test35 test36 test37 test38 test39 test40 test41
    test42 test43 test44 test45 test46 test47 test48 test49 test50 test51
    test52 test53 test54 test55 test56 test56_ipv6 test57 test58'

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
