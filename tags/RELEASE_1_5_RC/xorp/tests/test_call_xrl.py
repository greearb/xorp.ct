#!/usr/bin/env python

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

# $XORP: xorp/tests/test_call_xrl.py,v 1.2 2007/02/16 22:47:30 pavlin Exp $

import popen2

def call_xrl(builddir, command):
    """
    Call an XRL
    """

    #print command

    call_xrl_path=builddir + "libxipc/call_xrl"
    process = popen2.Popen4(call_xrl_path + " " + "\"" + command + "\"")

    out=""
    while 1:
        lines = process.fromchild.readline()
        if not lines:
            break
        out += lines
    status = process.wait()

    return status, out


# Local Variables:
# mode: python
# py-indent-offset: 4
# End:
