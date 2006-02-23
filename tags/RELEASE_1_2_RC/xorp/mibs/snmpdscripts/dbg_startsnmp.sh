#! /bin/sh

# This script starts snmpd and snmptrapd as non-root and loads the
# xorp_if_mib module

. dbg_config.sh

echo "dlmod xorp_if_mib_module  ${MIBS_DIR}/xorp_if_mib_module.so" > snmpd.conf
echo "trap2sink localhost xorp ${SNMPTRAPD_PORT}"                 >> snmpd.conf
echo "rwcommunity ${SNMP_DBG_COMMUNITY}"                          >> snmpd.conf

export LD_LIBRARY_PATH=${MIBS_DIR}

snmptrapd -o ${SNMPTRAPD_LOG} localhost:${SNMPTRAPD_PORT}
snmpd -f -r -Dxorp_if_mib_module,dlmod,usmUser,bgp4_mib_1657 -C -c ${PWD}/snmpd.conf -l ${SNMPD_LOG} localhost:${SNMPD_PORT}

killall -m snmp
