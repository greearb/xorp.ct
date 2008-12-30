#!/usr/bin/env python

# Copyright (c) 2001-2008 XORP, Inc.
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

# $XORP: xorp/tests/test_xorpsh.py,v 1.7 2008/07/23 05:11:49 pavlin Exp $

import popen2

def xorpsh(builddir, command, templates = '../templates'):
    """
    Send commands via the xorpsh
    """

    xorpsh_path = builddir + "rtrmgr/xorpsh -t %s" % templates

    process = popen2.Popen4(xorpsh_path)
    process.tochild.write(command)
    process.tochild.close()

    # XXX - This is not really a satisfactory way of determining if an
    # error has occurred
    error_responses = ["ERROR", "unknown command", "syntax error",
                       "Commit Failed"]

    output = ""
    while 1:
        line = process.fromchild.readline()
        if not line:
            break
        for i in error_responses:
            if line.startswith(i):
                raise Exception, line
        print line,
        output += line
    status = process.wait()

    if 0 == status:
        return True, output
    else:
        return False, output
