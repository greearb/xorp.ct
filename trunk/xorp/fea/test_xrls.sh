#!/bin/sh # -*- sh-indentation: 4; tab-width: 8; indent-tabs-mode: t -*-

#
# $XORP: xorp/fea/test_xrls.sh,v 1.1.1.1 2002/12/11 23:56:03 hodson Exp $
#

# This script is used to test all the xrl interfaces of the fea that
# pertain to devices.
#
# At the time this script was written the fea did not support IPV6
# thorugh click.
#

VIF0=ed0
ADDR0=10.0.0.1
ADDR1=10.0.0.3

basic_fea_test()
{
    cat <<EOF
#
# Stage 1: Add an interface, vif, and IPv4 address
# 
EOF

    local tid=`start_fea_transaction`

    create_interface $tid $VIF0
    enable_interface $tid $VIF0

    create_vif $tid $VIF0 $VIF0
    enable_vif $tid $VIF0 $VIF0

    create_address4 $tid $VIF0 $VIF0 $ADDR0
    enable_address4 $tid $VIF0 $VIF0 $ADDR0

    set_prefix4 $tid $VIF0 $VIF0 $ADDR0 25

    commit_fea_transaction $tid

    get_configured_vif_addresses4 $VIF0 $VIF0

cat<<EOF
#
# Stage 2: Delete existing IP address and add a replacement
# 
EOF
    local tid=`start_fea_transaction`

    delete_address4 $tid $VIF0 $VIF0 $ADDR0
    create_address4 $tid $VIF0 $VIF0 $ADDR1
    enable_address4 $tid $VIF0 $VIF0 $ADDR1

    set_prefix4 $tid $VIF0 $VIF0 $ADDR1 16

    commit_fea_transaction $tid
    get_configured_vif_addresses4 $VIF0 $VIF0

cat<<EOF
#
# Stage 3: Add back deleted IPv4 address (disabled)
#
EOF
    local tid=`start_fea_transaction`
    create_address4 $tid $VIF0 $VIF0 $ADDR0
    commit_fea_transaction $tid
    get_configured_vif_addresses4 $VIF0 $VIF0

cat<<EOF
#
# Stage 4: Enable IPv4 address added back
#
EOF
    local tid=`start_fea_transaction`
    enable_address4 $tid $VIF0 $VIF0 $ADDR0
    commit_fea_transaction $tid
    get_configured_vif_addresses4 $VIF0 $VIF0

}

. ./xrl_shell_funcs.sh
#set -e # Anything goes wrong bail out immediately
basic_fea_test
exit 0
