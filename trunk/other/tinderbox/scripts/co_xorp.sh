#!/bin/sh -e

# Copyright (c) 2001-2008 International Computer Science Institute
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software")
# to deal in the Software without restriction, subject to the conditions
# listed in the XORP LICENSE file. These conditions include: you must
# preserve this copyright notice, and you cannot mention the copyright
# holders in advertising related to the Software without their permission.
# The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
# notice is a summary of the XORP LICENSE file; the license in that file is
# legally binding.

# $XORP: other/tinderbox/scripts/co_xorp.sh,v 1.5 2008/01/04 03:01:42 pavlin Exp $

CONFIG="$(dirname $0)/config"
. $CONFIG

cd $ROOTDIR
XCVS='cvs -d :pserver:xorpcvs@anoncvs.xorp.org:/cvs'

rm -rf xorp
$XCVS co xorp
$XCVS co data/bgp

