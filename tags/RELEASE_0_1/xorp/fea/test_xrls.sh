#!/bin/sh # -*- sh-indentation: 4; tab-width: 8; indent-tabs-mode: t -*-

#
# $XORP: xorp/fea/test_xrls.sh,v 1.2 2002/12/09 11:13:05 pavlin Exp $
#

# This script is used to test all the xrl interfaces of the fea that
# pertain to devices.
#
# At the time this script was written the fea did not support IPV6
# thorugh click.
#

TEST_DEVICE1=dc0
IPV4=192.168.0.1
IPV4_PREFIX=16
IPV4_BROADCAST=192.168.255.255
#IPV6=fe80::1
#IPV6="::280:c8ff:feb9:3f5d"
#IPV6_PREFIX=64

setup()
{
    set_mac $1 "0xa:0xa:0xa:0xa:0xa:0xa"
    get_mac $1

    set_mtu $1 1500
    get_mtu $1

    create_interface $1
    enable_interface $1
    create_vif $1 $1
    enable_vif $1

    add_address4 $1 $IPV4
    set_prefix4 $1 $IPV4 $IPV4_PREFIX
    get_prefix4 $1 $IPV4
    set_broadcast4 $1 $IPV4 $IPV4_BROADCAST
    get_broadcast4 $1 $IPV4

#    add_address6 $1 $IPV6
#    set_prefix6 $1 $IPV6 $IPV6_PREFIX
#    get_prefix6 $1 $IPV6
}

teardown()
{
    delete_address4 $1 $IPV4
#    delete_address6 $1 $IPV6

    disable_vif $1
    delete_vif $1
    disable_interface $1
    delete_interface $1
}

status()
{
    get_all_interface_names

    get_configured_interface_names
    get_vif_names
    get_all_vif_addresses4 $1
    get_configured_vif_addresses4 $1
    get_all_vif_addresses6 $1
    get_configured_vif_addresses6 $1
}

test1()
{
    setup $1

    status $1

    teardown $1
}

. ./xrl_shell_funcs.sh
set -e # Anything goes wrong bail out immediately
test1 $TEST_DEVICE1
exit 0
