#!/usr/bin/env bash

#
# $XORP: xorp/bgp/harness/test_peering2.sh,v 1.15 2003/06/24 23:49:59 atanu Exp $
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
# 4) Run xorp "../bgp"
# 5) Run "./test_peer -s peer1"
# 6) Run "./test_peer -s peer2"
# 7) Run "./coord"
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

# IBGP
PORT1=10001
PEER1_PORT=20001
PEER1_AS=$AS

# EBGP
PORT2=10002
PEER2_PORT=20002
PEER2_AS=65000

HOLDTIME=5

TRAFFIC_FILES="${srcdir}/../../../data/bgp/icsi1.mrtd"

configure_bgp()
{
    local_config $AS $ID

    # Don't try and talk to the rib.
    register_rib ""

    PEER=$HOST
    NEXT_HOP=192.150.187.78
    add_peer $LOCALHOST $PORT1 $PEER $PEER1_PORT $PEER1_AS $NEXT_HOP $HOLDTIME
    enable_peer $LOCALHOST $PORT1 $PEER $PEER1_PORT

    PEER=$HOST
    NEXT_HOP=192.150.187.78
    add_peer $LOCALHOST $PORT2 $PEER $PEER2_PORT $PEER2_AS $NEXT_HOP $HOLDTIME
    enable_peer $LOCALHOST $PORT2 $PEER $PEER2_PORT
}

reset()
{
    coord reset

    coord target $HOST $PORT1
    coord initialise attach peer1

    coord target $HOST $PORT2
    coord initialise attach peer2

    bgp_not_established

    sleep 5
}

bgp_not_established()
{
    while ../tools/print_peers -v | grep 'Peer State: ESTABLISHED'
    do
	sleep 1
    done
}

bgp_updates_received()
{
    ../tools/print_peers  -v |
    grep "Updates Received" | awk "NR == $1 {print \$3}"  | sed 's/,//'
}

bgp_all_updates_received()
{
    rec1=a
    rec2=b
    until [ $rec1 == $rec2 ]
    do
	rec1=$(bgp_updates_received $1)
	sleep 1
	rec2=$(bgp_updates_received $1)
    done
}

bgp_updates_sent()
{
    ../tools/print_peers  -v |
    grep "Updates Sent" | awk "NR == $1 {print \$6}"
}

bgp_all_updates_sent()
{
    rec1=a
    rec2=b
    until [ $rec1 == $rec2 ]
    do
	rec1=$(bgp_updates_sent $1)
	sleep 1
	rec2=$(bgp_updates_sent $1)
    done
}

test1()
{
    TFILE=$1

    echo "TEST1 - Inject a saved feed then drop peering - $TFILE"

    # Reset the peers
    reset

    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.100
    coord peer2 assert established

    NOBLOCK=true coord peer2 send dump mrtd update $TFILE

    while pending | grep true
    do
	sleep 1
    done

    coord peer2 assert established

    # Reset the connection
    reset
    
    # Establish the new connection.
    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.100
    coord peer2 assert established
}

test2()
{
    TFILE=$1

    echo "TEST2 - Inject a saved feed then toggle another peering - $TFILE"

    # Reset the peers
    reset
    
    # Establish the EBGP peering.
    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.100
    coord peer2 assert established

    # send in the saved file
    NOBLOCK=true coord peer2 send dump mrtd update $TFILE

    # Wait for the file to be transmitted by the test peer.
    while pending | grep true
    do
	sleep 1
    done

    # Wait for the BGP process to receive all the updates
    bgp_all_updates_received 2

    # Bring up another peering to test the dump code.
    for i in 1 2
    do
	coord reset
	coord target $HOST $PORT1
	coord initialise attach peer1

	echo "Sleeping for ten seconds"
	sleep 10

	coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.100
	coord peer1 assert established

	# Wait for the BGP process to send all the updates
	bgp_all_updates_sent 1
    done

    coord target $HOST $PORT2
    coord initialise attach peer2

    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.100
    coord peer2 assert established

    # Tearing out peer2 will cause all the routes to be withdrawn, wait
    bgp_all_updates_sent 1

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
    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.100
    coord peer2 assert established

    # send in the saved file
    NOBLOCK=true coord peer2 send dump mrtd update $TFILE

    # Bring up another peering
    NOBLOCK=true coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.100

    # Allow some routes to be loaded
    sleep 5

    # Drop the injecting feed
    NOBLOCK=true coord peer2 disconnect

    # Wait for the BGP to stabilise
    bgp_all_updates_sent 1

    # Make sure that the peer1 connection is still established
    coord peer1 assert established
}

TESTS_NOT_FIXED='test3'
TESTS='test1 test2'

# Temporary fix to let TCP sockets created by call_xrl pass through TIME_WAIT
TIME_WAIT=`time_wait_seconds`

# Include command line
. ${srcdir}/args.sh

if [ $START_PROGRAMS = "yes" ]
then
CXRL="$CALLXRL -r 10"
    ../../utils/runit $QUIET $VERBOSE -c "$0 -s -c $*" <<EOF
    ../../libxipc/finder
    ../../fea/fea_dummy   = $CXRL finder://fea/common/0.1/get_target_name
    ../../rib/rib         = $CXRL finder://rib/common/0.1/get_target_name
    ../bgp                = $CXRL finder://bgp/common/0.1/get_target_name
    ./test_peer -s peer1  = $CXRL finder://peer1/common/0.1/get_target_name
    ./test_peer -s peer2  = $CXRL finder://peer1/common/0.1/get_target_name
    ./coord               = $CXRL finder://coord/common/0.1/get_target_name
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
    for t in $TRAFFIC_FILES
    do 
	if [ -f $t ]
	then
	    $i $t
	    echo "Waiting $TIME_WAIT seconds for TCP TIME_WAIT state timeout"
	    sleep $TIME_WAIT
	else
	    echo "Traffic file $t missing."
	fi
    done
done

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
