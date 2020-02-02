#!/usr/bin/env python2

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

# $XORP: xorp/tests/test_call_xrl.py,v 1.5 2008/10/02 21:58:30 bms Exp $

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
