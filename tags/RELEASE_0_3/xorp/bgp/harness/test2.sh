#!/usr/local/bin/bash
#!/bin/sh

#
# $XORP: xorp/bgp/harness/test2.sh,v 1.2 2003/05/24 23:35:25 mjh Exp $
#

#
# Preconditions
# 1) Run a finder process on tigger "../../xorp/libxipc/finder"
# 2) Run a test peer on xorp0.
#    "ssh xorp0 xorp/xorp/bgp/harness/test_peer -s peer1 -h tigger"
# 3) Run a RIB on tigger. "../../rib/rib"
# 4) Run a fea on xorp0 as root.
#    "ssh root@xorp0 xorp/xorp/fea/fea  -h tigger -b"
# 5) Run a bgp process on tigger. "../bgp"
#

set -e

. ./xrl_shell_funcs.sh
export CALLXRL

onexit()
{
    last=$?
    if [ $last = "0" ]
    then
	echo "$0: Tests Succeeded"
    else
	echo "$0: Tests Failed"
	#echo "Stopping BGP"
	#(cd ..;./xrl_shell_funcs.sh shutdown)
    fi
}

trap onexit 0

PORT=10000
VIF=dc1

configure_bgp()
{
    BGP=../xrl_shell_funcs.sh
    LOCALHOST=tigger.icir.org
    AS=65008
    ID=192.150.187.78
    HOLDTIME=60
    $BGP local_config $AS $ID
    $BGP register_rib rib

    PEER=xorp0
    PEER_AS=60001
    PEER_PORT=10000
    NEXT_HOP=192.150.187.100
    $BGP add_peer $LOCALHOST $PORT $PEER $PEER_PORT $PEER_AS $NEXT_HOP $HOLDTIME
    $BGP enable_peer $LOCALHOST $PORT $PEER $PEER_PORT $PEER_AS
}

configure_rib()
{
    RIB=../../rib/xrl_shell_funcs.sh
    $RIB make_rib_errors_fatal
    $RIB new_vif $VIF
    $RIB add_vif_addr4 $VIF 172.16.0.1 172.16.0.0/24
}

configure_fea()
{
    FEA=../../fea/xrl_shell_funcs.sh
    $FEA create_interface $VIF
    $FEA create_vif $VIF $VIF
    $FEA create_address4 $VIF 172.16.0.1
    $FEA set_prefix4 $VIF 172.16.0.1 24
    $FEA enable_vif $VIF
    $FEA enable_interface $VIF
}

configure_fea
set +e
configure_rib
configure_bgp
set -e

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

#
# Establish a bgp session with the BGP process
#
coord peer1 establish AS 60001 keepalive true holdtime 0 id 192.150.187.100

add()
{
coord peer1 send packet update \
    origin 2 \
    aspath "1,2,(3,4,5),6,(7,8),9" \
    nexthop 172.16.0.2 \
    nlri 172.16.1.0/24 \
    nlri 172.16.2.0/24
}
sleep 2
add

sleep 2
coord peer1 trie sent lookup 10.10.10.0/24 aspath "1,2,(3,4,5),6,(7,8),9"

withdraw()
{
    coord peer1 send packet update \
    origin 2 \
    aspath "1,2,(3,4,5),6,(7,8),9" \
    nexthop 172.16.0.2 \
    nlri 172.16.1.0/24 \
    nlri 172.16.2.0/24
}
sleep 2
#withdraw

#sleep 2
#coord peer1 trie sent lookup 10.10.10.0/24 aspath "1,2,(3,4,5),6,(7,8),9"

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
