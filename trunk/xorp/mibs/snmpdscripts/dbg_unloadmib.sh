#! /bin/sh
#
# Unload a mib module from snmpd in the position specified in the first
# parameter. Example:   
#     
#  $ unload 2
#
# Requires:
#
#  (0) that the finder is running
#
#  (1) that snmpd is running
#

. dbg_config.sh

if test "$1" = ""; then
    echo 'usage example:  '
    echo '    $ dbg_unloadmib.sh <mib_index>' 
    exit 1
fi


if test "$1" = "1"; then
    # we cannot unload the xrl interface module via an xrl...
    snmpset -v 2c -c ${SNMP_DBG_COMMUNITY} localhost:${SNMPD_PORT} UCD-DLMOD-MIB::dlmodStatus.$1 i unload
    snmpset -v 2c -c ${SNMP_DBG_COMMUNITY} localhost:${SNMPD_PORT} UCD-DLMOD-MIB::dlmodStatus.$1 i delete
else
    ../../libxipc/call_xrl "finder://xorp_if_mib/xorp_if_mib/0.1/unload_mib?mib_index:u32=$1"
fi

./dbg_dlmiblist.sh
