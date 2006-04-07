#!/usr/bin/env python

# Copyright (c) 2001-2006 International Computer Science Institute
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

# $XORP: xorp/tests/bgp/test_unh1.py,v 1.1 2006/04/07 05:13:09 atanu Exp $

#
# The tests in this file are based on the:
# University of New Hampshire
# InterOperability Laboratory
# IPv4 CONSORTIUM
# Border Gateway Protocol 4 (BGP) Operations Test Suite
# Technical Document Revision 3.3
#

import sys
import os
import time
import getopt
sys.path.append("..")
from test_call_xrl import call_xrl
from test_builddir import builddir
from test_bgp_config import \
     conf_IBGP, \
     conf_EBGP, \
     conf_EBGP_EBGP, \
     conf_EBGP_IBGP_IBGP, \
     conf_interfaces, \
     conf_redist_static, \
     conf_redist_static_incomplete, \
     conf_redist_static_no_export, \
     conf_add_static_route4

TESTS=[
    # Fields:
    # 0: Symbolic name for test
    # 1: Actual test function
    # 2: True if this test works.
    # 3: Optional Configuration String
    # 4: Optional Configuration Functions
    # NOTE: One of field 4 or 5 must be set and the other must be empty.
    ['1.1A', 'test1_1_A', True, '',
     ['conf_IBGP']],
    ['1.1B', 'test1_1_B', True, '',
     ['conf_EBGP']],
    ['1.10C', 'test1_10_C', True, '',
     ['conf_EBGP_IBGP_IBGP', 'conf_interfaces',
      'conf_redist_static_incomplete']],
    ['4.6A', 'test4_6_A', True, '',
     ['conf_EBGP_EBGP', 'conf_interfaces',
      'conf_redist_static_no_export']],
    ]

def coord(command):
    """
    Send a command to the coordinator
    """

    print command
    status, message = call_xrl(builddir(1), "finder://coord/coord/0.1/command?command:txt=%s" % command)
    if 0 != status:
        raise Exception, message

    # Wait up to five seconds for this command to complete
    for i in range(5):
        if pending() == False:
            return
        time.sleep(1)

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

def test1_1_A():
    """
    Direct connection Basic IBGP
    """

    # configure the test peering.
    coord("reset")
    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("peer1 establish AS 65000 holdtime 0 id 1.2.3.4 keepalive false")
    
    time.sleep(2)

    coord("peer1 assert established");

    return True

def test1_1_B():
    """
    Direct connection Basic IBGP
    """

    # configure the test peering.
    coord("reset")
    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("peer1 establish AS 65001 holdtime 0 id 1.2.3.4 keepalive false")
    
    time.sleep(2)

    coord("peer1 assert established");

    return True

def test1_10_C():
    """
    To verify that a BGP router properly generates the ORIGIN attribute
    """

    coord("reset")

    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("target 127.0.0.1 10002")
    coord("initialise attach peer2")

    coord("target 127.0.0.1 10003")
    coord("initialise attach peer3")

    coord("peer1 establish AS 65001 holdtime 0 id 10.0.0.1 keepalive false")
    coord("peer2 establish AS 65000 holdtime 0 id 10.0.0.2 keepalive false")
    coord("peer3 establish AS 65000 holdtime 0 id 10.0.0.3 keepalive false")
    
    time.sleep(2)

    coord("peer1 assert established");
    coord("peer2 assert established");
    coord("peer3 assert established");

    time.sleep(2)

    incomplete = 2
    coord("peer1 expect packet update \
    nexthop 127.0.0.1 \
    origin %s \
    aspath 65000 \
    med 0 nlri \
    172.16.0.0/16" % incomplete)

    if not conf_add_static_route4(builddir(1), "172.16.0.0/16"):
        return False

    time.sleep(10)

    coord("peer1 assert established")
    coord("peer2 assert established")
    coord("peer3 assert established")

    coord("peer1 assert queue 0")

    return True

def test4_6_A():
    """
    Verify that the NO_EXPORT community can be set on a redistributed route
    """
    coord("reset")

    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("target 127.0.0.1 10002")
    coord("initialise attach peer2")

    coord("peer1 establish AS 65001 holdtime 0 id 10.0.0.1 keepalive false")
    coord("peer2 establish AS 65002 holdtime 0 id 10.0.0.2 keepalive false")
    
    time.sleep(2)

    coord("peer1 assert established");
    coord("peer2 assert established");

    time.sleep(2)

    packet = "packet update \
    nexthop 127.0.0.1 \
    origin 0 \
    aspath 65000 \
    med 0 \
    nlri 172.16.0.0/16 \
    community NO_EXPORT"

    coord("peer1 expect %s" % packet)
    coord("peer2 expect %s" % packet)

    if not conf_add_static_route4(builddir(1), "172.16.0.0/16"):
        return False

    time.sleep(10)

    coord("peer1 assert established")
    coord("peer2 assert established")

    coord("peer1 assert queue 0")
    coord("peer2 assert queue 0")

    return True

def run_test(test, single, configure):
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
                print "debug", i[4][0]
                for f in i[4]:
                    conf_funcs.append(f + '(bdir)')
                test_func +=  '()'

    if not single:
        print "------ START PROGRAMS ------"

    print conf_funcs

    try: 
        if configure:
            for i in conf_funcs:
                if not eval(i):
                    print i, "FAILED"
                    return False
        if not eval(test_func):
            print test, "FAILED"
            return False
        else:
            print test, "SUCCEEDED"
    except Exception, (ErrorMessage):
        print ErrorMessage
        print test, "FAILED"
        return False

    return True
    
def main():
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
        if not run_test(i, single, configure):
            print "Test: " + i + " FAILED"
            sys.exit(-1)
            
    sys.exit(0)

main()
    
# Local Variables:
# mode: python
# py-indent-offset: 4
# End:
