#!/bin/sh

#
# $XORP: xorp/pim/xrl_rib_shell_funcs.sh,v 1.8 2004/05/20 23:45:45 pavlin Exp $
#

#
# Library of functions to sent XRLs to a running RIB process.
#

#
# TODO: this file is temporary in the "xorp/pim" directory
#

# Conditionally set ${srcdir} if it wasn't assigned (e.g., by `gmake check`)
if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi

. ${srcdir}/../utils/xrl_shell_lib.sh

#
# Conditionally set the target name
#
IP_VERSION=${IP_VERSION:?"IP_VERSION undefined. Must be defined to either IPV4 or IPV6"}
case "${IP_VERSION}" in
	IPV4)
		RIB_TARGET=${RIB_TARGET:="RIB"}
		;;
	IPV6)
		RIB_TARGET=${RIB_TARGET:="RIB"}
		;;
	*)
		echo "Error: invalid IP_VERSION = ${IP_VERSION}. Must be either IPV4 or IPV6"
		exit 1
		;;
esac


rib_enable_rib()
{
    echo "rib_enable_rib" $*
    XRL="finder://$RIB_TARGET/rib/0.1/enable_rib"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

rib_disable_rib()
{
    echo "rib_disable_rib" $*
    XRL="finder://$RIB_TARGET/rib/0.1/disable_rib"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

rib_start_rib()
{
    echo "rib_start_rib" $*
    XRL="finder://$RIB_TARGET/rib/0.1/start_rib"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

rib_stop_rib()
{
    echo "rib_stop_rib" $*
    XRL="finder://$RIB_TARGET/rib/0.1/stop_rib"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}
