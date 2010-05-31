#!/usr/bin/env bash

#
# $XORP: other/bgp/collect.sh,v 1.2 2003/09/17 03:36:51 atanu Exp $
#

#
# ICSI specific.
# Peer with our border router and save the traffic.
# The script will only work on a set of specific hosts at ICSI that our
# border router is configured for.
# Preconditons:
# 1) Run "xorp/libxipc/finder"
# 2) Run "xorp/bgp/harness/test_peer -s peer1 -v"
# 3) Run "xorp/bgp/harness/coord"

# First check that we know about this host.

if MY_AS=$(./host_to_as.sh)
then
    echo "MY AS is $MY_AS"
else
    echo "Unknown host"
    exit 2
fi

BORDER_ROUTER_NAME=router3-fast1-0-0
BORDER_ROUTER_PORT=179

TRAFFIC=/tmp/xorp_bgp_traffic.mrtd
DUMP=/tmp/xorp_bgp_debug.mrtd

TREETOP=${PWD}/../../xorp

#
# Start all the required processes
#
start_processes()
{
    (cd $TREETOP 
    for i in libxipc/xorp_finder "bgp/harness/test_peer -s peer1" \
		bgp/harness/coord
    do
	xterm -e $i &
	sleep 2
    done)
}

test_peer()
{
    echo "Configuring test peer"

    export CALLXRL=$TREETOP/libxipc/call_xrl
    TEST_FUNCS=$TREETOP/bgp/harness/xrl_shell_funcs.sh

    $TEST_FUNCS coord reset
    $TEST_FUNCS coord target $BORDER_ROUTER_NAME $BORDER_ROUTER_PORT
    $TEST_FUNCS coord initialise attach peer1
    
#    $TEST_FUNCS coord peer1 establish AS $MY_AS holdtime 0 id 192.150.187.100

    $TEST_FUNCS coord peer1 dump recv mrtd ipv4 traffic $TRAFFIC
    $TEST_FUNCS coord peer1 dump sent mrtd ipv4 traffic $TRAFFIC
    sleep 1

    $TEST_FUNCS coord peer1 establish AS 65008 holdtime 0 id 192.150.187.78

    #sleep 30
    #$TEST_FUNCS coord peer1 dump recv text debug $DUMP
    #$TEST_FUNCS coord peer1 dump recv mrtd debug $DUMP
}

# We have no arguments.
if [ $# == 0 ]
then
    start_processes
    test_peer
else
    $*
fi

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:

