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

../../libxipc/call_xrl "finder://xorp_if_mib/xorp_if_mib/0.1/unload_mib?mib_index:u32=$1"

echo "The module in position $1 should appear as unloaded"
snmptable -v 2c -c ${SNMP_DBG_COMMUNITY} localhost:${SNMPD_PORT} UCD-DLMOD-MIB::dlmodTable

