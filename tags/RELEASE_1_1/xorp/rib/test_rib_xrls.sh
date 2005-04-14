#!/bin/sh
#
# $XORP: xorp/rib/test_rib_xrls.sh.in,v 1.1 2003/05/22 18:11:20 hodson Exp $
#

if [ "X${srcdir}" = "X" ] ; then srcdir=`dirname $0` ; fi
./test_rib_xrls < ${srcdir}/commands

