#!/usr/bin/env bash

#
# $XORP: other/bgp/peer_with_icsi.sh,v 1.4 2003/10/21 05:21:35 atanu Exp $
#

#
# ICSI specific.
# Peer with our border router.
# The script will only work on a set of specific hosts at ICSI that our
# border router is configured for.

# Preconditons:
# 1) Run "xorp/libxipc/finder"
# 2) Run "xorp/fea/fea" or "xorp/fea/fea_dummy"
# 3) Run "xorp/rib/rib"
# 4) Run "xorp/bgp/bgp"
# 5) Run "xorp/bgp/harness/test_peer -s peer1 -v"
# 6) Run "xorp/bgp/harness/coord"

# First check that we know about this host.

if MY_AS=$(./host_to_as.sh)
then
    echo "MY AS is $MY_AS"
else
    echo "Unknown host"
    exit 2
fi

# Configure an interface and re-write all next hops to go through this
# interface
VIF0="dc0"
ADDR0="10.1.0.1"
NEXT_HOP_REWRITE1="10.1.0.2"
NEXT_HOP_REWRITE2="10.1.0.3"

LOCALHOST=$(hostname)
MY_IP=$(host $LOCALHOST | cut -f 4 -d ' ')

MY_ID=$MY_IP
MY_NAME=$LOCALHOST
MY_PORT=9000
NEXT_HOP=$MY_IP

BORDER_ROUTER_NAME=router3-fast1-0-0
BORDER_ROUTER_AS=64999
BORDER_ROUTER_PORT=179

TEST_PEER_AS=65001
TEST_PEER_PORT=9001

TREETOP=${PWD}/../../xorp

#
# Start all the required processes
#
start_processes()
{
    (cd $TREETOP 
    for i in libxipc/xorp_finder fea/xorp_fea_dummy rib/xorp_rib bgp/xorp_bgp \
	    "bgp/harness/test_peer -s peer1 -v" bgp/harness/coord
    do
	xterm -e $i &
	sleep 2
    done)
}

#
# No variables below here should need changing.
#
fea()
{
    echo "Configuring fea"

    export CALLXRL=$TREETOP/libxipc/call_xrl
    FEA_FUNCS=$TREETOP/fea/xrl_shell_funcs.sh
    local tid=$($FEA_FUNCS start_fea_transaction)

    $FEA_FUNCS create_interface $tid $VIF0
    $FEA_FUNCS enable_interface $tid $VIF0

    $FEA_FUNCS create_vif $tid $VIF0 $VIF0
    $FEA_FUNCS enable_vif $tid $VIF0 $VIF0

    $FEA_FUNCS create_address4 $tid $VIF0 $VIF0 $ADDR0
    $FEA_FUNCS enable_address4 $tid $VIF0 $VIF0 $ADDR0

    $FEA_FUNCS set_prefix4 $tid $VIF0 $VIF0 $ADDR0 25

    $FEA_FUNCS commit_fea_transaction $tid

    $FEA_FUNCS get_configured_vif_addresses4 $VIF0 $VIF0
}

bgp()
{
    echo "Configuring bgp"
    export CALLXRL=$TREETOP/libxipc/call_xrl
    BGP_FUNCS=$TREETOP/bgp/xrl_shell_funcs.sh

    PORT=$MY_PORT
    AS=$MY_AS
    ID=$MY_ID
    HOLDTIME=0
    $BGP_FUNCS local_config $AS $ID
#   register_rib "rib"
	
    # Our border router.

    PEER=$BORDER_ROUTER_NAME
    PEER_AS=$BORDER_ROUTER_AS
    PEER_PORT=$BORDER_ROUTER_PORT
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    $BGP_FUNCS add_peer $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    # Enable Multiprotocol IPv6 support
#    $BGP_FUNCS set_parameter $IPTUPLE MultiProtocol.IPv6.Unicast
    # Rewrite the next hop onto our test net
    $BGP_FUNCS next_hop_rewrite_filter $IPTUPLE $NEXT_HOP_REWRITE1
#    $BGP_FUNCS enable_peer $IPTUPLE

    # Our test peer for monitoring

    PEER=$LOCALHOST
    PEER_AS=$TEST_PEER_AS
    PEER_PORT=$TEST_PEER_PORT
    IPTUPLE="$LOCALHOST $PORT $PEER $PEER_PORT"
    $BGP_FUNCS add_peer $IPTUPLE $PEER_AS $NEXT_HOP $HOLDTIME
    # Enable Multiprotocol IPv6 support
    $BGP_FUNCS set_parameter $IPTUPLE MultiProtocol.IPv6.Unicast
    # Rewrite the next hop onto our test net
    $BGP_FUNCS next_hop_rewrite_filter $IPTUPLE $NEXT_HOP_REWRITE2
    $BGP_FUNCS enable_peer $IPTUPLE
}

bgp_enable()
{
    echo "Enable peering to main router"
    export CALLXRL=$TREETOP/libxipc/call_xrl
    BGP_FUNCS=$TREETOP/bgp/xrl_shell_funcs.sh

    PORT=$MY_PORT
    PEER=$BORDER_ROUTER_NAME
    PEER_AS=$BORDER_ROUTER_AS
    PEER_PORT=$BORDER_ROUTER_PORT

    $BGP_FUNCS enable_peer $LOCALHOST $PORT $PEER $PEER_PORT
}

bgp_disable()
{
    echo "Disable peering to main router"
    export CALLXRL=$TREETOP/libxipc/call_xrl
    BGP_FUNCS=$TREETOP/bgp/xrl_shell_funcs.sh

    PORT=$MY_PORT
    PEER=$BORDER_ROUTER_NAME
    PEER_AS=$BORDER_ROUTER_AS
    PEER_PORT=$BORDER_ROUTER_PORT

    $BGP_FUNCS disable_peer $LOCALHOST $PORT $PEER $PEER_PORT
}

test_peer()
{
    echo "Configuring test peer"
    export CALLXRL=$TREETOP/libxipc/call_xrl
    TEST_FUNCS=$TREETOP/bgp/harness/xrl_shell_funcs.sh

    PEER=$LOCALHOST
    PORT=$MY_PORT
    PEER_AS=$TEST_PEER_AS

    $TEST_FUNCS coord reset
    $TEST_FUNCS coord target $PEER $PORT
    $TEST_FUNCS coord initialise attach peer1
    
    $TEST_FUNCS coord peer1 establish AS $PEER_AS \
				      holdtime 0 \
				      id 192.150.187.100 \
				      ipv6 true
}

# We have no arguments.
if [ $# == 0 ]
then
    start_processes
    fea
    bgp
    bgp_enable
    test_peer
else
    $*
fi

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
