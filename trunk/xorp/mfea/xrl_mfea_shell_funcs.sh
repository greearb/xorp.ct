#!/bin/sh

#
# $XORP: xorp/mfea/xrl_mfea_shell_funcs.sh,v 1.7 2002/12/08 11:02:27 pavlin Exp $
#

#
# Library of functions to sent XRLs to a running MFEA process.
#

. ../mfea/xrl_shell_lib.sh

MFEA_TARGET="MFEA_4"

mfea_enable_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mfea_enable_vif <vif_name>"
	exit 1
    fi
    vif_name=$1
    
    echo "mfea_enable_vif" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/enable_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mfea_disable_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mfea_disable_vif <vif_name>"
	exit 1
    fi
    vif_name=$1
    
    echo "mfea_disable_vif" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/disable_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mfea_start_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mfea_start_vif <vif_name>"
	exit 1
    fi
    vif_name=$1
    
    echo "mfea_start_vif" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/start_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mfea_stop_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: mfea_stop_vif <vif_name>"
	exit 1
    fi
    vif_name=$1
    
    echo "mfea_stop_vif" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/stop_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mfea_enable_all_vifs()
{
    echo "mfea_enable_all_vifs" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/enable_all_vifs"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mfea_disable_all_vifs()
{
    echo "mfea_disable_all_vifs" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/disable_all_vifs"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mfea_start_all_vifs()
{
    echo "mfea_start_all_vifs" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/start_all_vifs"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mfea_stop_all_vifs()
{
    echo "mfea_stop_all_vifs" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/stop_all_vifs"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mfea_enable_mfea()
{
    echo "mfea_enable_mfea" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/enable_mfea"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mfea_disable_mfea()
{
    echo "mfea_disable_mfea" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/disable_mfea"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mfea_enable_cli()
{
    echo "mfea_enable_cli" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/enable_cli"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mfea_disable_cli()
{
    echo "mfea_disable_cli" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/disable_cli"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mfea_start_mfea()
{
    echo "mfea_start_mfea" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/start_mfea"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mfea_stop_mfea()
{
    echo "mfea_stop_mfea" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/stop_mfea"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mfea_start_cli()
{
    echo "mfea_start_cli" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/start_cli"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mfea_stop_cli()
{
    echo "mfea_stop_cli" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/stop_cli"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mfea_enable_log_trace()
{
    echo "mfea_enable_log_trace" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/enable_log_trace"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}

mfea_disable_log_trace()
{
    echo "mfea_disable_log_trace" $*
    XRL="finder://$MFEA_TARGET/mfea/0.1/disable_log_trace"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS fail:bool = false
}
