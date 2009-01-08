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

# $XORP: xorp/tests/test_main.py,v 1.7 2008/10/02 21:58:30 bms Exp $

# A common main to be used by all test programs plus the entry point into
# running the tests.

import getopt
import sys
import os
import time
import re
from test_builddir import builddir
from test_call_xrl import call_xrl

def delay(seconds):
    """
    Sleep for the number of seconds specified and provide some feedback
    """

    print "Sleeping for %d seconds" % seconds
    os.system("date")

    columns = 80

    slept = 0

    if seconds < columns:
        bars = columns / seconds
        for i in range(seconds):
            for b in range(bars):
                sys.stdout.write('-')
                sys.stdout.flush()
            time.sleep(1)
            slept += 1
    else:
        delay = seconds / columns
        remainder = seconds % columns
        for i in range(columns):
            sys.stdout.write('-')
            sys.stdout.flush()
            if 0 != remainder:
                snooze = delay + 1
                remainder -= 1
            else:
                snooze = delay
            time.sleep(snooze)
            slept += snooze

    print

    os.system("date")

    if slept != seconds:
        raise Exception, 'Delay was too short should have been %s was %s' % \
              (seconds, slept)

def coord(command, noblock=False):
    """
    Send a command to the coordinator
    """

    print command
    status, message = call_xrl(builddir(1), "finder://coord/coord/0.1/command?command:txt=%s" % command)
    if 0 != status:
        if noblock:
            print message
            return
        raise Exception, message

    # Wait up to five seconds for this command to complete
    for i in range(5):
        if pending() == False:
            return
        delay(1)

    print >> sys.stderr, "Still pending"

def pending():
    """
    Check the previous command has completed
    """

    status, message = call_xrl(builddir(1), "finder://coord/coord/0.1/pending")
    if message == "pending:bool=false\n":
        return False
    else:
        return True

def status(peer):
    """
    Get the status of a test peer.
    """

    status, message = call_xrl(builddir(1), "finder://coord/coord/0.1/status?peer:txt=" + peer)

    message = re.sub('^status:txt=', '', message)
    message = re.sub('\+', ' ', message)

    return message

def run_test(test, single, configure, TESTS, config_module, test_module):
    """
    Run the provided test
    """

    bdir = builddir(1)

    # First find the test if it exists
    test_func = ''
    conf_funcs = []
    for i in TESTS:
        if test == i[0]:
            test_func = i[1]
            if i[3] != '' and i[4] != '':
                print "Both fields should not be set"
                return False
            if i[3] != '':
                conf_funcs.append("UNKNOWN")
                test_func +=  '(bdir,conf)'
            if i[4] != '':
                print "debug", i[4]
                for f in i[4]:
                    conf_funcs.append(f + '(bdir)')
                test_func +=  '()'

    if not single:
        print "------ START PROGRAMS ------"

    conf_mod = __import__(config_module)
    test_mod = __import__(test_module)
    
    print conf_funcs

    try: 
        if configure:
            for i in conf_funcs:
                if not eval('conf_mod' + '.' + i):
                    print i, "FAILED"
                    return False
        if not eval('test_mod' + '.' + test_func):
            print test, "FAILED"
            return False
        else:
            print test, "SUCCEEDED"
    except Exception, (ErrorMessage):
        print ErrorMessage
        print test, "FAILED"
        return False

    return True

def test_main(TESTS, config_module, test_module):
    def usage():
        us = \
"usage: %s [-h|--help] [-t|--test] [-b|--bad] [-s|--single] [-c|--configure]"
        print us % sys.argv[0]
        

    try:
        opts, args = getopt.getopt(sys.argv[1:], "h:t:bsc", \
                                   ["help", \
                                    "test=", \
                                    "bad", \
                                    "single", \
                                    "configure", \
                                    ])
    except getopt.GetoptError:
        usage()
        sys.exit(1)


    bad = False
    single = False
    configure = True
    tests = []
    for o, a in opts:
	if o in ("-h", "--help"):
	    usage()
	    sys.exit()
        if o in ("-t", "--test"):
            tests.append(a)
        if o in ("-b", "--bad"):
            bad = True
        if o in ("-s", "--single"):
            single = True
            configure = False
        if o in ("-c", "--configure"):
            configure = True

    if not tests:
        for i in TESTS:
            if bad != i[2]:
                tests.append(i[0])

    print tests

    for i in tests:
        if not run_test(i,single,configure,TESTS,config_module,test_module):
            print "Test: " + i + " FAILED"
            sys.exit(-1)
            
    sys.exit(0)

# Local Variables:
# mode: python
# py-indent-offset: 4
# End:
