#! /bin/sh
#
# Set the environment variables needed to snmp scripts 
#
# usage:
# 
#     $ source set_env.sh
#
export XORP=`realpath ../..`
export MIBDIRS=+${XORP}/mibs/textual
export MIBS=+BGP4-MIB

# create snmp.conf in home directory.  This provides the default options to be
# passed to the Net-SNMP command line tools (version, community string, etc.)

if ! test -e ~/.snmp/snmp.conf; then
    if ! test -d ~/.snmp; then
	mkdir ~/.snmp
    fi
    echo 'copied default snmp.conf in directory ~/.snmp'
    cp ${XORP}/mibs/snmpconfig/snmp.conf ~/.snmp/
fi 
