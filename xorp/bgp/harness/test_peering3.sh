#!/usr/bin/env bash

#
# $XORP$
#

# XXX 2008-12-09 Atanu
#
# 1) The code in this file was copied from test_peering2.sh
#
# 2) This file was introduced to test a single bug with an associated data
# file bug837.mrtd.
#
# 3) The test was not added to test_peering2.sh because it is necessary to
# modify peer AS number, plus test_peering2.sh does not handle multiple
# data files.
#
# 4) The code in here should be rewritten to handle multiple data files with
# the tests associated with a particular data file.
#
# XXX

# http://bugzilla.xorp.org/bugzilla/show_bug.cgi?id=837

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
# 6) Run "./coord"
#
set -e

. ./setup_paths.sh

# srcdir is set by make for check target
if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi
. ${srcdir}/xrl_shell_funcs.sh ""
. $BGP_FUNCS ""
. $RIB_FUNCS ""
. ${srcdir}/notification_codes.sh

onexit()
{
    last=$?
    if [ $last = "0" ]
    then
	echo "$0: Tests Succeeded (BGP: $TESTS)"
    else
	echo "$0: Tests Failed (BGP: $TESTS)"
    fi

    trap '' 0 2
}

trap onexit 0 2

USE4BYTEAS=false

HOST=127.0.0.1
LOCALHOST=$HOST
ID=192.150.187.78
AS=65008

# EBGP - IPV4
PORT1=10001
PEER1_PORT=20001
PEER1_AS=6502

HOLDTIME=5

TRAFFIC_DIR="${srcdir}/../../../data/bgp"
TRAFFIC_FILES="${TRAFFIC_DIR}/bug837.mrtd"

# NOTE: The Win32 versions of coord and peer will perform
# path conversion and expansion of /tmp internally.
TMPDIR=${TMPDIR:-/tmp}
EXT=${LOGNAME:-unknown}

configure_bgp()
{
    local_config $AS $ID $USE4BYTEAS

    # Don't try and talk to the rib.
    register_rib ""

    # EBGP - IPV4
    PEER=$HOST
    NEXT_HOP=192.150.187.78
    add_peer lo $LOCALHOST $PORT1 $PEER $PEER1_PORT $PEER1_AS $NEXT_HOP $HOLDTIME
    set_parameter $LOCALHOST $PORT1 $PEER $PEER1_PORT MultiProtocol.IPv4.Unicast true
    enable_peer $LOCALHOST $PORT1 $PEER $PEER1_PORT
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
    for i in peer1
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

# http://bugzilla.xorp.org/bugzilla/show_bug.cgi?id=837

test1()
{
    TFILE=$1

    echo "TEST1 - Inject a saved feed then drop peering - $TFILE"

    # Reset the peers
    reset

    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.102
    coord peer1 assert established

    NOBLOCK=true coord peer1 send dump mrtd update $TFILE

    # Wait for the file to be transmitted by the test peer.
    bgp_peer_unchanged peer1

    coord peer1 assert established

    # Reset the connection
    reset

    if [ x"$OSTYPE" != xmsys ]; then
	uptime
	echo "NOTE: Occasionally, we fail to make a connection. See the comment in the reset function."
    fi

    # Establish the new connection.
    coord peer1 establish AS $PEER1_AS holdtime 0 id 192.150.187.102
    coord peer1 assert established
}

TESTS_NOT_FIXED='test1'
TESTS='test1'

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
    $XORP_FINDER
    $XORP_FEA_DUMMY      = $CXRL finder://fea/common/0.1/get_target_name
    $XORP_RIB            = $CXRL finder://rib/common/0.1/get_target_name
    $XORP_BGP            = $CXRL finder://bgp/common/0.1/get_target_name
    ./test_peer -s peer1 = $CXRL finder://peer1/common/0.1/get_target_name
    ./coord              = $CXRL finder://coord/common/0.1/get_target_name
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
