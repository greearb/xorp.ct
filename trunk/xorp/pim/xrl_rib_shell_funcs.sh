#!/bin/sh

#
# $XORP: xorp/pim/xrl_rib_shell_funcs.sh,v 1.2 2003/03/25 00:46:24 pavlin Exp $
#

#
# Library of functions to sent XRLs to a running RIB process.
#

#
# TODO: this file is temporary in the "xorp/pim" directory
#

. ../utils/xrl_shell_lib.sh

# Conditionally set the target name
RIB_TARGET=${RIB_TARGET:="RIB"}

rib_enable_rib()
{
    echo "rib_enable_rib" $*
    XRL="finder://$RIB_TARGET/rib/0.1/enable_rib"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_disable_rib()
{
    echo "rib_disable_rib" $*
    XRL="finder://$RIB_TARGET/rib/0.1/disable_rib"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_start_rib()
{
    echo "rib_start_rib" $*
    XRL="finder://$RIB_TARGET/rib/0.1/start_rib"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_stop_rib()
{
    echo "rib_stop_rib" $*
    XRL="finder://$RIB_TARGET/rib/0.1/stop_rib"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_add_rib_client4()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: rib_add_rib_client4 <target_name:txt> <unicast:bool> <multicast:bool>"
	exit 1
    fi
    target_name=$1
    unicast=$2
    multicast=$3
    
    echo "rib_add_rib_client4" $*
    XRL="finder://$RIB_TARGET/rib/0.1/add_rib_client4"
    XRL_ARGS="?target_name:txt=$target_name&unicast:bool=$unicast&multicast:bool=$multicast"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_add_rib_client6()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: rib_add_rib_client6 <target_name:txt> <unicast:bool> <multicast:bool>"
	exit 1
    fi
    target_name=$1
    unicast=$2
    multicast=$3
    
    echo "rib_add_rib_client6" $*
    XRL="finder://$RIB_TARGET/rib/0.1/add_rib_client6"
    XRL_ARGS="?target_name:txt=$target_name&unicast:bool=$unicast&multicast:bool=$multicast"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_delete_rib_client4()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: rib_delete_rib_client4 <target_name:txt> <unicast:bool> <multicast:bool>"
	exit 1
    fi
    target_name=$1
    unicast=$2
    multicast=$3
    
    echo "rib_delete_rib_client4" $*
    XRL="finder://$RIB_TARGET/rib/0.1/delete_rib_client4"
    XRL_ARGS="?target_name:txt=$target_name&unicast:bool=$unicast&multicast:bool=$multicast"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_delete_rib_client6()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: rib_delete_rib_client6 <target_name:txt> <unicast:bool> <multicast:bool>"
	exit 1
    fi
    target_name=$1
    unicast=$2
    multicast=$3
    
    echo "rib_delete_rib_client6" $*
    XRL="finder://$RIB_TARGET/rib/0.1/delete_rib_client6"
    XRL_ARGS="?target_name:txt=$target_name&unicast:bool=$unicast&multicast:bool=$multicast"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_enable_rib_client4()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: rib_enable_rib_client4 <target_name:txt> <unicast:bool> <multicast:bool>"
	exit 1
    fi
    target_name=$1
    unicast=$2
    multicast=$3
    
    echo "rib_enable_rib_client4" $*
    XRL="finder://$RIB_TARGET/rib/0.1/enable_rib_client4"
    XRL_ARGS="?target_name:txt=$target_name&unicast:bool=$unicast&multicast:bool=$multicast"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_enable_rib_client6()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: rib_enable_rib_client6 <target_name:txt> <unicast:bool> <multicast:bool>"
	exit 1
    fi
    target_name=$1
    unicast=$2
    multicast=$3
    
    echo "rib_enable_rib_client6" $*
    XRL="finder://$RIB_TARGET/rib/0.1/enable_rib_client6"
    XRL_ARGS="?target_name:txt=$target_name&unicast:bool=$unicast&multicast:bool=$multicast"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_disable_rib_client4()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: rib_disable_rib_client4 <target_name:txt> <unicast:bool> <multicast:bool>"
	exit 1
    fi
    target_name=$1
    unicast=$2
    multicast=$3
    
    echo "rib_disable_rib_client4" $*
    XRL="finder://$RIB_TARGET/rib/0.1/disable_rib_client4"
    XRL_ARGS="?target_name:txt=$target_name&unicast:bool=$unicast&multicast:bool=$multicast"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_disable_rib_client6()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: rib_disable_rib_client6 <target_name:txt> <unicast:bool> <multicast:bool>"
	exit 1
    fi
    target_name=$1
    unicast=$2
    multicast=$3
    
    echo "rib_disable_rib_client6" $*
    XRL="finder://$RIB_TARGET/rib/0.1/disable_rib_client6"
    XRL_ARGS="?target_name:txt=$target_name&unicast:bool=$unicast&multicast:bool=$multicast"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_no_fea()
{
    echo "rib_no_fea" $*
    XRL="finder://$RIB_TARGET/rib/0.1/no_fea"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_add_igp_table4()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: rib_add_igp_table4 <protocol:txt> <unicast:bool> <multicast:bool>"
	exit 1
    fi
    protocol=$1
    unicast=$2
    multicast=$3
    
    echo "rib_add_igp_table4" $*
    XRL="finder://$RIB_TARGET/rib/0.1/add_igp_table4"
    XRL_ARGS="?protocol:txt=$protocol&unicast:bool=$unicast&multicast:bool=$multicast"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_add_igp_table6()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: rib_add_igp_table6 <protocol:txt> <unicast:bool> <multicast:bool>"
	exit 1
    fi
    protocol=$1
    unicast=$2
    multicast=$3
    
    echo "rib_add_igp_table6" $*
    XRL="finder://$RIB_TARGET/rib/0.1/add_igp_table6"
    XRL_ARGS="?protocol:txt=$protocol&unicast:bool=$unicast&multicast:bool=$multicast"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_delete_igp_table4()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: rib_delete_igp_table4 <protocol:txt> <unicast:bool> <multicast:bool>"
	exit 1
    fi
    protocol=$1
    unicast=$2
    multicast=$3
    
    echo "rib_delete_igp_table4" $*
    XRL="finder://$RIB_TARGET/rib/0.1/delete_igp_table4"
    XRL_ARGS="?protocol:txt=$protocol&unicast:bool=$unicast&multicast:bool=$multicast"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_delete_igp_table6()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: rib_delete_igp_table6 <protocol:txt> <unicast:bool> <multicast:bool>"
	exit 1
    fi
    protocol=$1
    unicast=$2
    multicast=$3
    
    echo "rib_delete_igp_table6" $*
    XRL="finder://$RIB_TARGET/rib/0.1/delete_igp_table6"
    XRL_ARGS="?protocol:txt=$protocol&unicast:bool=$unicast&multicast:bool=$multicast"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_add_egp_table4()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: rib_add_egp_table4 <protocol:txt> <unicast:bool> <multicast:bool>"
	exit 1
    fi
    protocol=$1
    unicast=$2
    multicast=$3
    
    echo "rib_add_egp_table4" $*
    XRL="finder://$RIB_TARGET/rib/0.1/add_egp_table4"
    XRL_ARGS="?protocol:txt=$protocol&unicast:bool=$unicast&multicast:bool=$multicast"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_add_egp_table6()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: rib_add_egp_table6 <protocol:txt> <unicast:bool> <multicast:bool>"
	exit 1
    fi
    protocol=$1
    unicast=$2
    multicast=$3
    
    echo "rib_add_egp_table6" $*
    XRL="finder://$RIB_TARGET/rib/0.1/add_egp_table6"
    XRL_ARGS="?protocol:txt=$protocol&unicast:bool=$unicast&multicast:bool=$multicast"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_delete_egp_table4()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: rib_delete_egp_table4 <protocol:txt> <unicast:bool> <multicast:bool>"
	exit 1
    fi
    protocol=$1
    unicast=$2
    multicast=$3
    
    echo "rib_delete_egp_table4" $*
    XRL="finder://$RIB_TARGET/rib/0.1/delete_egp_table4"
    XRL_ARGS="?protocol:txt=$protocol&unicast:bool=$unicast&multicast:bool=$multicast"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_delete_egp_table6()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: rib_delete_egp_table6 <protocol:txt> <unicast:bool> <multicast:bool>"
	exit 1
    fi
    protocol=$1
    unicast=$2
    multicast=$3
    
    echo "rib_delete_egp_table6" $*
    XRL="finder://$RIB_TARGET/rib/0.1/delete_egp_table6"
    XRL_ARGS="?protocol:txt=$protocol&unicast:bool=$unicast&multicast:bool=$multicast"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_add_route4()
{
    if [ $# -lt 6 ] ; then
	echo "Usage: rib_add_route4 <protocol:txt> <unicast:bool> <multicast:bool> <network:ipv4net> <nexthop:ipv4> <metric:u32>"
	exit 1
    fi
    protocol=$1
    unicast=$2
    multicast=$3
    network=$4
    nexthop=$5
    metric=$6
    
    echo "rib_add_route4" $*
    XRL="finder://$RIB_TARGET/rib/0.1/add_route4"
    XRL_ARGS="?protocol:txt=$protocol&unicast:bool=$unicast&multicast:bool=$multicast&network:ipv4net=$network&nexthop:ipv4=$nexthop&metric:u32=$metric"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_add_route6()
{
    if [ $# -lt 6 ] ; then
	echo "Usage: rib_add_route6 <protocol:txt> <unicast:bool> <multicast:bool> <network:ipv6net> <nexthop:ipv6> <metric:u32>"
	exit 1
    fi
    protocol=$1
    unicast=$2
    multicast=$3
    network=$4
    nexthop=$5
    metric=$6
    
    echo "rib_add_route6" $*
    XRL="finder://$RIB_TARGET/rib/0.1/add_route6"
    XRL_ARGS="?protocol:txt=$protocol&unicast:bool=$unicast&multicast:bool=$multicast&network:ipv6net=$network&nexthop:ipv6=$nexthop&metric:u32=$metric"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_replace_route4()
{
    if [ $# -lt 6 ] ; then
	echo "Usage: rib_replace_route4 <protocol:txt> <unicast:bool> <multicast:bool> <network:ipv4net> <nexthop:ipv4> <metric:u32>"
	exit 1
    fi
    protocol=$1
    unicast=$2
    multicast=$3
    network=$4
    nexthop=$5
    metric=$6
    
    echo "rib_replace_route4" $*
    XRL="finder://$RIB_TARGET/rib/0.1/replace_route4"
    XRL_ARGS="?protocol:txt=$protocol&unicast:bool=$unicast&multicast:bool=$multicast&network:ipv4net=$network&nexthop:ipv4=$nexthop&metric:u32=$metric"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_replace_route6()
{
    if [ $# -lt 6 ] ; then
	echo "Usage: rib_replace_route6 <protocol:txt> <unicast:bool> <multicast:bool> <network:ipv6net> <nexthop:ipv6> <metric:u32>"
	exit 1
    fi
    protocol=$1
    unicast=$2
    multicast=$3
    network=$4
    nexthop=$5
    metric=$6
    
    echo "rib_replace_route6" $*
    XRL="finder://$RIB_TARGET/rib/0.1/replace_route6"
    XRL_ARGS="?protocol:txt=$protocol&unicast:bool=$unicast&multicast:bool=$multicast&network:ipv6net=$network&nexthop:ipv6=$nexthop&metric:u32=$metric"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_delete_route4()
{
    if [ $# -lt 4 ] ; then
	echo "Usage: rib_delete_route4 <protocol:txt> <unicast:bool> <multicast:bool> <network:ipv4net>"
	exit 1
    fi
    protocol=$1
    unicast=$2
    multicast=$3
    network=$4
    
    echo "rib_delete_route4" $*
    XRL="finder://$RIB_TARGET/rib/0.1/delete_route4"
    XRL_ARGS="?protocol:txt=$protocol&unicast:bool=$unicast&multicast:bool=$multicast&network:ipv4net=$network"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_delete_route6()
{
    if [ $# -lt 4 ] ; then
	echo "Usage: rib_delete_route6 <protocol:txt> <unicast:bool> <multicast:bool> <network:ipv6net>"
	exit 1
    fi
    protocol=$1
    unicast=$2
    multicast=$3
    network=$4
    
    echo "rib_delete_route6" $*
    XRL="finder://$RIB_TARGET/rib/0.1/delete_route6"
    XRL_ARGS="?protocol:txt=$protocol&unicast:bool=$unicast&multicast:bool=$multicast&network:ipv6net=$network"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_lookup_route4()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: rib_lookup_route4 <addr:ipv4> <unicast:bool> <multicast:bool>"
	exit 1
    fi
    addr=$1
    unicast=$2
    multicast=$3
    
    echo "rib_lookup_route4" $*
    XRL="finder://$RIB_TARGET/rib/0.1/lookup_route4"
    XRL_ARGS="?addr:ipv4=$addr&unicast:bool=$unicast&multicast:bool=$multicast"
    call_xrl -r 0 $XRL$XRL_ARGS
}

rib_lookup_route6()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: rib_lookup_route6 <addr:ipv6> <unicast:bool> <multicast:bool>"
	exit 1
    fi
    addr=$1
    unicast=$2
    multicast=$3
    
    echo "rib_lookup_route6" $*
    XRL="finder://$RIB_TARGET/rib/0.1/lookup_route6"
    XRL_ARGS="?addr:ipv6=$addr&unicast:bool=$unicast&multicast:bool=$multicast"
    call_xrl -r 0 $XRL$XRL_ARGS
}
