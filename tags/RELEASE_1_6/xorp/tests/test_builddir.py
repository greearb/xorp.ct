#!/usr/bin/env python

# Copyright (c) 2001-2009 XORP, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, Version 2, June
# 1991 as published by the Free Software Foundation. Redistribution
# and/or modification of this program under the terms of any other
# version of the GNU General Public License is not permitted.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
# see the GNU General Public License, Version 2, a copy of which can be
# found in the XORP LICENSE.gpl file.
# 
# XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
# http://xorp.net

# $XORP: xorp/tests/test_builddir.py,v 1.8 2008/10/02 21:58:30 bms Exp $

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
