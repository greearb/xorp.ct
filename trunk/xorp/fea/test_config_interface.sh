#!/bin/sh

#
# $XORP: xorp/fea/test_config_interface.sh,v 1.3 2003/10/21 02:05:57 pavlin Exp $
#

#
# Test interaction between the FEA and the kernel network interfaces:
#   - configure network interfaces, retrive network interface status, etc.
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

ADDR="10.20.0.1"
PREFIX_LEN="24"
BROADCAST="10.20.0.255"
ENDPOINT="INVALID"	# XXX: may be overwritten by host configuration
MTU="1500"
TEST_MTU="1400"

case ${HOSTNAME} in
	xorp1)
	IFNAME="fxp3"
	MAC="00:02:b3:10:e3:e7"
	TEST_MAC="00:02:b3:10:e3:e8"
	PIF_INDEX="5"
	VIF_FLAG_BROADCAST="true"
	VIF_FLAG_LOOPBACK="false"
	VIF_FLAG_POINT_TO_POINT="false"
	VIF_FLAG_MULTICAST="true"
	;;

	xorp4)
	IFNAME="eth3"
	MAC="00:04:5A:49:5D:11"
	TEST_MAC="0:4:5a:49:5d:12"
	PIF_INDEX="5"
	VIF_FLAG_BROADCAST="true"
	VIF_FLAG_LOOPBACK="false"
	VIF_FLAG_POINT_TO_POINT="false"
	VIF_FLAG_MULTICAST="true"
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
test_cleanup_interface4()
{
    echo "TEST: Cleanup interface ${IFNAME}"

    #
    # To make sure that the interface is cleaned, first explicity
    # try to add an address to it, then delete that address. In the process,
    # ignore any errors.
    #

    #
    # Add the address, and enable the interface
    #
    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_create_vif ${tid} ${IFNAME} ${VIFNAME}
    fea_ifmgr_create_address4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR}
    fea_ifmgr_set_prefix4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR} ${PREFIX_LEN}
    if [ "${VIF_FLAG_BROADCAST}" = "true" ] ; then
	fea_ifmgr_set_broadcast4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR} ${BROADCAST}
    fi
    if [ "${VIF_FLAG_POINT_TO_POINT}" = "true" ] ; then
	fea_ifmgr_set_endpoint4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR} ${ENDPOINT}
    fi
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} true
    fea_ifmgr_set_vif_enabled ${tid} ${IFNAME} ${VIFNAME} true
    fea_ifmgr_set_address_enabled4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR} true
    fea_ifmgr_commit_transaction ${tid}

    #
    # Delete the address, disable the interface, and reset the MTU
    #
    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} false
    fea_ifmgr_set_vif_enabled ${tid} ${IFNAME} ${VIFNAME} false
    fea_ifmgr_delete_address4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR}
    fea_ifmgr_set_mtu ${tid} ${IFNAME} ${MTU}
    fea_ifmgr_commit_transaction ${tid}

    #
    # Reset the MAC address to its original value
    #
    # Note that the interface must be DOWN to set the MAC address
    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_set_mac ${tid} ${IFNAME} ${MAC}
    fea_ifmgr_commit_transaction ${tid}

    #
    # Delete the configured vif, and delete the configured interface
    #
    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_delete_vif ${tid} ${IFNAME} ${VIFNAME}
    fea_ifmgr_delete_interface ${tid} ${IFNAME}
    fea_ifmgr_commit_transaction ${tid}
}

test_create_interface()
{
    echo "TEST: Create interface ${IFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_commit_transaction ${tid}
}

test_delete_interface()
{
    echo "TEST: Delete interface ${IFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_delete_interface ${tid} ${IFNAME}
    fea_ifmgr_commit_transaction ${tid}
}

test_is_interface_configured()
{
    local _xrl_result _ret_value _ifnames _ifnames_list _found

    echo "TEST: Check whether interface ${IFNAME} is configured"

    _xrl_result=`fea_ifmgr_get_configured_interface_names 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot get the list of configured interfaces:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _found=""
    _ifnames=`get_xrl_variable_value "${_xrl_result}" ifnames:list`
    # Get the list of space-separated interface names
    _ifnames_list=`split_xrl_list_values "${_ifnames}" txt`
    for i in ${_ifnames_list} ; do
	if [ "${IFNAME}" = "$i" ] ; then
	    _found="yes"
	    break;
	fi
    done

    if [ "${_found}" != "yes" ] ; then
	echo "ERROR: Expected interface ${IFNAME} not found:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_is_interface_not_configured()
{
    local _xrl_result _ret_value _ifnames _ifnames_list _found

    echo "TEST: Check whether interface ${IFNAME} is not configured"

    _xrl_result=`fea_ifmgr_get_configured_interface_names 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot get the list of configured interfaces:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _found=""
    _ifnames=`get_xrl_variable_value "${_xrl_result}" ifnames:list`
    # Get the list of space-separated interface names
    _ifnames_list=`split_xrl_list_values "${_ifnames}" txt`
    for i in ${_ifnames_list} ; do
	if [ "${IFNAME}" = "$i" ] ; then
	    _found="yes"
	    break;
	fi
    done

    if [ "${_found}" = "yes" ] ; then
	echo "ERROR: Non-expected interface ${IFNAME} was found:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_enable_interface()
{
    echo "TEST: Enable interface ${IFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} true
    fea_ifmgr_commit_transaction ${tid}
}

test_disable_interface()
{
    echo "TEST: Disable interface ${IFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} false
    fea_ifmgr_commit_transaction ${tid}
}

test_is_interface_enabled()
{
    local _xrl_result _ret_value _enabled

    echo "TEST: Whether interface ${IFNAME} is enabled"

    _xrl_result=`fea_ifmgr_get_configured_interface_enabled ${IFNAME} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot test whether interface ${IFNAME} is enabled:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _enabled=`get_xrl_variable_value "${_xrl_result}" enabled:bool`
    if [ "${_enabled}" != "true" ] ; then
	echo "ERROR: enabled status is ${_enabled}; expecting true"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_is_interface_disabled()
{
    local _xrl_result _ret_value _enabled

    echo "TEST: Whether interface ${IFNAME} is disabled"

    _xrl_result=`fea_ifmgr_get_configured_interface_enabled ${IFNAME} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot test whether interface ${IFNAME} is disabled:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _enabled=`get_xrl_variable_value "${_xrl_result}" enabled:bool`
    if [ "${_enabled}" != "false" ] ; then
	echo "ERROR: enabled status is ${_enabled}; expecting false"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_set_interface_mtu()
{
    echo "TEST: Set MTU on interface ${IFNAME} to ${TEST_MTU}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_set_mtu ${tid} ${IFNAME} ${TEST_MTU}
    fea_ifmgr_commit_transaction ${tid}
}

test_get_interface_mtu()
{
    local _xrl_result _ret_value _mtu

    echo "TEST: Get MTU on interface ${IFNAME}"

    _xrl_result=`fea_ifmgr_get_configured_mtu ${IFNAME} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot get the MTU:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _mtu=`get_xrl_variable_value "${_xrl_result}" mtu:u32`
    if [ "${_mtu}" != "${TEST_MTU}" ] ; then
	echo "ERROR: MTU is ${_mtu}; expecting ${TEST_MTU}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_set_interface_mac()
{
    echo "TEST: Set MAC address on interface ${IFNAME} to ${TEST_MAC}"

    #
    # XXX: Note that we must take the interface DOWN to set the MAC address
    # (limitation imposed by Linux).
    #

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} false
    fea_ifmgr_set_mac ${tid} ${IFNAME} ${TEST_MAC}
    fea_ifmgr_commit_transaction ${tid}
}

test_get_interface_mac()
{
    local _xrl_result _ret_value _mac

    echo "TEST: Get MAC address on interface ${IFNAME}"

    _xrl_result=`fea_ifmgr_get_configured_mac ${IFNAME} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot get the MAC address:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _mac=`get_xrl_variable_value "${_xrl_result}" mac:mac`
    if [ "${_mac}" != "${TEST_MAC}" ] ; then
	echo "ERROR: MAC address is ${_mac}; expecting ${TEST_MAC}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_create_vif()
{
    echo "TEST: Create vif ${VIFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_create_vif ${tid} ${IFNAME} ${VIFNAME}
    fea_ifmgr_commit_transaction ${tid}
}

test_delete_vif()
{
    echo "TEST: Delete vif ${VIFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_delete_vif ${tid} ${IFNAME} ${VIFNAME}
    fea_ifmgr_commit_transaction ${tid}
}

test_is_vif_configured()
{
    local _xrl_result _ret_value _vifs _vifs_list _found

    echo "TEST: Check whether vif ${VIFNAME} is configured"

    _xrl_result=`fea_ifmgr_get_configured_vif_names ${IFNAME} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot get the list of configured vifs:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _found=""
    _vifs=`get_xrl_variable_value "${_xrl_result}" vifs:list`
    # Get the list of space-separated vif names
    _vifs_list=`split_xrl_list_values "${_vifs}" txt`
    for i in ${_vifs_list} ; do
	if [ "${VIFNAME}" = "$i" ] ; then
	    _found="yes"
	    break;
	fi
    done

    if [ "${_found}" != "yes" ] ; then
	echo "ERROR: Expected vif ${IVFNAME} not found:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_is_vif_not_configured()
{
    local _xrl_result _ret_value _vifs _vifs_list _found

    echo "TEST: Check whether vif ${VIFNAME} is not configured"

    _xrl_result=`fea_ifmgr_get_configured_vif_names ${IFNAME} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot get the list of configured vifs:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _found=""
    _vifs=`get_xrl_variable_value "${_xrl_result}" vifs:list`
    # Get the list of space-separated vif names
    _vifs_list=`split_xrl_list_values "${_vifs}" txt`
    for i in ${_vifs_list} ; do
	if [ "${VIFNAME}" = "$i" ] ; then
	    _found="yes"
	    break;
	fi
    done

    if [ "${_found}" = "yes" ] ; then
	echo "ERROR: Non-expected vif ${VIFNAME} was found:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_enable_vif()
{
    echo "TEST: Enable vif ${VIFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_create_vif ${tid} ${IFNAME} ${VIFNAME}
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} true
    fea_ifmgr_set_vif_enabled ${tid} ${IFNAME} ${VIFNAME} true
    fea_ifmgr_commit_transaction ${tid}
}

test_disable_vif()
{
    echo "TEST: Disable vif ${VIFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_set_vif_enabled ${tid} ${IFNAME} ${VIFNAME} false
    fea_ifmgr_commit_transaction ${tid}
}

test_is_vif_enabled()
{
    local _xrl_result _ret_value _enabled

    echo "TEST: Whether vif ${VIFNAME} is enabled"

    _xrl_result=`fea_ifmgr_get_configured_vif_enabled ${IFNAME} ${VIFNAME} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot test whether vif ${VIFNAME} is enabled:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _enabled=`get_xrl_variable_value "${_xrl_result}" enabled:bool`
    if [ "${_enabled}" != "true" ] ; then
	echo "ERROR: enabled status is ${_enabled}; expecting true"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_is_vif_disabled()
{
    local _xrl_result _ret_value _enabled

    echo "TEST: Whether vif ${VIFNAME} is disabled"

    _xrl_result=`fea_ifmgr_get_configured_vif_enabled ${IFNAME} ${VIFNAME} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot test whether vif ${VIFNAME} is disabled:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _enabled=`get_xrl_variable_value "${_xrl_result}" enabled:bool`
    if [ "${_enabled}" != "false" ] ; then
	echo "ERROR: enabled status is ${_enabled}; expecting false"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_get_vif_pif_index()
{
    local _xrl_result _ret_value _pif_index

    echo "TEST: Get physical interface index on vif ${VIFNAME}"

    _xrl_result=`fea_ifmgr_get_configured_vif_pif_index ${IFNAME} ${VIFNAME} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot get the physical interface index:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _pif_index=`get_xrl_variable_value "${_xrl_result}" pif_index:u32`
    if [ "${_pif_index}" != "${PIF_INDEX}" ] ; then
	echo "ERROR: physical interface index is ${_pif_index}; expecting ${PIF_INDEX}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_get_vif_flags()
{
    local _xrl_result _ret_value _enabled _broadcast _loopback _point_to_point
    local _multicast

    echo "TEST: Get flags on vif ${VIFNAME}"

    #
    # First explicitly create and enable the interface
    #
    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_create_vif ${tid} ${IFNAME} ${VIFNAME}
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} true
    fea_ifmgr_set_vif_enabled ${tid} ${IFNAME} ${VIFNAME} true
    fea_ifmgr_commit_transaction ${tid}

    _xrl_result=`fea_ifmgr_get_configured_vif_flags ${IFNAME} ${VIFNAME} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot get the flags:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _enabled=`get_xrl_variable_value "${_xrl_result}" enabled:bool`
    if [ "${_enabled}" != "true" ] ; then
	echo "ERROR: enabled flag is ${_enabled}; expecting true"
	return 1
    fi
    _broadcast=`get_xrl_variable_value "${_xrl_result}" broadcast:bool`
    if [ "${_broadcast}" != "${VIF_FLAG_BROADCAST}" ] ; then
	echo "ERROR: broadcast flag is ${_broadcast}; expecting ${VIF_FLAG_BROADCAST}"
	return 1
    fi
    _loopback=`get_xrl_variable_value "${_xrl_result}" loopback:bool`
    if [ "${_loopback}" != "${VIF_FLAG_LOOPBACK}" ] ; then
	echo "ERROR: loopback flag is ${_loopback}; expecting ${VIF_FLAG_LOOPBACK}"
	return 1
    fi
    _point_to_point=`get_xrl_variable_value "${_xrl_result}" point_to_point:bool`
    if [ "${_point_to_point}" != "${VIF_FLAG_POINT_TO_POINT}" ] ; then
	echo "ERROR: point_to_point flag is ${_point_to_point}; expecting ${VIF_FLAG_POINT_TO_POINT}"
	return 1
    fi
    _multicast=`get_xrl_variable_value "${_xrl_result}" multicast:bool`
    if [ "${_multicast}" != "${VIF_FLAG_MULTICAST}" ] ; then
	echo "ERROR: multicast flag is ${_multicast}; expecting ${VIF_FLAG_MULTICAST}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_create_address4()
{
    echo "TEST: Create address ${ADDR} on interface ${IFNAME} vif ${VIFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_create_vif ${tid} ${IFNAME} ${VIFNAME}
    fea_ifmgr_create_address4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR}
    fea_ifmgr_set_prefix4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR} ${PREFIX_LEN}
    if [ "${VIF_FLAG_BROADCAST}" = "true" ] ; then
	fea_ifmgr_set_broadcast4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR} ${BROADCAST}
    fi
    if [ "${VIF_FLAG_POINT_TO_POINT}" = "true" ] ; then
	fea_ifmgr_set_endpoint4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR} ${ENDPOINT}
    fi
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} true
    fea_ifmgr_set_vif_enabled ${tid} ${IFNAME} ${VIFNAME} true
    fea_ifmgr_set_address_enabled4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR} true
    fea_ifmgr_commit_transaction ${tid}
}

test_delete_address4()
{
    echo "TEST: Delete address ${ADDR} on interface ${IFNAME} vif ${VIFNAME}"

    # Delete the address
    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_delete_address4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR}
    fea_ifmgr_commit_transaction ${tid}
}

test_get_address4()
{
    local _xrl_result _ret_value _addresses _addresses_list _found

    echo "TEST: Get IPv4 address on interface ${IFNAME} vif ${VIFNAME}"

    _xrl_result=`fea_ifmgr_get_configured_vif_addresses4 ${IFNAME} ${VIFNAME} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot get the IPv4 addresses on interface ${IFNAME}} vif ${VIFNAME}:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _found=""
    _addresses=`get_xrl_variable_value "${_xrl_result}" addresses:list`
    # Get the list of space-separated addresses
    _addresses_list=`split_xrl_list_values "${_addresses}" ipv4`
    for i in ${_addresses_list} ; do
	if [ "${ADDR}" = "$i" ] ; then
	    _found="yes"
	    break;
	fi
    done

    if [ "${_found}" != "yes" ] ; then
	echo "ERROR: Expected address ${ADDR} not found:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_get_prefix4()
{
    local _xrl_result _ret_value _prefix_len

    echo "TEST: Get prefix length on address ${ADDR} on interface ${IFNAME} vif ${VIF}"

    _xrl_result=`fea_ifmgr_get_configured_prefix4 ${IFNAME} ${VIFNAME} ${ADDR} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot get the prefix length:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _prefix_len=`get_xrl_variable_value "${_xrl_result}" prefix_len:u32`
    if [ "${_prefix_len}" != "${PREFIX_LEN}" ] ; then
	echo "ERROR: prefix length is ${_prefix_len}; expecting ${PREFIX_LEN}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_get_broadcast4()
{
    local _xrl_result _ret_value _broadcast

    echo "TEST: Get broadcast address on address ${ADDR} on interface ${IFNAME} vif ${VIFNAME}"

    if [ "${VIF_FLAG_BROADCAST}" = "false" ] ; then
	echo "INFO: ignoring: the interface is not broadcast-capable"
	return 0
    fi

    _xrl_result=`fea_ifmgr_get_configured_broadcast4 ${IFNAME} ${VIFNAME} ${ADDR} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot get the broadcast address:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _broadcast=`get_xrl_variable_value "${_xrl_result}" broadcast:ipv4`
    if [ "${_broadcast}" != "${BROADCAST}" ] ; then
	echo "ERROR: broadcast address is ${_broadcast}; expecting ${BROADCAST}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_get_endpoint4()
{
    local _xrl_result _ret_value _endpoint

    echo "TEST: Get endpoint address on address ${ADDR} on interface ${IFNAME} vif ${VIFNAME}"

    if [ "${VIF_FLAG_POINT_TO_POINT}" = "false" ] ; then
	echo "INFO: Test ignored: the interface is not point-to-point"
	return 0
    fi

    _xrl_result=`fea_ifmgr_get_configured_endpoint4 ${IFNAME} ${VIFNAME} ${ADDR} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot get the endpoint address:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _endpoint=`get_xrl_variable_value "${_xrl_result}" endpoint:ipv4`
    if [ "${_endpoint}" != "${ENDPOINT}" ] ; then
	echo "ERROR: endpoint address is ${_endpoint}; expecting ${ENDPOINT}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_get_address_flags4()
{
    local _xrl_result _ret_value _enabled _broadcast _loopback _point_to_point
    local _multicast

    echo "TEST: Get address flags on address ${ADDR} on interface ${IFNAME} vif ${VIFNAME}"

    _xrl_result=`fea_ifmgr_get_configured_address_flags4 ${IFNAME} ${VIFNAME} ${ADDR} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot get the address flags:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _enabled=`get_xrl_variable_value "${_xrl_result}" enabled:bool`
    if [ "${_enabled}" != "true" ] ; then
	echo "ERROR: enabled flag is ${_enabled}; expecting true"
	return 1
    fi
    _broadcast=`get_xrl_variable_value "${_xrl_result}" broadcast:bool`
    if [ "${_broadcast}" != "${VIF_FLAG_BROADCAST}" ] ; then
	echo "ERROR: broadcast flag is ${_broadcast}; expecting ${VIF_FLAG_BROADCAST}"
	return 1
    fi
    _loopback=`get_xrl_variable_value "${_xrl_result}" loopback:bool`
    if [ "${_loopback}" != "${VIF_FLAG_LOOPBACK}" ] ; then
	echo "ERROR: loopback flag is ${_loopback}; expecting ${VIF_FLAG_LOOPBACK}"
	return 1
    fi
    _point_to_point=`get_xrl_variable_value "${_xrl_result}" point_to_point:bool`
    if [ "${_point_to_point}" != "${VIF_FLAG_POINT_TO_POINT}" ] ; then
	echo "ERROR: point_to_point flag is ${_point_to_point}; expecting ${VIF_FLAG_POINT_TO_POINT}"
	return 1
    fi
    _multicast=`get_xrl_variable_value "${_xrl_result}" multicast:bool`
    if [ "${_multicast}" != "${VIF_FLAG_MULTICAST}" ] ; then
	echo "ERROR: multicast flag is ${_multicast}; expecting ${VIF_FLAG_MULTICAST}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_get_deleted_address4()
{
    local _xrl_result _ret_value _addresses _addresses_list _found

    echo "TEST: Get deleted IPv4 address on interface ${IFNAME} vif ${VIFNAME}"

    _xrl_result=`fea_ifmgr_get_configured_vif_addresses4 ${IFNAME} ${VIFNAME} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot get the IPv4 addresses on interface ${IFNAME} vif ${VIFNAME}:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _found=""
    _addresses=`get_xrl_variable_value "${_xrl_result}" addresses:list`
    # Get the list of space-separated addresses
    _addresses_list=`split_xrl_list_values "${_addresses}" ipv4`
    for i in ${_addresses_list} ; do
	if [ "${ADDR}" = "$i" ] ; then
	    _found="yes"
	    break;
	fi
    done

    if [ "${_found}" = "yes" ] ; then
	echo "ERROR: Non-expected address ${ADDR} was found:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_enable_address4()
{
    echo "TEST: Enable address ${ADDR} on interface ${IFNAME} vif ${VIFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_create_vif ${tid} ${IFNAME} ${VIFNAME}
    fea_ifmgr_create_address4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR}
    fea_ifmgr_set_prefix4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR} ${PREFIX_LEN}
    if [ "${VIF_FLAG_BROADCAST}" = "true" ] ; then
	fea_ifmgr_set_broadcast4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR} ${BROADCAST}
    fi
    if [ "${VIF_FLAG_POINT_TO_POINT}" = "true" ] ; then
	fea_ifmgr_set_endpoint4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR} ${ENDPOINT}
    fi
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} true
    fea_ifmgr_set_vif_enabled ${tid} ${IFNAME} ${VIFNAME} true
    fea_ifmgr_set_address_enabled4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR} true
    fea_ifmgr_commit_transaction ${tid}
}

test_disable_address4()
{
    echo "TEST: Disable address ${ADDR} on interface ${IFNAME} vif ${VIFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_set_address_enabled4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR} false
    fea_ifmgr_commit_transaction ${tid}
}

test_is_address_enabled4()
{
    local _xrl_result _ret_value _enabled

    echo "TEST: Whether address ${ADDR} is enabled on interface ${IFNAME} vif ${VIFNAME}"

    _xrl_result=`fea_ifmgr_get_configured_address_enabled4 ${IFNAME} ${VIFNAME} ${ADDR} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot test whether address ${ADDR} is enabled:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _enabled=`get_xrl_variable_value "${_xrl_result}" enabled:bool`
    if [ "${_enabled}" != "true" ] ; then
	echo "ERROR: enabled status is ${_enabled}; expecting true"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

test_is_address_disabled4()
{
    local _xrl_result _ret_value _enabled
    local _xrl_result2 _ret_value2 _addresses _addresses_list _found

    echo "TEST: Whether address ${ADDR} is disabled on interface ${IFNAME} vif ${VIFNAME}"

    _xrl_result=`fea_ifmgr_get_configured_address_enabled4 ${IFNAME} ${VIFNAME} ${ADDR} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	# Probably OK: e.g., if the address was deleted.
	# Verifying whether this is the case...
	_xrl_result2=`fea_ifmgr_get_configured_vif_addresses4 ${IFNAME} ${VIFNAME} 2>&1`
	_ret_value2=$?
	if [ ${_ret_value2} -ne 0 ] ; then
	    echo "ERROR: cannot get the IPv4 addresses on interface ${IFNAME} vif ${VIFNAME}:"
	    echo "${_xrl_result2}"
	    return 1
	fi

	#
	# Check the result
	#
	_found=""
	_addresses=`get_xrl_variable_value "${_xrl_result2}" addresses:list`
	# Get the list of space-separated addresses
	_addresses_list=`split_xrl_list_values "${_addresses}" ipv4`
	for i in ${_addresses_list} ; do
	    if [ "${ADDR}" = "$i" ] ; then
		_found="yes"
		break;
	    fi
	done

	if [ "${_found}" != "yes" ] ; then
	    echo "RESULT: OK: Address ${ADDR} was not found"
	    return 0
	fi

	echo "ERROR: cannot test whether address ${ADDR} is disabled:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _enabled=`get_xrl_variable_value "${_xrl_result}" enabled:bool`
    if [ "${_enabled}" != "false" ] ; then
	echo "ERROR: enabled status is ${_enabled}; expecting false"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

#
# The tests
#
TESTS=""
TESTS="$TESTS test_cleanup_interface4"
TESTS="$TESTS test_create_interface"
TESTS="$TESTS test_is_interface_configured"
TESTS="$TESTS test_delete_interface"
TESTS="$TESTS test_is_interface_not_configured"
TESTS="$TESTS test_enable_interface"
TESTS="$TESTS test_is_interface_enabled"
TESTS="$TESTS test_disable_interface"
TESTS="$TESTS test_is_interface_disabled"
TESTS="$TESTS test_set_interface_mtu"
TESTS="$TESTS test_get_interface_mtu"
TESTS="$TESTS test_set_interface_mac"
TESTS="$TESTS test_get_interface_mac"
TESTS="$TESTS test_create_vif"
TESTS="$TESTS test_is_vif_configured"
TESTS="$TESTS test_delete_vif"
TESTS="$TESTS test_is_vif_not_configured"
TESTS="$TESTS test_enable_vif"
TESTS="$TESTS test_is_vif_enabled"
TESTS="$TESTS test_disable_vif"
TESTS="$TESTS test_is_vif_disabled"
TESTS="$TESTS test_get_vif_pif_index"
TESTS="$TESTS test_get_vif_flags"
TESTS="$TESTS test_create_address4"
TESTS="$TESTS test_get_address4"
TESTS="$TESTS test_get_prefix4"
TESTS="$TESTS test_get_broadcast4"
TESTS="$TESTS test_get_endpoint4"
TESTS="$TESTS test_get_address_flags4"
TESTS="$TESTS test_delete_address4"
TESTS="$TESTS test_get_deleted_address4"
TESTS="$TESTS test_enable_address4"
TESTS="$TESTS test_is_address_enabled4"
TESTS="$TESTS test_disable_address4"
TESTS="$TESTS test_is_address_disabled4"
TESTS="$TESTS test_cleanup_interface4"

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
