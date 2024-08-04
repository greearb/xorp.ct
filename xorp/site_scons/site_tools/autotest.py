#!/usr/bin/env python2
# vim:set sts=4 ts=4 sw=4:
# coding=UTF-8
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
#
# $XORP$
#
# Wrapper for legacy Automake-style regression tests.
#
# Loosely based on the CxxTest builder for SCons.
# Usage is identical to the Program() builder.
#
# This script should be placed in a file called autotest.py, somewhere in the
# scons toolpath. (default: #/site_scons/site_tools/)
#

from SCons.Script import *
from SCons.Builder import Builder
import SCons.Warnings
import os

# A warning class to notify users of problems
class ToolAutoTestWarning(SCons.Warnings.SConsWarning):
    pass

SCons.Warnings.enableWarningClass(ToolAutoTestWarning)

def multiget(dictlist, key, default = None):
    """
    Takes a list of dictionaries as its 1st argument. Checks if the key exists
    in each one and returns the 1st one it finds. If the key is found in no
    dictionaries, the default is returned.
    """
    for d in dictlist:
        if key in d:
            return d[key]
    else:
        return default

def _UnitTest(env, target, source = [], **kwargs):
    """
    Prepares the Program call arguments, calls Program and adds the result to
    the check target.
    """
    # get the c and cxx flags to process.
    ccflags   = Split( multiget([kwargs, env], 'CCFLAGS' ))
    cxxflags  = Split( multiget([kwargs, env], 'CXXFLAGS'))
    libpath  = Split( multiget([kwargs, env], 'LIBPATH'))
    rpath  = Split( multiget([kwargs, env], 'RPATH'))
    linkflags  = Split( multiget([kwargs, env], 'LINKFLAGS'))

    # For a test, take the our passed in LIBPATH, expand it, and prepend
    # to our passed in RPATH, so that tests can build using shared
    # libraries, even though they are not installed.
    # Tests are not intended to be installed, so we don't do any
    # further RPATH magic here.
    myrpath = rpath
    if 'SHAREDLIBS' in env:
        baserpath = Dir(env['BUILDDIR']).abspath
        myrpath += [ x.replace('$BUILDDIR', baserpath) for x in libpath ]

    # fill the flags into kwargs
    kwargs["CXXFLAGS"] = cxxflags
    kwargs["CCFLAGS"]  = ccflags
    kwargs["LIBPATH"]  = libpath
    kwargs["RPATH"]  = myrpath
    kwargs["LINKFLAGS"]  = linkflags

    # build the test program
    test = env.Program(target, source = source, **kwargs)

    # FIXME: Skip this step if we are cross-compiling (don't run the runner).
    if multiget([kwargs, env], 'AUTOTEST_SKIP_ERRORS', False):
        runner = env.Action(test[0].abspath, exitstatfunc=lambda x:0)
    else:
        runner = env.Action(test[0].abspath)
    env.Alias(env['AUTOTEST_TARGET'], test, runner)
    env.AlwaysBuild(env['AUTOTEST_TARGET'])

    return test

def generate(env, **kwargs):
    """
    Accepted keyword arguments:
    AUTOTEST_TARGET  - the target to append the tests to.  Default: check
    AUTOTEST_SKIP_ERRORS - set to True to continue running the next test if
                             one test fails. Default: False
    ... and all others that Program() accepts, like CPPPATH etc.
    """

    #
    # Expected behaviour: keyword arguments override environment variables;
    # environment variables override default settings.
    #
    env.SetDefault( AUTOTEST_TARGET  = 'check' )
    env.SetDefault( AUTOTEST_SKIP_ERRORS = False )

    #Here's where keyword arguments are applied
    # apply(env.Replace, (), kwargs)

    def AutoTest(env, target, source = None, **kwargs):
        """Usage:
        The function is modelled to be called as the Program() call is.
        """
        return _UnitTest(env, target = target, source = source, **kwargs)

    env.Append( BUILDERS = { "AutoTest" : AutoTest } )

def exists(env):
    return True
