#!/usr/bin/env python2
#
# vim:set sts=4 ts=8 syntax=python:
#
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

# Boost configure checks derived from Mapnik/Wesnoth (LGPL/GPL2)

import sys

def _CheckBoostLib(context, boost_lib):
    env = context.env
    boost_headers = {
        # library basename : include file
        "date_time"       : "date_time.hpp",
        "filesystem"      : "filesystem.hpp",
        "iostreams"       : "iostreams/constants.hpp",
        "program_options" : "program_options.hpp",
        "regex"           : "regex/config.hpp",
        "signals"         : "signal.hpp",
        "system"          : "system/config.hpp",
        "thread"          : "thread.hpp",
    }

    header_name = boost_headers.get(boost_lib, boost_lib + ".hpp")
    libname = "boost_" + boost_lib + env.get("boost_suffix", "")

    # XXX prepending to global environment LIBS
    env.PrependUnique(LIBS = [libname])

    test_program = """
        #include <boost/%s>
        \n""" % header_name

    test_program += """
        int main()
        {
        }
        \n"""
    if context.TryLink(test_program, ".cpp"):
        return True
    return False

def CheckBoostLibrary(context, boost_lib):
    """Check for a Boost library.
       XXX Assumes CPPPATH and LIBPATH contain boost path somewhere."""
    context.Message("Checking for Boost %s library... " % boost_lib)
    check_result = _CheckBoostLib(context, boost_lib)
    # Try again with mt variant if no boost_suffix specified
    if not check_result and not context.env.get("boost_suffix"):
        context.env["boost_suffix"] = "-mt"
        check_result = _CheckBoostLib(context, boost_lib)
    if check_result:
        context.Result("yes")
    else:
        context.Result("no")
    return check_result

def CheckBoost(context, require_version = None):
    """ Check for Boost itself, by checking for version.hpp.
        A version may or may not be specified.
        XXX Assumes CPPPATH and LIBPATH contain boost path somewhere."""

    check_result = False
    if require_version:
        context.Message("Checking for Boost version >= %s... " % \
            require_version)
    else:
        context.Message("Checking for Boost... ")

    test_program = "#include <boost/version.hpp>\n"
    if require_version:
        version = require_version.split(".", 2)
        major = int(version[0])
        minor = int(version[1])
        try:
            sub_minor = int(version[2])
        except (ValueError, IndexError):
            sub_minor = 0
        test_program += """
            #if BOOST_VERSION < %d
             #error Boost version is too old!
            #endif
            """ % (major * 100000 + minor * 100 + sub_minor)
    test_program += """
        int main()
        {
        }
        """
    if context.TryCompile(test_program, ".cpp"):
        check_result = True
    if check_result:
        context.Result("yes")
    else:
        context.Result("no")
    return check_result
