#!/usr/bin/env bash

#
# $XORP: xorp/bgp/harness/test_path_attribute1.sh,v 1.9 2007/07/03 20:36:13 atanu Exp $
#

#
# Test various 
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
# 8) Run "./coord"
#
set -e
. ./setup_paths.sh

# srcdir is set by make for check target
if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi
. ${srcdir}/xrl_shell_funcs.sh ""
. $BGP_FUNCS ""

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
AS=2.65008
ASTRAN=23456
USE4BYTEAS=true
NEXT_HOP=192.150.187.78

# EBGP
PORT1=10001
PEER1_PORT=20001
PEER1_AS=65001

# EBGP
PORT2=10002
PEER2_PORT=20002
PEER2_AS=65002

HOLDTIME=5

configure_bgp()
{
    local_config $AS $ID $USE4BYTEAS

    # Don't try and talk to the rib.
    register_rib ""

    PEER=$HOST
    IPTUPLE="$LOCALHOST $PORT1 $PEER $PEER1_PORT"
    add_peer lo $IPTUPLE $PEER1_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv4.Unicast true
    enable_peer $IPTUPLE

    PEER=$HOST
    IPTUPLE="$LOCALHOST $PORT2 $PEER $PEER2_PORT"
    add_peer lo $IPTUPLE $PEER2_AS $NEXT_HOP $HOLDTIME
    set_parameter $IPTUPLE MultiProtocol.IPv4.Unicast true
    enable_peer $IPTUPLE
}

reset()
{
    coord reset

    coord target $HOST $PORT1
    coord initialise attach peer1

    coord target $HOST $PORT2
    coord initialise attach peer2
}

test1()
{
    echo "TEST1:"
    echo "	1) Send an update packet to BGP with a 4-byte ASnum and check that"
    echo "	   it forwards an ASpath including AS_TRAN to its peers."

    # Reset the peers
    reset

    # Establish a connection
    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.100
    coord peer1 assert established

    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.101
    coord peer2 assert established

    ASPATH="$PEER1_AS,1,2,[3,4,5],6,[7,8],9"
    ASTRANPATH="23456,$PEER1_AS,1,2,[3,4,5],6,[7,8],9"
    NEXTHOP="20.20.20.20"

    PACKET1="packet update
	origin 2
	aspath $ASPATH
	nexthop $NEXTHOP
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    PACKET2="packet update
	origin 2
	aspath $ASTRANPATH
	nexthop $NEXT_HOP
	med 1
	as4path $AS,$ASPATH
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    coord peer2 expect $PACKET2

    coord peer1 send $PACKET1

    sleep 2
    coord peer2 assert queue 0

    # Verify that the peers are still connected.
    coord peer1 assert established
    coord peer2 assert established
}

test2()
{
    echo "TEST2:"
    echo "	1) Send an update packet to BGP with a 4-byte ASnum and check that"
    echo "	   it forwards a 4byte ASpath to its peers."

    # Reset the peers
    reset

    # Establish a connection
    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.100
    coord peer1 assert established

    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.101 use_4byte_asnums true
    coord peer2 assert established

    ASPATH="$PEER1_AS,1,2,[3,4,5],6,[7,8],9"
    RECVASPATH="$AS,$PEER1_AS,1,2,[3,4,5],6,[7,8],9"
    NEXTHOP="20.20.20.20"

    PACKET1="packet update
	origin 2
	aspath $ASPATH
	nexthop $NEXTHOP
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    PACKET2="packet update
	origin 2
	aspath $RECVASPATH
	nexthop $NEXT_HOP
	med 1
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    coord peer2 expect $PACKET2

    coord peer1 send $PACKET1

    sleep 2
    coord peer2 assert queue 0

    # Verify that the peers are still connected.
    coord peer1 assert established
    coord peer2 assert established
}

test3()
{
    echo "TEST3:"
    echo "	1) Send an update packet to BGP with a 4-byte ASnum and check that"
    echo "	   it can merge AS4Path and ASPath information."

    # Reset the peers
    reset

    # Establish a connection
    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.100
    coord peer1 assert established

    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.101 use_4byte_asnums true
    coord peer2 assert established

    ASPATH="$PEER1_AS,1,$ASTRAN,[3,4,5],6,[7,8],9"
    AS4PATH="$PEER1_AS,1,2.2,[3,4,5],6,[7,8],9"
    RECVASPATH="$AS,$PEER1_AS,1,2.2,[3,4,5],6,[7,8],9"
    NEXTHOP="20.20.20.20"

    PACKET1="packet update
	origin 2
	aspath $ASPATH
	nexthop $NEXTHOP
	as4path $AS4PATH
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    PACKET2="packet update
	origin 2
	aspath $RECVASPATH
	nexthop $NEXT_HOP
	med 1
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    coord peer2 expect $PACKET2

    coord peer1 send $PACKET1

    sleep 2
    coord peer2 assert queue 0

    # Verify that the peers are still connected.
    coord peer1 assert established
    coord peer2 assert established
}

test4()
{
    echo "TEST4:"
    echo "	1) Send an update packet to BGP with a 4-byte ASnum and check that"
    echo "	   it can merge AS4Path and ASPath information.  "
    echo "         In this case the 2-byte ASes have added data that isn't in the AS4Path."

    # Reset the peers
    reset

    # Establish a connection
    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.100
    coord peer1 assert established

    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.101 use_4byte_asnums true
    coord peer2 assert established

    ASPATH="$PEER1_AS,1,2,$ASTRAN,[3,4,5],6,[7,8],9"
    AS4PATH="10.10,[3,4,5],6,[7,8],9"
    RECVASPATH="$AS,$PEER1_AS,1,2,10.10,[3,4,5],6,[7,8],9"
    NEXTHOP="20.20.20.20"

    PACKET1="packet update
	origin 2
	aspath $ASPATH
	nexthop $NEXTHOP
	as4path $AS4PATH
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    PACKET2="packet update
	origin 2
	aspath $RECVASPATH
	nexthop $NEXT_HOP
	med 1
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    coord peer2 expect $PACKET2

    coord peer1 send $PACKET1

    sleep 2
    coord peer2 assert queue 0

    # Verify that the peers are still connected.
    coord peer1 assert established
    coord peer2 assert established
}

test5()
{
    echo "TEST5:"
    echo "	1) Send an update packet to BGP with a 4-byte ASnum and check that"
    echo "	   it can merge AS4Path and ASPath information.  "
    echo "         In this case the 2-byte ASes have added data that isn't in the AS4Path."

    # Reset the peers
    reset

    # Establish a connection
    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.100
    coord peer1 assert established

    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.101 use_4byte_asnums true
    coord peer2 assert established

    ASPATH="$PEER1_AS,1,2,$ASTRAN,$ASTRAN,[3,4,$ASTRAN],6,[$ASTRAN,8],$ASTRAN"
    AS4PATH="11.11,10.10,[3,4,5.5],6,[7.7,8],9.9"
    RECVASPATH="$AS,$PEER1_AS,1,2,11.11,10.10,[3,4,5.5],6,[7.7,8],9.9"
    NEXTHOP="20.20.20.20"

    PACKET1="packet update
	origin 2
	aspath $ASPATH
	nexthop $NEXTHOP
	as4path $AS4PATH
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    PACKET2="packet update
	origin 2
	aspath $RECVASPATH
	nexthop $NEXT_HOP
	med 1
	nlri 10.10.10.0/24
	nlri 20.20.20.20/24"

    coord peer2 expect $PACKET2

    coord peer1 send $PACKET1

    sleep 2
    coord peer2 assert queue 0

    # Verify that the peers are still connected.
    coord peer1 assert established
    coord peer2 assert established
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
    ./test_peer -s peer2      = $CXRL finder://peer1/common/0.1/get_target_name
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
