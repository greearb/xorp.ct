#!/bin/sh

#
# $XORP: xorp/fea/xrl_mfea_shell_funcs.sh,v 1.8 2003/12/20 01:43:34 pavlin Exp $
#

#
# Library of functions to sent XRLs to a running MFEA process.
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
		MFEA_TARGET=${MFEA_TARGET:="MFEA_4"}
		;;
	IPV6)
		MFEA_TARGET=${MFEA_TARGET:="MFEA_6"}
		;;
	*)
		echo "Error: invalid IP_VERSION = ${IP_VERSION}. Must be either IPV4 or IPV6"
		exit 1
		;;
esac

mfea_have_multicast_routing4()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: mfea_have_multicast_routing4"
	exit 1
    fi
    
    echo "mfea_have_multicast_routing4" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/have_multicast_routing4"
    XRL_ARGS=""
    call_xrl_wrapper -p result:bool $XRL$XRL_ARGS
}

mfea_have_multicast_routing6()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: mfea_have_multicast_routing6"
	exit 1
    fi
    
    echo "mfea_have_multicast_routing6" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/have_multicast_routing6"
    XRL_ARGS=""
    call_xrl_wrapper -p result:bool $XRL$XRL_ARGS
}

mfea_enable_vif()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: mfea_enable_vif <vif_name:txt> <enable:bool>"
	exit 1
    fi
    vif_name=$1
    enable=$2
    
    echo "mfea_enable_vif" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/enable_vif"
    XRL_ARGS="?vif_name:txt=$vif_name&enable:bool=$enable"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mfea_start_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mfea_start_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "mfea_start_vif" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/start_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mfea_stop_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mfea_stop_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "mfea_stop_vif" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/stop_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mfea_enable_all_vifs()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mfea_enable_all_vifs <enable:bool>"
	exit 1
    fi
    enable=$1

    echo "mfea_enable_all_vifs" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/enable_all_vifs"
    XRL_ARGS="?enable:bool=$enable"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mfea_start_all_vifs()
{
    echo "mfea_start_all_vifs" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/start_all_vifs"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mfea_stop_all_vifs()
{
    echo "mfea_stop_all_vifs" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/stop_all_vifs"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mfea_enable_mfea()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mfea_enable_mfea <enable:bool>"
	exit 1
    fi
    enable=$1

    echo "mfea_enable_mfea" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/enable_mfea"
    XRL_ARGS="?enable:bool=$enable"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mfea_start_mfea()
{
    echo "mfea_start_mfea" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/start_mfea"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mfea_stop_mfea()
{
    echo "mfea_stop_mfea" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/stop_mfea"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mfea_enable_cli()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mfea_enable_cli <enable:bool>"
	exit 1
    fi
    enable=$1

    echo "mfea_enable_cli" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/enable_cli"
    XRL_ARGS="?enable:bool=$enable"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mfea_start_cli()
{
    echo "mfea_start_cli" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/start_cli"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mfea_stop_cli()
{
    echo "mfea_stop_cli" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/stop_cli"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mfea_log_trace_all()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mfea_log_trace_all <enable:bool>"
	exit 1
    fi
    enable=$1

    echo "mfea_log_trace_all" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/log_trace_all"
    XRL_ARGS="?enable:bool=$enable"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

mfea_get_mrib_table_default_metric_preference()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: mfea_get_mrib_table_default_metric_preference"
	exit 1
    fi
    
    echo "mfea_get_mrib_table_default_metric_preference" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/get_mrib_table_default_metric_preference"
    XRL_ARGS=""
    call_xrl_wrapper -p metric_preference:u32 $XRL$XRL_ARGS
}

mfea_set_mrib_table_default_metric_preference()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mfea_set_mrib_table_default_metric_preference <metric_preference:u32>"
	exit 1
    fi
    metric_preference=$1
    
    echo "mfea_set_mrib_table_default_metric_preference" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/set_mrib_table_default_metric_preference"
    XRL_ARGS="?metric_preference:u32=$metric_preference"
    call_xrl_wrapper $XRL$XRL_ARGS
}

mfea_reset_mrib_table_default_metric_preference()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: mfea_reset_mrib_table_default_metric_preference"
	exit 1
    fi
    
    echo "mfea_reset_mrib_table_default_metric_preference" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/reset_mrib_table_default_metric_preference"
    XRL_ARGS=""
    call_xrl_wrapper $XRL$XRL_ARGS
}

mfea_get_mrib_table_default_metric()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: mfea_get_mrib_table_default_metric"
	exit 1
    fi
    
    echo "mfea_get_mrib_table_default_metric" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/get_mrib_table_default_metric"
    XRL_ARGS=""
    call_xrl_wrapper -p metric:u32 $XRL$XRL_ARGS
}

mfea_set_mrib_table_default_metric()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mfea_set_mrib_table_default_metric <metric:u32>"
	exit 1
    fi
    metric=$1
    
    echo "mfea_set_mrib_table_default_metric" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/set_mrib_table_default_metric"
    XRL_ARGS="?metric:u32=$metric"
    call_xrl_wrapper $XRL$XRL_ARGS
}

mfea_reset_mrib_table_default_metric()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: mfea_reset_mrib_table_default_metric"
	exit 1
    fi
    
    echo "mfea_reset_mrib_table_default_metric" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/reset_mrib_table_default_metric"
    XRL_ARGS=""
    call_xrl_wrapper $XRL$XRL_ARGS
}
