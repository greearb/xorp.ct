#! /bin/sh
#
# Load a mib module 
#     
#  $ loadbgp /home/jsmith/mymibs/bgp4_mib_1657.so 2
#
# Requires:
#
#  (0) that the finder is running
#
#  (1) that snmpd is running
#
#  (2) that the XORP interface module (xorp_if_mib_module) is loaded
#

. dbg_config.sh

if test "$1" = "" -o "$2" = "" ; then
    echo 'usage example:  '
    echo '    $ ./dbg_loadmib.sh bgp4_mib_1657 2' 
    exit 1
fi

MIBS_PATH=`(cd ..; pwd)`
MOD_FILENAME="${MIBS_PATH}/$1.so" 

echo "Loading module $1 from ${MOD_FILENAME}..."

../../libxipc/call_xrl "finder://xorp_if_mib/xorp_if_mib/0.1/load_mib?mod_name:txt=$1&abs_path:txt=${MOD_FILENAME}"


echo "The module $1 should appear loaded in position $2"
./dbg_dlmiblist.sh
