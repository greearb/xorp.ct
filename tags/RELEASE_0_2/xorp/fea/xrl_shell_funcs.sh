#!/bin/sh

CALLXRL=${CALLXRL:-../libxipc/call_xrl}
XRLDIR=${XRLDIR:-../xrl}

get_all_interface_names()
{
    echo -n "get_all_interface_names"
    $CALLXRL "finder://fea/ifmgr/0.1/get_all_interface_names"
}

get_configured_interface_names()
{
    echo -n "get_configured_interface_names"
    $CALLXRL "finder://fea/ifmgr/0.1/get_configured_interface_names"
}

start_fea_transaction()
{
    if [ $# -eq 0 ] ; then
	tid=`$CALLXRL "finder://fea/ifmgr/0.1/start_transaction" | sed 's/.*=//'`
	err=$?
	echo $tid
	return $err
    fi

    cat >&2 <<EOF
usage: start_transaction 
       resulting transaction id is echoed on stdout on success
EOF
    return 255
}

commit_fea_transaction()
{
    echo -n "commit_transaction"
    if [ $# -eq 1 ] ; then
	$CALLXRL "finder://fea/ifmgr/0.1/commit_transaction?tid:u32=$1"
	return $?
    fi
    cat >&2 <<EOF
usage: commit_transaction <tid>
       where <tid> is the id of the transaction to be committed.
EOF
    return 255
}

abort_fea_transaction()
{
    echo -n "abort_transaction"
    if [ $# -eq 1 ] ; then
	$CALLXRL "finder://fea/ifmgr/0.1/abort_transaction?tid:u32=$1"
	return $?
    fi
    cat >&2 <<EOF
usage: abort_transaction <tid>
       where <tid> is the id of the transaction to be aborted.
EOF
    return 255
}

create_interface()
{
    echo -n "create_interface" $*
    $CALLXRL "finder://fea/ifmgr/0.1/create_interface?tid:u32=$1&ifname:txt=$2"
}

delete_interface()
{
    echo -n "delete_interface" $*
    $CALLXRL "finder://fea/ifmgr/0.1/delete_interface?tid:u32=$1&ifname:txt=$2"
}

enable_interface()
{
    echo -n "enable_interface" $*
    $CALLXRL "finder://fea/ifmgr/0.1/set_interface_enabled?tid:u32=$1&ifname:txt=$2&enabled:bool=true"
}

disable_interface()
{
    echo -n "disable_interface" $*
    $CALLXRL "finder://fea/ifmgr/0.1/set_interface_enabled?tid:u32=$1&ifname:txt=$2&enabled:bool=false"
}

set_mac()
{
    echo -n "set_mac" $*
    $CALLXRL "finder://fea/ifmgr/0.1/set_mac?tid:u32=$1&ifname:txt=$2&mac:mac=$3"
}

get_mac()
{
    echo -n "get_mac" $*
    $CALLXRL "finder://fea/ifmgr/0.1/get_mac?ifname:txt=$1"
}

set_mtu()
{
    echo -n "set_mtu" $*
    $CALLXRL "finder://fea/ifmgr/0.1/set_mtu?tid:u32=$1&ifname:txt=$2&mtu:u32=$3"
}

get_mtu()
{
    echo -n "get_mtu" $*
    $CALLXRL "finder://fea/ifmgr/0.1/get_mtu?ifname:txt=$1"
}

get_all_vif_names()
{
    echo -n "get_all_vif_names" $*
    $CALLXRL "finder://fea/ifmgr/0.1/get_all_vif_names?ifname:txt=$1"
}

get_configured_vif_names()
{
    echo -n "get_configured_vif_names" $*
    $CALLXRL "finder://fea/ifmgr/0.1/get_configured_vif_names?ifname:txt=$1"
}

create_vif()
{
    echo -n "create_vif" $*
    $CALLXRL "finder://fea/ifmgr/0.1/create_vif?tid:u32=$1&ifname:txt=$2&vif:txt=$3"
}

delete_vif()
{
    echo -n "delete_vif" $*
    $CALLXRL "finder://fea/ifmgr/0.1/delete_vif?tid:u32=$1&ifname:txt=$2&vif:txt=$3"
}

enable_vif()
{
    echo -n "enable_vif" $*
    $CALLXRL "finder://fea/ifmgr/0.1/set_vif_enabled?tid:u32=$1&ifname:txt=$2&vif:txt=$3&enabled:bool=true"
}

disable_vif()
{
    echo -n "disable_vif" $*
    $CALLXRL "finder://fea/ifmgr/0.1/set_vif_enabled?tid:u32=$1&ifname:txt=$2&vif:txt=$3&enabled:bool=false"
}

get_all_vif_addresses4()
{
    echo -n "get_all_vif_addresses4" $*
    $CALLXRL "finder://fea/ifmgr/0.1/get_all_vif_addresses4?ifname:txt=$1&vif:txt=$2"
}

get_configured_vif_addresses4()
{
    echo -n "get_configured_vif_addresses4" $*
    $CALLXRL "finder://fea/ifmgr/0.1/get_configured_vif_addresses4?ifname:txt=$1&vif:txt=$2"
}

create_address4()
{
    echo -n "create_address4" $*
    $CALLXRL "finder://fea/ifmgr/0.1/create_address4?tid:u32=$1&ifname:txt=$2&vif:txt=$3&address:ipv4=$4"
}

delete_address4()
{
    echo -n "delete_address4" $*
    $CALLXRL "finder://fea/ifmgr/0.1/delete_address4?tid:u32=$1&ifname:txt=$2&vif:txt=$3&address:ipv4=$4"
}

enable_address4()
{
    echo -n "enable_address4" $*
    $CALLXRL "finder://fea/ifmgr/0.1/set_address_enabled4?tid:u32=$1&ifname:txt=$2&vif:txt=$3&address:ipv4=$4&enabled:bool=true"
}

disable_address4()
{
    echo -n "disable_address4" $*
    $CALLXRL "finder://fea/ifmgr/0.1/set_address_enabled4?tid:u32=$1&ifname:txt=$2&vif:txt=$3&address:ipv4=$4&enabled:bool=false"
}

set_prefix4()
{
    echo -n "set_prefix4" $*
    $CALLXRL "finder://fea/ifmgr/0.1/set_prefix4?tid:u32=$1&ifname:txt=$2&vif:txt=$3&address:ipv4=$4&prefix:u32=$5"
}

get_prefix4()
{
    echo -n "get_prefix4" $*
    $CALLXRL "finder://fea/ifmgr/0.1/get_prefix4?ifname:txt=$1&vif:txt=$2&address:ipv4=$3"
}

set_broadcast4()
{
    echo -n "set_broadcast4" $*
    $CALLXRL "finder://fea/ifmgr/0.1/set_broadcast4?tid:u32=$1&ifname:txt=$2&vif:txt=$3&address:ipv4=$4&broadcast:ipv4=$5"
}

get_broadcast4()
{
    echo -n "get_broadcast4" $*
    $CALLXRL "finder://fea/ifmgr/0.1/get_broadcast4?ifname:txt=$1&vif:txt=$2&address:ipv4=$3"
}

get_all_vif_addresses6()
{
    echo -n "get_vif_addresses6" $*
    $CALLXRL "finder://fea/ifmgr/0.1/get_all_vif_addresses6?ifname:txt=$1&vif:txt=$2"
}

get_configured_vif_addresses6()
{
    echo -n "get_configured_vif_addresses6" $*
    $CALLXRL "finder://fea/ifmgr/0.1/get_configured_vif_addresses6?ifname:txt=$1&vif:txt=$2"
}

create_address6()
{
    echo -n "create_address6" $*
    $CALLXRL "finder://fea/ifmgr/0.1/create_address6?tid:u32=$1&ifname:txt=$2&vif:txt=$3&address:ipv6=$4"
}

delete_address6()
{
    echo -n "delete_address6" $*
    $CALLXRL "finder://fea/ifmgr/0.1/delete_address6?tid:u32=$1&ifname:txt=$2&vif:txt=$3&address:ipv6=$4"
}

enable_address6()
{
    echo -n "enable_address6" $*
    $CALLXRL "finder://fea/ifmgr/0.1/set_address_enabled6?tid:u32=$1&ifname:txt=$2&vif:txt=$3&address:ipv6=$4&enabled:bool=true"
}

disable_address6()
{
    echo -n "disable_address6" $*
    $CALLXRL "finder://fea/ifmgr/0.1/set_address_enabled6?tid:u32=$1&ifname:txt=$2&vif:txt=$3&address:ipv6=$4&enabled:bool=false"
}

set_prefix6()
{
    echo -n "set_prefix6" $*
    $CALLXRL "finder://fea/ifmgr/0.1/set_prefix6?tid:u32=$1&ifname:txt=$2&vif:txt=$3&address:ipv6=$4&prefix:u32=$5"
}

get_prefix6()
{
    echo -n "get_prefix6" $*
    $CALLXRL "finder://fea/ifmgr/0.1/get_prefix6?ifname:txt=$1&vif:txt=$2&address:ipv6=$3"
}

start_fti_transaction()
{
    if [ $# -eq 0 ] ; then
	tid=`$CALLXRL "finder://fea/fti/0.1/start_transaction" | sed 's/.*=//'`
	err=$?
	echo $tid
	return $err
    fi

    cat >&2 <<EOF
usage: start_transaction 
       resulting transaction id is echoed on stdout on success
EOF
    return 255
}

commit_fti_transaction()
{
    echo -n "commit_transaction"
    if [ $# -eq 1 ] ; then
	$CALLXRL "finder://fea/fti/0.1/commit_transaction?tid:u32=$1"
	return $?
    fi
    cat >&2 <<EOF
usage: commit_transaction <tid>
       where <tid> is the id of the transaction to be committed.
EOF
    return 255
}

abort_fti_transaction()
{
    echo -n "abort_transaction"
    if [ $# -eq 1 ] ; then
	$CALLXRL "finder://fea/fti/0.1/abort_transaction?tid:u32=$1"
	return $?
    fi
    cat >&2 <<EOF
usage: abort_transaction <tid>
       where <tid> is the id of the transaction to be aborted.
EOF
    return 255
}

add_entry4()
{
    if [ $# -ne 5 ] ; then
	cat >&2 <<EOF
usage: add_entry4 <tid> <dest net> <gw> <ifname> <vifname>
eg:    add_entry4 6987662 187.1.0.0/16 164.27.13.1 ed0
EOF
	return 127
    fi

    $CALLXRL "finder://fea/fti/0.1/add_entry4?tid:u32=$1&dst:ipv4net=$2&gateway:ipv4=$3&ifname:txt=$4&vifname:txt=$5"
}

delete_entry4()
{
    if [ $# -ne 2 ] ; then
	cat >&2 <<EOF
usage: delete_entry4 <tid> <dest net>
eg:    delete_entry4 276567373 187.1.0.0/16
EOF
	return 127
    fi

    $CALLXRL "finder://fea/fti/0.1/delete_entry4?tid:u32=$1&dst:ipv4net=$2"
}

lookup_route4()
{
    echo -n "lookup_route4" $* "-> " 
    $CALLXRL "finder://fea/fti/0.1/lookup_route4?dst:ipv4=$1"
}

lookup_entry4()
{
    echo -n "lookup_entry4" $* "-> " 
    $CALLXRL "finder://fea/fti/0.1/lookup_entry4?dst:ipv4net=$1"
}

validate_xrls()
{
#
# Check that xrls in this script are valid.  Order of arguments is
# expected to match those in file containing Xrls: this is a laziness
# requirement and not an xrl requirement. A shell is almost certainly
# the wrong place for this.
#
# STOPLOOKING
#
    script_name="xrl_shell_funcs.sh"
    script_xrls=`cat $script_name | sed -n '1,/STOPLOOKING/p' | sed -n '/finder:\/\// p' | sed 's/[^"]*"\([^"]*\).*/\1/g' | sed 's/=[^-&]*//g' | sed 's/->.*//g'`
    source_xrl_file="$XRLDIR/targets/fea.xrls"
    source_xrls=`cat $source_xrl_file | grep '://' | sed 's/->.*//g'`

    match_count=0
    bad_count=0
    for i in $script_xrls ; do
	found="no"
	for j in $source_xrls ; do
	    if [ $i = $j ] ; then
		found="yes"
		match_count=`expr $match_count + 1`
		break
	    fi
	    stripped_i=`echo $i | sed 's/\?.*//'`
	    stripped_j=`echo $j | sed 's/\?.*//'`
	    if [ $stripped_i = $stripped_j ] ; then
		found="yes"
		echo "Warning mismatch:"
		echo "	script has \"$i\""
		echo "	file has \"$j\""
		bad_count=`expr $bad_count + 1`
		break
	    fi
	done
	if [ "$found" = "no" ] ; then
	    echo "No match for $i in source_xrl_file"
	    bad_count=`expr $bad_count + 1`
	fi
    done
    status="Summary: $match_count xrls okay, $bad_count xrls bad."
    rule=`echo $status | sed 's/[A-z0-9,:. ]/-/g'`
    echo $rule
    echo $status
    echo $rule
    echo $*
    unset script_xrls
    unset source_xrls
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
