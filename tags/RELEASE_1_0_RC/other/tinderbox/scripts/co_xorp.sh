#!/bin/sh -e

CONFIG="$(dirname $0)/config"
. $CONFIG

cd $ROOTDIR
XCVS='cvs -d /usr/local/www/data/cvs'

rm -rf xorp
$XCVS co xorp
$XCVS co data/bgp

