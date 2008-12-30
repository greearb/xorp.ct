#! /bin/sh
#
# Load a mib module 
#     
#  $ loadbgp /home/jsmith/mymibs/bgp4_mib_1657.so 2
#
# Requires:
#
#  (0) that snmpd is running

. dbg_config.sh

MIBS_PATH=`(cd ..; pwd)`
MOD_FILENAME="${MIBS_PATH}/$1.so" 

echo ""
echo "This is the list of currently loaded dynamic mib modules:"
echo "-----------------------------------------------------------------------"
snmptable -Cw 79 -Ci -v 2c -c ${SNMP_DBG_COMMUNITY} localhost:${SNMPD_PORT} UCD-DLMOD-MIB::dlmodTable
