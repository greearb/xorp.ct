#!/usr/bin/env bash

#
# $XORP: xorp/bgp/harness/test_peering2.sh,v 1.2 2003/01/29 05:15:49 atanu Exp $
#

#
# Feed saved data to our BGP process.
#
# This script started with no arguments will start all required process and
# terminate them at the end of the tests.
#
# Preconditons
# 1) Run a finder process
# 2) Run xorp "../bgp"
# 3) Run "./test_peer -s peer1"
# 4) Run "./test_peer -s peer2"
# 5) Run "./coord"
#
set -e

. ../xrl_shell_funcs.sh ""
. ./xrl_shell_funcs.sh ""

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

TRAFFIC_FILES="../../../data/bgp/icsi1.mrtd"

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

    sleep 5
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
    
    sleep 5
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
    
    sleep 5
    # Establish the EBGP peering.
    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.100
    coord peer2 assert established

    # send in the saved file
    NOBLOCK=true coord peer2 send dump mrtd update $TFILE

    while pending | grep true
    do
	sleep 1
    done

    # Bring up another peering to test the dump code.
    for i in 1 2
    do
	coord reset
	coord target $HOST $PORT1
	coord initialise attach peer1
	sleep 5
	coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.100
	coord peer1 assert established

	sleep 10
    done

    coord target $HOST $PORT2
    coord initialise attach peer2
    sleep 5
    coord peer2 establish AS $PEER2_AS holdtime 0 id 192.150.187.100
    coord peer2 assert established

    sleep 60

    # Make sure that the connections are still established.
    coord peer1 assert established
    coord peer2 assert established
}

TESTS_NOT_FIXED='test2'
TESTS='test1'

# Temporary fix to let TCP sockets created by call_xrl pass through TIME_WAIT
TIME_WAIT=`time_wait_seconds`

# Include command line
. ./args.sh

if [ $START_PROGRAMS = "yes" ]
then
CXRL="$CALLXRL -r 10"
    ../../utils/runit $QUIET $VERBOSE -c "$0 -s -c $*" <<EOF
    ../../libxipc/finder
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
