#!/usr/bin/env bash

#
# $XORP: xorp/bgp/harness/test_peering1.sh,v 1.10 2003/09/03 00:49:07 atanu Exp $
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
LOCALHOST=$HOST
ID=192.150.187.78
AS=65008

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
    local_config $AS $ID

    # Don't try and talk to the rib.
    register_rib ""

    # IBGP - IPV4
    PEER=$PEER1
    NEXT_HOP=192.150.187.78
    add_peer $LOCALHOST $PORT1 $PEER $PEER1_PORT $PEER1_AS $NEXT_HOP $HOLDTIME
    enable_peer $LOCALHOST $PORT1 $PEER $PEER1_PORT

    # EBGP - IPV4
    PEER=$PEER2
    NEXT_HOP=192.150.187.78
    add_peer $LOCALHOST $PORT2 $PEER $PEER2_PORT $PEER2_AS $NEXT_HOP $HOLDTIME
    enable_peer $LOCALHOST $PORT2 $PEER $PEER2_PORT

    # IBGP - IPV6
    PEER=$PEER3
    NEXT_HOP=192.150.187.78
    add_peer $LOCALHOST $PORT3 $PEER $PEER3_PORT $PEER3_AS $NEXT_HOP $HOLDTIME
    set_parameter $LOCALHOST $PORT3 $PEER $PEER3_PORT MultiProtocolIPv6
    enable_peer $LOCALHOST $PORT3 $PEER $PEER3_PORT

    # IBGP - IPV6
    PEER=$PEER4
    NEXT_HOP=192.150.187.78
    add_peer $LOCALHOST $PORT4 $PEER $PEER4_PORT $PEER4_AS $NEXT_HOP $HOLDTIME
    set_parameter $LOCALHOST $PORT4 $PEER $PEER4_PORT MultiProtocolIPv6
    enable_peer $LOCALHOST $PORT4 $PEER $PEER4_PORT
}

HOLD_TIMER=4
FSM_ERROR=5
OPEN_ERROR=2
UNACCEPTHOLDTIME=6

UPDATEMSGERR=3		# Update error
MISSWATTR=3		# Missing Well-known Attribute

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

    coord peer1 expect packet open asnum $AS bgpid $ID holdtime $HOLDTIME
    coord peer1 expect packet keepalive
    coord peer1 expect packet keepalive
    coord peer1 expect packet keepalive
    coord peer1 expect packet keepalive
    coord peer1 expect packet notify $HOLD_TIMER
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
    echo "TEST2 - Make a connection and send an update packet"

    reset

    coord peer1 connect

    sleep 2
    coord peer1 expect packet notify $FSM_ERROR
    coord peer1 send packet update

    sleep 2
    coord peer1 assert queue 0
}

test3()
{
    echo "TEST3 - Try and send an illegal hold time of 1"

    reset

    coord peer1 expect packet open asnum $AS bgpid $ID holdtime $HOLDTIME
    coord peer1 expect packet notify $OPEN_ERROR $UNACCEPTHOLDTIME
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
	aspath 1,2,(3,4,5),6,(7,8),9
	nexthop 20.20.20.20 
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24'

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

    echo "Sleeping $HOLDTIME"
    sleep $HOLDTIME

    coord peer1 expect packet notify $HOLD_TIMER
}

test6()
{
    echo "TEST6 - Send an update packet without an origin"
    PACKET='packet update
	aspath 1,2,(3,4,5),6,(7,8),9
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

test9()
{
    echo "TEST9 - Send an update packet as an IBGP peer with no local pref"
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
    echo "TEST10 - Send an update packet as an EBGP peer with local pref"
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
    echo "TEST11 - Send an update packet with only withdrawn routes"

    PACKET_1="packet update
	origin 1
	aspath $PEER2_AS
	nexthop 20.20.20.20
	localpref 100
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

test13()
{
    echo "TEST13 - Send an update packet on an IBGP peer with no local pref twice"
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
	localpref 100
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
    echo "TEST23 - EBGP Establish an IPV6 peering and send an update packet"

    coord reset
    coord target $HOST $PORT3
    coord initialise attach peer1

    coord peer1 establish AS $PEER3_AS holdtime 0 id 192.150.187.100 ipv6 true

    PACKET_1="packet update
	origin 1
	aspath $PEER3_AS
	nexthop6 20:20:20:20:20:20:20:20
	nlri6 2000::/3"
    
    coord peer1 send $PACKET_1

    coord peer1 assert established
}

TESTS_NOT_FIXED=''
TESTS='test1 test2 test3 test4 test5 test6 test7 test8 test9 test10 test11
    test12 test13 test14 test15 test16 test17 test18 test19 test20 test21
    test22 test23'

# Temporary fix to let TCP sockets created by call_xrl pass through TIME_WAIT
TIME_WAIT=`time_wait_seconds`

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

if [ $CONFIGURE = "yes" ]
then
    configure_bgp
fi

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
