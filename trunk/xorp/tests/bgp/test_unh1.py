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

# $XORP: xorp/tests/bgp/test_unh1.py,v 1.6 2006/04/13 00:06:51 atanu Exp $

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
     conf_RUT_as2_TR1_as1_TR2_as2_TR3_as3, \
     conf_RUT_as2_TR1_as1_TR2_as3, \
     conf_interfaces, \
     conf_redist_static, \
     conf_redist_static_incomplete, \
     conf_redist_static_no_export, \
     conf_add_static_route4, \
     conf_import_med_change, \
     conf_export_med_change, \
     conf_import_origin_change, \
     conf_export_origin_change, \
     conf_preference_TR1, \
     conf_damping

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

    ['4.11A', 'test4_11_A', True, '',
     ['conf_RUT_as2_TR1_as1_TR2_as2_TR3_as3', 'conf_interfaces',
      'conf_damping']],

    ['4.11BCD', 'test4_11_BCD', True, '',
     ['conf_RUT_as2_TR1_as1_TR2_as2_TR3_as3', 'conf_interfaces',
      'conf_preference_TR1', 'conf_damping']],

    ['4.12', 'test4_12', True, '',
     ['conf_RUT_as2_TR1_as1_TR2_as3', 'conf_interfaces',
      'conf_damping']],

    # Move these tests to a separate policy test script.
#    ['test_import_med1', 'test_policy_med1', False, '',
#     ['conf_RUT_as2_TR1_as1_TR2_as2_TR3_as3', 'conf_interfaces',
#      'conf_import_med_change']],

    ['test_export_med1', 'test_policy_med1', True, '',
     ['conf_RUT_as2_TR1_as1_TR2_as2_TR3_as3', 'conf_interfaces',
      'conf_export_med_change']],

    ['test_import_origin1', 'test_policy_origin1', True, '',
     ['conf_RUT_as2_TR1_as1_TR2_as2_TR3_as3', 'conf_interfaces',
      'conf_import_origin_change']],

    ['test_export_origin1', 'test_policy_origin1', True, '',
     ['conf_RUT_as2_TR1_as1_TR2_as2_TR3_as3', 'conf_interfaces',
      'conf_export_origin_change']],
    ]

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

def test1_1_A():
    """
    Direct connection Basic IBGP
    """

    # configure the test peering.
    coord("reset")
    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("peer1 establish AS 65000 holdtime 0 id 1.2.3.4 keepalive false")
    
    delay(2)

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
    
    delay(2)

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
    
    delay(2)

    coord("peer1 assert established");
    coord("peer2 assert established");
    coord("peer3 assert established");

    delay(2)

    incomplete = 2
    coord("peer1 expect packet update \
    nexthop 127.0.0.1 \
    origin %s \
    aspath 65000 \
    med 0 nlri \
    172.16.0.0/16" % incomplete)

    if not conf_add_static_route4(builddir(1), "172.16.0.0/16"):
        return False

    delay(10)

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
    
    delay(2)

    coord("peer1 assert established");
    coord("peer2 assert established");

    delay(2)

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

    delay(10)

    coord("peer1 assert established")
    coord("peer2 assert established")

    coord("peer1 assert queue 0")
    coord("peer2 assert queue 0")

    return True

def test4_11_A():
    """
    Test route flap damping
    """

    coord("reset")

    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("target 127.0.0.1 10002")
    coord("initialise attach peer2")

    coord("target 127.0.0.1 10003")
    coord("initialise attach peer3")

    coord("peer1 establish AS 1 holdtime 0 id 10.0.0.1 keepalive false")
    coord("peer2 establish AS 1 holdtime 0 id 10.0.0.2 keepalive false")
    coord("peer3 establish AS 3 holdtime 0 id 10.0.0.3 keepalive false")
    
    delay(2)

    coord("peer1 assert established");
    coord("peer2 assert established");
    coord("peer3 assert established");

    delay(2)

    packet = "packet update \
    nexthop %s \
    origin 0 \
    aspath %s \
    med 0 \
    nlri 192.1.0.0/16"

    spacket = packet % ("127.0.0.2", "1")
    epacket = packet % ("127.0.0.1", "2,1")
    
    coord("peer1 expect %s" % epacket)
    coord("peer2 expect %s" % epacket)
    coord("peer3 expect %s" % epacket)

    delay(2)

    coord("peer1 send %s" % spacket)

    delay(10)

    coord("peer1 assert established")
    coord("peer2 assert established")
    coord("peer3 assert established")

    coord("peer1 assert queue 1")
    coord("peer2 assert queue 1")
    coord("peer3 assert queue 0")

    return True

def test4_11_BCD():
    """
    Test route flap damping
    """

    coord("reset")

    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("target 127.0.0.1 10002")
    coord("initialise attach peer2")

    coord("target 127.0.0.1 10003")
    coord("initialise attach peer3")

    coord("peer1 establish AS 1 holdtime 0 id 10.0.0.1 keepalive false")
    coord("peer2 establish AS 1 holdtime 0 id 10.0.0.2 keepalive false")
    coord("peer3 establish AS 3 holdtime 0 id 10.0.0.3 keepalive false")
    
    delay(2)

    coord("peer1 assert established");
    coord("peer2 assert established");
    coord("peer3 assert established");

    delay(2)

    packet = "packet update \
    nexthop %s \
    origin 0 \
    aspath %s \
    med 0 \
    nlri 192.1.0.0/16"

    spacket = packet % ("127.0.0.3", "1")
    epacket = packet % ("127.0.0.1", "2,1")
    
    coord("peer1 expect %s" % epacket)
    coord("peer2 expect %s" % epacket)
    coord("peer3 expect %s" % epacket)

    delay(2)

    # 9. TR2 Sends an update message with a route of 192.1.0.0/16 ...

    coord("peer2 send %s" % spacket)

    coord("peer1 assert established")
    coord("peer2 assert established")
    coord("peer3 assert established")

    coord("peer1 assert queue 1")
    coord("peer2 assert queue 1")
    coord("peer3 assert queue 0")

    spacket = packet % ("127.0.0.2", "1")

    coord("peer3 expect %s" % epacket)
    coord("peer3 expect %s" % epacket)

    coord("peer3 expect %s" % epacket)
    coord("peer3 expect %s" % epacket)

    delay(2)

    # Flap the route from peer1

    spacket = packet % ("127.0.0.2", "1")

    coord("peer1 send %s" % spacket)
    coord("peer1 send packet update withdraw 192.1.0.0/16")

    coord("peer1 send %s" % spacket)
    coord("peer1 send packet update withdraw 192.1.0.0/16")

    delay(2)

    coord("peer1 assert established")
    coord("peer2 assert established")
    coord("peer3 assert established")

    coord("peer1 assert queue 1")
    coord("peer2 assert queue 1")
    coord("peer3 assert queue 0")
    
    # Part C

    coord("peer1 send %s" % spacket)
    coord("peer1 send packet update withdraw 192.1.0.0/16")

    coord("peer1 send %s" % spacket)
#    coord("peer1 send packet update withdraw 192.1.0.0/16")

    # Part D

    delay(10)

    # The release of the damped packet.
    coord("peer3 expect %s" % epacket)
    
    sleep = 5 * 60

    print 'Sleeping for %d seconds, waiting for damped route' % sleep

    delay(sleep)

    # Make sure that at the end of the test all the connections still exist.

    coord("peer1 assert established")
    coord("peer2 assert established")
    coord("peer3 assert established")

    coord("peer1 assert queue 1")
    coord("peer2 assert queue 1")
    coord("peer3 assert queue 0")

    return True

def test4_12():
    """
    Test route flap damping
    """

    coord("reset")

    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("target 127.0.0.1 10002")
    coord("initialise attach peer2")

    coord("peer1 establish AS 1 holdtime 0 id 10.0.0.1 keepalive false")
    coord("peer2 establish AS 3 holdtime 0 id 10.0.0.2 keepalive false")

    delay(2)

    coord("peer1 assert established");
    coord("peer2 assert established");

    packet = "packet update \
    nexthop %s \
    origin 0 \
    aspath %s \
    med 0 \
    nlri 192.1.0.0/16"
    
    spacket1 = packet % ("127.0.0.3", "1,12")
    spacket2 = packet % ("127.0.0.3", "1,14")

    epacket1 = packet % ("127.0.0.1", "2,1,12")
    epacket2 = packet % ("127.0.0.1", "2,1,14")

    # This is a hack to test that no packets arrive on this peer.
    coord("peer1 expect %s" % spacket1)

    coord("peer2 expect %s" % epacket1)
    coord("peer2 expect %s" % epacket2)
    coord("peer2 send packet update withdraw 192.1.0.0/16")
    
    for i in range(5):
        coord("peer1 send %s" % spacket1)
        coord("peer1 send %s" % spacket2)

    delay(10)

    coord("peer1 assert established")
    coord("peer2 assert established")

    coord("peer1 assert queue 1")
    coord("peer2 assert queue 0")

    # The release of the damped packet.
    coord("peer2 expect %s" % epacket2)
    
    sleep = 5 * 60

    print 'Sleeping for %d seconds, waiting for damped route' % sleep

    delay(sleep)

    # Make sure that at the end of the test all the connections still exist.

    coord("peer1 assert established")
    coord("peer2 assert established")

    coord("peer1 assert queue 1")
    coord("peer2 assert queue 0")

    return True

def test_policy_med1():
    """
    Introduce a med of 0 and expect a med of 2 at the peers.
    Allows the testing of import and export policies to change the med.
    """

    coord("reset")

    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("target 127.0.0.1 10002")
    coord("initialise attach peer2")

    coord("target 127.0.0.1 10003")
    coord("initialise attach peer3")

    coord("peer1 establish AS 1 holdtime 0 id 10.0.0.1 keepalive false")
    coord("peer2 establish AS 1 holdtime 0 id 10.0.0.2 keepalive false")
    coord("peer3 establish AS 3 holdtime 0 id 10.0.0.3 keepalive false")
    
    delay(2)

    coord("peer1 assert established");
    coord("peer2 assert established");
    coord("peer3 assert established");

    delay(2)

    packet = "packet update \
    nexthop %s \
    origin 0 \
    aspath %s \
    med %s \
    nlri 192.1.0.0/16"

    spacket = packet % ("127.0.0.2", "1", "0")
    epacket = packet % ("127.0.0.1", "2,1", "42")
    
    coord("peer1 expect %s" % epacket)
    coord("peer2 expect %s" % epacket)
    coord("peer3 expect %s" % epacket)

    delay(2)

    coord("peer2 send %s" % spacket)

    delay(10)

    coord("peer1 assert established")
    coord("peer2 assert established")
    coord("peer3 assert established")

    coord("peer1 assert queue 1")
    coord("peer2 assert queue 1")
    coord("peer3 assert queue 0")

    return True

def test_policy_origin1():
    """
    Introduce an origin of 0 and expect and origin of 2 at the peers.
    Allows the testing of import and export policies to change the origin.
    """

    coord("reset")

    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("target 127.0.0.1 10002")
    coord("initialise attach peer2")

    coord("target 127.0.0.1 10003")
    coord("initialise attach peer3")

    coord("peer1 establish AS 1 holdtime 0 id 10.0.0.1 keepalive false")
    coord("peer2 establish AS 1 holdtime 0 id 10.0.0.2 keepalive false")
    coord("peer3 establish AS 3 holdtime 0 id 10.0.0.3 keepalive false")
    
    delay(2)

    coord("peer1 assert established");
    coord("peer2 assert established");
    coord("peer3 assert established");

    delay(2)

    packet = "packet update \
    nexthop %s \
    origin %s \
    aspath %s \
    med 0 \
    nlri 192.1.0.0/16"

    spacket = packet % ("127.0.0.2", "0", "1")
    epacket = packet % ("127.0.0.1", "2", "2,1")
    
    coord("peer1 expect %s" % epacket)
    coord("peer2 expect %s" % epacket)
    coord("peer3 expect %s" % epacket)

    delay(2)

    coord("peer2 send %s" % spacket)

    delay(10)

    coord("peer1 assert established")
    coord("peer2 assert established")
    coord("peer3 assert established")

    coord("peer1 assert queue 1")
    coord("peer2 assert queue 1")
    coord("peer3 assert queue 0")

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
