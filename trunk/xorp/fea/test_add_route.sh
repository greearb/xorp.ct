#!/bin/sh

#
# $XORP$
#

#
# Test interaction between the FEA and the kernel unicast forwarding engine:
#   - add/delete unicast forwarding entries, lookup, etc.
#
# Preconditions:
# 1) Run a finder process (libxipc/xorp_finder)
# 2) Run a FEA process (fea/xorp_fea)
#

# Conditionally set ${srcdir} if it wasn't assigned (e.g., by `gmake check`)
if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi

. ${srcdir}/../utils/xrl_shell_lib.sh


HOSTNAME=`hostname`
OS=`uname -s`

#
# Hosts configuration
#

DEST="10.30.0.0/24"
METRIC="10"
ADMIN_DISTANCE="20"
PROTOCOL_ORIGIN="BGP"

case ${HOSTNAME} in
    xorp1)
    IFNAME="dc1"
    GATEWAY="10.2.0.2"
    ;;

    xorp4)
    IFNAME="eth2"
    GATEWAY="10.2.0.2"
    ;;

    *)
    echo "Unknown host : ${HOSTNAME}"
    exit 1
    ;;
esac

# XXX: for now the vifname is same as the ifname
VIFNAME="${IFNAME}"

#
# XRL helper functions
#
# TODO: those should go somewhere else...
#
fea_fti_start_transaction()
{
    if [ $# -ne 0 ] ; then
	echo "Usage: fea_fti_start_transaction"
	exit 1
    fi

    XRL="finder://fea/fti/0.2/start_transaction"
    call_xrl_wrapper -p all $XRL
}

fea_fti_commit_transaction()
{
    if [ $# -ne 1 ] ; then
	echo "Usage: fea_fti_commit_transaction <tid:u32>"
	exit 1
    fi

    XRL="finder://fea/fti/0.2/commit_transaction?tid:u32=$1"
    call_xrl_wrapper -p all $XRL
}

fea_fti_add_entry4()
{
    if [ $# -ne 8 ] ; then
	echo "Usage: fea_fti_add_entry4 <tid:u32> <dst:ipv4net> <gateway:ipv4> <ifname:txt> <vifname:txt> <metric:u32> <admin_distance:u32> <protocol_origin:txt>"
	exit 1
    fi

    XRL="finder://fea/fti/0.2/add_entry4?tid:u32=$1&dst:ipv4net=$2&gateway:ipv4=$3&ifname:txt=$4&vifname:txt=$5&metric:u32=$6&admin_distance:u32=$7&protocol_origin:txt=$8"
    call_xrl_wrapper -p all $XRL
}

fea_fti_delete_entry4()
{
    if [ $# -ne 2 ] ; then
	echo "Usage: fea_fti_delete_entry4 <tid:u32> <dst:ipv4net>"
	exit 1
    fi

    XRL="finder://fea/fti/0.2/delete_entry4?tid:u32=$1&dst:ipv4net=$2"
    call_xrl_wrapper -p all $XRL
}

fea_fti_lookup_entry4()
{
    if [ $# -ne 1 ] ; then
	echo "Usage: fea_fti_lookup_entry4 <dst:ipv4net>"
	exit 1
    fi

    XRL="finder://fea/fti/0.2/lookup_entry4?dst:ipv4net=$1"
    call_xrl_wrapper -p all $XRL
}

#
# Test functions
#
test_cleanup_gateway4()
{
    echo "TEST: Cleaning-up gateway (if any) for destination ${DEST}"

    # Lookup the entry
    gateway=`get_xrl_variable_value \`fea_fti_lookup_entry4 ${DEST}\` gateway:ipv4`
    if [ "${gateway}" = "" ] ; then
	echo "TEST RESULT: No gateway to delete for destination ${DEST}"
    else
	tid=`get_xrl_variable_value \`fea_fti_start_transaction\` tid:u32`
	fea_fti_delete_entry4 ${tid} ${DEST}
	fea_fti_commit_transaction ${tid}
    fi
}

test_add_entry4()
{
    echo "TEST: Adding ${GATEWAY} as gateway for destination ${DEST}"

    tid=`get_xrl_variable_value \`fea_fti_start_transaction\` tid:u32`
    fea_fti_add_entry4 ${tid} ${DEST} ${GATEWAY} ${IFNAME} ${VIFNAME} ${METRIC} ${ADMIN_DISTANCE} ${PROTOCOL_ORIGIN}
    fea_fti_commit_transaction ${tid}
}

test_lookup_entry4()
{
    local _xrl_result _xrl_value _gateway _ifname _vifname _metric
    local _admin_distance _protocol_origin

    echo "TEST: Looking-up gateway for destination ${DEST}"

    _xrl_result=`fea_fti_lookup_entry4 ${DEST}`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: ${_xrl_result}"
	return ${_ret_value}
    fi
    _gateway=`get_xrl_variable_value ${_xrl_result} gateway:ipv4`
    _ifname=`get_xrl_variable_value ${_xrl_result} ifname:txt`
    _vifname=`get_xrl_variable_value ${_xrl_result} vifname:txt`
    _metric=`get_xrl_variable_value ${_xrl_result} metric:u32`
    _admin_distance=`get_xrl_variable_value ${_xrl_result} admin_distance:u32`
    _protocol_origin=`get_xrl_variable_value ${_xrl_result} protocol_origin:txt`
    
    #
    # Check the result
    #
    if [ "${_gateway}" != "${GATEWAY}" ] ; then
	echo "ERROR: gateway is ${_gateway}; expecting ${GATEWAY}"
	return 1
    fi

    if [ "${_ifname}" != "${IFNAME}" ] ; then
	echo "ERROR: ifname is ${_ifname}; expecting ${IFNAME}"
	return 1
    fi
    if [ "${_vifname}" != "${VIFNAME}" ] ; then
	echo "ERROR: vifname is ${_vifname}; expecting ${VIFNAME}"
	return 1
    fi
    if [ "${_metric}" != "${METRIC}" ] ; then
	# XXX: for now we ignore the metric mismatch
	echo "WARNING: metric is ${_metric}; expecting ${METRIC}. Ignoring..."
	#return 1
    fi
    if [ "${_admin_distance}" != "${ADMIN_DISTANCE}" ] ; then
	# XXX: for now we ignore the admin_distance mismatch
	echo "WARNING: admin_distance is ${_admin_distance}; expecting ${ADMIN_DISTANCE}. Ignoring..."
	#return 1
    fi
    if [ "${_protocol_origin}" != "${PROTOCOL_ORIGIN}" ] ; then
	# XXX: for now we ignore the protocol_origin mismatch
	echo "WARNING: protocol_origin is ${_protocol_origin}; expecting ${PROTOCOL_ORIGIN}. Ignoring..."
	#return 1
    fi

    #
    # Print the result
    #
    echo "TEST RESULT:"
    echo ${_xrl_result}
}

test_delete_entry4()
{
    echo "TEST: Deleting the gateway for destination ${DEST}"

    tid=`get_xrl_variable_value \`fea_fti_start_transaction\` tid:u32`
    fea_fti_delete_entry4 ${tid} ${DEST}
    fea_fti_commit_transaction ${tid}
}

test_lookup_deleted_entry4()
{
    local _xrl_result _xrl_value _gateway _ifname _vifname _metric
    local _admin_distance _protocol_origin

    echo "TEST: Looking-up deleted gateway for destination ${DEST}"

    _xrl_result=`fea_fti_lookup_entry4 ${DEST}`
    _ret_value=$?
    if [ ${_ret_value} -eq 0 ] ; then
	echo "ERROR: routing entry was not deleted:"
	echo "${_xrl_result}"
	return 1
    fi

    return 0
}

#
# The tests
#
TESTS="test_cleanup_gateway4 test_add_entry4 test_lookup_entry4 test_delete_entry4 test_lookup_deleted_entry4"

for t in ${TESTS} ; do
    $t
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo
	echo "$0: Tests Failed"
	exit ${_ret_value}
    fi
done

echo
echo "$0: Tests Succeeded"

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
