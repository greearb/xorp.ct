#!/bin/sh

#
# $XORP: xorp/pim/xrl_pim_shell_funcs.sh,v 1.10 2003/06/03 18:52:19 pavlin Exp $
#

#
# Library of functions to sent XRLs to a running PIM process.
#

. ../utils/xrl_shell_lib.sh

# Conditionally set the target name
PIM_TARGET=${PIM_TARGET:="PIMSM_4"}

pim_enable_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_enable_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_enable_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/enable_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_disable_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_disable_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_disable_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/disable_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_start_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_start_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_start_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/start_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_stop_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_stop_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_stop_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/stop_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_enable_all_vifs()
{
    echo "pim_enable_all_vifs" $*
    XRL="finder://$PIM_TARGET/pim/0.1/enable_all_vifs"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_disable_all_vifs()
{
    echo "pim_disable_all_vifs" $*
    XRL="finder://$PIM_TARGET/pim/0.1/disable_all_vifs"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_start_all_vifs()
{
    echo "pim_start_all_vifs" $*
    XRL="finder://$PIM_TARGET/pim/0.1/start_all_vifs"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_stop_all_vifs()
{
    echo "pim_stop_all_vifs" $*
    XRL="finder://$PIM_TARGET/pim/0.1/stop_all_vifs"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_is_vif_setup_completed()
{
    echo "pim_is_vif_setup_completed" $*
    XRL="finder://$PIM_TARGET/mfea_client/0.1/is_vif_setup_completed"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS is_completed:bool = true
}

pim_enable_cli()
{
    echo "pim_enable_cli" $*
    XRL="finder://$PIM_TARGET/pim/0.1/enable_cli"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_disable_cli()
{
    echo "pim_disable_cli" $*
    XRL="finder://$PIM_TARGET/pim/0.1/disable_cli"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_start_cli()
{
    echo "pim_start_cli" $*
    XRL="finder://$PIM_TARGET/pim/0.1/start_cli"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_stop_cli()
{
    echo "pim_stop_cli" $*
    XRL="finder://$PIM_TARGET/pim/0.1/stop_cli"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARG
}

pim_enable_pim()
{
    echo "pim_enable_pim" $*
    XRL="finder://$PIM_TARGET/pim/0.1/enable_pim"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_disable_pim()
{
    echo "pim_disable_pim" $*
    XRL="finder://$PIM_TARGET/pim/0.1/disable_pim"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_start_pim()
{
    echo "pim_start_pim" $*
    XRL="finder://$PIM_TARGET/pim/0.1/start_pim"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_stop_pim()
{
    echo "pim_stop_pim" $*
    XRL="finder://$PIM_TARGET/pim/0.1/stop_pim"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_enable_bsr()
{
    echo "pim_enable_bsr" $*
    XRL="finder://$PIM_TARGET/pim/0.1/enable_bsr"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_disable_bsr()
{
    echo "pim_disable_bsr" $*
    XRL="finder://$PIM_TARGET/pim/0.1/disable_bsr"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_start_bsr()
{
    echo "pim_start_bsr" $*
    XRL="finder://$PIM_TARGET/pim/0.1/start_bsr"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_stop_bsr()
{
    echo "pim_stop_bsr" $*
    XRL="finder://$PIM_TARGET/pim/0.1/stop_bsr"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

#
# Add/delete scope zone configuration.
#
pim_add_config_scope_zone_by_vif_name4()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_add_config_scope_zone_by_vif_name4 <scope_zone_id:ipv4net> <vif_name:txt>"
	exit 1
    fi
    scope_zone_id=$1
    vif_name=$2
    
    echo "pim_add_config_scope_zone_by_vif_name4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_config_scope_zone_by_vif_name4"
    XRL_ARGS="?scope_zone_id:ipv4net=$scope_zone_id&vif_name:txt=$vif_name"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_add_config_scope_zone_by_vif_name6()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_add_config_scope_zone_by_vif_name6 <scope_zone_id:ipv6net> <vif_name:txt>"
	exit 1
    fi
    scope_zone_id=$1
    vif_name=$2
    
    echo "pim_add_config_scope_zone_by_vif_name6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_config_scope_zone_by_vif_name6"
    XRL_ARGS="?scope_zone_id:ipv6net=$scope_zone_id&vif_name:txt=$vif_name"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_add_config_scope_zone_by_vif_addr4()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_add_config_scope_zone_by_vif_addr4 <scope_zone_id:ipv4net> <vif_addr:ipv4>"
	exit 1
    fi
    scope_zone_id=$1
    vif_addr=$2
    
    echo "pim_add_config_scope_zone_by_vif_addr4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_config_scope_zone_by_vif_addr4"
    XRL_ARGS="?scope_zone_id:ipv4net=$scope_zone_id&vif_addr:ipv4=$vif_addr"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_add_config_scope_zone_by_vif_addr6()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_add_config_scope_zone_by_vif_addr6 <scope_zone_id:ipv6net> <vif_addr:ipv6>"
	exit 1
    fi
    scope_zone_id=$1
    vif_addr=$2
    
    echo "pim_add_config_scope_zone_by_vif_addr6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_config_scope_zone_by_vif_addr6"
    XRL_ARGS="?scope_zone_id:ipv6net=$scope_zone_id&vif_addr:ipv6=$vif_addr"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_delete_config_scope_zone_by_vif_name4()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_delete_config_scope_zone_by_vif_name4 <scope_zone_id:ipv4net> <vif_name:txt>"
	exit 1
    fi
    scope_zone_id=$1
    vif_name=$2
    
    echo "pim_delete_config_scope_zone_by_vif_name4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/delete_config_scope_zone_by_vif_name4"
    XRL_ARGS="?scope_zone_id:ipv4net=$scope_zone_id&vif_name:txt=$vif_name"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_delete_config_scope_zone_by_vif_name6()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_delete_config_scope_zone_by_vif_name6 <scope_zone_id:ipv6net> <vif_name:txt>"
	exit 1
    fi
    scope_zone_id=$1
    vif_name=$2
    
    echo "pim_delete_config_scope_zone_by_vif_name6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/delete_config_scope_zone_by_vif_name6"
    XRL_ARGS="?scope_zone_id:ipv6net=$scope_zone_id&vif_name:txt=$vif_name"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_delete_config_scope_zone_by_vif_addr4()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_delete_config_scope_zone_by_vif_addr4 <scope_zone_id:ipv4net> <vif_addr:ipv4>"
	exit 1
    fi
    scope_zone_id=$1
    vif_addr=$2
    
    echo "pim_delete_config_scope_zone_by_vif_addr4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/delete_config_scope_zone_by_vif_addr4"
    XRL_ARGS="?scope_zone_id:ipv4net=$scope_zone_id&vif_addr:ipv4=$vif_addr"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_delete_config_scope_zone_by_vif_addr6()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_delete_config_scope_zone_by_vif_addr6 <scope_zone_id:ipv6net> <vif_addr:ipv6>"
	exit 1
    fi
    scope_zone_id=$1
    vif_addr=$2
    
    echo "pim_delete_config_scope_zone_by_vif_addr6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/delete_config_scope_zone_by_vif_addr6"
    XRL_ARGS="?scope_zone_id:ipv6net=$scope_zone_id&vif_addr:ipv6=$vif_addr"
    call_xrl -r 0 $XRL$XRL_ARGS
}

#
# Add/delete Candidate-RP configuration.
#
pim_add_config_cand_bsr_by_vif_name4()
{
    if [ $# -lt 5 ] ; then
	echo "Usage: pim_add_config_cand_bsr_by_vif_name4 <scope_zone_id:ipv4net> <is_scope_zone:bool> <vif_name:txt> <bsr_priority:u32> <hash_masklen:u32>"
	exit 1
    fi
    scope_zone_id=$1
    is_scope_zone=$2
    vif_name=$3
    bsr_priority=$4
    hash_masklen=$5
    
    echo "pim_add_config_cand_bsr_by_vif_name4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_config_cand_bsr_by_vif_name4"
    XRL_ARGS="?scope_zone_id:ipv4net=$scope_zone_id&is_scope_zone:bool=$is_scope_zone&vif_name:txt=$vif_name&bsr_priority:u32=$bsr_priority&hash_masklen:u32=$hash_masklen"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_add_config_cand_bsr_by_vif_name6()
{
    if [ $# -lt 5 ] ; then
	echo "Usage: pim_add_config_cand_bsr_by_vif_name6 <scope_zone_id:ipv6net> <is_scope_zone:bool> <vif_name:txt> <bsr_priority:u32> <hash_masklen:u32>"
	exit 1
    fi
    scope_zone_id=$1
    is_scope_zone=$2
    vif_name=$3
    bsr_priority=$4
    hash_masklen=$5
    
    echo "pim_add_config_cand_bsr_by_vif_name6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_config_cand_bsr_by_vif_name6"
    XRL_ARGS="?scope_zone_id:ipv6net=$scope_zone_id&is_scope_zone:bool=$is_scope_zone&vif_name:txt=$vif_name&bsr_priority:u32=$bsr_priority&hash_masklen:u32=$hash_masklen"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_add_config_cand_bsr_by_addr4()
{
    if [ $# -lt 5 ] ; then
	echo "Usage: pim_add_config_cand_bsr_by_addr4 <scope_zone_id:ipv4net> <is_scope_zone:bool> <cand_bsr_addr:ipv4> <bsr_priority:u32> <hash_masklen:u32>"
	exit 1
    fi
    scope_zone_id=$1
    is_scope_zone=$2
    cand_bsr_addr=$3
    bsr_priority=$4
    hash_masklen=$5
    
    echo "pim_add_config_cand_bsr_by_addr4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_config_cand_bsr_by_addr4"
    XRL_ARGS="?scope_zone_id:ipv4net=$scope_zone_id&is_scope_zone:bool=$is_scope_zone&cand_bsr_addr:ipv4=$cand_bsr_addr&bsr_priority:u32=$bsr_priority&hash_masklen:u32=$hash_masklen"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_add_config_cand_bsr_by_addr6()
{
    if [ $# -lt 5 ] ; then
	echo "Usage: pim_add_config_cand_bsr_by_addr6 <scope_zone_id:ipv6net> <is_scope_zone:bool> <cand_bsr_addr:ipv6> <bsr_priority:u32> <hash_masklen:u32>"
	exit 1
    fi
    scope_zone_id=$1
    is_scope_zone=$2
    cand_bsr_addr=$3
    bsr_priority=$4
    hash_masklen=$5
    
    echo "pim_add_config_cand_bsr_by_addr6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_config_cand_bsr_by_addr6"
    XRL_ARGS="?scope_zone_id:ipv6net=$scope_zone_id&is_scope_zone:bool=$is_scope_zone&cand_bsr_addr:ipv6=$cand_bsr_addr&bsr_priority:u32=$bsr_priority&hash_masklen:u32=$hash_masklen"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_delete_config_cand_bsr4()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_delete_config_cand_bsr4 <scope_zone_id:ipv4net> <is_scope_zone:bool>"
	exit 1
    fi
    scope_zone_id=$1
    is_scope_zone=$2
    
    echo "pim_delete_config_cand_bsr4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/delete_config_cand_bsr4"
    XRL_ARGS="?scope_zone_id:ipv4net=$scope_zone_id&is_scope_zone:bool=$is_scope_zone"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_delete_config_cand_bsr6()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_delete_config_cand_bsr6 <scope_zone_id:ipv6net> <is_scope_zone:bool>"
	exit 1
    fi
    scope_zone_id=$1
    is_scope_zone=$2
    
    echo "pim_delete_config_cand_bsr6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/delete_config_cand_bsr6"
    XRL_ARGS="?scope_zone_id:ipv6net=$scope_zone_id&is_scope_zone:bool=$is_scope_zone"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_add_config_cand_rp_by_vif_name4()
{
    if [ $# -lt 5 ] ; then
	echo "Usage: pim_add_config_cand_rp_by_vif_name4 <group_prefix:ipv4net> <is_scope_zone:bool> <vif_name:txt> <rp_priority:u32> <rp_holdtime:u32>"
	exit 1
    fi
    group_prefix=$1
    is_scope_zone=$2
    vif_name=$3
    rp_priority=$4
    rp_holdtime=$5
    
    echo "pim_add_config_cand_rp_by_vif_name4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_config_cand_rp_by_vif_name4"
    XRL_ARGS="?group_prefix:ipv4net=$group_prefix&is_scope_zone:bool=$is_scope_zone&vif_name:txt=$vif_name&rp_priority:u32=$rp_priority&rp_holdtime:u32=$rp_holdtime"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_add_config_cand_rp_by_vif_name6()
{
    if [ $# -lt 5 ] ; then
	echo "Usage: pim_add_config_cand_rp_by_vif_name6 <group_prefix:ipv6net> <is_scope_zone:bool> <vif_name:txt> <rp_priority:u32> <rp_holdtime:u32>"
	exit 1
    fi
    group_prefix=$1
    is_scope_zone=$2
    vif_name=$3
    rp_priority=$4
    rp_holdtime=$5
    
    echo "pim_add_config_cand_rp_by_vif_name6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_config_cand_rp_by_vif_name6"
    XRL_ARGS="?group_prefix:ipv6net=$group_prefix&is_scope_zone:bool=$is_scope_zone&vif_name:txt=$vif_name&rp_priority:u32=$rp_priority&rp_holdtime:u32=$rp_holdtime"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_add_config_cand_rp_by_addr4()
{
    if [ $# -lt 5 ] ; then
	echo "Usage: pim_add_config_cand_rp_by_addr4 <group_prefix:ipv4net> <is_scope_zone:bool> <cand_rp_addr:ipv4> <rp_priority:u32> <rp_holdtime:u32>"
	exit 1
    fi
    group_prefix=$1
    is_scope_zone=$2
    cand_rp_addr=$3
    rp_priority=$4
    rp_holdtime=$5
    
    echo "pim_add_config_cand_rp_by_addr4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_config_cand_rp_by_addr4"
    XRL_ARGS="?group_prefix:ipv4net=$group_prefix&is_scope_zone:bool=$is_scope_zone&cand_rp_addr:ipv4=$cand_rp_addr&rp_priority:u32=$rp_priority&rp_holdtime:u32=$rp_holdtime"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_add_config_cand_rp_by_addr6()
{
    if [ $# -lt 5 ] ; then
	echo "Usage: pim_add_config_cand_rp_by_addr6 <group_prefix:ipv6net> <is_scope_zone:bool> <cand_rp_addr:ipv6> <rp_priority:u32> <rp_holdtime:u32>"
	exit 1
    fi
    group_prefix=$1
    is_scope_zone=$2
    cand_rp_addr=$3
    rp_priority=$4
    rp_holdtime=$5
    
    echo "pim_add_config_cand_rp_by_addr6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_config_cand_rp_by_addr6"
    XRL_ARGS="?group_prefix:ipv6net=$group_prefix&is_scope_zone:bool=$is_scope_zone&cand_rp_addr:ipv6=$cand_rp_addr&rp_priority:u32=$rp_priority&rp_holdtime:u32=$rp_holdtime"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_delete_config_cand_rp_by_vif_name4()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: pim_delete_config_cand_rp_by_vif_name4 <group_prefix:ipv4net> <is_scope_zone:bool> <vif_name:txt>"
	exit 1
    fi
    group_prefix=$1
    is_scope_zone=$2
    vif_name=$3
    
    echo "pim_delete_config_cand_rp_by_vif_name4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/delete_config_cand_rp_by_vif_name4"
    XRL_ARGS="?group_prefix:ipv4net=$group_prefix&is_scope_zone:bool=$is_scope_zone&vif_name:txt=$vif_name"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_delete_config_cand_rp_by_vif_name6()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: pim_delete_config_cand_rp_by_vif_name6 <group_prefix:ipv6net> <is_scope_zone:bool> <vif_name:txt>"
	exit 1
    fi
    group_prefix=$1
    is_scope_zone=$2
    vif_name=$3
    
    echo "pim_delete_config_cand_rp_by_vif_name6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/delete_config_cand_rp_by_vif_name6"
    XRL_ARGS="?group_prefix:ipv6net=$group_prefix&is_scope_zone:bool=$is_scope_zone&vif_name:txt=$vif_name"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_delete_config_cand_rp_by_addr4()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: pim_delete_config_cand_rp_by_addr4 <group_prefix:ipv4net> <is_scope_zone:bool> <cand_rp_addr:ipv4>"
	exit 1
    fi
    group_prefix=$1
    is_scope_zone=$2
    cand_rp_addr=$3
    
    echo "pim_delete_config_cand_rp_by_addr4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/delete_config_cand_rp_by_addr4"
    XRL_ARGS="?group_prefix:ipv4net=$group_prefix&is_scope_zone:bool=$is_scope_zone&cand_rp_addr:ipv4=$cand_rp_addr"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_delete_config_cand_rp_by_addr6()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: pim_delete_config_cand_rp_by_addr6 <group_prefix:ipv6net> <is_scope_zone:bool> <cand_rp_addr:ipv6>"
	exit 1
    fi
    group_prefix=$1
    is_scope_zone=$2
    cand_rp_addr=$3
    
    echo "pim_delete_config_cand_rp_by_addr6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/delete_config_cand_rp_by_addr6"
    XRL_ARGS="?group_prefix:ipv6net=$group_prefix&is_scope_zone:bool=$is_scope_zone&cand_rp_addr:ipv6=$cand_rp_addr"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_add_config_static_rp4()
{
    if [ $# -lt 4 ] ; then
	echo "Usage: pim_add_config_static_rp4 <group_prefix:ipv4net> <rp_addr:ipv4> <rp_priority:u32> <hash_masklen:u32>"
	exit 1
    fi
    group_prefix=$1
    rp_addr=$2
    rp_priority=$3
    hash_masklen=$4
    
    echo "pim_add_config_static_rp4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_config_static_rp4"
    XRL_ARGS="?group_prefix:ipv4net=$group_prefix&rp_addr:ipv4=$rp_addr&rp_priority:u32=$rp_priority&hash_masklen:u32=$hash_masklen"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_add_config_static_rp6()
{
    if [ $# -lt 4 ] ; then
	echo "Usage: pim_add_config_static_rp6 <group_prefix:ipv6net> <rp_addr:ipv6> <rp_priority:u32> <hash_masklen:u32>"
	exit 1
    fi
    group_prefix=$1
    rp_addr=$2
    rp_priority=$3
    hash_masklen=$4
    
    echo "pim_add_config_static_rp6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_config_static_rp6"
    XRL_ARGS="?group_prefix:ipv6net=$group_prefix&rp_addr:ipv6=$rp_addr&rp_priority:u32=$rp_priority&hash_masklen:u32=$hash_masklen"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_delete_config_static_rp4()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_delete_config_static_rp4 <group_prefix:ipv4net> <rp_addr:ipv4>"
	exit 1
    fi
    group_prefix=$1
    rp_addr=$2
    
    echo "pim_delete_config_static_rp4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/delete_config_static_rp4"
    XRL_ARGS="?group_prefix:ipv4net=$group_prefix&rp_addr:ipv4=$rp_addr"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_delete_config_static_rp6()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_delete_config_static_rp6 <group_prefix:ipv6net> <rp_addr:ipv6>"
	exit 1
    fi
    group_prefix=$1
    rp_addr=$2
    
    echo "pim_delete_config_static_rp6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/delete_config_static_rp6"
    XRL_ARGS="?group_prefix:ipv6net=$group_prefix&rp_addr:ipv6=$rp_addr"
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_config_static_rp_done()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_config_static_rp_done"
	exit 1
    fi
    
    echo "pim_config_static_rp_done" $*
    XRL="finder://$PIM_TARGET/pim/0.1/config_static_rp_done"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

#
# Configure PIM Hello-related metrics.
#
pim_get_vif_proto_version()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_get_vif_proto_version <vif_name:txt>"
	exit 1
    fi
    vif_name=$1

    echo "pim_get_vif_proto_version" $*
    XRL="finder://$PIM_TARGET/pim/0.1/get_vif_proto_version"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -p proto_version:u32 $XRL$XRL_ARGS
}

pim_set_vif_proto_version()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_set_vif_proto_version <vif_name:txt> <proto_version:u32>"
	exit 1
    fi
    vif_name=$1
    proto_version=$2
    
    echo "pim_set_vif_proto_version" $*
    XRL="finder://$PIM_TARGET/pim/0.1/set_vif_proto_version"
    XRL_ARGS="?vif_name:txt=$vif_name&proto_version:u32=$proto_version"
    call_xrl $XRL$XRL_ARGS
}

pim_reset_vif_proto_version()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_reset_vif_proto_version <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_reset_vif_proto_version" $*
    XRL="finder://$PIM_TARGET/pim/0.1/reset_vif_proto_version"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl $XRL$XRL_ARGS
}

pim_get_vif_hello_triggered_delay()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_get_vif_hello_triggered_delay <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_get_vif_hello_triggered_delay" $*
    XRL="finder://$PIM_TARGET/pim/0.1/get_vif_hello_triggered_delay"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -p hello_triggered_delay:u32 $XRL$XRL_ARGS
}

pim_set_vif_hello_triggered_delay()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_set_vif_hello_triggered_delay <vif_name:txt> <hello_triggered_delay:u32>"
	exit 1
    fi
    vif_name=$1
    hello_triggered_delay=$2
    
    echo "pim_set_vif_hello_triggered_delay" $*
    XRL="finder://$PIM_TARGET/pim/0.1/set_vif_hello_triggered_delay"
    XRL_ARGS="?vif_name:txt=$vif_name&hello_triggered_delay:u32=$hello_triggered_delay"
    call_xrl $XRL$XRL_ARGS
}

pim_reset_vif_hello_triggered_delay()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_reset_vif_hello_triggered_delay <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_reset_vif_hello_triggered_delay" $*
    XRL="finder://$PIM_TARGET/pim/0.1/reset_vif_hello_triggered_delay"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl $XRL$XRL_ARGS
}

pim_get_vif_hello_period()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_get_vif_hello_period <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_get_vif_hello_period" $*
    XRL="finder://$PIM_TARGET/pim/0.1/get_vif_hello_period"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -p hello_period:u32 $XRL$XRL_ARGS
}

pim_set_vif_hello_period()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_set_vif_hello_period <vif_name:txt> <hello_period:u32>"
	exit 1
    fi
    vif_name=$1
    hello_period=$2
    
    echo "pim_set_vif_hello_period" $*
    XRL="finder://$PIM_TARGET/pim/0.1/set_vif_hello_period"
    XRL_ARGS="?vif_name:txt=$vif_name&hello_period:u32=$hello_period"
    call_xrl $XRL$XRL_ARGS
}

pim_reset_vif_hello_period()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_reset_vif_hello_period <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_reset_vif_hello_period" $*
    XRL="finder://$PIM_TARGET/pim/0.1/reset_vif_hello_period"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl $XRL$XRL_ARGS
}

pim_get_vif_hello_holdtime()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_get_vif_hello_holdtime <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_get_vif_hello_holdtime" $*
    XRL="finder://$PIM_TARGET/pim/0.1/get_vif_hello_holdtime"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -p hello_holdtime:u32 $XRL$XRL_ARGS
}

pim_set_vif_hello_holdtime()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_set_vif_hello_holdtime <vif_name:txt> <hello_holdtime:u32>"
	exit 1
    fi
    vif_name=$1
    hello_holdtime=$2
    
    echo "pim_set_vif_hello_holdtime" $*
    XRL="finder://$PIM_TARGET/pim/0.1/set_vif_hello_holdtime"
    XRL_ARGS="?vif_name:txt=$vif_name&hello_holdtime:u32=$hello_holdtime"
    call_xrl $XRL$XRL_ARGS
}

pim_reset_vif_hello_holdtime()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_reset_vif_hello_holdtime <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_reset_vif_hello_holdtime" $*
    XRL="finder://$PIM_TARGET/pim/0.1/reset_vif_hello_holdtime"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl $XRL$XRL_ARGS
}

pim_get_vif_dr_priority()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_get_vif_dr_priority <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_get_vif_dr_priority" $*
    XRL="finder://$PIM_TARGET/pim/0.1/get_vif_dr_priority"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -p dr_priority:u32 $XRL$XRL_ARGS
}

pim_set_vif_dr_priority()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_set_vif_dr_priority <vif_name:txt> <dr_priority:u32>"
	exit 1
    fi
    vif_name=$1
    dr_priority=$2
    
    echo "pim_set_vif_dr_priority" $*
    XRL="finder://$PIM_TARGET/pim/0.1/set_vif_dr_priority"
    XRL_ARGS="?vif_name:txt=$vif_name&dr_priority:u32=$dr_priority"
    call_xrl $XRL$XRL_ARGS
}

pim_reset_vif_dr_priority()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_reset_vif_dr_priority <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_reset_vif_dr_priority" $*
    XRL="finder://$PIM_TARGET/pim/0.1/reset_vif_dr_priority"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl $XRL$XRL_ARGS
}

pim_get_vif_lan_delay()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_get_vif_lan_delay <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_get_vif_lan_delay" $*
    XRL="finder://$PIM_TARGET/pim/0.1/get_vif_lan_delay"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -p lan_delay:u32 $XRL$XRL_ARGS
}

pim_set_vif_lan_delay()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_set_vif_lan_delay <vif_name:txt> <lan_delay:u32>"
	exit 1
    fi
    vif_name=$1
    lan_delay=$2
    
    echo "pim_set_vif_lan_delay" $*
    XRL="finder://$PIM_TARGET/pim/0.1/set_vif_lan_delay"
    XRL_ARGS="?vif_name:txt=$vif_name&lan_delay:u32=$lan_delay"
    call_xrl $XRL$XRL_ARGS
}

pim_reset_vif_lan_delay()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_reset_vif_lan_delay <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_reset_vif_lan_delay" $*
    XRL="finder://$PIM_TARGET/pim/0.1/reset_vif_lan_delay"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl $XRL$XRL_ARGS
}

pim_get_vif_override_interval()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_get_vif_override_interval <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_get_vif_override_interval" $*
    XRL="finder://$PIM_TARGET/pim/0.1/get_vif_override_interval"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -p override_interval:u32 $XRL$XRL_ARGS
}

pim_set_vif_override_interval()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_set_vif_override_interval <vif_name:txt> <override_interval:u32>"
	exit 1
    fi
    vif_name=$1
    override_interval=$2
    
    echo "pim_set_vif_override_interval" $*
    XRL="finder://$PIM_TARGET/pim/0.1/set_vif_override_interval"
    XRL_ARGS="?vif_name:txt=$vif_name&override_interval:u32=$override_interval"
    call_xrl $XRL$XRL_ARGS
}

pim_reset_vif_override_interval()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_reset_vif_override_interval <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_reset_vif_override_interval" $*
    XRL="finder://$PIM_TARGET/pim/0.1/reset_vif_override_interval"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl $XRL$XRL_ARGS
}

pim_get_vif_is_tracking_support_disabled()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_get_vif_is_tracking_support_disabled <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_get_vif_is_tracking_support_disabled" $*
    XRL="finder://$PIM_TARGET/pim/0.1/get_vif_is_tracking_support_disabled"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -p is_tracking_support_disabled:bool $XRL$XRL_ARGS
}

pim_set_vif_is_tracking_support_disabled()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_set_vif_is_tracking_support_disabled <vif_name:txt> <is_tracking_support_disabled:bool>"
	exit 1
    fi
    vif_name=$1
    is_tracking_support_disabled=$2
    
    echo "pim_set_vif_is_tracking_support_disalbed" $*
    XRL="finder://$PIM_TARGET/pim/0.1/set_vif_is_tracking_support_disabled"
    XRL_ARGS="?vif_name:txt=$vif_name&is_tracking_support_disabled:bool=$is_tracking_support_disabled"
    call_xrl $XRL$XRL_ARGS
}

pim_reset_vif_is_tracking_support_disabled()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_reset_vif_is_tracking_support_disabled <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_reset_vif_is_tracking_support_disabled" $*
    XRL="finder://$PIM_TARGET/pim/0.1/reset_vif_is_tracking_support_disabled"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl $XRL$XRL_ARGS
}

pim_get_vif_accept_nohello_neighbors()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_get_vif_accept_nohello_neighbors <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_get_vif_accept_nohello_neighbors" $*
    XRL="finder://$PIM_TARGET/pim/0.1/get_vif_accept_nohello_neighbors"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -p accept_nohello_neighbors:bool $XRL$XRL_ARGS
}

pim_set_vif_accept_nohello_neighbors()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_set_vif_accept_nohello_neighbors <vif_name:txt> <accept_nohello_neighbors:bool>"
	exit 1
    fi
    vif_name=$1
    accept_nohello_neighbors=$2
    
    echo "pim_set_vif_accept_nohello_neighbors" $*
    XRL="finder://$PIM_TARGET/pim/0.1/set_vif_accept_nohello_neighbors"
    XRL_ARGS="?vif_name:txt=$vif_name&accept_nohello_neighbors:bool=$accept_nohello_neighbors"
    call_xrl $XRL$XRL_ARGS
}

pim_reset_vif_accept_nohello_neighbors()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_reset_vif_accept_nohello_neighbors <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_reset_vif_accept_nohello_neighbors" $*
    XRL="finder://$PIM_TARGET/pim/0.1/reset_vif_accept_nohello_neighbors"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl $XRL$XRL_ARGS
}

pim_get_vif_join_prune_period()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_get_vif_join_prune_period <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_get_vif_join_prune_period" $*
    XRL="finder://$PIM_TARGET/pim/0.1/get_vif_join_prune_period"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl -p join_prune_period:u32 $XRL$XRL_ARGS
}

pim_set_vif_join_prune_period()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_set_vif_join_prune_period <vif_name:txt> <join_prune_period:u32>"
	exit 1
    fi
    vif_name=$1
    join_prune_period=$2
    
    echo "pim_set_vif_join_prune_period" $*
    XRL="finder://$PIM_TARGET/pim/0.1/set_vif_join_prune_period"
    XRL_ARGS="?vif_name:txt=$vif_name&join_prune_period:u32=$join_prune_period"
    call_xrl $XRL$XRL_ARGS
}

pim_reset_vif_join_prune_period()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_reset_vif_join_prune_period <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_reset_vif_join_prune_period" $*
    XRL="finder://$PIM_TARGET/pim/0.1/reset_vif_join_prune_period"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl $XRL$XRL_ARGS
}

pim_get_switch_to_spt_threshold()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_get_switch_to_spt_threshold"
	exit 1
    fi
    
    echo "pim_get_switch_to_spt_threshold" $*
    XRL="finder://$PIM_TARGET/pim/0.1/get_switch_to_spt_threshold"
    XRL_ARGS=""
    call_xrl -p is_enabled:bool -p interval_sec:u32 -p bytes:u32 $XRL$XRL_ARGS
}

pim_set_switch_to_spt_threshold()
{
    if [ $# -lt 3 ] ; then
	echo "Usage: pim_set_switch_to_spt_threshold <is_enabled:bool> <interval_sec:u32> <bytes:u32>"
	exit 1
    fi
    is_enabled=$1
    interval_sec=$2
    bytes=$3
    
    echo "pim_set_switch_to_spt_threshold" $*
    XRL="finder://$PIM_TARGET/pim/0.1/set_switch_to_spt_threshold"
    XRL_ARGS="?is_enabled:bool=$is_enabled&interval_sec:u32=$interval_sec&bytes:u32=$bytes"
    call_xrl $XRL$XRL_ARGS
}

pim_reset_switch_to_spt_threshold()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_reset_switch_to_spt_threshold"
	exit 1
    fi
    
    echo "pim_reset_switch_to_spt_threshold" $*
    XRL="finder://$PIM_TARGET/pim/0.1/reset_switch_to_spt_threshold"
    XRL_ARGS=""
    call_xrl $XRL$XRL_ARGS
}

pim_enable_log_trace()
{
    echo "pim_enable_log_trace" $*
    XRL="finder://$PIM_TARGET/pim/0.1/enable_log_trace"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_disable_log_trace()
{
    echo "pim_disable_log_trace" $*
    XRL="finder://$PIM_TARGET/pim/0.1/disable_log_trace"
    XRL_ARGS=""
    call_xrl -r 0 $XRL$XRL_ARGS
}

pim_add_test_jp_entry4()
{
    if [ $# -lt 7 ] ; then
	echo "Usage: pim_add_test_jp_entry4 <source_addr:ipv4> <group_addr:ipv4> <group_masklen:u32> <mrt_entry_type:txt (SG, SG_RPT, WC, RP)> <action_jp:txt (JOIN, PRUNE)> <holdtime:u32> <new_group_bool:bool>"
	exit 1
    fi
    source_addr=$1
    group_addr=$2
    group_masklen=$3
    mrt_entry_type=$4	# Must be one of: SG, SG_RPT, WC, RP
    action_jp=$5	# Must be one of: JOIN, PRUNE
    holdtime=$6
    new_group_bool=$7
    
    echo "pim_add_test_jp_entry4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_test_jp_entry4"
    XRL_ARGS="?source_addr:ipv4=$source_addr&group_addr:ipv4=$group_addr&group_masklen:u32=$group_masklen&mrt_entry_type:txt=$mrt_entry_type&action_jp:txt=$action_jp&holdtime:u32=$holdtime&new_group_bool:bool=$new_group_bool"
    call_xrl $XRL$XRL_ARGS
}

pim_add_test_jp_entry6()
{
    if [ $# -lt 7 ] ; then
	echo "Usage: pim_add_test_jp_entry6 <source_addr:ipv6> <group_addr:ipv6> <group_masklen:u32> <mrt_entry_type:txt (SG, SG_RPT, WC, RP)> <action_jp:txt (JOIN, PRUNE)> <holdtime:u32> <new_group_bool:bool>"
	exit 1
    fi
    source_addr=$1
    group_addr=$2
    group_masklen=$3
    mrt_entry_type=$4	# Must be one of: SG, SG_RPT, WC, RP
    action_jp=$5	# Must be one of: JOIN, PRUNE
    holdtime=$6
    new_group_bool=$7
    
    echo "pim_add_test_jp_entry6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_test_jp_entry6"
    XRL_ARGS="?source_addr:ipv6=$source_addr&group_addr:ipv6=$group_addr&group_masklen:u32=$group_masklen&mrt_entry_type:txt=$mrt_entry_type&action_jp:txt=$action_jp&holdtime:u32=$holdtime&new_group_bool:bool=$new_group_bool"
    call_xrl $XRL$XRL_ARGS
}

pim_send_test_jp_entry4()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_send_test_jp_entry4 <nbr_addr:ipv4>"
	exit 1
    fi
    nbr_addr=$1
    
    echo "pim_send_test_jp_entry4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/send_test_jp_entry4"
    XRL_ARGS="?nbr_addr:ipv4=$nbr_addr"
    call_xrl $XRL$XRL_ARGS
}

pim_send_test_jp_entry6()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_send_test_jp_entry6 <nbr_addr:ipv6>"
	exit 1
    fi
    nbr_addr=$1
    
    echo "pim_send_test_jp_entry6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/send_test_jp_entry6"
    XRL_ARGS="?nbr_addr:ipv6=$nbr_addr"
    call_xrl $XRL$XRL_ARGS
}

pim_send_test_assert4()
{
    if [ $# -lt 6 ] ; then
	echo "Usage: pim_send_test_assert4 <vif_name:txt> <source_addr:ipv4> <group_addr:ipv4> <rpt_bit:bool> <metric_preference:u32> <metric:u32>"
	exit 1
    fi
    vif_name=$1
    source_addr=$2
    group_addr=$3
    rpt_bit=$4
    metric_preference=$5
    metric=$6
    
    echo "pim_send_test_assert4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/send_test_assert4"
    XRL_ARGS="?vif_name:txt=$vif_name&source_addr:ipv4=$source_addr&group_addr:ipv4=$group_addr&rpt_bit:bool=$rpt_bit&metric_preference:u32=$metric_preference&metric:u32=$metric"
    call_xrl $XRL$XRL_ARGS
}

pim_send_test_assert6()
{
    if [ $# -lt 6 ] ; then
	echo "Usage: pim_send_test_assert6 <vif_name:txt> <source_addr:ipv6> <group_addr:ipv6> <rpt_bit:bool> <metric_preference:u32> <metric:u32>"
	exit 1
    fi
    vif_name=$1
    source_addr=$2
    group_addr=$3
    rpt_bit=$4
    metric_preference=$5
    metric=$6
    
    echo "pim_send_test_assert6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/send_test_assert6"
    XRL_ARGS="?vif_name:txt=$vif_name&source_addr:ipv6=$source_addr&group_addr:ipv6=$group_addr&rpt_bit:bool=$rpt_bit&metric_preference:u32=$metric_preference&metric:u32=$metric"
    call_xrl $XRL$XRL_ARGS
}

pim_add_test_bsr_zone4()
{
    if [ $# -lt 6 ] ; then
	echo "Usage: pim_add_test_bsr_zone4 <zone_id_scope_zone_prefix:ipv4net> <zone_id_is_scope_zone:bool> <bsr_addr:ipv4> <bsr_priority:u32> <hash_masklen:u32> <fragment_tag:u32>"
	exit 1
    fi
    zone_id_scope_zone_prefix=$1
    zone_id_is_scope_zone=$2
    bsr_addr=$3
    bsr_priority=$4
    hash_masklen=$5
    fragment_tag=$6
    
    echo "pim_add_test_bsr_zone4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_test_bsr_zone4"
    XRL_ARGS="?zone_id_scope_zone_prefix:ipv4net=$zone_id_scope_zone_prefix&zone_id_is_scope_zone:bool=$zone_id_is_scope_zone&bsr_addr:ipv4=$bsr_addr&bsr_priority:u32=$bsr_priority&hash_masklen:u32=$hash_masklen&fragment_tag:u32=$fragment_tag"
    call_xrl $XRL$XRL_ARGS
}

pim_add_test_bsr_zone6()
{
    if [ $# -lt 6 ] ; then
	echo "Usage: pim_add_test_bsr_zone6 <zone_id_scope_zone_prefix:ipv6net> <zone_id_is_scope_zone:bool> <bsr_addr:ipv6> <bsr_priority:u32> <hash_masklen:u32> <fragment_tag:u32>"
	exit 1
    fi
    zone_id_scope_zone_prefix=$1
    zone_id_is_scope_zone=$2
    bsr_addr=$3
    bsr_priority=$4
    hash_masklen=$5
    fragment_tag=$6
    
    echo "pim_add_test_bsr_zone6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_test_bsr_zone6"
    XRL_ARGS="?zone_id_scope_zone_prefix:ipv6net=$zone_id_scope_zone_prefix&zone_id_is_scope_zone:bool=$zone_id_is_scope_zone&bsr_addr:ipv6=$bsr_addr&bsr_priority:u32=$bsr_priority&hash_masklen:u32=$hash_masklen&fragment_tag:u32=$fragment_tag"
    call_xrl $XRL$XRL_ARGS
}

pim_add_test_bsr_group_prefix4()
{
    if [ $# -lt 5 ] ; then
	echo "Usage: pim_add_test_bsr_group_prefix4 <zone_id_scope_zone_prefix:ipv4net> <zone_id_is_scope_zone:bool> <group_prefix:ipv4net> <is_scope_zone:bool> <expected_rp_count:u32>"
	exit 1
    fi
    zone_id_scope_zone_prefix=$1
    zone_id_is_scope_zone=$2
    group_prefix=$3
    is_scope_zone=$4
    expected_rp_count=$5
    
    echo "pim_add_test_bsr_group_prefix4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_test_bsr_group_prefix4"
    XRL_ARGS="?zone_id_scope_zone_prefix:ipv4net=$zone_id_scope_zone_prefix&zone_id_is_scope_zone:bool=$zone_id_is_scope_zone&group_prefix:ipv4net=$group_prefix&is_scope_zone:bool=$is_scope_zone&expected_rp_count:u32=$expected_rp_count"
    call_xrl $XRL$XRL_ARGS
}

pim_add_test_bsr_group_prefix6()
{
    if [ $# -lt 5 ] ; then
	echo "Usage: pim_add_test_bsr_group_prefix6 <zone_id_scope_zone_prefix:ipv6net> <zone_id_is_scope_zone:bool> <group_prefix:ipv6net> <is_scope_zone:bool> <expected_rp_count:u32>"
	exit 1
    fi
    zone_id_scope_zone_prefix=$1
    zone_id_is_scope_zone=$2
    group_prefix=$3
    is_scope_zone=$4
    expected_rp_count=$5
    
    echo "pim_add_test_bsr_group_prefix6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_test_bsr_group_prefix6"
    XRL_ARGS="?zone_id_scope_zone_prefix:ipv6net=$zone_id_scope_zone_prefix&zone_id_is_scope_zone:bool=$zone_id_is_scope_zone&group_prefix:ipv6net=$group_prefix&is_scope_zone:bool=$is_scope_zone&expected_rp_count:u32=$expected_rp_count"
    call_xrl $XRL$XRL_ARGS
}

pim_add_test_bsr_rp4()
{
    if [ $# -lt 6 ] ; then
	echo "Usage: pim_add_test_bsr_rp4 <zone_id_scope_zone_prefix:ipv4net> <zone_id_is_scope_zone:bool> <group_prefix:ipv4net> <rp_addr:ipv4> <rp_priority:u32> <rp_holdtime:u32>"
	exit 1
    fi
    zone_id_scope_zone_prefix=$1
    zone_id_is_scope_zone=$2
    group_prefix=$3
    rp_addr=$4
    rp_priority=$5
    rp_holdtime=$6
    
    echo "pim_add_test_bsr_rp4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_test_bsr_rp4"
    XRL_ARGS="?zone_id_scope_zone_prefix:ipv4net=$zone_id_scope_zone_prefix&zone_id_is_scope_zone:bool=$zone_id_is_scope_zone&group_prefix:ipv4net=$group_prefix&rp_addr:ipv4=$rp_addr&rp_priority:u32=$rp_priority&rp_holdtime:u32=$rp_holdtime"
    call_xrl $XRL$XRL_ARGS
}

pim_add_test_bsr_rp6()
{
    if [ $# -lt 6 ] ; then
	echo "Usage: pim_add_test_bsr_rp6 <zone_id_scope_zone_prefix:ipv6net> <zone_id_is_scope_zone:bool> <group_prefix:ipv6net> <rp_addr:ipv6> <rp_priority:u32> <rp_holdtime:u32>"
	exit 1
    fi
    zone_id_scope_zone_prefix=$1
    zone_id_is_scope_zone=$2
    group_prefix=$3
    rp_addr=$4
    rp_priority=$5
    rp_holdtime=$6
    
    echo "pim_add_test_bsr_rp6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_test_bsr_rp6"
    XRL_ARGS="?zone_id_scope_zone_prefix:ipv6net=$zone_id_scope_zone_prefix&zone_id_is_scope_zone:bool=$zone_id_is_scope_zone&group_prefix:ipv6net=$group_prefix&rp_addr:ipv6=$rp_addr&rp_priority:u32=$rp_priority&rp_holdtime:u32=$rp_holdtime"
    call_xrl $XRL$XRL_ARGS
}

pim_send_test_bootstrap()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_send_test_bootstrap <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_send_test_bootstrap" $*
    XRL="finder://$PIM_TARGET/pim/0.1/send_test_bootstrap"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl $XRL$XRL_ARGS
}

pim_send_test_bootstrap_by_dest4()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_send_test_bootstrap_by_dest4 <vif_name:txt> <dest_addr:ipv4>"
	exit 1
    fi
    vif_name=$1
    dest_addr=$2
    
    echo "pim_send_test_bootstrap_by_dest4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/send_test_bootstrap_by_dest4"
    XRL_ARGS="?vif_name:txt=$vif_name&dest_addr:ipv4=$dest_addr"
    call_xrl $XRL$XRL_ARGS
}

pim_send_test_bootstrap_by_dest6()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_send_test_bootstrap_by_dest6 <vif_name:txt> <dest_addr:ipv6>"
	exit 1
    fi
    vif_name=$1
    dest_addr=$2
    
    echo "pim_send_test_bootstrap_by_dest6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/send_test_bootstrap_by_dest6"
    XRL_ARGS="?vif_name:txt=$vif_name&dest_addr:ipv6=$dest_addr"
    call_xrl $XRL$XRL_ARGS
}

pim_send_test_cand_rp_adv()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_send_test_bootstrap"
	exit 1
    fi
    
    echo "pim_send_test_cand_rp_adv" $*
    XRL="finder://$PIM_TARGET/pim/0.1/send_test_cand_rp_adv"
    XRL_ARGS=""
    call_xrl $XRL$XRL_ARGS
}
