#! /bin/sh
#
# Set the environment variables needed to snmp scripts 
#
# usage:
# 
#     $ . snmpenv.sh
#

if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi

XORP_TOP_SRCDIR=`(cd ${srcdir}/../..; pwd)`
XORP_TOP_BUILDDIR=`(cd ${PWD}/../..; pwd)`

MIBDIRS=+${XORP_TOP_SRCDIR}/mibs/textual
MIBS=+BGP4-MIB
LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${XORP_TOP_BUILDDIR}/mibs
 
export XORP_TOP_SRCDIR XORP_TOP_BUILDDIR MIBDIRS MIBS LD_LIBRARY_PATH

# create snmp.conf in home directory.  This provides the default options to be
# passed to the Net-SNMP command line tools (version, community string, etc.)

if ! test -e ~/.snmp/snmp.conf; then
	mkdir -p ~/.snmp
    cp ${XORP_TOP_SRCDIR}/mibs/snmpconfig/snmp.conf ~/.snmp/
    echo 'copied default snmp.conf in directory ~/.snmp'
fi 

# create an entry in snmpd.conf to automatically load the XORP interface mib
# module.  After that module is loaded, we can use the xorp_if_mib interface
# to load other modules using XRL's
#
NEW_SNMPD_CONF=${XORP_TOP_BUILDDIR}/mibs/snmpconfig/snmpd.conf
mkdir -p ${XORP_TOP_BUILDDIR}/mibs/snmpconfig/
cp ${XORP_TOP_SRCDIR}/mibs/snmpconfig/snmpd.preconf ${NEW_SNMPD_CONF}
echo "dlmod xorp_if_mib_module "					\
     "${XORP_TOP_BUILDDIR}/mibs/xorp_if_mib_module.so"			\
	>> ${NEW_SNMPD_CONF} 
