#!/bin/sh

#
# $XORP: xorp/fea/test_add_route.sh,v 1.15 2003/10/28 07:34:43 pavlin Exp $
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

# TODO: the IPv6 addresses are bogus

DEST4="10.30.0.0/24"
DEST4_2="10.40.0.0/24"
DEST_HOST4="10.30.0.10"
DEST_HOST4_2="10.40.0.10"
DEST6="fe80:aaaa::0000/64"
DEST6_2="fe80:bbbb::0000/64"
DEST_HOST6="fe80:aaaa::aaaa"
DEST_HOST6_2="fe80:bbbb::bbbb"
METRIC="10"
ADMIN_DISTANCE="20"
PROTOCOL_ORIGIN="BGP"
HAVE_IPV6="true"	# XXX: may be overwritten by host configuration

case ${HOSTNAME} in
    xorp1)
    HAVE_IPV6="false"
    IFNAME="dc2"
    GATEWAY4="10.8.0.2"
    GATEWAY6="fe80:aaaa::1111"
    # TODO: IPv6 is temporary disabled
    ;;

    xorp4)
    HAVE_IPV6="false"
    IFNAME="eth6"
    GATEWAY4="10.8.0.1"
    GATEWAY6="fe80:aaaa::1111"
    ;;

    carp | carp.icir.org)
    case ${OS} in
	Linux)
	HAVE_IPV6="false"
	IFNAME="eth0"
	;;

	FreeBSD)
	HAVE_IPV6="false"
	IFNAME="xl0"
	;;

	NetBSD)
	HAVE_IPV6="false"
	IFNAME="ex0"
	;;

	OpenBSD)
	HAVE_IPV6="false"
	IFNAME="xl0"
	;;

	*)
	echo "Unknown OS : ${OS}"
	exit 1
	;;
    esac

    GATEWAY4="172.16.0.1"
    GATEWAY6="fe80:aaaa::1111"
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

    #
    # Check the result
    #
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

test_have_ipv6()
{
    local _xrl_result _ret_value _result

    echo "TEST: Whether the underlying system supports IPv6"

    _xrl_result=`fea_fti_have_ipv6 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot test whether the underlying system supports IPv6:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _result=`get_xrl_variable_value "${_xrl_result}" result:bool`
    if [ "${_result}" != "${HAVE_IPV6}" ] ; then
	echo "ERROR: result is ${_result}; expecting ${HAVE_IPV6}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

config_prepare_unicast_forwarding4()
{
    local _xrl_result _ret_value

    echo "INFO: Prepare IPv4 unicast forwarding tests (explicitly enable forwarding)"

    _xrl_result=`fea_fti_set_unicast_forwarding_enabled4 true 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot prepare IPv4 unicast forwarding tests:"
	echo "${_xrl_result}"
	return 1
    fi
}

config_prepare_unicast_forwarding6()
{
    local _xrl_result _ret_value

    echo "INFO: Prepare IPv6 unicast forwarding tests (explicitly enable forwarding)"

    _xrl_result=`fea_fti_set_unicast_forwarding_enabled6 true 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot prepare IPv6 unicast forwarding tests:"
	echo "${_xrl_result}"
	return 1
    fi
}

subtest_enable_unicast_forwarding4()
{
    local _xrl_result _ret_value

    echo "SUBTEST: Enable IPv4 unicast forwarding"

    _xrl_result=`fea_fti_set_unicast_forwarding_enabled4 true 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot enable IPv4 unicast forwarding:"
	echo "${_xrl_result}"
	return 1
    fi
}

subtest_enable_unicast_forwarding6()
{
    local _xrl_result _ret_value

    echo "SUBTEST: Enable IPv6 unicast forwarding"

    _xrl_result=`fea_fti_set_unicast_forwarding_enabled6 true 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot enable IPv6 unicast forwarding:"
	echo "${_xrl_result}"
	return 1
    fi
}

subtest_disable_unicast_forwarding4()
{
    local _xrl_result _ret_value

    echo "SUBTEST: Disable IPv4 unicast forwarding"

    _xrl_result=`fea_fti_set_unicast_forwarding_enabled4 false 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot disable IPv4 unicast forwarding:"
	echo "${_xrl_result}"
	return 1
    fi
}

subtest_disable_unicast_forwarding6()
{
    local _xrl_result _ret_value

    echo "SUBTEST: Disable IPv6 unicast forwarding"

    _xrl_result=`fea_fti_set_unicast_forwarding_enabled6 false 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot disable IPv6 unicast forwarding:"
	echo "${_xrl_result}"
	return 1
    fi
}

subtest_get_enabled_unicast_forwarding4()
{
    local _xrl_result _ret_value _enabled

    echo "SUBTEST: Whether IPv4 unicast forwarding is enabled"

    _xrl_result=`fea_fti_get_unicast_forwarding_enabled4 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot test whether IPv4 unicast forwarding is enabled:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
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

subtest_get_enabled_unicast_forwarding6()
{
    local _xrl_result _ret_value _enabled

    echo "SUBTEST: Whether IPv6 unicast forwarding is enabled"

    _xrl_result=`fea_fti_get_unicast_forwarding_enabled6 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot test whether IPv6 unicast forwarding is enabled:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
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

subtest_get_disabled_unicast_forwarding4()
{
    local _xrl_result _ret_value _enabled

    echo "SUBTEST: Whether IPv4 unicast forwarding is disabled"

    _xrl_result=`fea_fti_get_unicast_forwarding_enabled4 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot test whether IPv4 unicast forwarding is disabled:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
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

subtest_get_disabled_unicast_forwarding6()
{
    local _xrl_result _ret_value _enabled

    echo "SUBTEST: Whether IPv6 unicast forwarding is disabled"

    _xrl_result=`fea_fti_get_unicast_forwarding_enabled6 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot test whether IPv6 unicast forwarding is disabled:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
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

test_enable_disable_unicast_forwarding4()
{
    local _ret_value _subtests

    echo "TEST: Enable/disable IPv4 unicast forwarding"

    _subtests=""
    _subtests="${_subtests} config_prepare_unicast_forwarding4"
    _subtests="${_subtests} subtest_disable_unicast_forwarding4"
    _subtests="${_subtests} subtest_get_disabled_unicast_forwarding4"
    _subtests="${_subtests} subtest_enable_unicast_forwarding4"
    _subtests="${_subtests} subtest_get_enabled_unicast_forwarding4"

    for t in ${_subtests} ; do
	$t
	_ret_value=$?
	if [ ${_ret_value} -ne 0 ] ; then
	    return ${_ret_value}
	fi
    done
}

test_enable_disable_unicast_forwarding6()
{
    local _ret_value _subtests

    echo "TEST: Enable/disable IPv6 unicast forwarding"

    _subtests=""
    _subtests="${_subtests} config_prepare_unicast_forwarding6"
    _subtests="${_subtests} subtest_disable_unicast_forwarding6"
    _subtests="${_subtests} subtest_get_disabled_unicast_forwarding6"
    _subtests="${_subtests} subtest_enable_unicast_forwarding6"
    _subtests="${_subtests} subtest_get_enabled_unicast_forwarding6"

    for t in ${_subtests} ; do
	$t
	_ret_value=$?
	if [ ${_ret_value} -ne 0 ] ; then
	    return ${_ret_value}
	fi
    done
}

config_cleanup_gateway4()
{
    local _xrl_result _ret_value _gateway

    echo "INFO: Cleanup gateway (if any) for destination ${DEST4}"

    # Lookup the entry
    _gateway=""
    _xrl_result=`fea_fti_lookup_entry4 ${DEST4} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -eq 0 ] ; then
	_gateway=`get_xrl_variable_value "${_xrl_result}" gateway:ipv4`
    fi
    if [ "${_gateway}" = "" ] ; then
	# No gateway to delete for destination ${DEST4}"
	return 0
    else
	tid=`get_xrl_variable_value \`fea_fti_start_transaction\` tid:u32`
	if [ "${tid}" = "" ] ; then
	    echo "ERROR: cannot start transaction: cannot get transaction ID"
	    return 1
	fi
	fea_fti_delete_entry4 ${tid} ${DEST4}
	fea_fti_commit_transaction ${tid}
    fi
}

config_cleanup_gateway6()
{
    local _xrl_result _ret_value _gateway

    echo "INFO: Cleanup gateway (if any) for destination ${DEST6}"

    # Lookup the entry
    _gateway=""
    _xrl_result=`fea_fti_lookup_entry6 ${DEST6} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -eq 0 ] ; then
	_gateway=`get_xrl_variable_value "${_xrl_result}" gateway:ipv6`
    fi
    if [ "${_gateway}" = "" ] ; then
	# No gateway to delete for destination ${DEST6}"
	return 0
    else
	tid=`get_xrl_variable_value \`fea_fti_start_transaction\` tid:u32`
	if [ "${tid}" = "" ] ; then
	    echo "ERROR: cannot start transaction: cannot get transaction ID"
	    return 1
	fi
	fea_fti_delete_entry6 ${tid} ${DEST6}
	fea_fti_commit_transaction ${tid}
    fi
}

subtest_add_entry4()
{
    echo "SUBTEST: Add ${GATEWAY4} as gateway for destination ${DEST4}"

    tid=`get_xrl_variable_value \`fea_fti_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_fti_add_entry4 ${tid} ${DEST4} ${GATEWAY4} ${IFNAME} ${VIFNAME} ${METRIC} ${ADMIN_DISTANCE} ${PROTOCOL_ORIGIN}
    fea_fti_commit_transaction ${tid}
}

subtest_add_entry6()
{
    echo "SUBTEST: Add ${GATEWAY6} as gateway for destination ${DEST6}"

    tid=`get_xrl_variable_value \`fea_fti_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_fti_add_entry6 ${tid} ${DEST6} ${GATEWAY6} ${IFNAME} ${VIFNAME} ${METRIC} ${ADMIN_DISTANCE} ${PROTOCOL_ORIGIN}
    fea_fti_commit_transaction ${tid}
}

subtest_lookup_entry4()
{
    local _xrl_result _ret_value _gateway _ifname _vifname _metric
    local _admin_distance _protocol_origin

    echo "SUBTEST: Lookup gateway for destination ${DEST4}"

    _xrl_result=`fea_fti_lookup_entry4 ${DEST4} 2>&1`
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
    if [ "${_gateway}" != "${GATEWAY4}" ] ; then
	echo "ERROR: gateway is ${_gateway}; expecting ${GATEWAY4}"
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

subtest_lookup_entry6()
{
    local _xrl_result _ret_value _gateway _ifname _vifname _metric
    local _admin_distance _protocol_origin

    echo "SUBTEST: Lookup gateway for destination ${DEST6}"

    _xrl_result=`fea_fti_lookup_entry6 ${DEST6} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: routing entry was not found:"
	echo "${_xrl_result}"
	return ${_ret_value}
    fi
    _gateway=`get_xrl_variable_value ${_xrl_result} gateway:ipv6`
    _ifname=`get_xrl_variable_value ${_xrl_result} ifname:txt`
    _vifname=`get_xrl_variable_value ${_xrl_result} vifname:txt`
    _metric=`get_xrl_variable_value ${_xrl_result} metric:u32`
    _admin_distance=`get_xrl_variable_value ${_xrl_result} admin_distance:u32`
    _protocol_origin=`get_xrl_variable_value ${_xrl_result} protocol_origin:txt`

    #
    # Check the result
    #
    if [ "${_gateway}" != "${GATEWAY6}" ] ; then
	echo "ERROR: gateway is ${_gateway}; expecting ${GATEWAY6}"
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

subtest_lookup_route4()
{
    local _xrl_result _ret_value _gateway _ifname _vifname _metric
    local _admin_distance _protocol_origin

    echo "SUBTEST: Lookup route for destination ${DEST_HOST4}"

    _xrl_result=`fea_fti_lookup_route4 ${DEST_HOST4} 2>&1`
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
    if [ "${_gateway}" != "${GATEWAY4}" ] ; then
	echo "ERROR: gateway is ${_gateway}; expecting ${GATEWAY4}"
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

subtest_lookup_route6()
{
    local _xrl_result _ret_value _gateway _ifname _vifname _metric
    local _admin_distance _protocol_origin

    echo "SUBTEST: Lookup route for destination ${DEST_HOST6}"

    _xrl_result=`fea_fti_lookup_route6 ${DEST_HOST6} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: route was not found:"
	echo "${_xrl_result}"
	return ${_ret_value}
    fi
    _gateway=`get_xrl_variable_value ${_xrl_result} gateway:ipv6`
    _ifname=`get_xrl_variable_value ${_xrl_result} ifname:txt`
    _vifname=`get_xrl_variable_value ${_xrl_result} vifname:txt`
    _metric=`get_xrl_variable_value ${_xrl_result} metric:u32`
    _admin_distance=`get_xrl_variable_value ${_xrl_result} admin_distance:u32`
    _protocol_origin=`get_xrl_variable_value ${_xrl_result} protocol_origin:txt`

    #
    # Check the result
    #
    if [ "${_gateway}" != "${GATEWAY6}" ] ; then
	echo "ERROR: gateway is ${_gateway}; expecting ${GATEWAY6}"
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

subtest_delete_entry4()
{
    echo "SUBTEST: Delete the gateway for destination ${DEST4}"

    tid=`get_xrl_variable_value \`fea_fti_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_fti_delete_entry4 ${tid} ${DEST4}
    fea_fti_commit_transaction ${tid}
}

subtest_delete_entry6()
{
    echo "SUBTEST: Delete the gateway for destination ${DEST6}"

    tid=`get_xrl_variable_value \`fea_fti_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_fti_delete_entry6 ${tid} ${DEST6}
    fea_fti_commit_transaction ${tid}
}

subtest_lookup_deleted_entry4()
{
    local _xrl_result _ret_value

    echo "SUBTEST: Lookup deleted entry for destination ${DEST4}"

    _xrl_result=`fea_fti_lookup_entry4 ${DEST4} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -eq 0 ] ; then
	echo "ERROR: routing entry was not deleted:"
	echo "${_xrl_result}"
	return 1
    fi

    return 0
}

subtest_lookup_deleted_entry6()
{
    local _xrl_result _ret_value

    echo "SUBTEST: Lookup deleted entry for destination ${DEST6}"

    _xrl_result=`fea_fti_lookup_entry6 ${DEST6} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -eq 0 ] ; then
	echo "ERROR: routing entry was not deleted:"
	echo "${_xrl_result}"
	return 1
    fi

    return 0
}

subtest_lookup_deleted_route4()
{
    local _xrl_result _ret_value _ipv4net

    echo "SUBTEST: Lookup deleted route for destination ${DEST_HOST4}"

    if [ "${OS}" = "Linux" ] ; then
	echo "INFO: Sleeping for 3 seconds in case of Linux (to timeout any obsoleted cloned entries)..."
	sleep 3
    fi

    _xrl_result=`fea_fti_lookup_route4 ${DEST_HOST4} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	# OK: the entry was deleted
	return 0
    fi

    # There is a matching routing entry: check that this is not the default one
    _ipv4net=`get_xrl_variable_value "${_xrl_result}" netmask:ipv4net`
    if [ "${_ipv4net}" = "0.0.0.0/0" ] ; then
	# OK: this is the default routing entry
	return 0
    fi

    echo "ERROR: routing entry was not deleted:"
    echo "${_xrl_result}"
    return 1
}

subtest_lookup_deleted_route6()
{
    local _xrl_result _ret_value _ipv6net

    echo "SUBTEST: Lookup deleted route for destination ${DEST_HOST6}"

    if [ "${OS}" = "Linux" ] ; then
	echo "INFO: Sleeping for 3 seconds in case of Linux (to timeout any obsoleted cloned entries)..."
	sleep 3
    fi

    _xrl_result=`fea_fti_lookup_route6 ${DEST_HOST6} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	# OK: the entry was deleted
	return 0
    fi

    # There is a matching routing entry: check that this is not the default one
    _ipv6net=`get_xrl_variable_value "${_xrl_result}" netmask:ipv6net`
    if [ "${_ipv6net}" = "::" ] ; then
	# OK: this is the default routing entry
	return 0
    fi

    echo "ERROR: routing entry was not deleted:"
    echo "${_xrl_result}"
    return 1
}

test_add_delete_unicast_forwarding_entry4()
{
    local _ret_value _subtests

    echo "TEST: Add/delete IPv4 unicast forwarding entry"

    _subtests=""
    _subtests="${_subtests} config_cleanup_gateway4"
    _subtests="${_subtests} subtest_add_entry4"
    _subtests="${_subtests} subtest_lookup_entry4"
    _subtests="${_subtests} subtest_lookup_route4"
    _subtests="${_subtests} subtest_delete_entry4"
    _subtests="${_subtests} subtest_lookup_deleted_entry4"
    # Comment-out the test below, because in case of Linux a cloned entry
    # from the default route may be kept in the kernel for very long time.
    # _subtests="${_subtests} subtest_lookup_deleted_route4"

    for t in ${_subtests} ; do
	$t
	_ret_value=$?
	if [ ${_ret_value} -ne 0 ] ; then
	    return ${_ret_value}
	fi
    done
}

test_add_delete_unicast_forwarding_entry6()
{
    local _ret_value _subtests

    echo "TEST: Add/delete IPv6 unicast forwarding entry"

    _subtests=""
    _subtests="${_subtests} config_cleanup_gateway6"
    _subtests="${_subtests} subtest_add_entry6"
    _subtests="${_subtests} subtest_lookup_entry6"
    _subtests="${_subtests} subtest_lookup_route6"
    _subtests="${_subtests} subtest_delete_entry6"
    _subtests="${_subtests} subtest_lookup_deleted_entry6"
    # Comment-out the test below, because in case of Linux a cloned entry
    # from the default route may be kept in the kernel for very long time.
    #_subtests="${_subtests} subtest_lookup_deleted_route6"

    for t in ${_subtests} ; do
	$t
	_ret_value=$?
	if [ ${_ret_value} -ne 0 ] ; then
	    return ${_ret_value}
	fi
    done
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
    fea_fti_add_entry4 ${tid} ${DEST4} ${GATEWAY4} ${IFNAME} ${VIFNAME} ${METRIC} ${ADMIN_DISTANCE} ${PROTOCOL_ORIGIN}
    fea_fti_add_entry4 ${tid} ${DEST4_2} ${GATEWAY4} ${IFNAME} ${VIFNAME} ${METRIC} ${ADMIN_DISTANCE} ${PROTOCOL_ORIGIN}
    fea_fti_commit_transaction ${tid}

    # Check that the entries were installed
    _xrl_result=`fea_fti_lookup_entry4 ${DEST4} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: routing entry for ${DEST4} was not found:"
	echo "${_xrl_result}"
	return ${_ret_value}
    fi
    _xrl_result=`fea_fti_lookup_entry4 ${DEST4_2} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: routing entry for ${DEST4_2} was not found:"
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
    _xrl_result=`fea_fti_lookup_entry4 ${DEST4} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -eq 0 ] ; then
	echo "ERROR: routing entry for ${DEST4} was not deleted:"
	echo "${_xrl_result}"
	return 1
    fi
    _xrl_result=`fea_fti_lookup_entry4 ${DEST4_2} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -eq 0 ] ; then
	echo "ERROR: routing entry for ${DEST4_2} was not deleted:"
	echo "${_xrl_result}"
	return 1
    fi
}

test_delete_all_entries6()
{
    echo "TEST: Delete all entries installed by XORP"

    # Add the entries
    tid=`get_xrl_variable_value \`fea_fti_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_fti_add_entry6 ${tid} ${DEST6} ${GATEWAY6} ${IFNAME} ${VIFNAME} ${METRIC} ${ADMIN_DISTANCE} ${PROTOCOL_ORIGIN}
    fea_fti_add_entry6 ${tid} ${DEST6_2} ${GATEWAY6} ${IFNAME} ${VIFNAME} ${METRIC} ${ADMIN_DISTANCE} ${PROTOCOL_ORIGIN}
    fea_fti_commit_transaction ${tid}

    # Check that the entries were installed
    _xrl_result=`fea_fti_lookup_entry6 ${DEST6} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: routing entry for ${DEST6} was not found:"
	echo "${_xrl_result}"
	return ${_ret_value}
    fi
    _xrl_result=`fea_fti_lookup_entry6 ${DEST6_2} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: routing entry for ${DEST6_2} was not found:"
	echo "${_xrl_result}"
	return ${_ret_value}
    fi

    # Delete all entries
    tid=`get_xrl_variable_value \`fea_fti_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_fti_delete_all_entries6 ${tid}
    fea_fti_commit_transaction ${tid}

    # Check that the entries were deleted
    _xrl_result=`fea_fti_lookup_entry6 ${DEST6} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -eq 0 ] ; then
	echo "ERROR: routing entry for ${DEST6} was not deleted:"
	echo "${_xrl_result}"
	return 1
    fi
    _xrl_result=`fea_fti_lookup_entry6 ${DEST6_2} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -eq 0 ] ; then
	echo "ERROR: routing entry for ${DEST6_2} was not deleted:"
	echo "${_xrl_result}"
	return 1
    fi
}

#
# The tests
#
TESTS_NOT_FIXED=""
TESTS=""
TESTS="$TESTS test_have_ipv4"
# Test IPv4 unicast forwarding enabling/disabling
TESTS="$TESTS test_enable_disable_unicast_forwarding4"
# Test adding/deleting and lookup of IPv4 forwarding entries
TESTS="$TESTS test_add_delete_unicast_forwarding_entry4"
TESTS="$TESTS test_delete_all_entries4"
if [ "${HAVE_IPV6}" = "true" ] ; then
    # Test IPv6 unicast forwarding enabling/disabling
    TESTS="$TESTS test_enable_disable_unicast_forwarding6"
    # Test adding/deleting and lookup of IPv6 forwarding entries
    TESTS="$TESTS test_add_delete_unicast_forwarding_entry6"
    TESTS="$TESTS test_delete_all_entries6"
fi

# Include command line
. ${srcdir}/../utils/args.sh

if [ $START_PROGRAMS = "yes" ] ; then
    CXRL="../libxipc/call_xrl -w 10 -r 10"
    ../utils/runit $QUIET $VERBOSE -c "$0 -s -c $*" <<EOF
    ../libxipc/xorp_finder
    ../fea/xorp_fea        = $CXRL finder://fea/common/0.1/get_target_name
EOF
    exit $?
fi

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
