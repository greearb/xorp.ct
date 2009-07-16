# Copyright (c) 2009 XORP, Inc.
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

# $Id$

import sys

_comment = """Define to 1 if your processor stores words with the %s significant byte
   first (%s Motorola and SPARC, %s Intel and VAX)."""

def _WordsEndian(context, big):
    """
    Store result of the autotools-style endian test in context.havedict and
    context.headerfilename.
    "big" is a Boolean which specifies if the target endianness is big.
    """
    key = "WORDS_%sENDIAN" % {True:"BIG", False:"SMALL"}[big]
    comment = _comment % ({True:"most", False:"least"}[big], {True:"like", False:"unlike"}[big], {True:"unlike", False:"like"}[big])
    context.havedict[key] = ''
    lines = "\n/* %s */\n" % comment
    lines += "#define %s\n" % key
    if context.headerfilename:
        f = open(context.headerfilename, "a")
        f.write(lines)
        f.close()
    elif hasattr(context,'config_h'):
        context.config_h = context.config_h + lines

# FIXME not for cross-compiles.
def CheckEndianness(context):
    context.Message("Checking whether byte ordering is bigendian... ")
    ret = (sys.byteorder != "little")
    context.Result({True:"yes", False:"no"}[ret])
    _WordsEndian(context, ret)
    return ret
