#!/usr/bin/env bash

LOC="unknown"

if [ "_$CALLXRL" == "_" ]
then

    if [ "_$TREETOP" == "_" ]
	then
	if [ -f "../../xorp_config.h" ]
	    then
	    # in build tree
	    TREETOP="../.."
	fi
    fi

    if [ -x "$TREETOP/libxipc/call_xrl" ]
	then
	# Must be in build tree
	LOC="build tree"
	CALLXRL=$TREETOP/libxipc/call_xrl
	# Shell funcs are not copied into build-dir.
	FEA_FUNCS=$TREETOP/../../fea/fea_xrl_shell_funcs.sh
	BGP_FUNCS=$TREETOP/../../bgp/bgp_xrl_shell_funcs.sh
	RIB_FUNCS=$TREETOP/../../rib/rib_xrl_shell_funcs.sh
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TREETOP/fea/data_plane/managers
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TREETOP/fea/data_plane/fibconfig
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TREETOP/fea/data_plane/ifconfig
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TREETOP/fea/data_plane/io
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TREETOP/fea/data_plane/control_socket
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TREETOP/fea/data_plane/firewall
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TREETOP/cli
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TREETOP/cli/libtecla
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TREETOP/xrl/interfaces
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TREETOP/libxipc
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$TREETOP/lib
    else
	if [ -x "$TREETOP/sbin/call_xrl" ]
	    then
            # Must be installed
	    LOC="installed"
	    CALLXRL=$TREETOP/sbin/call_xrl
	    FEA_FUNCS=$TREETOP/sbin/fea_xrl_shell_funcs.sh
	    BGP_FUNCS=$TREETOP/sbin/bgp_xrl_shell_funcs.sh
	    RIB_FUNCS=$TREETOP/sbin/rib_xrl_shell_funcs.sh
	else
	    LOC="somewhere else"
	    FEA_FUNCS=/usr/local/xorp/sbin/fea_xrl_shell_funcs.sh
	    BGP_FUNCS=/usr/local/xorp/sbin/bgp_xrl_shell_funcs.sh
	    RIB_FUNCS=/usr/local/xorp/sbin/rib_xrl_shell_funcs.sh
	    # Look in $PATH and default install location.
	    if which call_xrl > /dev/null 2>&1
		then
		CALLXRL=call_xrl
	    else
		CALLXRL=/usr/local/xorp/sbin/call_xrl
	    fi
	fi
    fi
fi

if [ "_$XORP_FINDER" == "_" ]
then
    if [ -x ../../libxipc/xorp_finder ]
	then
	XORP_FINDER="../../libxipc/xorp_finder"
    else
	XORP_FINDER="../../lib/xorp/sbin/xorp_finder"
    fi
fi

if [ "_$XORP_FEA_DUMMY" == "_" ]
then
    if [ -x ../../fea/xorp_fea_dummy ]
	then
	XORP_FEA_DUMMY="../../fea/xorp_fea_dummy"
    else
	XORP_FEA_DUMMY="../../lib/xorp/sbin/xorp_fea_dummy"
    fi
fi

if [ "_$XORP_RIB" == "_" ]
then
    if [ -x ../../rib/xorp_rib ]
	then
	XORP_RIB="../../rib/xorp_rib"
    else
	XORP_RIB="../../lib/xorp/sbin/xorp_rib"
    fi
fi

if [ "_$XORP_BGP" == "_" ]
then
    if [ -x ../xorp_bgp ]
	then
	XORP_BGP="../xorp_bgp"
    else
	XORP_BGP="../../lib/xorp/sbin/xorp_bgp"
    fi
fi

if [ "_$PRINT_PEERS" == "_" ]
then
    if [ -x ../tools/bgp_print_peers ]
	then
	PRINT_PEERS="../tools/bgp_print_peers"
    else
	PRINT_PEERS="../../lib/xorp/bin/bgp_print_peers"
    fi
fi



export CALLXRL
export XORP_FINDER
export FEA_FUNCS
export BGP_FUNCS
export RIB_FUNCS
export XORP_FEA_DUMMY
export XORP_RIB
export XORP_BGP
export PRINT_PEERS
export LD_LIBRARY_PATH

#echo "setup-script: LOC: $LOC  pwd: `pwd`"
#echo "   TREETOP: $TREETOP"
#echo "   LD_LIBRARY_PATH: $LD_LIBRARY_PATH"
