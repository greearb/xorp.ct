#!/bin/sh

#
# $XORP: xorp/bgp/harness/test1.sh,v 1.9 2002/12/09 10:59:37 pavlin Exp $
#

#
# Simple test
# A bgp process is tested using three peers.
#
# Peers 1 and 2 are E-BGP Peer 3 is I-BGP
#
# 1) Add a route (A) via peer1
# 2) Verify that route (A) appears at peer3	
# 3) Add a better route (B) via peer2
# 4) Verify that route (B) appears at peer3
# 5) Withdraw route (b) via peer2
# 6) Verify that route (A) appears at peer3
#

set -e
#set -x

. ../xrl_shell_funcs.sh
. ./xrl_shell_funcs.sh

PORT=10000

configure_bgp()
{
    LOCALHOST=tigger.icir.org
    AS=65008
    ID=192.150.187.78
    HOLDTIME=60
    local_config $AS $ID
    register_rib rib

    PEER=xorp-c4000.icir.org
    PEER_AS=65000
    PEER_PORT=179
    NEXT_HOP=192.150.187.78
    add_peer $LOCALHOST $PORT $PEER $PEER_PORT $PEER_AS $NEXT_HOP $HOLDTIME
    enable_peer $LOCALHOST $PORT $PEER $PEER_PORT $PEER_AS

    PEER=xorp0
    PEER_AS=60001
    PEER_PORT=10000
    NEXT_HOP=192.150.187.100
    add_peer $LOCALHOST $PORT $PEER $PEER_PORT $PEER_AS $NEXT_HOP $HOLDTIME
    enable_peer $LOCALHOST $PORT $PEER $PEER_PORT $PEER_AS

    PEER=xorp6
    PEER_AS=60002
    PEER_PORT=10000
    NEXT_HOP=192.150.187.106
    add_peer $LOCALHOST $PORT $PEER $PEER_PORT $PEER_AS $NEXT_HOP $HOLDTIME
    enable_peer $LOCALHOST $PORT $PEER $PEER_PORT $PEER_AS

    PEER=xorp7
    PEER_AS=65008
    PEER_PORT=10000
    NEXT_HOP=192.150.187.107
    add_peer $LOCALHOST $PORT $PEER $PEER_PORT $PEER_AS $NEXT_HOP $HOLDTIME
    enable_peer $LOCALHOST $PORT $PEER $PEER_PORT $PEER_AS
}

set +e
configure_bgp
set -e

#BASE=peer1 disconnect

#BASE=peer1 register coord
#BASE=peer1 connect tigger.icir.org 10000
# Send at least 19 bytes.
#BASE=peer1 send "1234567890123456789"

#BASE=peer2 disconnect
#BASE=peer2 connect tigger.icir.org 10000

#BASE=peer3 disconnect
#BASE=peer3 connect tigger.icir.org 10000

#
# Reset the coordinator.
#
coord reset

#
# Select the BGP process which is to be tested
#
coord target tigger.icir.org $PORT

#
# Attach to the test peers.
#
coord initialise attach peer1
coord initialise attach peer2
coord initialise attach peer3

#
# Establish a bgp session with the BGP process
#
coord peer1 establish AS 60001 holdtime 0 id 192.150.187.100
coord peer2 establish AS 60002 holdtime 0 id 192.150.187.106
coord peer3 establish AS 65008 holdtime 0 id 192.150.187.107

# Introduce an update message
sleep 2
coord peer1 send packet update \
    origin 2 \
    aspath "1,2,(3,4,5),6,(7,8),9" \
    nexthop 10.10.10.10 \
    nlri 10.10.10.0/24 \
    nlri 20.20.20.20/24

sleep 2
coord peer1 trie sent lookup 10.10.10.0/24 aspath "1,2,(3,4,5),6,(7,8),9"
coord peer2 trie recv lookup 10.10.10.0/24 aspath "65008,1,2,(3,4,5),6,(7,8),9"
coord peer3 trie recv lookup 10.10.10.0/24 aspath "1,2,(3,4,5),6,(7,8),9" 

sleep 2
coord peer2 send packet update \
    origin 2 \
    aspath "1" \
    nexthop 20.20.20.20 \
    nlri 10.10.10.10/24 \
    nlri 20.20.20.20/24

sleep 2
coord peer1 trie recv lookup 10.10.10.0/24 aspath "65008,1"
coord peer2 trie sent lookup 10.10.10.0/24 aspath "1"
coord peer3 trie recv lookup 10.10.10.0/24 aspath "1"

sleep 2
coord peer2 send packet update \
    origin 2 \
    aspath "1" \
    nexthop 20.20.20.20 \
    withdraw 10.10.10.10/24 \
    withdraw 20.20.20.20/24

sleep 2
coord peer1 trie sent lookup 10.10.10.0/24 aspath "1,2,(3,4,5),6,(7,8),9"
coord peer2 trie recv lookup 10.10.10.0/24 aspath "65008,1,2,(3,4,5),6,(7,8),9"
coord peer3 trie recv lookup 10.10.10.0/24 aspath "1,2,(3,4,5),6,(7,8),9"

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
