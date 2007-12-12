#!/usr/bin/env python

# Copyright (c) 2001-2007 International Computer Science Institute
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

# $XORP$

# Test to see if BGP is correctly dumping routes when new sessions are formed.

import sys
import os
import time
sys.path.append("..")
from test_main import test_main
from test_main import coord,pending,status
from test_main import delay
from test_builddir import builddir
import test_bgp_config as config

TESTS=[
    # Fields:
    # 0: Symbolic name for test
    # 1: Actual test function
    # 2: True if this test works.
    # 3: Optional Configuration String
    # 4: Optional Configuration Functions
    # NOTE: One of field 3 or 4 must be set and the other must be empty.

    ['test_dump_1', 'test_dump_1', True, '',
     ['conf_test_dump_1', 'conf_interfaces']],
    ]

def bgp_peer_unchanged(peer):
    """
    Wait for this peer to become quiet
    """
    while True:
        s1 = status(peer)
        print s1,
        time.sleep(10)
        s2 = status(peer)
        print s2,
        if s1 == s2:
            break
            
def test_dump_1():
    """
    Test that new sessions correctly receive all the routes that they should.
    Peer1 populated with routes.
    Peer2 repeatedly connects to verify that the BGP dump code is working
    correctly 
    """
    coord("reset")

    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("peer1 establish AS 65000 holdtime 0 id 75.75.75.75 keepalive false")
    
    delay(2)

    coord("peer1 assert established")

    delay(2)

    coord("peer1 send dump mrtd update ../../data/bgp/icsi1.mrtd", \
          noblock=True)

    bgp_peer_unchanged("peer1")

    coord("target 127.0.0.1 10002")
    coord("initialise attach peer2")

    for i in range(10):
        os.system("date")
        print "Iteration: ", i
        coord("peer2 establish AS 65001 holdtime 0 id 1.1.1.1 keepalive false",noblock=True)
        delay(2)
        bgp_peer_unchanged("peer2")
            
        coord("peer2 assert established")
        coord("peer2 disconnect")

    return True


test_main(TESTS, 'test_bgp_config', 'test_bgp_dump1')

# Local Variables:
# mode: python
# py-indent-offset: 4
# End:
