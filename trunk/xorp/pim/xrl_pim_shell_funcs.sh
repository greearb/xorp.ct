#!/bin/sh

#
# $XORP: xorp/pim/xrl_pim_shell_funcs.sh,v 1.22 2004/01/05 20:40:51 pavlin Exp $
#

#
# Library of functions to sent XRLs to a running PIM process.
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
		PIM_TARGET=${PIM_TARGET:="PIMSM_4"}
		;;
	IPV6)
		PIM_TARGET=${PIM_TARGET:="PIMSM_6"}
		;;
	*)
		echo "Error: invalid IP_VERSION = ${IP_VERSION}. Must be either IPV4 or IPV6"
		exit 1
		;;
esac


pim_enable_vif()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_enable_vif <vif_name:txt> <enable:bool>"
	exit 1
    fi
    vif_name=$1
    enable=$2
    
    echo "pim_enable_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/enable_vif"
    XRL_ARGS="?vif_name:txt=$vif_name&enable:bool=$enable"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

pim_enable_all_vifs()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_enable_all_vifs <enable:bool>"
	exit 1
    fi
    enable=$1

    echo "pim_enable_all_vifs" $*
    XRL="finder://$PIM_TARGET/pim/0.1/enable_all_vifs"
    XRL_ARGS="?enable:bool=$enable"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

pim_start_all_vifs()
{
    echo "pim_start_all_vifs" $*
    XRL="finder://$PIM_TARGET/pim/0.1/start_all_vifs"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

pim_stop_all_vifs()
{
    echo "pim_stop_all_vifs" $*
    XRL="finder://$PIM_TARGET/pim/0.1/stop_all_vifs"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

pim_is_vif_setup_completed()
{
    echo "pim_is_vif_setup_completed" $*
    XRL="finder://$PIM_TARGET/mfea_client/0.1/is_vif_setup_completed"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS is_completed:bool = true
}

pim_enable_pim()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_enable_pim <enable:bool>"
	exit 1
    fi
    enable=$1

    echo "pim_enable_pim" $*
    XRL="finder://$PIM_TARGET/pim/0.1/enable_pim"
    XRL_ARGS="?enable:bool=$enable"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

pim_start_pim()
{
    echo "pim_start_pim" $*
    XRL="finder://$PIM_TARGET/pim/0.1/start_pim"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

pim_stop_pim()
{
    echo "pim_stop_pim" $*
    XRL="finder://$PIM_TARGET/pim/0.1/stop_pim"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

pim_enable_cli()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_enable_cli <enable:bool>"
	exit 1
    fi
    enable=$1

    echo "pim_enable_cli" $*
    XRL="finder://$PIM_TARGET/pim/0.1/enable_cli"
    XRL_ARGS="?enable:bool=$enable"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

pim_start_cli()
{
    echo "pim_start_cli" $*
    XRL="finder://$PIM_TARGET/pim/0.1/start_cli"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

pim_stop_cli()
{
    echo "pim_stop_cli" $*
    XRL="finder://$PIM_TARGET/pim/0.1/stop_cli"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARG
}

pim_enable_bsr()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_enable_bsr <enable:bool>"
	exit 1
    fi
    enable=$1

    echo "pim_enable_bsr" $*
    XRL="finder://$PIM_TARGET/pim/0.1/enable_bsr"
    XRL_ARGS="?enable:bool=$enable"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

pim_start_bsr()
{
    echo "pim_start_bsr" $*
    XRL="finder://$PIM_TARGET/pim/0.1/start_bsr"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

pim_stop_bsr()
{
    echo "pim_stop_bsr" $*
    XRL="finder://$PIM_TARGET/pim/0.1/stop_bsr"
    XRL_ARGS=""
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

#
# Add/delete Candidate-RP configuration.
#
pim_add_config_cand_bsr_by_vif_name4()
{
    if [ $# -lt 5 ] ; then
	echo "Usage: pim_add_config_cand_bsr_by_vif_name4 <scope_zone_id:ipv4net> <is_scope_zone:bool> <vif_name:txt> <bsr_priority:u32> <hash_mask_len:u32>"
	exit 1
    fi
    scope_zone_id=$1
    is_scope_zone=$2
    vif_name=$3
    bsr_priority=$4
    hash_mask_len=$5
    
    echo "pim_add_config_cand_bsr_by_vif_name4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_config_cand_bsr_by_vif_name4"
    XRL_ARGS="?scope_zone_id:ipv4net=$scope_zone_id&is_scope_zone:bool=$is_scope_zone&vif_name:txt=$vif_name&bsr_priority:u32=$bsr_priority&hash_mask_len:u32=$hash_mask_len"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

pim_add_config_cand_bsr_by_vif_name6()
{
    if [ $# -lt 5 ] ; then
	echo "Usage: pim_add_config_cand_bsr_by_vif_name6 <scope_zone_id:ipv6net> <is_scope_zone:bool> <vif_name:txt> <bsr_priority:u32> <hash_mask_len:u32>"
	exit 1
    fi
    scope_zone_id=$1
    is_scope_zone=$2
    vif_name=$3
    bsr_priority=$4
    hash_mask_len=$5
    
    echo "pim_add_config_cand_bsr_by_vif_name6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_config_cand_bsr_by_vif_name6"
    XRL_ARGS="?scope_zone_id:ipv6net=$scope_zone_id&is_scope_zone:bool=$is_scope_zone&vif_name:txt=$vif_name&bsr_priority:u32=$bsr_priority&hash_mask_len:u32=$hash_mask_len"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

pim_add_config_cand_bsr_by_addr4()
{
    if [ $# -lt 5 ] ; then
	echo "Usage: pim_add_config_cand_bsr_by_addr4 <scope_zone_id:ipv4net> <is_scope_zone:bool> <cand_bsr_addr:ipv4> <bsr_priority:u32> <hash_mask_len:u32>"
	exit 1
    fi
    scope_zone_id=$1
    is_scope_zone=$2
    cand_bsr_addr=$3
    bsr_priority=$4
    hash_mask_len=$5
    
    echo "pim_add_config_cand_bsr_by_addr4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_config_cand_bsr_by_addr4"
    XRL_ARGS="?scope_zone_id:ipv4net=$scope_zone_id&is_scope_zone:bool=$is_scope_zone&cand_bsr_addr:ipv4=$cand_bsr_addr&bsr_priority:u32=$bsr_priority&hash_mask_len:u32=$hash_mask_len"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

pim_add_config_cand_bsr_by_addr6()
{
    if [ $# -lt 5 ] ; then
	echo "Usage: pim_add_config_cand_bsr_by_addr6 <scope_zone_id:ipv6net> <is_scope_zone:bool> <cand_bsr_addr:ipv6> <bsr_priority:u32> <hash_mask_len:u32>"
	exit 1
    fi
    scope_zone_id=$1
    is_scope_zone=$2
    cand_bsr_addr=$3
    bsr_priority=$4
    hash_mask_len=$5
    
    echo "pim_add_config_cand_bsr_by_addr6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_config_cand_bsr_by_addr6"
    XRL_ARGS="?scope_zone_id:ipv6net=$scope_zone_id&is_scope_zone:bool=$is_scope_zone&cand_bsr_addr:ipv6=$cand_bsr_addr&bsr_priority:u32=$bsr_priority&hash_mask_len:u32=$hash_mask_len"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

pim_add_config_static_rp4()
{
    if [ $# -lt 4 ] ; then
	echo "Usage: pim_add_config_static_rp4 <group_prefix:ipv4net> <rp_addr:ipv4> <rp_priority:u32> <hash_mask_len:u32>"
	exit 1
    fi
    group_prefix=$1
    rp_addr=$2
    rp_priority=$3
    hash_mask_len=$4
    
    echo "pim_add_config_static_rp4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_config_static_rp4"
    XRL_ARGS="?group_prefix:ipv4net=$group_prefix&rp_addr:ipv4=$rp_addr&rp_priority:u32=$rp_priority&hash_mask_len:u32=$hash_mask_len"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

pim_add_config_static_rp6()
{
    if [ $# -lt 4 ] ; then
	echo "Usage: pim_add_config_static_rp6 <group_prefix:ipv6net> <rp_addr:ipv6> <rp_priority:u32> <hash_mask_len:u32>"
	exit 1
    fi
    group_prefix=$1
    rp_addr=$2
    rp_priority=$3
    hash_mask_len=$4
    
    echo "pim_add_config_static_rp6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_config_static_rp6"
    XRL_ARGS="?group_prefix:ipv6net=$group_prefix&rp_addr:ipv6=$rp_addr&rp_priority:u32=$rp_priority&hash_mask_len:u32=$hash_mask_len"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
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
    call_xrl_wrapper -p proto_version:u32 $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper -p hello_triggered_delay:u32 $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper -p hello_period:u32 $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper -p hello_holdtime:u32 $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper -p dr_priority:u32 $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper -p lan_delay:u32 $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper -p override_interval:u32 $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper -p is_tracking_support_disabled:bool $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper -p accept_nohello_neighbors:bool $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper -p join_prune_period:u32 $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper -p is_enabled:bool -p interval_sec:u32 -p bytes:u32 $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
}

pim_add_alternative_subnet4()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_add_alternative_subnet4 <vif_name:txt> <subnet:ipv4net>"
	exit 1
    fi
    vif_name=$1
    subnet=$2
    
    echo "pim_add_alternative_subnet4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_alternative_subnet4"
    XRL_ARGS="?vif_name:txt=$vif_name&subnet:ipv4net=$subnet"
    call_xrl_wrapper $XRL$XRL_ARGS
}

pim_add_alternative_subnet6()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_add_alternative_subnet6 <vif_name:txt> <subnet:ipv6net>"
	exit 1
    fi
    vif_name=$1
    subnet=$2
    
    echo "pim_add_alternative_subnet6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_alternative_subnet6"
    XRL_ARGS="?vif_name:txt=$vif_name&subnet:ipv6net=$subnet"
    call_xrl_wrapper $XRL$XRL_ARGS
}

pim_delete_alternative_subnet4()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_delete_alternative_subnet4 <vif_name:txt> <subnet:ipv4net>"
	exit 1
    fi
    vif_name=$1
    subnet=$2
    
    echo "pim_delete_alternative_subnet4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/delete_alternative_subnet4"
    XRL_ARGS="?vif_name:txt=$vif_name&subnet:ipv4net=$subnet"
    call_xrl_wrapper $XRL$XRL_ARGS
}

pim_delete_alternative_subnet6()
{
    if [ $# -lt 2 ] ; then
	echo "Usage: pim_delete_alternative_subnet6 <vif_name:txt> <subnet:ipv6net>"
	exit 1
    fi
    vif_name=$1
    subnet=$2
    
    echo "pim_delete_alternative_subnet6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/delete_alternative_subnet6"
    XRL_ARGS="?vif_name:txt=$vif_name&subnet:ipv6net=$subnet"
    call_xrl_wrapper $XRL$XRL_ARGS
}

pim_remove_all_alternative_subnets()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_remove_all_alternative_subnets <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_remove_all_alternative_subnets" $*
    XRL="finder://$PIM_TARGET/pim/0.1/remove_all_alternative_subnets"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper $XRL$XRL_ARGS
}

pim_log_trace_all()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_log_trace_all <enable:bool>"
	exit 1
    fi
    enable=$1

    echo "pim_log_trace_all" $*

    XRL="finder://$PIM_TARGET/pim/0.1/log_trace_all"
    XRL_ARGS="?enable:bool=$enable"
    call_xrl_wrapper -r 0 $XRL$XRL_ARGS
}

pim_add_test_jp_entry4()
{
    if [ $# -lt 7 ] ; then
	echo "Usage: pim_add_test_jp_entry4 <source_addr:ipv4> <group_addr:ipv4> <group_mask_len:u32> <mrt_entry_type:txt (SG, SG_RPT, WC, RP)> <action_jp:txt (JOIN, PRUNE)> <holdtime:u32> <new_group_bool:bool>"
	exit 1
    fi
    source_addr=$1
    group_addr=$2
    group_mask_len=$3
    mrt_entry_type=$4	# Must be one of: SG, SG_RPT, WC, RP
    action_jp=$5	# Must be one of: JOIN, PRUNE
    holdtime=$6
    new_group_bool=$7
    
    echo "pim_add_test_jp_entry4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_test_jp_entry4"
    XRL_ARGS="?source_addr:ipv4=$source_addr&group_addr:ipv4=$group_addr&group_mask_len:u32=$group_mask_len&mrt_entry_type:txt=$mrt_entry_type&action_jp:txt=$action_jp&holdtime:u32=$holdtime&new_group_bool:bool=$new_group_bool"
    call_xrl_wrapper $XRL$XRL_ARGS
}

pim_add_test_jp_entry6()
{
    if [ $# -lt 7 ] ; then
	echo "Usage: pim_add_test_jp_entry6 <source_addr:ipv6> <group_addr:ipv6> <group_mask_len:u32> <mrt_entry_type:txt (SG, SG_RPT, WC, RP)> <action_jp:txt (JOIN, PRUNE)> <holdtime:u32> <new_group_bool:bool>"
	exit 1
    fi
    source_addr=$1
    group_addr=$2
    group_mask_len=$3
    mrt_entry_type=$4	# Must be one of: SG, SG_RPT, WC, RP
    action_jp=$5	# Must be one of: JOIN, PRUNE
    holdtime=$6
    new_group_bool=$7
    
    echo "pim_add_test_jp_entry6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_test_jp_entry6"
    XRL_ARGS="?source_addr:ipv6=$source_addr&group_addr:ipv6=$group_addr&group_mask_len:u32=$group_mask_len&mrt_entry_type:txt=$mrt_entry_type&action_jp:txt=$action_jp&holdtime:u32=$holdtime&new_group_bool:bool=$new_group_bool"
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
}

pim_add_test_bsr_zone4()
{
    if [ $# -lt 6 ] ; then
	echo "Usage: pim_add_test_bsr_zone4 <zone_id_scope_zone_prefix:ipv4net> <zone_id_is_scope_zone:bool> <bsr_addr:ipv4> <bsr_priority:u32> <hash_mask_len:u32> <fragment_tag:u32>"
	exit 1
    fi
    zone_id_scope_zone_prefix=$1
    zone_id_is_scope_zone=$2
    bsr_addr=$3
    bsr_priority=$4
    hash_mask_len=$5
    fragment_tag=$6
    
    echo "pim_add_test_bsr_zone4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_test_bsr_zone4"
    XRL_ARGS="?zone_id_scope_zone_prefix:ipv4net=$zone_id_scope_zone_prefix&zone_id_is_scope_zone:bool=$zone_id_is_scope_zone&bsr_addr:ipv4=$bsr_addr&bsr_priority:u32=$bsr_priority&hash_mask_len:u32=$hash_mask_len&fragment_tag:u32=$fragment_tag"
    call_xrl_wrapper $XRL$XRL_ARGS
}

pim_add_test_bsr_zone6()
{
    if [ $# -lt 6 ] ; then
	echo "Usage: pim_add_test_bsr_zone6 <zone_id_scope_zone_prefix:ipv6net> <zone_id_is_scope_zone:bool> <bsr_addr:ipv6> <bsr_priority:u32> <hash_mask_len:u32> <fragment_tag:u32>"
	exit 1
    fi
    zone_id_scope_zone_prefix=$1
    zone_id_is_scope_zone=$2
    bsr_addr=$3
    bsr_priority=$4
    hash_mask_len=$5
    fragment_tag=$6
    
    echo "pim_add_test_bsr_zone6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/add_test_bsr_zone6"
    XRL_ARGS="?zone_id_scope_zone_prefix:ipv6net=$zone_id_scope_zone_prefix&zone_id_is_scope_zone:bool=$zone_id_is_scope_zone&bsr_addr:ipv6=$bsr_addr&bsr_priority:u32=$bsr_priority&hash_mask_len:u32=$hash_mask_len&fragment_tag:u32=$fragment_tag"
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
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
    call_xrl_wrapper $XRL$XRL_ARGS
}

pim_pimstat_neighbors4()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_neighbors4"
	exit 1
    fi
    
    echo "pim_pimstat_neighbors4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_neighbors4"
    XRL_ARGS=""
    call_xrl_wrapper -p all $XRL$XRL_ARGS
}

pim_pimstat_neighbors6()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_neighbors6"
	exit 1
    fi
    
    echo "pim_pimstat_neighbors6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_neighbors6"
    XRL_ARGS=""
    call_xrl_wrapper -p all $XRL$XRL_ARGS
}

pim_pimstat_interface4()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_interface4 <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_interface4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_interface4"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p all $XRL$XRL_ARGS
}

pim_pimstat_interface6()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_interface6 <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_interface6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_interface6"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p all $XRL$XRL_ARGS
}

pim_pimstat_rps4()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rps4"
	exit 1
    fi
    
    echo "pim_pimstat_rps4" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rps4"
    XRL_ARGS=""
    call_xrl_wrapper -p all $XRL$XRL_ARGS
}

pim_pimstat_rps6()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rps6"
	exit 1
    fi
    
    echo "pim_pimstat_rps6" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rps6"
    XRL_ARGS=""
    call_xrl_wrapper -p all $XRL$XRL_ARGS
}

pim_clear_pim_statistics()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_clear_pim_statistics"
	exit 1
    fi
    
    echo "pim_clear_pim_statistics" $*
    XRL="finder://$PIM_TARGET/pim/0.1/clear_pim_statistics"
    XRL_ARGS=""
    call_xrl_wrapper $XRL$XRL_ARGS
}

pim_clear_pim_statistics_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_clear_pim_statistics_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_clear_pim_statistics_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/clear_pim_statistics_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper $XRL$XRL_ARGS
}

#
# Statistics-related counters and values
#
pim_pimstat_hello_messages_received()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_hello_messages_received"
	exit 1
    fi
    
    echo "pim_pimstat_hello_messages_received" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_hello_messages_received"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_hello_messages_sent()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_hello_messages_sent"
	exit 1
    fi
    
    echo "pim_pimstat_hello_messages_sent" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_hello_messages_sent"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_hello_messages_rx_errors()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_hello_messages_rx_errors"
	exit 1
    fi
    
    echo "pim_pimstat_hello_messages_rx_errors" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_hello_messages_rx_errors"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_register_messages_received()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_register_messages_received"
	exit 1
    fi
    
    echo "pim_pimstat_register_messages_received" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_register_messages_received"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_register_messages_sent()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_register_messages_sent"
	exit 1
    fi
    
    echo "pim_pimstat_register_messages_sent" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_register_messages_sent"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_register_messages_rx_errors()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_register_messages_rx_errors"
	exit 1
    fi
    
    echo "pim_pimstat_register_messages_rx_errors" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_register_messages_rx_errors"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_register_stop_messages_received()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_register_stop_messages_received"
	exit 1
    fi
    
    echo "pim_pimstat_register_stop_messages_received" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_register_stop_messages_received"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_register_stop_messages_sent()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_register_stop_messages_sent"
	exit 1
    fi
    
    echo "pim_pimstat_register_stop_messages_sent" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_register_stop_messages_sent"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_register_stop_messages_rx_errors()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_register_stop_messages_rx_errors"
	exit 1
    fi
    
    echo "pim_pimstat_register_stop_messages_rx_errors" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_register_stop_messages_rx_errors"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_join_prune_messages_received()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_join_prune_messages_received"
	exit 1
    fi
    
    echo "pim_pimstat_join_prune_messages_received" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_join_prune_messages_received"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_join_prune_messages_sent()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_join_prune_messages_sent"
	exit 1
    fi
    
    echo "pim_pimstat_join_prune_messages_sent" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_join_prune_messages_sent"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_join_prune_messages_rx_errors()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_join_prune_messages_rx_errors"
	exit 1
    fi
    
    echo "pim_pimstat_join_prune_messages_rx_errors" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_join_prune_messages_rx_errors"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_bootstrap_messages_received()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_bootstrap_messages_received"
	exit 1
    fi
    
    echo "pim_pimstat_bootstrap_messages_received" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_bootstrap_messages_received"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_bootstrap_messages_sent()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_bootstrap_messages_sent"
	exit 1
    fi
    
    echo "pim_pimstat_bootstrap_messages_sent" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_bootstrap_messages_sent"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_bootstrap_messages_rx_errors()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_bootstrap_messages_rx_errors"
	exit 1
    fi
    
    echo "pim_pimstat_bootstrap_messages_rx_errors" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_bootstrap_messages_rx_errors"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_assert_messages_received()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_assert_messages_received"
	exit 1
    fi
    
    echo "pim_pimstat_assert_messages_received" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_assert_messages_received"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_assert_messages_sent()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_assert_messages_sent"
	exit 1
    fi
    
    echo "pim_pimstat_assert_messages_sent" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_assert_messages_sent"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_assert_messages_rx_errors()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_assert_messages_rx_errors"
	exit 1
    fi
    
    echo "pim_pimstat_assert_messages_rx_errors" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_assert_messages_rx_errors"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_graft_messages_received()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_graft_messages_received"
	exit 1
    fi
    
    echo "pim_pimstat_graft_messages_received" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_graft_messages_received"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_graft_messages_sent()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_graft_messages_sent"
	exit 1
    fi
    
    echo "pim_pimstat_graft_messages_sent" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_graft_messages_sent"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_graft_messages_rx_errors()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_graft_messages_rx_errors"
	exit 1
    fi
    
    echo "pim_pimstat_graft_messages_rx_errors" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_graft_messages_rx_errors"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_graft_ack_messages_received()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_graft_ack_messages_received"
	exit 1
    fi
    
    echo "pim_pimstat_graft_ack_messages_received" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_graft_ack_messages_received"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_graft_ack_messages_sent()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_graft_ack_messages_sent"
	exit 1
    fi
    
    echo "pim_pimstat_graft_ack_messages_sent" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_graft_ack_messages_sent"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_graft_ack_messages_rx_errors()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_graft_ack_messages_rx_errors"
	exit 1
    fi
    
    echo "pim_pimstat_graft_ack_messages_rx_errors" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_graft_ack_messages_rx_errors"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_candidate_rp_messages_received()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_candidate_rp_messages_received"
	exit 1
    fi
    
    echo "pim_pimstat_candidate_rp_messages_received" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_candidate_rp_messages_received"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_candidate_rp_messages_sent()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_candidate_rp_messages_sent"
	exit 1
    fi
    
    echo "pim_pimstat_candidate_rp_messages_sent" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_candidate_rp_messages_sent"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_candidate_rp_messages_rx_errors()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_candidate_rp_messages_rx_errors"
	exit 1
    fi
    
    echo "pim_pimstat_candidate_rp_messages_rx_errors" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_candidate_rp_messages_rx_errors"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_unknown_type_messages()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_unknown_type_messages"
	exit 1
    fi
    
    echo "pim_pimstat_unknown_type_messages" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_unknown_type_messages"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_unknown_version_messages()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_unknown_version_messages"
	exit 1
    fi
    
    echo "pim_pimstat_unknown_version_messages" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_unknown_version_messages"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_neighbor_unknown_messages()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_neighbor_unknown_messages"
	exit 1
    fi
    
    echo "pim_pimstat_neighbor_unknown_messages" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_neighbor_unknown_messages"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_bad_length_messages()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_bad_length_messages"
	exit 1
    fi
    
    echo "pim_pimstat_bad_length_messages" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_bad_length_messages"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_bad_checksum_messages()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_bad_checksum_messages"
	exit 1
    fi
    
    echo "pim_pimstat_bad_checksum_messages" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_bad_checksum_messages"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_bad_receive_interface_messages()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_bad_receive_interface_messages"
	exit 1
    fi
    
    echo "pim_pimstat_bad_receive_interface_messages" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_bad_receive_interface_messages"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_interface_disabled_messages()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rx_interface_disabled_messages"
	exit 1
    fi
    
    echo "pim_pimstat_rx_interface_disabled_messages" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_interface_disabled_messages"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_register_not_rp()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rx_register_not_rp"
	exit 1
    fi
    
    echo "pim_pimstat_rx_register_not_rp" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_register_not_rp"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rp_filtered_source()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rp_filtered_source"
	exit 1
    fi
    
    echo "pim_pimstat_rp_filtered_source" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rp_filtered_source"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_unknown_register_stop()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_unknown_register_stop"
	exit 1
    fi
    
    echo "pim_pimstat_unknown_register_stop" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_unknown_register_stop"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_join_prune_no_state()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rx_join_prune_no_state"
	exit 1
    fi
    
    echo "pim_pimstat_rx_join_prune_no_state" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_join_prune_no_state"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_graft_graft_ack_no_state()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rx_graft_graft_ack_no_state"
	exit 1
    fi
    
    echo "pim_pimstat_rx_graft_graft_ack_no_state" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_graft_graft_ack_no_state"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_graft_on_upstream_interface()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rx_graft_on_upstream_interface"
	exit 1
    fi
    
    echo "pim_pimstat_rx_graft_on_upstream_interface" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_graft_on_upstream_interface"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_candidate_rp_not_bsr()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rx_candidate_rp_not_bsr"
	exit 1
    fi
    
    echo "pim_pimstat_rx_candidate_rp_not_bsr" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_candidate_rp_not_bsr"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_bsr_when_bsr()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rx_bsr_when_bsr"
	exit 1
    fi
    
    echo "pim_pimstat_rx_bsr_when_bsr" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_bsr_when_bsr"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_bsr_not_rpf_interface()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rx_bsr_not_rpf_interface"
	exit 1
    fi
    
    echo "pim_pimstat_rx_bsr_not_rpf_interface" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_bsr_not_rpf_interface"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_unknown_hello_option()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rx_unknown_hello_option"
	exit 1
    fi
    
    echo "pim_pimstat_rx_unknown_hello_option" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_unknown_hello_option"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_data_no_state()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rx_data_no_state"
	exit 1
    fi
    
    echo "pim_pimstat_rx_data_no_state" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_data_no_state"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_rp_no_state()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rx_rp_no_state"
	exit 1
    fi
    
    echo "pim_pimstat_rx_rp_no_state" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_rp_no_state"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_aggregate()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rx_aggregate"
	exit 1
    fi
    
    echo "pim_pimstat_rx_aggregate" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_aggregate"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_malformed_packet()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rx_malformed_packet"
	exit 1
    fi
    
    echo "pim_pimstat_rx_malformed_packet" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_malformed_packet"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_no_rp()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_no_rp"
	exit 1
    fi
    
    echo "pim_pimstat_no_rp" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_no_rp"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_no_route_upstream()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_no_route_upstream"
	exit 1
    fi
    
    echo "pim_pimstat_no_route_upstream" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_no_route_upstream"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rp_mismatch()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rp_mismatch"
	exit 1
    fi
    
    echo "pim_pimstat_rp_mismatch" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rp_mismatch"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rpf_neighbor_unknown()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rpf_neighbor_unknown"
	exit 1
    fi
    
    echo "pim_pimstat_rpf_neighbor_unknown" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rpf_neighbor_unknown"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_join_rp()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rx_join_rp"
	exit 1
    fi
    
    echo "pim_pimstat_rx_join_rp" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_join_rp"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_prune_rp()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rx_prune_rp"
	exit 1
    fi
    
    echo "pim_pimstat_rx_prune_rp" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_prune_rp"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_join_wc()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rx_join_wc"
	exit 1
    fi
    
    echo "pim_pimstat_rx_join_wc" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_join_wc"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_prune_wc()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rx_prune_wc"
	exit 1
    fi
    
    echo "pim_pimstat_rx_prune_wc" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_prune_wc"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_join_sg()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rx_join_sg"
	exit 1
    fi
    
    echo "pim_pimstat_rx_join_sg" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_join_sg"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_prune_sg()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rx_prune_sg"
	exit 1
    fi
    
    echo "pim_pimstat_rx_prune_sg" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_prune_sg"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_join_sg_rpt()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rx_join_sg_rpt"
	exit 1
    fi
    
    echo "pim_pimstat_rx_join_sg_rpt" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_join_sg_rpt"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_prune_sg_rpt()
{
    if [ $# -lt 0 ] ; then
	echo "Usage: pim_pimstat_rx_prune_sg_rpt"
	exit 1
    fi
    
    echo "pim_pimstat_rx_prune_sg_rpt" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_prune_sg_rpt"
    XRL_ARGS=""
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_hello_messages_received_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_hello_messages_received_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_hello_messages_received_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_hello_messages_received_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_hello_messages_sent_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_hello_messages_sent_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_hello_messages_sent_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_hello_messages_sent_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_hello_messages_rx_errors_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_hello_messages_rx_errors_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_hello_messages_rx_errors_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_hello_messages_rx_errors_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_register_messages_received_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_register_messages_received_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_register_messages_received_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_register_messages_received_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_register_messages_sent_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_register_messages_sent_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_register_messages_sent_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_register_messages_sent_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_register_messages_rx_errors_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_register_messages_rx_errors_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_register_messages_rx_errors_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_register_messages_rx_errors_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_register_stop_messages_received_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_register_stop_messages_received_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_register_stop_messages_received_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_register_stop_messages_received_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_register_stop_messages_sent_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_register_stop_messages_sent_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_register_stop_messages_sent_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_register_stop_messages_sent_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_register_stop_messages_rx_errors_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_register_stop_messages_rx_errors_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_register_stop_messages_rx_errors_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_register_stop_messages_rx_errors_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_join_prune_messages_received_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_join_prune_messages_received_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_join_prune_messages_received_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_join_prune_messages_received_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_join_prune_messages_sent_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_join_prune_messages_sent_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_join_prune_messages_sent_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_join_prune_messages_sent_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_join_prune_messages_rx_errors_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_join_prune_messages_rx_errors_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_join_prune_messages_rx_errors_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_join_prune_messages_rx_errors_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_bootstrap_messages_received_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_bootstrap_messages_received_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_bootstrap_messages_received_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_bootstrap_messages_received_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_bootstrap_messages_sent_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_bootstrap_messages_sent_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_bootstrap_messages_sent_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_bootstrap_messages_sent_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_bootstrap_messages_rx_errors_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_bootstrap_messages_rx_errors_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_bootstrap_messages_rx_errors_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_bootstrap_messages_rx_errors_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_assert_messages_received_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_assert_messages_received_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_assert_messages_received_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_assert_messages_received_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_assert_messages_sent_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_assert_messages_sent_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_assert_messages_sent_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_assert_messages_sent_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_assert_messages_rx_errors_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_assert_messages_rx_errors_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_assert_messages_rx_errors_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_assert_messages_rx_errors_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_graft_messages_received_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_graft_messages_received_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_graft_messages_received_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_graft_messages_received_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_graft_messages_sent_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_graft_messages_sent_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_graft_messages_sent_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_graft_messages_sent_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_graft_messages_rx_errors_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_graft_messages_rx_errors_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_graft_messages_rx_errors_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_graft_messages_rx_errors_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_graft_ack_messages_received_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_graft_ack_messages_received_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_graft_ack_messages_received_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_graft_ack_messages_received_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_graft_ack_messages_sent_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_graft_ack_messages_sent_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_graft_ack_messages_sent_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_graft_ack_messages_sent_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_graft_ack_messages_rx_errors_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_graft_ack_messages_rx_errors_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_graft_ack_messages_rx_errors_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_graft_ack_messages_rx_errors_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_candidate_rp_messages_received_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_candidate_rp_messages_received_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_candidate_rp_messages_received_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_candidate_rp_messages_received_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_candidate_rp_messages_sent_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_candidate_rp_messages_sent_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_candidate_rp_messages_sent_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_candidate_rp_messages_sent_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_candidate_rp_messages_rx_errors_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_candidate_rp_messages_rx_errors_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_candidate_rp_messages_rx_errors_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_candidate_rp_messages_rx_errors_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_unknown_type_messages_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_unknown_type_messages_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_unknown_type_messages_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_unknown_type_messages_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_unknown_version_messages_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_unknown_version_messages_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_unknown_version_messages_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_unknown_version_messages_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_neighbor_unknown_messages_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_neighbor_unknown_messages_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_neighbor_unknown_messages_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_neighbor_unknown_messages_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_bad_length_messages_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_bad_length_messages_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_bad_length_messages_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_bad_length_messages_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_bad_checksum_messages_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_bad_checksum_messages_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_bad_checksum_messages_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_bad_checksum_messages_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_bad_receive_interface_messages_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_bad_receive_interface_messages_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_bad_receive_interface_messages_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_bad_receive_interface_messages_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_interface_disabled_messages_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rx_interface_disabled_messages_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rx_interface_disabled_messages_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_interface_disabled_messages_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_register_not_rp_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rx_register_not_rp_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rx_register_not_rp_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_register_not_rp_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rp_filtered_source_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rp_filtered_source_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rp_filtered_source_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rp_filtered_source_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_unknown_register_stop_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_unknown_register_stop_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_unknown_register_stop_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_unknown_register_stop_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_join_prune_no_state_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rx_join_prune_no_state_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rx_join_prune_no_state_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_join_prune_no_state_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_graft_graft_ack_no_state_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rx_graft_graft_ack_no_state_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rx_graft_graft_ack_no_state_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_graft_graft_ack_no_state_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_graft_on_upstream_interface_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rx_graft_on_upstream_interface_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rx_graft_on_upstream_interface_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_graft_on_upstream_interface_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_candidate_rp_not_bsr_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rx_candidate_rp_not_bsr_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rx_candidate_rp_not_bsr_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_candidate_rp_not_bsr_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_bsr_when_bsr_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rx_bsr_when_bsr_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rx_bsr_when_bsr_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_bsr_when_bsr_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS

}

pim_pimstat_rx_bsr_not_rpf_interface_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rx_bsr_not_rpf_interface_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rx_bsr_not_rpf_interface_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_bsr_not_rpf_interface_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_unknown_hello_option_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rx_unknown_hello_option_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rx_unknown_hello_option_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_unknown_hello_option_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_data_no_state_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rx_data_no_state_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rx_data_no_state_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_data_no_state_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_rp_no_state_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rx_rp_no_state_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rx_rp_no_state_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_rp_no_state_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_aggregate_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rx_aggregate_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rx_aggregate_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_aggregate_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_malformed_packet_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rx_malformed_packet_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rx_malformed_packet_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_malformed_packet_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_no_rp_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_no_rp_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_no_rp_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_no_rp_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_no_route_upstream_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_no_route_upstream_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_no_route_upstream_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_no_route_upstream_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rp_mismatch_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rp_mismatch_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rp_mismatch_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rp_mismatch_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rpf_neighbor_unknown_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rpf_neighbor_unknown_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rpf_neighbor_unknown_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rpf_neighbor_unknown_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_join_rp_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rx_join_rp_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rx_join_rp_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_join_rp_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_prune_rp_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rx_prune_rp_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rx_prune_rp_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_prune_rp_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_join_wc_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rx_join_wc_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rx_join_wc_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_join_wc_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_prune_wc_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rx_prune_wc_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rx_prune_wc_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_prune_wc_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_join_sg_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rx_join_sg_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rx_join_sg_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_join_sg_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_prune_sg_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rx_prune_sg_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rx_prune_sg_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_prune_sg_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_join_sg_rpt_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rx_join_sg_rpt_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rx_join_sg_rpt_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_join_sg_rpt_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}

pim_pimstat_rx_prune_sg_rpt_per_vif()
{
    if [ $# -lt 1 ] ; then
	echo "Usage: pim_pimstat_rx_prune_sg_rpt_per_vif <vif_name:txt>"
	exit 1
    fi
    vif_name=$1
    
    echo "pim_pimstat_rx_prune_sg_rpt_per_vif" $*
    XRL="finder://$PIM_TARGET/pim/0.1/pimstat_rx_prune_sg_rpt_per_vif"
    XRL_ARGS="?vif_name:txt=$vif_name"
    call_xrl_wrapper -p value:u32 $XRL$XRL_ARGS
}
