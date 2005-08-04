#!/bin/sh

LO0=lo1
VIF0=eth65539

# Conditionally set ${srcdir} if it wasn't assigned (e.g., by `gmake check`)
if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi

. ${srcdir}/../fea/xrl_shell_funcs.sh

config_host()
{
    local tid=`start_fea_transaction`

    configure_interface_from_system $tid $LO0
    configure_interface_from_system $tid $VIF0
    commit_fea_transaction $tid

    # get_configured_vif_addresses4 $VIF0 $VIF0
}

unconfig_host()
{
    local tid=`start_fea_transaction`

    disable_interface $tid $VIF0
    disable_interface $tid $LO0
    commit_fea_transaction $tid
}

config_host
