#!/usr/bin/env python

# Copyright (c) 2001-2008 XORP, Inc.
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

# $XORP: xorp/tests/test_builddir.py,v 1.6 2008/01/04 03:17:49 pavlin Exp $

BUILDDIR='../'

def builddir(depth = 0):
    """
    Return the top of the build directory
    """

    global BUILDDIR
    bdir = ''
    for i in range(depth):
        bdir += '../'

    return bdir + BUILDDIR

# Local Variables:
# mode: python
# py-indent-offset: 4
# End:
