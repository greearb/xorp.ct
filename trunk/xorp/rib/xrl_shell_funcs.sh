#!/bin/sh

#
# $XORP: xorp/rib/xrl_shell_funcs.sh,v 1.8 2004/08/03 05:02:56 pavlin Exp $
#

CALLXRL=${CALLXRL:-../libxipc/call_xrl}

make_rib_errors_fatal()
{
    echo -n "make_rib_errors_fatal" $1
    $CALLXRL "finder://rib/rib/0.1/make_errors_fatal"
}

add_igp_table4()
{
    echo -n "add_igp_table4" $*
    
    $CALLXRL "finder://rib/rib/0.1/add_igp_table4?protocol:txt=$1&target_class:txt=$2&target_instance:txt=$3&unicast:bool=$4&multicast:bool=$5"
}

delete_igp_table4()
{
    echo -n "delete_igp_table4" $*
    
    $CALLXRL "finder://rib/rib/0.1/delete_igp_table4?protocol:txt=$1&target_class:txt=$2&target_instance:txt=$3&unicast:bool=$4&multicast:bool=$5"
}

new_vif()
{
    echo -n "new_vif" $1
    $CALLXRL "finder://rib/rib/0.1/new_vif?name:txt=$1"
}

add_vif_addr4()
{
    echo -n "add_vif_addr4" $*
    $CALLXRL "finder://rib/rib/0.1/add_vif_addr4?name:txt=$1&addr:ipv4=$2&subnet:ipv4net=$3"
}

add_vif_addr6()
{
    echo -n "add_vif_addr6" $*
    $CALLXRL "finder://rib/rib/0.1/add_vif_addr6?name:txt=$1&addr:ipv6=$2&subnet:ipv6net=$3"
}

add_route4()
{
    echo -n "add_route4" $*
    $CALLXRL "finder://rib/rib/0.1/add_route4?protocol:txt=$1&unicast:bool=$2&multicast:bool=$3&network:ipv4net=$4&nexthop:ipv4=$5&metric:u32=$6&policytags:list=$7"
}

add_route6()
{
    echo -n "add_route6" $*
    $CALLXRL "finder://rib/rib/0.1/add_route6?protocol:txt=$1&unicast:bool=$2&multicast:bool=$3&network:ipv6net=$4&nexthop:ipv6=$5&metric:u32=$6&policytags:list=$7"
}

delete_route4()
{
    echo -n "delete_route4" $*
    $CALLXRL "finder://rib/rib/0.1/delete_route4?protocol:txt=$1&unicast:bool=$2&multicast:bool=$3&network:ipv4net=$4"
}

delete_route6()
{
    echo -n "delete_route6" $*
    $CALLXRL "finder://rib/rib/0.1/delete_route6?protocol:txt=$1&unicast:bool=$2&multicast:bool=$3&network:ipv6net=$4"
}

lookup_route_by_dest4()
{
    echo -n "lookup_route_by_dest4" $*
    $CALLXRL "finder://rib/rib/0.1/lookup_route_by_dest4?addr:ipv4=$1&unicast:bool=$2&multicast:bool=$3"
}

# We have arguments.
if [ $# != 0 ]
then
    $*
fi

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
