#!/bin/sh

#
# $XORP: xorp/fea/test_add_route.sh,v 1.6 2003/10/21 18:59:31 pavlin Exp $
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

#
# XXX: file "xrl_fea_shell_funcs.sh" included below should have been
# auto-generated in the built directory
#
. ./xrl_fea_shell_funcs.sh


HOSTNAME=`hostname`
OS=`uname -s`

#
# Hosts configuration
#

DEST="10.30.0.0/24"
DEST2="10.40.0.0/24"
DEST_HOST="10.30.0.10"
DEST_HOST2="10.40.0.10"
METRIC="10"
ADMIN_DISTANCE="20"
PROTOCOL_ORIGIN="BGP"

case ${HOSTNAME} in
    xorp1)
    IFNAME="dc2"
    GATEWAY="10.8.0.2"
    ;;

    xorp4)
    IFNAME="eth6"
    GATEWAY="10.8.0.1"
    ;;

    *)
    echo "Unknown host : ${HOSTNAME}"
    exit 1
    ;;
esac

# XXX: for now the vifname is same as the ifname
VIFNAME="${IFNAME}"

#
# Test functions
#
test_have_ipv4()
{
    local _xrl_result _ret_value _result

    echo "TEST: Whether the underlying system supports IPv4"

    _xrl_result=`fea_fti_have_ipv4 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot test whether the underlying system supports IPv4:"
	echo "${_xrl_result}"
	return 1
    fi
    _result=`get_xrl_variable_value "${_xrl_result}" result:bool`
    if [ "${_result}" != "true" ] ; then
	echo "ERROR: result is ${_result}; expecting true"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_prepare_unicast_forwarding4()
{
    local _xrl_result _ret_value

    echo "TEST: Prepare IPv4 unicast forwarding tests (explicitly disable forwarding)"

    _xrl_result=`fea_fti_set_unicast_forwarding_enabled4 false 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot prepare IPv4 unicast forwarding tests:"
	echo "${_xrl_result}"
	return 1
    fi
}

test_enable_unicast_forwarding4()
{
    local _xrl_result _ret_value

    echo "TEST: Enable IPv4 unicast forwarding"

    _xrl_result=`fea_fti_set_unicast_forwarding_enabled4 true 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot enable IPv4 unicast forwarding:"
	echo "${_xrl_result}"
	return 1
    fi
}

test_disable_unicast_forwarding4()
{
    local _xrl_result _ret_value

    echo "TEST: Enable IPv4 unicast forwarding"

    _xrl_result=`fea_fti_set_unicast_forwarding_enabled4 false 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot enable IPv4 unicast forwarding:"
	echo "${_xrl_result}"
	return 1
    fi
}

test_get_enabled_unicast_forwarding4()
{
    local _xrl_result _ret_value _enabled

    echo "TEST: Whether IPv4 unicast forwarding is enabled"

    _xrl_result=`fea_fti_get_unicast_forwarding_enabled4 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot test whether IPv4 unicast forwarding is enabled:"
	echo "${_xrl_result}"
	return 1
    fi
    _enabled=`get_xrl_variable_value "${_xrl_result}" enabled:bool`
    if [ "${_enabled}" != "true" ] ; then
	echo "ERROR: enabled is ${_enabled}; expecting true"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
    
}

test_get_disabled_unicast_forwarding4()
{
    local _xrl_result _ret_value _disabled

    echo "TEST: Whether IPv4 unicast forwarding is disabled"

    _xrl_result=`fea_fti_get_unicast_forwarding_enabled4 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot test whether IPv4 unicast forwarding is disabled:"
	echo "${_xrl_result}"
	return 1
    fi
    _enabled=`get_xrl_variable_value "${_xrl_result}" enabled:bool`
    if [ "${_enabled}" != "false" ] ; then
	echo "ERROR: enabled is ${_enabled}; expecting false"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
    
}

test_cleanup_gateway4()
{
    local _xrl_result _ret_value _gateway

    echo "TEST: Clean-up gateway (if any) for destination ${DEST}"

    # Lookup the entry
    _gateway=""
    _xrl_result=`fea_fti_lookup_entry4 ${DEST} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -eq 0 ] ; then
	_gateway=`get_xrl_variable_value "${_xrl_result}" gateway:ipv4`
    fi
    if [ "${_gateway}" = "" ] ; then
	echo "RESULT: No gateway to delete for destination ${DEST}"
    else
	tid=`get_xrl_variable_value \`fea_fti_start_transaction\` tid:u32`
	if [ "${tid}" = "" ] ; then
	    echo "ERROR: cannot start transaction: cannot get transaction ID"
	    return 1
	fi
	fea_fti_delete_entry4 ${tid} ${DEST}
	fea_fti_commit_transaction ${tid}
    fi
}

test_add_entry4()
{
    echo "TEST: Add ${GATEWAY} as gateway for destination ${DEST}"

    tid=`get_xrl_variable_value \`fea_fti_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_fti_add_entry4 ${tid} ${DEST} ${GATEWAY} ${IFNAME} ${VIFNAME} ${METRIC} ${ADMIN_DISTANCE} ${PROTOCOL_ORIGIN}
    fea_fti_commit_transaction ${tid}
}

test_lookup_entry4()
{
    local _xrl_result _ret_value _gateway _ifname _vifname _metric
    local _admin_distance _protocol_origin

    echo "TEST: Look-up gateway for destination ${DEST}"

    _xrl_result=`fea_fti_lookup_entry4 ${DEST} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: routing entry was not found:"
	echo "${_xrl_result}"
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
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_lookup_route4()
{
    local _xrl_result _ret_value _gateway _ifname _vifname _metric
    local _admin_distance _protocol_origin

    echo "TEST: Look-up route for destination ${DEST_HOST}"

    _xrl_result=`fea_fti_lookup_route4 ${DEST_HOST} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: route was not found:"
	echo "${_xrl_result}"
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
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_delete_entry4()
{
    echo "TEST: Delete the gateway for destination ${DEST}"

    tid=`get_xrl_variable_value \`fea_fti_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_fti_delete_entry4 ${tid} ${DEST}
    fea_fti_commit_transaction ${tid}
}

test_lookup_deleted_entry4()
{
    local _xrl_result _ret_value

    echo "TEST: Look-up deleted entry for destination ${DEST}"

    _xrl_result=`fea_fti_lookup_entry4 ${DEST} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -eq 0 ] ; then
	echo "ERROR: routing entry was not deleted:"
	echo "${_xrl_result}"
	return 1
    fi

    return 0
}

test_lookup_deleted_route4()
{
    local _xrl_result _ret_value

    echo "TEST: Look-up deleted route for destination ${DEST_HOST}"

    echo "INFO: Sleeping for 3 seconds (to timeout any obsoleted cloned entries)..."
    sleep 3

    _xrl_result=`fea_fti_lookup_route4 ${DEST_HOST} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -eq 0 ] ; then
	echo "ERROR: routing entry was not deleted:"
	echo "${_xrl_result}"
	return 1
    fi

    return 0
}

test_delete_all_entries4()
{
    echo "TEST: Delete all entries installed by XORP"

    # Add the entries
    tid=`get_xrl_variable_value \`fea_fti_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_fti_add_entry4 ${tid} ${DEST} ${GATEWAY} ${IFNAME} ${VIFNAME} ${METRIC} ${ADMIN_DISTANCE} ${PROTOCOL_ORIGIN}
    fea_fti_add_entry4 ${tid} ${DEST2} ${GATEWAY} ${IFNAME} ${VIFNAME} ${METRIC} ${ADMIN_DISTANCE} ${PROTOCOL_ORIGIN}
    fea_fti_commit_transaction ${tid}

    # Check that the entries were installed
    _xrl_result=`fea_fti_lookup_entry4 ${DEST} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: routing entry for ${DEST} was not found:"
	echo "${_xrl_result}"
	return ${_ret_value}
    fi
    _xrl_result=`fea_fti_lookup_entry4 ${DEST2} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: routing entry for ${DEST2} was not found:"
	echo "${_xrl_result}"
	return ${_ret_value}
    fi

    # Delete all entries
    tid=`get_xrl_variable_value \`fea_fti_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_fti_delete_all_entries4 ${tid}
    fea_fti_commit_transaction ${tid}

    # Check that the entries were deleted
    _xrl_result=`fea_fti_lookup_entry4 ${DEST} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -eq 0 ] ; then
	echo "ERROR: routing entry for ${DEST} was not deleted:"
	echo "${_xrl_result}"
	return 1
    fi
    _xrl_result=`fea_fti_lookup_entry4 ${DEST2} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -eq 0 ] ; then
	echo "ERROR: routing entry for ${DEST2} was not deleted:"
	echo "${_xrl_result}"
	return 1
    fi
}

#
# The tests
#
TESTS=""
TESTS="$TESTS test_have_ipv4"
#
TESTS="$TESTS test_prepare_unicast_forwarding4"
TESTS="$TESTS test_enable_unicast_forwarding4"
TESTS="$TESTS test_get_enabled_unicast_forwarding4"
TESTS="$TESTS test_disable_unicast_forwarding4"
TESTS="$TESTS test_get_disabled_unicast_forwarding4"
#
TESTS="$TESTS test_cleanup_gateway4"
TESTS="$TESTS test_add_entry4"
TESTS="$TESTS test_lookup_entry4"
TESTS="$TESTS test_lookup_route4"
TESTS="$TESTS test_delete_entry4"
TESTS="$TESTS test_lookup_deleted_entry4"
TESTS="$TESTS test_lookup_deleted_route4"
TESTS="$TESTS test_delete_all_entries4"

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
exit 0

# Local Variables:
# mode: shell-script
# sh-indentation: 4
# End:
