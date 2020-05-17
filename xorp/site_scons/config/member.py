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

import re
import string
from SCons.Conftest import LogInputFiles, LogErrorMessages

# TODO: Support C++.
def CheckTypeMember(context, type_name, member_name, includes = None,
                    language = None):
    """
    Configure check for a C or C++ type "type_name" with "member_name".
    Optional "includes" can be defined to include a header file.
    "language" should be "C" or None. Default is "C".
    Sets HAVE_type_name in context.havedict according to the result.
    Note that this uses the current value of compiler and linker flags, make
    sure $CFLAGS, $CPPFLAGS and $LIBS are set correctly.
    Returns an empty string for success, an error message for failure.
    """

    # Include "confdefs.h" first, so that the header can use HAVE_HEADER_H.
    if context.headerfilename:
        includetext = '#include "%s"' % context.headerfilename
    else:
        includetext = ''
    if not includes:
        includes = ""
    if language is not None and language != 'C':
        msg = 'Unsupported language'
        context.Display("Cannot check for %s type: %s\n" % (type_name, msg))
        return msg
    lang = 'C'
    suffix = '.c'

    src = includetext + includes

    src = src + """
int main()
{
  static %(type_name)s scons_aggr;
  if (sizeof (scons_aggr.%(member_name)s))
    return 0;
  return 0;
}
""" % { 'type_name': type_name,
        'member_name': member_name }

    context.Display("Checking whether %s type %s has member %s... " %
        (lang, type_name, member_name))
    ret = context.BuildProg(src, suffix)
    _YesNoResult(context, ret, "HAVE_" + type_name + '_' + member_name, src,
                 "Define to 1 if `%s' is member of `%s'." %
                 (member_name, type_name))
    # XXX
    context.did_show_result = 1
    return ret


#
# END OF PUBLIC FUNCTIONS
# Following cloned from Conftest.py.
#

def _YesNoResult(context, ret, key, text, comment = None):
    """
    Handle the result of a test with a "yes" or "no" result.
    "ret" is the return value: empty if OK, error message when not.
    "key" is the name of the symbol to be defined (HAVE_foo).
    "text" is the source code of the program used for testing.
    "comment" is the C comment to add above the line defining the symbol (the
    comment is automatically put inside a /* */). If None, no comment is added.
    """
    if key:
        _Have(context, key, not ret, comment)
    if ret:
        context.Display("no\n")
        _LogFailed(context, text, ret)
    else:
        context.Display("yes\n")


def _Have(context, key, have, comment = None):
    """
    Store result of a test in context.havedict and context.headerfilename.
    "key" is a "HAVE_abc" name.  It is turned into all CAPITALS and non-
    alphanumerics are replaced by an underscore.
    The value of "have" can be:
    1      - Feature is defined, add "#define key".
    0      - Feature is not defined, add "/* #undef key */".
             Adding "undef" is what autoconf does.  Not useful for the
             compiler, but it shows that the test was done.
    number - Feature is defined to this number "#define key have".
             Doesn't work for 0 or 1, use a string then.
    string - Feature is defined to this string "#define key have".
             Give "have" as is should appear in the header file, include quotes
             when desired and escape special characters!
    """
    key_up = key.upper()
    key_up = re.sub('[^A-Z0-9_]', '_', key_up)
    context.havedict[key_up] = have
    if have == 1:
        line = "#define %s 1\n" % key_up
    elif have == 0:
        line = "/* #undef %s */\n" % key_up
    elif type(have) == int:
        line = "#define %s %d\n" % (key_up, have)
    else:
        line = "#define %s %s\n" % (key_up, str(have))

    if comment is not None:
        lines = "\n/* %s */\n" % comment + line
    else:
        lines = "\n" + line

    if context.headerfilename:
        f = open(context.headerfilename, "a")
        f.write(lines)
        f.close()
    elif hasattr(context,'config_h'):
        context.config_h = context.config_h + lines


def _LogFailed(context, text, msg):
    """
    Write to the log about a failed program.
    Add line numbers, so that error messages can be understood.
    """
    if LogInputFiles:
        context.Log("Failed program was:\n")
        lines = string.split(text, '\n')
        if len(lines) and lines[-1] == '':
            lines = lines[:-1]              # remove trailing empty line
        n = 1
        for line in lines:
            context.Log("%d: %s\n" % (n, line))
            n = n + 1
    if LogErrorMessages:
        context.Log("Error message: %s\n" % msg)
