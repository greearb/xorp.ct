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
