#! /bin/sh
#
# Set the environment variables needed to snmp scripts 
#
# usage:
# 
#     $ . snmpenv.sh
#
XORP=`(cd ../..; pwd)`
MIBDIRS=+${XORP}/mibs/textual
MIBS=+BGP4-MIB
LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${XORP}/mibs
 
export XORP MIBDIRS MIBS LD_LIBRARY_PATH

# create snmp.conf in home directory.  This provides the default options to be
# passed to the Net-SNMP command line tools (version, community string, etc.)

if ! test -e ~/.snmp/snmp.conf; then
    if ! test -d ~/.snmp; then
	mkdir ~/.snmp
    fi
    echo 'copied default snmp.conf in directory ~/.snmp'
    cp ${XORP}/mibs/snmpconfig/snmp.conf ~/.snmp/

fi 

# create an entry in snmpd.conf to automatically load the XORP interface mib
# module.  After that module is loaded, we can use the xorp_if_mib interface
# to load other modules using XRL's
#
NEW_SNMPD_CONF=${XORP}/mibs/snmpconfig/snmpd.conf
cp ${XORP}/mibs/snmpconfig/snmpd.preconf ${NEW_SNMPD_CONF}
echo "dlmod xorp_if_mib_module ${XORP}/mibs/xorp_if_mib_module.so"  \
    >> ${NEW_SNMPD_CONF} 
