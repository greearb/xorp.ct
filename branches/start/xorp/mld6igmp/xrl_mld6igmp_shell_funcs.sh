#!/bin/sh

#
# $XORP: xorp/mld6igmp/xrl_mld6igmp_shell_funcs.sh,v 1.8 2002/12/08 11:02:36 pavlin Exp $
#

#
# Library of functions to sent XRLs to a running MLD6IGMP process.
#

. ../mfea/xrl_shell_lib.sh

MLD6IGMP_TARGET="IGMP"

mld6igmp_enable_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_enable_vif <vif_name>"
	exit 1
    fi
    vif_name=$1
    
    echo "mld6igmp_enable_vif" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/enable_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mld6igmp_disable_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_disable_vif <vif_name>"
	exit 1
    fi
    vif_name=$1
    
    echo "mld6igmp_disable_vif" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/disable_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mld6igmp_start_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_start_vif <vif_name>"
	exit 1
    fi
    vif_name=$1
    
    echo "mld6igmp_start_vif" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/start_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mld6igmp_stop_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mld6igmp_stop_vif <vif_name>"
	exit 1
    fi
    vif_name=$1
    
    echo "mld6igmp_stop_vif" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/stop_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mld6igmp_enable_all_vifs()
{
    echo "mld6igmp_enable_all_vifs" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/enable_all_vifs"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mld6igmp_disable_all_vifs()
{
    echo "mld6igmp_disable_all_vifs" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/disable_all_vifs"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mld6igmp_start_all_vifs()
{
    echo "mld6igmp_start_all_vifs" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/start_all_vifs"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mld6igmp_stop_all_vifs()
{
    echo "mld6igmp_stop_all_vifs" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/stop_all_vifs"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mld6igmp_is_vif_setup_completed()
{
    echo "mld6igmp_is_vif_setup_completed" $*
    XRL="finder://$MLD6IGMP_TARGET/mfea_client/0.1/is_vif_setup_completed"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS is_completed:bool = true
}

mld6igmp_enable_mld6igmp()
{
    echo "mld6igmp_enable_mld6igmp" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/enable_mld6igmp"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mld6igmp_disable_mld6igmp()
{
    echo "mld6igmp_disable_mld6igmp" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/disable_mld6igmp"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mld6igmp_enable_cli()
{
    echo "mld6igmp_enable_cli" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/enable_cli"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mld6igmp_disable_cli()
{
    echo "mld6igmp_disable_cli" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/disable_cli"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mld6igmp_start_mld6igmp()
{
    echo "mld6igmp_start_mld6igmp" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/start_mld6igmp"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mld6igmp_stop_mld6igmp()
{
    echo "mld6igmp_stop_mld6igmp" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/stop_mld6igmp"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mld6igmp_start_cli()
{
    echo "mld6igmp_start_cli" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/start_cli"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mld6igmp_stop_cli()
{
    echo "mld6igmp_stop_cli" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/stop_cli"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mld6igmp_enable_log_trace()
{
    echo "mld6igmp_enable_log_trace" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/enable_log_trace"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mld6igmp_disable_log_trace()
{
    echo "mld6igmp_disable_log_trace" $*
    XRL="finder://$MLD6IGMP_TARGET/mld6igmp/0.1/disable_log_trace"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}
