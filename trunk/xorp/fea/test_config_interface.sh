#!/bin/sh

#
# $XORP: xorp/fea/test_add_route.sh,v 1.3 2003/10/16 22:15:46 pavlin Exp $
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
NETMASK="24"
BROADCAST="10.20.0.255"
MTU="1500"
TEST_MTU="1400"

case ${HOSTNAME} in
	xorp1)
	IFNAME="fxp3"
	MAC="00:02:b3:10:e3:e7"
	TEST_MAC="00:02:b3:10:e3:e8"
	;;

	xorp4)
	IFNAME="eth3"
	MAC="00:04:5A:49:5D:11"
	TEST_MAC="00:04:5A:49:5D:12"
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
    echo "TEST: Clean-up interface ${IFNAME}"

    #
    # To make sure that the interface is cleaned, first explicity
    # try to add an address to it, then delete that address. In the process,
    # ignore any errors.
    #

    # Add the address, and enable the interface
    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_create_vif ${tid} ${IFNAME} ${VIFNAME}
    fea_ifmgr_create_address4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR}
    fea_ifmgr_set_prefix4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR} ${NETMASK}
    fea_ifmgr_set_broadcast4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR} ${BROADCAST}
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} true
    fea_ifmgr_set_vif_enabled ${tid} ${IFNAME} ${VIFNAME} true
    fea_ifmgr_set_address_enabled4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR} true
    fea_ifmgr_commit_transaction ${tid}

    # Delete the address, disable the interface, and reset the MTU
    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} false
    fea_ifmgr_set_vif_enabled ${tid} ${IFNAME} ${VIFNAME} false
    fea_ifmgr_delete_address4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR}
    fea_ifmgr_set_mtu ${tid} ${IFNAME} ${MTU}
    fea_ifmgr_commit_transaction ${tid}

    # Reset the MAC address to its original value
    # Note that the interface must be DOWN to set the MAC address
    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    fea_ifmgr_set_mac ${tid} ${IFNAME} ${MAC}
    fea_ifmgr_commit_transaction ${tid}
}

test_enable_interface()
{
    echo "TEST: Enable interface ${IFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_create_vif ${tid} ${IFNAME} ${VIFNAME}
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} true
    fea_ifmgr_set_vif_enabled ${tid} ${IFNAME} ${VIFNAME} true
    fea_ifmgr_commit_transaction ${tid}
}

test_disable_interface()
{
    echo "TEST: Disable interface ${IFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} false
    fea_ifmgr_commit_transaction ${tid}
}

test_set_interface_mtu()
{
    echo "TEST: Set MTU of interface ${IFNAME} to ${TEST_MTU}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_set_mtu ${tid} ${IFNAME} ${TEST_MTU}
    fea_ifmgr_commit_transaction ${tid}
}

test_set_interface_mac()
{
    echo "TEST: Set MAC address of interface ${IFNAME} to ${TEST_MAC}"

    # First make sure that the interface is DOWN
    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} false
    fea_ifmgr_commit_transaction ${tid}

    # Set the interface MAC address
    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_set_mac ${tid} ${IFNAME} ${TEST_MAC}
    fea_ifmgr_commit_transaction ${tid}
}

test_create_address4()
{
    echo "TEST: Create address ${ADDR} on interface ${IFNAME}"

    # Add the address, and enable the interface
    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_create_vif ${tid} ${IFNAME} ${VIFNAME}
    fea_ifmgr_create_address4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR}
    fea_ifmgr_set_prefix4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR} ${NETMASK}
    fea_ifmgr_set_broadcast4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR} ${BROADCAST}
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} true
    fea_ifmgr_set_vif_enabled ${tid} ${IFNAME} ${VIFNAME} true
    fea_ifmgr_set_address_enabled4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR} true
    fea_ifmgr_commit_transaction ${tid}
}

test_delete_address4()
{
    echo "TEST: Delete address ${ADDR} on interface ${IFNAME}"

    # Delete the address
    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_create_vif ${tid} ${IFNAME} ${VIFNAME}
    fea_ifmgr_delete_address4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR}
    fea_ifmgr_commit_transaction ${tid}
}

test_is_interface_enabled()
{
    local _enabled

    echo "TEST: Is interface ${IFNAME} enabled"

    _enabled=`get_xrl_variable_value \`fea_ifmgr_get_configured_interface_enabled ${IFNAME}\` enabled:bool`

    #
    # Check the result
    #
    if [ "${_enabled}" != "true" ] ; then
	echo "ERROR: enabled status is ${_enabled}; expecting true"
	return 1
    fi
}

test_is_interface_disabled()
{
    local _enabled

    echo "TEST: Is interface ${IFNAME} disabled"

    _enabled=`get_xrl_variable_value \`fea_ifmgr_get_configured_interface_enabled ${IFNAME}\` enabled:bool`

    #
    # Check the result
    #
    if [ "${_enabled}" != "false" ] ; then
	echo "ERROR: enabled status is ${_enabled}; expecting false"
	return 1
    fi
}

test_get_interface_mtu()
{
    local _mtu

    echo "TEST: Get MTU of interface ${IFNAME}"

    _mtu=`get_xrl_variable_value \`fea_ifmgr_get_configured_mtu ${IFNAME}\` mtu:u32`

    #
    # Check the result
    #
    if [ "${_mtu}" != "${TEST_MTU}" ] ; then
	echo "ERROR: MTU is ${_mtu}; expecting ${TEST_MTU}"
	return 1
    fi
}

test_get_interface_mac()
{
    local _mac

    echo "TEST: Get MAC address of interface ${IFNAME}"

    _mac=`get_xrl_variable_value \`fea_ifmgr_get_configured_mac ${IFNAME}\` mac:mac`

    #
    # Check the result
    #
    if [ "${_mac}" != "${TEST_MAC}" ] ; then
	echo "ERROR: MAC address is ${_mac}; expecting ${TEST_MAC}"
	return 1
    fi
}

test_get_interface_address4()
{
    local _xrl_result _addresses _addresses_list

    echo "TEST: Get IPv4 address of interface ${IFNAME}"

    _xrl_result=`fea_ifmgr_get_configured_vif_addresses4 ${IFNAME} ${VIFNAME}`
    _addresses=`get_xrl_variable_value "${_xrl_result}" addresses:list`

    # Get the list of space-separated addresses
    _addresses_list=`split_xrl_list_values "${_addresses}" ipv4`

    for i in ${_addresses_list} ; do
	if [ "${ADDR}" = "$i" ] ; then
	    return 0
	fi
    done

    echo "TEST RESULT:"
    echo "${_xrl_result}"
    echo "ERROR: Expected address ${ADDR} not found"
    return 1
}

test_get_deleted_interface_address4()
{
    local _xrl_result _addresses _addresses_list

    echo "TEST: Get deleted IPv4 address of interface ${IFNAME}"

    _xrl_result=`fea_ifmgr_get_configured_vif_addresses4 ${IFNAME} ${VIFNAME}`
    _addresses=`get_xrl_variable_value "${_xrl_result}" addresses:list`

    # Get the list of space-separated addresses
    _addresses_list=`split_xrl_list_values "${_addresses}" ipv4`

    for i in ${_addresses_list} ; do
	if [ "${ADDR}" = "$i" ] ; then
	    echo "ERROR: address was not deleted:"
	    echo "${_xrl_result}"
	    return 1
	fi
    done

    return 0
}


#
# The tests
#
TESTS=""
TESTS="$TESTS test_cleanup_interface4"
TESTS="$TESTS test_enable_interface"
TESTS="$TESTS test_is_interface_enabled"
TESTS="$TESTS test_disable_interface"
TESTS="$TESTS test_is_interface_disabled"
TESTS="$TESTS test_set_interface_mtu"
TESTS="$TESTS test_get_interface_mtu"
TESTS="$TESTS test_set_interface_mac"
TESTS="$TESTS test_get_interface_mac"
TESTS="$TESTS test_create_address4"
TESTS="$TESTS test_get_interface_address4"
TESTS="$TESTS test_delete_address4"
TESTS="$TESTS test_get_deleted_interface_address4"
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
