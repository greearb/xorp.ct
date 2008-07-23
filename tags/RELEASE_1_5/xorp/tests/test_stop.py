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

# $XORP: xorp/tests/test_stop.py,v 1.3 2008/01/04 03:17:50 pavlin Exp $

from test_xorpsh import xorpsh
from test_builddir import builddir

def stop():
    """
    Stop all the rtrmgr subprocesses by loading an empty configuration
    """

    xorpsh_commands = \
"""configure
load empty.boot
commit
"""

    if not xorpsh(builddir(), xorpsh_commands, 'templates'):
        return False

    return True

stop()    

# Local Variables:
# mode: python
# py-indent-offset: 4
# End:
