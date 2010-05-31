#!/bin/sh

#
# $XORP: xorp/mld6igmp/xrl_mld6igmp_shell_funcs.sh,v 1.15 2005/06/01 09:34:01 pavlin Exp $
#

#
# Library of functions to sent XRLs to a running MLD6IGMP process.
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
		MLD6IGMP_TARGET=${MLD6IGMP_TARGET:="IGMP"}
		;;
	IPV6)
		MLD6IGMP_TARGET=${MLD6IGMP_TARGET:="MLD"}
		;;
	*)
		echo "Error: invalid IP_VERSION = ${IP_VERSION}. Must be either IPV4 or IPV6"
		exit 1
		;;
esac


mld6igmp_enable_vif()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: mld6igmp_enable_vif <vif_name:txt> <enable:bool>"
	exit 1
    fi
    vif_name=$1
    enable=$2
    
    echo "mld6igmp_enable_vif" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/enable_vif"
    XRL_ARGS="?vif_name:txt=$vif_name&enable:bool=$enable"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mld6igmp_start_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_start_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "mld6igmp_start_vif" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/start_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mld6igmp_stop_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_stop_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "mld6igmp_stop_vif" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/stop_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mld6igmp_enable_all_vifs()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_enable_all_vifs <enable:bool>"
	exit 1
    fi
    enable=$1

    echo "mld6igmp_enable_all_vifs" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/enable_all_vifs"
    XRL_ARGS="?enable:bool=$enable"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mld6igmp_start_all_vifs()
{
    echo "mld6igmp_start_all_vifs" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/start_all_vifs"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mld6igmp_stop_all_vifs()
{
    echo "mld6igmp_stop_all_vifs" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/stop_all_vifs"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mld6igmp_enable_mld6igmp()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_enable_mld6igmp <enable:bool>"
	exit 1
    fi
    enable=$1

    echo "mld6igmp_enable_mld6igmp" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/enable_mld6igmp"
    XRL_ARGS="?enable:bool=$enable"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mld6igmp_start_mld6igmp()
{
    echo "mld6igmp_start_mld6igmp" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/start_mld6igmp"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mld6igmp_stop_mld6igmp()
{
    echo "mld6igmp_stop_mld6igmp" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/stop_mld6igmp"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mld6igmp_enable_cli()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_enable_cli <enable:bool>"
	exit 1
    fi
    enable=$1

    echo "mld6igmp_enable_cli" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/enable_cli"
    XRL_ARGS="?enable:bool=$enable"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mld6igmp_start_cli()
{
    echo "mld6igmp_start_cli" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/start_cli"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mld6igmp_stop_cli()
{
    echo "mld6igmp_stop_cli" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/stop_cli"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

#
# Configure MLD6IGMP interface-related metrics.
#
mld6igmp_get_vif_proto_version()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_get_vif_proto_version <vif_name:txt>"
	exit 1
    fi
    vif_name=$1

    echo "mld6igmp_get_vif_proto_version" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/get_vif_proto_version"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p proto_version:u32 $XRL$XRL_ARGS
}

mld6igmp_set_vif_proto_version()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: mld6igmp_set_vif_proto_version <vif_name:txt> <proto_version:u32>"
	exit 1
    fi
    vif_name=$1
    proto_version=$2
    
    echo "mld6igmp_set_vif_proto_version" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/set_vif_proto_version"
    XRL_ARGS="?vif_name:txt=$vif_name&proto_version:u32=$proto_version"
    call_xrl_wrapper $XRL$XRL_ARGS
}

mld6igmp_reset_vif_proto_version()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_reset_vif_proto_version <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "mld6igmp_reset_vif_proto_version" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/reset_vif_proto_version"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper $XRL$XRL_ARGS
}

mld6igmp_get_vif_ip_router_alert_option_check()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_get_vif_ip_router_alert_option_check <vif_name:txt>"
	exit 1
    fi
    vif_name=$1

    echo "mld6igmp_get_vif_ip_router_alert_option_check" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/get_vif_ip_router_alert_option_check"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p enabled:bool $XRL$XRL_ARGS
}

mld6igmp_set_vif_ip_router_alert_option_check()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: mld6igmp_set_vif_ip_router_alert_option_check <vif_name:txt> <enable:bool>"
	exit 1
    fi
    vif_name=$1
    enable=$2
    
    echo "mld6igmp_set_vif_ip_router_alert_option_check" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/set_vif_ip_router_alert_option_check"
    XRL_ARGS="?vif_name:txt=$vif_name&enable:bool=$enable"
    call_xrl_wrapper $XRL$XRL_ARGS
}

mld6igmp_reset_vif_ip_router_alert_option_check()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_reset_vif_ip_router_alert_option_check <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "mld6igmp_reset_vif_ip_router_alert_option_check" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/reset_vif_ip_router_alert_option_check"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper $XRL$XRL_ARGS
}

mld6igmp_get_vif_query_interval()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_get_vif_query_interval <vif_name:txt>"
	exit 1
    fi
    vif_name=$1

    echo "mld6igmp_get_vif_query_interval" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/get_vif_query_interval"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p interval_sec:u32 -p interval_usec:u32 $XRL$XRL_ARGS
}

mld6igmp_set_vif_query_interval()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: mld6igmp_set_vif_query_interval <vif_name:txt> <interval_sec:u32> <interval_usec:u32>"
	exit 1
    fi
    vif_name=$1
    interval_sec=$2
    interval_usec=$3
    
    echo "mld6igmp_set_vif_query_interval" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/set_vif_query_interval"
    XRL_ARGS="?vif_name:txt=$vif_name&interval_sec:u32=$interval_sec&interval_usec:u32=$interval_usec"
    call_xrl_wrapper $XRL$XRL_ARGS
}

mld6igmp_reset_vif_query_interval()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_reset_vif_query_interval <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "mld6igmp_reset_vif_query_interval" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/reset_vif_query_interval"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper $XRL$XRL_ARGS
}

mld6igmp_get_vif_query_last_member_interval()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_get_vif_query_last_member_interval <vif_name:txt>"
	exit 1
    fi
    vif_name=$1

    echo "mld6igmp_get_vif_query_last_member_interval" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/get_vif_query_last_member_interval"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p interval_sec:u32 -p interval_usec:u32 $XRL$XRL_ARGS
}

mld6igmp_set_vif_query_last_member_interval()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: mld6igmp_set_vif_query_last_member_interval <vif_name:txt> <interval_sec:u32> <interval_usec:u32>"
	exit 1
    fi
    vif_name=$1
    interval_sec=$2
    interval_usec=$3
    
    echo "mld6igmp_set_vif_query_last_member_interval" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/set_vif_query_last_member_interval"
    XRL_ARGS="?vif_name:txt=$vif_name&interval_sec:u32=$interval_sec&interval_usec:u32=$interval_usec"
    call_xrl_wrapper $XRL$XRL_ARGS
}

mld6igmp_reset_vif_query_last_member_interval()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_reset_vif_query_last_member_interval <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "mld6igmp_reset_vif_query_last_member_interval" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/reset_vif_query_last_member_interval"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper $XRL$XRL_ARGS
}

mld6igmp_get_vif_query_response_interval()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_get_vif_query_response_interval <vif_name:txt>"
	exit 1
    fi
    vif_name=$1

    echo "mld6igmp_get_vif_query_response_interval" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/get_vif_query_response_interval"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p interval_sec:u32 -p interval_usec:u32 $XRL$XRL_ARGS
}

mld6igmp_set_vif_query_response_interval()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: mld6igmp_set_vif_query_response_interval <vif_name:txt> <interval_sec:u32> <interval_usec:u32>"
	exit 1
    fi
    vif_name=$1
    interval_sec=$2
    interval_usec=$3
    
    echo "mld6igmp_set_vif_query_response_interval" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/set_vif_query_response_interval"
    XRL_ARGS="?vif_name:txt=$vif_name&interval_sec:u32=$interval_sec&interval_usec:u32=$interval_usec"
    call_xrl_wrapper $XRL$XRL_ARGS
}

mld6igmp_reset_vif_query_response_interval()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_reset_vif_query_response_interval <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "mld6igmp_reset_vif_query_response_interval" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/reset_vif_query_response_interval"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper $XRL$XRL_ARGS
}

mld6igmp_get_vif_robust_count()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_get_vif_robust_count <vif_name:txt>"
	exit 1
    fi
    vif_name=$1

    echo "mld6igmp_get_vif_robust_count" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/get_vif_robust_count"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p robust_count:u32 $XRL$XRL_ARGS
}

mld6igmp_set_vif_robust_count()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: mld6igmp_set_vif_robust_count <vif_name:txt> <robust_count:u32>"
	exit 1
    fi
    vif_name=$1
    robust_count=$2
    
    echo "mld6igmp_set_vif_robust_count" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/set_vif_robust_count"
    XRL_ARGS="?vif_name:txt=$vif_name&robust_count:u32=$robust_count"
    call_xrl_wrapper $XRL$XRL_ARGS
}

mld6igmp_reset_vif_robust_count()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_reset_vif_robust_count <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "mld6igmp_reset_vif_robust_count" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/reset_vif_robust_count"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper $XRL$XRL_ARGS
}

mld6igmp_log_trace_all()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_log_trace_all <enable:bool>"
	exit 1
    fi
    enable=$1

    echo "mld6igmp_log_trace_all" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/log_trace_all"
    XRL_ARGS="?enable:bool=$enable"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}
