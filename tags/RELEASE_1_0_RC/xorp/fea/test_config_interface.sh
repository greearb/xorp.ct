#!/bin/sh

#
# $XORP: xorp/fea/test_config_interface.sh,v 1.12 2004/05/21 19:34:12 pavlin Exp $
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

ADDR4="10.20.0.1"
PREFIX_LEN4="24"
BROADCAST4="10.20.0.255"
ENDPOINT4="INVALID"	# XXX: may be overwritten by host configuration
ADDR6="fe80::202:b3ff:fe10:e3e8"
PREFIX_LEN6="64"
ENDPOINT6="INVALID"	# XXX: may be overwritten by host configuration
MTU="1500"
TEST_MTU="1400"
HAVE_IPV6="true"	# XXX: may be overwritten by host configuration

case ${HOSTNAME} in
    xorp1)
    HAVE_IPV6="true"
    IFNAME="fxp3"
    MAC="00:02:b3:10:e3:e7"
    TEST_MAC="00:02:b3:10:e3:e8"
    PIF_INDEX="5"
    VIF_FLAG_BROADCAST="true"
    VIF_FLAG_LOOPBACK="false"
    VIF_FLAG_POINT_TO_POINT="false"
    VIF_FLAG_MULTICAST="true"
    ;;

    xorp3)
    HAVE_IPV6="true"
    IFNAME="fxp3"
    MAC="00:02:b3:10:e2:ed"
    TEST_MAC="00:02:b3:10:e2:ee"
    PIF_INDEX="5"
    VIF_FLAG_BROADCAST="true"
    VIF_FLAG_LOOPBACK="false"
    VIF_FLAG_POINT_TO_POINT="false"
    VIF_FLAG_MULTICAST="true"
    ;;

    xorp4)
    HAVE_IPV6="false"
    IFNAME="eth3"
    MAC="00:04:5A:49:5D:11"
    TEST_MAC="0:4:5a:49:5d:12"
    PIF_INDEX="5"
    VIF_FLAG_BROADCAST="true"
    VIF_FLAG_LOOPBACK="false"
    VIF_FLAG_POINT_TO_POINT="false"
    VIF_FLAG_MULTICAST="true"
    ;;

    carp | carp.icir.org)
    MAC="00:01:02:71:1B:48"
    TEST_MAC="0:1:2:71:1b:49"
    VIF_FLAG_BROADCAST="true"
    VIF_FLAG_LOOPBACK="false"
    VIF_FLAG_POINT_TO_POINT="false"
    VIF_FLAG_MULTICAST="true"
    case ${OS} in
	Linux)
	HAVE_IPV6="false"
	IFNAME="eth1"
	PIF_INDEX="3"
	;;

	FreeBSD)
	HAVE_IPV6="true"
	IFNAME="xl0"
	MAC="00:01:02:71:1B:48"
	TEST_MAC="00:01:02:71:1b:49"
	PIF_INDEX="1"
	;;

	NetBSD)
	HAVE_IPV6="true"
	IFNAME="ex0"
	PIF_INDEX="1"
	;;

	OpenBSD)
	HAVE_IPV6="true"
	IFNAME="xl0"
	PIF_INDEX="1"
	;;

	*)
	echo "Unknown OS : ${OS}"
	exit 1
	;;
    esac
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
config_cleanup_interface()
{
    echo "INFO: Cleanup interface ${IFNAME}"

    #
    # To make sure that the interface is cleaned, first explicity
    # try to add an address to it, then delete that address. In the process,
    # ignore any errors.
    #

    #
    # Add the IPv4 address, and enable the interface
    #
    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_create_vif ${tid} ${IFNAME} ${VIFNAME}
    fea_ifmgr_create_address4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR4}
    fea_ifmgr_set_prefix4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR4} ${PREFIX_LEN4}
    if [ "${VIF_FLAG_BROADCAST}" = "true" ] ; then
	fea_ifmgr_set_broadcast4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR4} ${BROADCAST4}
    fi
    if [ "${VIF_FLAG_POINT_TO_POINT}" = "true" ] ; then
	fea_ifmgr_set_endpoint4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR4} ${ENDPOINT4}
    fi
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} true
    fea_ifmgr_set_vif_enabled ${tid} ${IFNAME} ${VIFNAME} true
    fea_ifmgr_set_address_enabled4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR4} true
    fea_ifmgr_commit_transaction ${tid}

    #
    # Add the IPv6 address, and enable the interface
    #
    if [ "${HAVE_IPV6}" = "true" ] ; then
	tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
	if [ "${tid}" = "" ] ; then
	    echo "ERROR: cannot start transaction: cannot get transaction ID"
	    return 1
	fi
	fea_ifmgr_create_interface ${tid} ${IFNAME}
	fea_ifmgr_create_vif ${tid} ${IFNAME} ${VIFNAME}
	fea_ifmgr_create_address6 ${tid} ${IFNAME} ${VIFNAME} ${ADDR6}
	fea_ifmgr_set_prefix6 ${tid} ${IFNAME} ${VIFNAME} ${ADDR6} ${PREFIX_LEN4}
	if [ "${VIF_FLAG_POINT_TO_POINT}" = "true" ] ; then
	    fea_ifmgr_set_endpoint6 ${tid} ${IFNAME} ${VIFNAME} ${ADDR6} ${ENDPOINT6}
	fi
	fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} true
	fea_ifmgr_set_vif_enabled ${tid} ${IFNAME} ${VIFNAME} true
	fea_ifmgr_set_address_enabled6 ${tid} ${IFNAME} ${VIFNAME} ${ADDR6} true
	fea_ifmgr_commit_transaction ${tid}
    fi

    #
    # Delete the IPv4 address
    #
    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_delete_address4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR4}
    fea_ifmgr_commit_transaction ${tid}

    #
    # Delete the IPv6 address
    #
    if [ "${HAVE_IPV6}" = "true" ] ; then
	tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
	if [ "${tid}" = "" ] ; then
	    echo "ERROR: cannot start transaction: cannot get transaction ID"
	    return 1
	fi
	fea_ifmgr_delete_address6 ${tid} ${IFNAME} ${VIFNAME} ${ADDR6}
	fea_ifmgr_commit_transaction ${tid}
    fi

    #
    # Disable the interface, and reset the MTU
    #
    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} false
    fea_ifmgr_set_vif_enabled ${tid} ${IFNAME} ${VIFNAME} false
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

subtest_create_interface()
{
    echo "SUBTEST: Create interface ${IFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_commit_transaction ${tid}
}

subtest_delete_interface()
{
    echo "SUBTEST: Delete interface ${IFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_delete_interface ${tid} ${IFNAME}
    fea_ifmgr_commit_transaction ${tid}
}

subtest_is_interface_configured()
{
    local _xrl_result _ret_value _ifnames _ifnames_list _found

    echo "SUBTEST: Check whether interface ${IFNAME} is configured"

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

subtest_is_interface_not_configured()
{
    local _xrl_result _ret_value _ifnames _ifnames_list _found

    echo "SUBTEST: Check whether interface ${IFNAME} is not configured"

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

test_configure_interface()
{
    local _ret_value _subtests

    echo "TEST: Configure interface"

    _subtests=""
    _subtests="${_subtests} subtest_create_interface"
    _subtests="${_subtests} subtest_is_interface_configured"
    _subtests="${_subtests} subtest_delete_interface"
    _subtests="${_subtests} subtest_is_interface_not_configured"

    for t in ${_subtests} ; do
	$t
	_ret_value=$?
	if [ ${_ret_value} -ne 0 ] ; then
	    return ${_ret_value}
	fi
    done
}

subtest_enable_interface()
{
    echo "SUBTEST: Enable interface ${IFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} true
    fea_ifmgr_commit_transaction ${tid}
}

subtest_disable_interface()
{
    echo "SUBTEST: Disable interface ${IFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} false
    fea_ifmgr_commit_transaction ${tid}
}

subtest_is_interface_enabled()
{
    local _xrl_result _ret_value _enabled

    echo "SUBTEST: Whether interface ${IFNAME} is enabled"

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

subtest_is_interface_disabled()
{
    local _xrl_result _ret_value _enabled

    echo "SUBTEST: Whether interface ${IFNAME} is disabled"

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

test_enable_disable_interface()
{
    local _ret_value _subtests

    echo "TEST: Enable/disable interface"

    _subtests=""
    _subtests="${_subtests} subtest_enable_interface"
    _subtests="${_subtests} subtest_is_interface_enabled"
    _subtests="${_subtests} subtest_disable_interface"
    _subtests="${_subtests} subtest_is_interface_disabled"

    for t in ${_subtests} ; do
	$t
	_ret_value=$?
	if [ ${_ret_value} -ne 0 ] ; then
	    return ${_ret_value}
	fi
    done
}

subtest_set_interface_mtu()
{
    echo "SUBTEST: Set MTU on interface ${IFNAME} to ${TEST_MTU}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_set_mtu ${tid} ${IFNAME} ${TEST_MTU}
    fea_ifmgr_commit_transaction ${tid}
}

subtest_get_interface_mtu()
{
    local _xrl_result _ret_value _mtu

    echo "SUBTEST: Get MTU on interface ${IFNAME}"

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

test_set_interface_mtu()
{
    local _ret_value _subtests

    echo "TEST: Set interface MTU"

    _subtests=""
    _subtests="${_subtests} config_cleanup_interface"
    _subtests="${_subtests} subtest_set_interface_mtu"
    _subtests="${_subtests} subtest_get_interface_mtu"

    for t in ${_subtests} ; do
	$t
	_ret_value=$?
	if [ ${_ret_value} -ne 0 ] ; then
	    return ${_ret_value}
	fi
    done
}

subtest_set_interface_mac()
{
    echo "SUBTEST: Set MAC address on interface ${IFNAME} to ${TEST_MAC}"

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

subtest_get_interface_mac()
{
    local _xrl_result _ret_value _mac

    echo "SUBTEST: Get MAC address on interface ${IFNAME}"

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

test_set_interface_mac()
{
    local _ret_value _subtests

    echo "TEST: Set interface MAC address"

    case ${OS} in
	NetBSD | OpenBSD)
	# XXX: currently (NetBSD-1.6.1 and OpenBSD-3.3) don't support
	# setting the MAC address.
	echo "WARNING: this system OS (${OS}) doesn't support setting the MAC address. Test skipped."
	return 0
	;;

	*)
	;;
    esac

    _subtests=""
    _subtests="${_subtests} config_cleanup_interface"
    _subtests="${_subtests} subtest_set_interface_mac"
    _subtests="${_subtests} subtest_get_interface_mac"

    for t in ${_subtests} ; do
	$t
	_ret_value=$?
	if [ ${_ret_value} -ne 0 ] ; then
	    return ${_ret_value}
	fi
    done
}

subtest_create_vif()
{
    echo "SUBTEST: Create vif ${VIFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_create_vif ${tid} ${IFNAME} ${VIFNAME}
    fea_ifmgr_commit_transaction ${tid}
}

subtest_delete_vif()
{
    echo "SUBTEST: Delete vif ${VIFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_delete_vif ${tid} ${IFNAME} ${VIFNAME}
    fea_ifmgr_commit_transaction ${tid}
}

subtest_is_vif_configured()
{
    local _xrl_result _ret_value _vifs _vifs_list _found

    echo "SUBTEST: Check whether vif ${VIFNAME} is configured"

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

subtest_is_vif_not_configured()
{
    local _xrl_result _ret_value _vifs _vifs_list _found

    echo "SUBTEST: Check whether vif ${VIFNAME} is not configured"

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

test_configure_vif()
{
    local _ret_value _subtests

    echo "TEST: Configure vif"

    _subtests=""
    _subtests="${_subtests} subtest_create_vif"
    _subtests="${_subtests} subtest_is_vif_configured"
    _subtests="${_subtests} subtest_delete_vif"
    _subtests="${_subtests} subtest_is_vif_not_configured"

    for t in ${_subtests} ; do
	$t
	_ret_value=$?
	if [ ${_ret_value} -ne 0 ] ; then
	    return ${_ret_value}
	fi
    done
}

subtest_enable_vif()
{
    echo "SUBTEST: Enable vif ${VIFNAME}"

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

subtest_disable_vif()
{
    echo "SUBTEST: Disable vif ${VIFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_set_vif_enabled ${tid} ${IFNAME} ${VIFNAME} false
    fea_ifmgr_commit_transaction ${tid}
}

subtest_is_vif_enabled()
{
    local _xrl_result _ret_value _enabled

    echo "SUBTEST: Whether vif ${VIFNAME} is enabled"

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

subtest_is_vif_disabled()
{
    local _xrl_result _ret_value _enabled

    echo "SUBTEST: Whether vif ${VIFNAME} is disabled"

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

test_enable_disable_vif()
{
    local _ret_value _subtests

    echo "TEST: Enable/disable vif"

    _subtests=""
    _subtests="${_subtests} subtest_enable_vif"
    _subtests="${_subtests} subtest_is_vif_enabled"
    _subtests="${_subtests} subtest_disable_vif"
    _subtests="${_subtests} subtest_is_vif_disabled"

    for t in ${_subtests} ; do
	$t
	_ret_value=$?
	if [ ${_ret_value} -ne 0 ] ; then
	    return ${_ret_value}
	fi
    done
}

test_get_vif_pif_index()
{
    local _xrl_result _ret_value _pif_index

    echo "TEST: Get physical interface index on vif ${VIFNAME}"

    #
    # First explicitly create the interface
    #
    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} true
    fea_ifmgr_create_vif ${tid} ${IFNAME} ${VIFNAME}
    fea_ifmgr_set_vif_enabled ${tid} ${IFNAME} ${VIFNAME} true
    fea_ifmgr_commit_transaction ${tid}

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

subtest_create_address4()
{
    echo "SUBTEST: Create address ${ADDR4} on interface ${IFNAME} vif ${VIFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_create_vif ${tid} ${IFNAME} ${VIFNAME}
    fea_ifmgr_create_address4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR4}
    fea_ifmgr_set_prefix4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR4} ${PREFIX_LEN4}
    if [ "${VIF_FLAG_BROADCAST}" = "true" ] ; then
	fea_ifmgr_set_broadcast4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR4} ${BROADCAST4}
    fi
    if [ "${VIF_FLAG_POINT_TO_POINT}" = "true" ] ; then
	fea_ifmgr_set_endpoint4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR4} ${ENDPOINT4}
    fi
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} true
    fea_ifmgr_set_vif_enabled ${tid} ${IFNAME} ${VIFNAME} true
    fea_ifmgr_set_address_enabled4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR4} true
    fea_ifmgr_commit_transaction ${tid}
}

subtest_create_address6()
{
    echo "SUBTEST: Create address ${ADDR6} on interface ${IFNAME} vif ${VIFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_create_vif ${tid} ${IFNAME} ${VIFNAME}
    fea_ifmgr_create_address6 ${tid} ${IFNAME} ${VIFNAME} ${ADDR6}
    fea_ifmgr_set_prefix6 ${tid} ${IFNAME} ${VIFNAME} ${ADDR6} ${PREFIX_LEN6}
    if [ "${VIF_FLAG_POINT_TO_POINT}" = "true" ] ; then
	fea_ifmgr_set_endpoint6 ${tid} ${IFNAME} ${VIFNAME} ${ADDR6} ${ENDPOINT6}
    fi
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} true
    fea_ifmgr_set_vif_enabled ${tid} ${IFNAME} ${VIFNAME} true
    fea_ifmgr_set_address_enabled6 ${tid} ${IFNAME} ${VIFNAME} ${ADDR6} true
    fea_ifmgr_commit_transaction ${tid}
}

subtest_delete_address4()
{
    echo "SUBTEST: Delete address ${ADDR4} on interface ${IFNAME} vif ${VIFNAME}"

    # Delete the address
    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_delete_address4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR4}
    fea_ifmgr_commit_transaction ${tid}
}

subtest_delete_address6()
{
    echo "SUBTEST: Delete address ${ADDR6} on interface ${IFNAME} vif ${VIFNAME}"

    # Delete the address
    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_delete_address6 ${tid} ${IFNAME} ${VIFNAME} ${ADDR6}
    fea_ifmgr_commit_transaction ${tid}
}

subtest_get_address4()
{
    local _xrl_result _ret_value _addresses _addresses_list _found

    echo "SUBTEST: Get IPv4 address on interface ${IFNAME} vif ${VIFNAME}"

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
	if [ "${ADDR4}" = "$i" ] ; then
	    _found="yes"
	    break;
	fi
    done

    if [ "${_found}" != "yes" ] ; then
	echo "ERROR: Expected address ${ADDR4} not found:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

subtest_get_address6()
{
    local _xrl_result _ret_value _addresses _addresses_list _found

    echo "SUBTEST: Get IPv6 address on interface ${IFNAME} vif ${VIFNAME}"

    _xrl_result=`fea_ifmgr_get_configured_vif_addresses6 ${IFNAME} ${VIFNAME} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot get the IPv6 addresses on interface ${IFNAME}} vif ${VIFNAME}:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _found=""
    _addresses=`get_xrl_variable_value "${_xrl_result}" addresses:list`
    # Get the list of space-separated addresses
    _addresses_list=`split_xrl_list_values "${_addresses}" ipv6`
    for i in ${_addresses_list} ; do
	if [ "${ADDR6}" = "$i" ] ; then
	    _found="yes"
	    break;
	fi
    done

    if [ "${_found}" != "yes" ] ; then
	echo "ERROR: Expected address ${ADDR6} not found:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

subtest_get_prefix4()
{
    local _xrl_result _ret_value _prefix_len

    echo "SUBTEST: Get prefix length on address ${ADDR4} on interface ${IFNAME} vif ${VIF}"

    _xrl_result=`fea_ifmgr_get_configured_prefix4 ${IFNAME} ${VIFNAME} ${ADDR4} 2>&1`
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
    if [ "${_prefix_len}" != "${PREFIX_LEN4}" ] ; then
	echo "ERROR: prefix length is ${_prefix_len}; expecting ${PREFIX_LEN4}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

subtest_get_prefix6()
{
    local _xrl_result _ret_value _prefix_len

    echo "SUBTEST: Get prefix length on address ${ADDR6} on interface ${IFNAME} vif ${VIF}"

    _xrl_result=`fea_ifmgr_get_configured_prefix6 ${IFNAME} ${VIFNAME} ${ADDR6} 2>&1`
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
    if [ "${_prefix_len}" != "${PREFIX_LEN6}" ] ; then
	echo "ERROR: prefix length is ${_prefix_len}; expecting ${PREFIX_LEN6}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

subtest_get_broadcast4()
{
    local _xrl_result _ret_value _broadcast

    echo "SUBTEST: Get broadcast address on address ${ADDR4} on interface ${IFNAME} vif ${VIFNAME}"

    if [ "${VIF_FLAG_BROADCAST}" = "false" ] ; then
	echo "INFO: ignoring: the interface is not broadcast-capable"
	return 0
    fi

    _xrl_result=`fea_ifmgr_get_configured_broadcast4 ${IFNAME} ${VIFNAME} ${ADDR4} 2>&1`
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
    if [ "${_broadcast}" != "${BROADCAST4}" ] ; then
	echo "ERROR: broadcast address is ${_broadcast}; expecting ${BROADCAST4}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

subtest_get_endpoint4()
{
    local _xrl_result _ret_value _endpoint

    echo "SUBTEST: Get endpoint address on address ${ADDR4} on interface ${IFNAME} vif ${VIFNAME}"

    if [ "${VIF_FLAG_POINT_TO_POINT}" = "false" ] ; then
	echo "INFO: Test ignored: the interface is not point-to-point"
	return 0
    fi

    _xrl_result=`fea_ifmgr_get_configured_endpoint4 ${IFNAME} ${VIFNAME} ${ADDR4} 2>&1`
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
    if [ "${_endpoint}" != "${ENDPOINT4}" ] ; then
	echo "ERROR: endpoint address is ${_endpoint}; expecting ${ENDPOINT4}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

subtest_get_endpoint6()
{
    local _xrl_result _ret_value _endpoint

    echo "SUBTEST: Get endpoint address on address ${ADDR6} on interface ${IFNAME} vif ${VIFNAME}"

    if [ "${VIF_FLAG_POINT_TO_POINT}" = "false" ] ; then
	echo "INFO: Test ignored: the interface is not point-to-point"
	return 0
    fi

    _xrl_result=`fea_ifmgr_get_configured_endpoint6 ${IFNAME} ${VIFNAME} ${ADDR6} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot get the endpoint address:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _endpoint=`get_xrl_variable_value "${_xrl_result}" endpoint:ipv6`
    if [ "${_endpoint}" != "${ENDPOINT6}" ] ; then
	echo "ERROR: endpoint address is ${_endpoint}; expecting ${ENDPOINT6}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

subtest_get_address_flags4()
{
    local _xrl_result _ret_value _enabled _broadcast _loopback _point_to_point
    local _multicast

    echo "SUBTEST: Get address flags on address ${ADDR4} on interface ${IFNAME} vif ${VIFNAME}"

    _xrl_result=`fea_ifmgr_get_configured_address_flags4 ${IFNAME} ${VIFNAME} ${ADDR4} 2>&1`
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

subtest_get_address_flags6()
{
    local _xrl_result _ret_value _enabled _broadcast _loopback _point_to_point
    local _multicast

    echo "SUBTEST: Get address flags on address ${ADDR6} on interface ${IFNAME} vif ${VIFNAME}"

    _xrl_result=`fea_ifmgr_get_configured_address_flags6 ${IFNAME} ${VIFNAME} ${ADDR6} 2>&1`
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
    # XXX: the IPv6 address has no broadcast flag
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

subtest_get_deleted_address4()
{
    local _xrl_result _ret_value _addresses _addresses_list _found

    echo "SUBTEST: Get deleted IPv4 address on interface ${IFNAME} vif ${VIFNAME}"

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
	if [ "${ADDR4}" = "$i" ] ; then
	    _found="yes"
	    break;
	fi
    done

    if [ "${_found}" = "yes" ] ; then
	echo "ERROR: Non-expected address ${ADDR4} was found:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Print the result
    #
    echo "RESULT:"
    echo "${_xrl_result}"
}

subtest_get_deleted_address6()
{
    local _xrl_result _ret_value _addresses _addresses_list _found

    echo "SUBTEST: Get deleted IPv6 address on interface ${IFNAME} vif ${VIFNAME}"

    _xrl_result=`fea_ifmgr_get_configured_vif_addresses6 ${IFNAME} ${VIFNAME} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot get the IPv6 addresses on interface ${IFNAME} vif ${VIFNAME}:"
	echo "${_xrl_result}"
	return 1
    fi

    #
    # Check the result
    #
    _found=""
    _addresses=`get_xrl_variable_value "${_xrl_result}" addresses:list`
    # Get the list of space-separated addresses
    _addresses_list=`split_xrl_list_values "${_addresses}" ipv6`
    for i in ${_addresses_list} ; do
	if [ "${ADDR6}" = "$i" ] ; then
	    _found="yes"
	    break;
	fi
    done

    if [ "${_found}" = "yes" ] ; then
	echo "ERROR: Non-expected address ${ADDR6} was found:"
	echo "${_xrl_result}"
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
    local _ret_value _subtests

    echo "TEST: Create IPv4 address"

    _subtests=""
    _subtests="${_subtests} config_cleanup_interface"
    _subtests="${_subtests} subtest_create_address4"
    _subtests="${_subtests} subtest_get_address4"
    _subtests="${_subtests} subtest_get_prefix4"
    _subtests="${_subtests} subtest_get_broadcast4"
    _subtests="${_subtests} subtest_get_endpoint4"
    _subtests="${_subtests} subtest_get_address_flags4"
    _subtests="${_subtests} subtest_delete_address4"
    _subtests="${_subtests} subtest_get_deleted_address4"

    for t in ${_subtests} ; do
	$t
	_ret_value=$?
	if [ ${_ret_value} -ne 0 ] ; then
	    return ${_ret_value}
	fi
    done
}

test_create_address6()
{
    local _ret_value _subtests

    echo "TEST: Create IPv6 address"

    _subtests=""
    _subtests="${_subtests} config_cleanup_interface"
    _subtests="${_subtests} subtest_create_address6"
    _subtests="${_subtests} subtest_get_address6"
    _subtests="${_subtests} subtest_get_prefix6"
    _subtests="${_subtests} subtest_get_broadcast6"
    _subtests="${_subtests} subtest_get_endpoint6"
    _subtests="${_subtests} subtest_get_address_flags6"
    _subtests="${_subtests} subtest_delete_address6"
    _subtests="${_subtests} subtest_get_deleted_address6"

    for t in ${_subtests} ; do
	$t
	_ret_value=$?
	if [ ${_ret_value} -ne 0 ] ; then
	    return ${_ret_value}
	fi
    done
}

subtest_enable_address4()
{
    echo "SUBTEST: Enable address ${ADDR4} on interface ${IFNAME} vif ${VIFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_create_vif ${tid} ${IFNAME} ${VIFNAME}
    fea_ifmgr_create_address4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR4}
    fea_ifmgr_set_prefix4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR4} ${PREFIX_LEN4}
    if [ "${VIF_FLAG_BROADCAST}" = "true" ] ; then
	fea_ifmgr_set_broadcast4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR4} ${BROADCAST4}
    fi
    if [ "${VIF_FLAG_POINT_TO_POINT}" = "true" ] ; then
	fea_ifmgr_set_endpoint4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR4} ${ENDPOINT4}
    fi
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} true
    fea_ifmgr_set_vif_enabled ${tid} ${IFNAME} ${VIFNAME} true
    fea_ifmgr_set_address_enabled4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR4} true
    fea_ifmgr_commit_transaction ${tid}
}

subtest_enable_address6()
{
    echo "SUBTEST: Enable address ${ADDR6} on interface ${IFNAME} vif ${VIFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_create_interface ${tid} ${IFNAME}
    fea_ifmgr_create_vif ${tid} ${IFNAME} ${VIFNAME}
    fea_ifmgr_create_address6 ${tid} ${IFNAME} ${VIFNAME} ${ADDR6}
    fea_ifmgr_set_prefix6 ${tid} ${IFNAME} ${VIFNAME} ${ADDR6} ${PREFIX_LEN6}
    if [ "${VIF_FLAG_POINT_TO_POINT}" = "true" ] ; then
	fea_ifmgr_set_endpoint6 ${tid} ${IFNAME} ${VIFNAME} ${ADDR6} ${ENDPOINT6}
    fi
    fea_ifmgr_set_interface_enabled ${tid} ${IFNAME} true
    fea_ifmgr_set_vif_enabled ${tid} ${IFNAME} ${VIFNAME} true
    fea_ifmgr_set_address_enabled6 ${tid} ${IFNAME} ${VIFNAME} ${ADDR6} true
    fea_ifmgr_commit_transaction ${tid}
}

subtest_disable_address4()
{
    echo "SUBTEST: Disable address ${ADDR4} on interface ${IFNAME} vif ${VIFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_set_address_enabled4 ${tid} ${IFNAME} ${VIFNAME} ${ADDR4} false
    fea_ifmgr_commit_transaction ${tid}
}

subtest_disable_address6()
{
    echo "SUBTEST: Disable address ${ADDR6} on interface ${IFNAME} vif ${VIFNAME}"

    tid=`get_xrl_variable_value \`fea_ifmgr_start_transaction\` tid:u32`
    if [ "${tid}" = "" ] ; then
	echo "ERROR: cannot start transaction: cannot get transaction ID"
	return 1
    fi
    fea_ifmgr_set_address_enabled6 ${tid} ${IFNAME} ${VIFNAME} ${ADDR6} false
    fea_ifmgr_commit_transaction ${tid}
}

subtest_is_address_enabled4()
{
    local _xrl_result _ret_value _enabled

    echo "SUBTEST: Whether address ${ADDR4} is enabled on interface ${IFNAME} vif ${VIFNAME}"

    _xrl_result=`fea_ifmgr_get_configured_address_enabled4 ${IFNAME} ${VIFNAME} ${ADDR4} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot test whether address ${ADDR4} is enabled:"
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

subtest_is_address_enabled6()
{
    local _xrl_result _ret_value _enabled

    echo "SUBTEST: Whether address ${ADDR6} is enabled on interface ${IFNAME} vif ${VIFNAME}"

    _xrl_result=`fea_ifmgr_get_configured_address_enabled6 ${IFNAME} ${VIFNAME} ${ADDR6} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	echo "ERROR: cannot test whether address ${ADDR6} is enabled:"
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

subtest_is_address_disabled4()
{
    local _xrl_result _ret_value _enabled
    local _xrl_result2 _ret_value2 _addresses _addresses_list _found

    echo "SUBTEST: Whether address ${ADDR4} is disabled on interface ${IFNAME} vif ${VIFNAME}"

    _xrl_result=`fea_ifmgr_get_configured_address_enabled4 ${IFNAME} ${VIFNAME} ${ADDR4} 2>&1`
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
	    if [ "${ADDR4}" = "$i" ] ; then
		_found="yes"
		break;
	    fi
	done

	if [ "${_found}" != "yes" ] ; then
	    echo "RESULT: OK: Address ${ADDR4} was not found"
	    return 0
	fi

	echo "ERROR: cannot test whether address ${ADDR4} is disabled:"
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

subtest_is_address_disabled6()
{
    local _xrl_result _ret_value _enabled
    local _xrl_result2 _ret_value2 _addresses _addresses_list _found

    echo "SUBTEST: Whether address ${ADDR6} is disabled on interface ${IFNAME} vif ${VIFNAME}"

    _xrl_result=`fea_ifmgr_get_configured_address_enabled6 ${IFNAME} ${VIFNAME} ${ADDR6} 2>&1`
    _ret_value=$?
    if [ ${_ret_value} -ne 0 ] ; then
	# Probably OK: e.g., if the address was deleted.
	# Verifying whether this is the case...
	_xrl_result2=`fea_ifmgr_get_configured_vif_addresses6 ${IFNAME} ${VIFNAME} 2>&1`
	_ret_value2=$?
	if [ ${_ret_value2} -ne 0 ] ; then
	    echo "ERROR: cannot get the IPv6 addresses on interface ${IFNAME} vif ${VIFNAME}:"
	    echo "${_xrl_result2}"
	    return 1
	fi

	#
	# Check the result
	#
	_found=""
	_addresses=`get_xrl_variable_value "${_xrl_result2}" addresses:list`
	# Get the list of space-separated addresses
	_addresses_list=`split_xrl_list_values "${_addresses}" ipv6`
	for i in ${_addresses_list} ; do
	    if [ "${ADDR6}" = "$i" ] ; then
		_found="yes"
		break;
	    fi
	done

	if [ "${_found}" != "yes" ] ; then
	    echo "RESULT: OK: Address ${ADDR6} was not found"
	    return 0
	fi

	echo "ERROR: cannot test whether address ${ADDR6} is disabled:"
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

test_enable_disable_address4()
{
    local _ret_value _subtests

    echo "TEST: Enable/disable IPv4 address"

    _subtests=""
    _subtests="${_subtests} config_cleanup_interface"
    _subtests="${_subtests} subtest_enable_address4"
    _subtests="${_subtests} subtest_is_address_enabled4"
    _subtests="${_subtests} subtest_disable_address4"
    _subtests="${_subtests} subtest_is_address_disabled4"

    for t in ${_subtests} ; do
	$t
	_ret_value=$?
	if [ ${_ret_value} -ne 0 ] ; then
	    return ${_ret_value}
	fi
    done
}

test_enable_disable_address6()
{
    local _ret_value _subtests

    echo "TEST: Enable/disable IPv6 address"

    _subtests=""
    _subtests="${_subtests} config_cleanup_interface"
    _subtests="${_subtests} subtest_enable_address6"
    _subtests="${_subtests} subtest_is_address_enabled6"
    _subtests="${_subtests} subtest_disable_address6"
    _subtests="${_subtests} subtest_is_address_disabled6"

    for t in ${_subtests} ; do
	$t
	_ret_value=$?
	if [ ${_ret_value} -ne 0 ] ; then
	    return ${_ret_value}
	fi
    done
}

#
# The tests
#
TESTS_NOT_FIXED=""
TESTS=""
# Interface and vif tests
TESTS="$TESTS test_configure_interface"
TESTS="$TESTS test_enable_disable_interface"
TESTS="$TESTS test_set_interface_mtu"
TESTS="$TESTS test_set_interface_mac"
TESTS="$TESTS test_configure_vif"
TESTS="$TESTS test_enable_disable_vif"
TESTS="$TESTS test_get_vif_pif_index"
TESTS="$TESTS test_get_vif_flags"
# IPv4 tests
TESTS="$TESTS test_create_address4"
TESTS="$TESTS test_enable_disable_address4"
# IPv6 tests
if [ "${HAVE_IPV6}" = "true" ] ; then
    TESTS="$TESTS test_create_address4"
    TESTS="$TESTS test_enable_disable_address4"
fi
# Clean-up the mess
TESTS="$TESTS config_cleanup_interface"

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
