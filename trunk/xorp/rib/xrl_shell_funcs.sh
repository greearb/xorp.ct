#!/bin/sh

#
# $XORP: xorp/rib/xrl_shell_funcs.sh,v 1.2 2003/03/10 20:26:48 mjh Exp $
#

CALLXRL=${CALLXRL:-../libxipc/call_xrl}

no_fea()
{
    echo -n "no_fea" $1
    $CALLXRL "finder://rib/rib/0.1/no_fea"
}

make_rib_errors_fatal()
{
    echo -n "make_rib_errors_fatal" $1
    $CALLXRL "finder://rib/rib/0.1/make_errors_fatal"
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

add_route4()
{
    echo -n "add_route4" $*
    $CALLXRL "finder://rib/rib/0.1/add_route4?protocol:txt=$1&unicast:bool=$2&multicast:bool=$3&network:ipv4net=$4&nexthop:ipv4=$5&metric:u32=$6"
}

delete_route4()
{
    echo -n "delete_route4" $*
    $CALLXRL "finder://rib/rib/0.1/delete_route4?protocol:txt=$1&unicast:bool=$2&multicast:bool=$3&network:ipv4net=$4"
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
