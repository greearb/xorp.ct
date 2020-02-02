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

# $XORP: xorp/tests/bgp/test_unh1.py,v 1.26 2008/10/02 21:58:31 bms Exp $

#
# The tests in this file are based on the:
# University of New Hampshire
# InterOperability Laboratory
# IPv4 CONSORTIUM
# Border Gateway Protocol 4 (BGP) Operations Test Suite
# Technical Document Revision 3.3
#

import sys
sys.path.append("..")
from test_main import test_main
from test_main import coord
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
    ['1.1A', 'test1_1_A', True, '',
     ['conf_IBGP']],

    ['1.1B', 'test1_1_B', True, '',
     ['conf_EBGP']],

    ['1.4C', 'test1_4_C', True, '',
     ['conf_EBGP', 'conf_interfaces']],

    ['1.6B', 'test1_6_B', True, '',
     ['conf_EBGP', 'conf_interfaces']],

    ['1.10C', 'test1_10_C', True, '',
     ['conf_EBGP_IBGP_IBGP', 'conf_interfaces',
      'conf_redist_static_incomplete']],

    ['1.12AB', 'test1_12_AB', True, '',
     ['conf_RUT_as2_TR1_as1_TR2_as2', 'conf_interfaces',
      'conf_redist_static']],

    ['1.13A', 'test1_13_A', True, '',
     ['conf_RUT_as2_TR1_as1_TR2_as2', 'conf_interfaces',
      'conf_redist_static_med']],

    ['1.13B', 'test1_13_B', True, '',
     ['conf_RUT_as2_TR1_as1_TR2_as2', 'conf_interfaces']],

    ['1.13C', 'test1_13_C', True, '',
     ['conf_RUT_as2_TR1_as1_TR2_as2', 'conf_interfaces']],

    ['1.15A', 'test1_15_A', True, '',
     ['conf_RUT_as3_TR1_as1_TR2_as2_TR3_as4', 'conf_interfaces']],

    ['1.15B', 'test1_15_B', True, '',
     ['conf_RUT_as3_TR1_as1_TR2_as2_TR3_as4', 'conf_interfaces',
      'conf_aggregate_brief']],

    ['3.8A', 'test3_8_A', True, '',
     ['conf_EBGP']],

    ['4.6A', 'test4_6_A', True, '',
     ['conf_EBGP_EBGP', 'conf_interfaces',
      'conf_redist_static_no_export']],

    ['4.8A', 'test4_8_A', True, '',
     ['conf_RUT_as2_TR1_as1', 'conf_interfaces',
      'conf_redist_static', 'conf_multiprotocol']],

    ['4.11A', 'test4_11_A', True, '',
     ['conf_RUT_as2_TR1_as1_TR2_as1_TR3_as3', 'conf_interfaces',
      'conf_damping']],

    ['4.11BCD', 'test4_11_BCD', True, '',
     ['conf_RUT_as2_TR1_as1_TR2_as1_TR3_as3', 'conf_interfaces',
      'conf_preference_TR1', 'conf_damping']],

    ['4.12', 'test4_12', True, '',
     ['conf_RUT_as2_TR1_as1_TR2_as3', 'conf_interfaces',
      'conf_damping']],

    ]

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
    Direct connection Basic EBGP
    """

    # configure the test peering.
    coord("reset")
    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("peer1 establish AS 65001 holdtime 0 id 1.2.3.4 keepalive false")
    
    delay(2)

    coord("peer1 assert established");

    return True

def test1_4_C():
    """
    Set the holdtime to an illegal value of 2 seconds
    """

    # Try and set the holdtime to 2 seconds which is illegal.
    try:
        if not config.conf_set_holdtime(builddir(1), "127.0.0.1", 2):
            return True
    except Exception, (ErrorMessage):
        print ErrorMessage
        return True

    return False

def test1_6_B():
    """
    Cease notification message
    """

    # Set the prefix limit to 3
    config.conf_set_prefix_limit(builddir(1), "peer1", 3)

    coord("reset")
    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("peer1 establish AS 65001 holdtime 0 id 1.2.3.4 keepalive false")
    
    delay(2)

    coord("peer1 assert established");

    packet = "packet update \
    nexthop 127.0.0.2 \
    origin 0 \
    aspath 65001 \
    med 0\
    nlri %s"

    cease = 6
    coord("peer1 expect packet notify %s" % cease)

    # Send four NRLIs to trigger a cease

    coord("peer1 send %s" % (packet % "172.16.0.0/16"))
    coord("peer1 send %s" % (packet % "172.16.0.0/17"))
    coord("peer1 send %s" % (packet % "172.16.0.0/18"))
    coord("peer1 send %s" % (packet % "172.16.0.0/19"))

    delay(2)

    coord("peer1 assert idle")

    coord("peer1 assert queue 0")

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

    if not config.conf_add_static_route4(builddir(1), "172.16.0.0/16"):
        return False

    delay(10)

    coord("peer1 assert established")
    coord("peer2 assert established")
    coord("peer3 assert established")

    coord("peer1 assert queue 0")

    return True

def test1_12_AB():
    """
    Check the rewriting of the NEXT_HOP attribute.
    """

    coord("reset")

    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("target 127.0.0.1 10002")
    coord("initialise attach peer2")

    coord("peer1 establish AS 1 holdtime 0 id 10.0.0.1 keepalive false")
    coord("peer2 establish AS 2 holdtime 0 id 10.0.0.2 keepalive false")
    
    delay(2)

    coord("peer1 assert established");
    coord("peer2 assert established");

    delay(2)

    packet = "packet update \
    nexthop 127.0.0.2 \
    origin 0 \
    nlri 172.16.0.0/16 %s"
    
    coord("peer1 expect %s" % (packet % "aspath 2 med 0"))
    coord("peer2 expect %s" % (packet % "aspath empty localpref 100"))

    if not config.conf_add_static_route4(builddir(1), "172.16.0.0/16", "127.0.0.2"):
        return False

    delay(10)

    coord("peer1 assert established")
    coord("peer2 assert established")

    coord("peer1 assert queue 0")
    coord("peer2 assert queue 0")

    return True

def test1_13_A():
    """
    MULTI_EXIT_DISC Attribute
    """

    coord("reset")

    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("target 127.0.0.1 10002")
    coord("initialise attach peer2")

    coord("peer1 establish AS 1 holdtime 0 id 10.0.0.1 keepalive false")
    coord("peer2 establish AS 2 holdtime 0 id 10.0.0.2 keepalive false")

    packet1 = "packet update \
    nexthop 127.0.0.1 \
    origin 0 \
    aspath 2 \
    med 42 \
    nlri 172.16.0.0/16"

    packet2 = "packet update \
    nexthop 127.0.0.1 \
    origin 0 \
    aspath empty \
    localpref 100 \
    nlri 172.16.0.0/16"

    coord("peer1 expect %s" % packet1)
    coord("peer2 expect %s" % packet2)

    if not config.conf_add_static_route4(builddir(1), "172.16.0.0/16"):
        return False

    delay(10)

    coord("peer1 assert established")
    coord("peer2 assert established")

    coord("peer1 assert queue 0")
    coord("peer2 assert queue 0")

    return True

def test1_13_B():
    """
    MULTI_EXIT_DISC Attribute
    """

    coord("reset")

    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("target 127.0.0.1 10002")
    coord("initialise attach peer2")

    coord("peer1 establish AS 1 holdtime 0 id 10.0.0.1 keepalive false")
    coord("peer2 establish AS 2 holdtime 0 id 10.0.0.2 keepalive false")

    packet = "packet update \
    nexthop 127.0.0.2 \
    origin 0 \
    aspath 1 \
    %s \
    nlri 172.16.0.0/16"

    spacket = packet % "med 42"

    epacket = packet % "localpref 100"

    coord("peer1 expect %s" % epacket)
    coord("peer2 expect %s" % epacket)

    coord("peer1 send %s" % spacket)

    delay(10)

    coord("peer1 assert established")
    coord("peer2 assert established")

    coord("peer1 assert queue 1")
    coord("peer2 assert queue 0")

    return True

def test1_13_C():
    """
    MULTI_EXIT_DISC Attribute
    """

    coord("reset")

    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("target 127.0.0.1 10002")
    coord("initialise attach peer2")

    coord("peer1 establish AS 1 holdtime 0 id 10.0.0.1 keepalive false")
    coord("peer2 establish AS 2 holdtime 0 id 10.0.0.2 keepalive false")

    packet = "packet update \
    origin 0 \
    %s \
    nlri 172.16.0.0/16"

    spacket = packet % "nexthop 127.0.0.2 aspath empty med 42"
    # The nexthop is not re-written as it is on a common subnet.
    epacket = packet % "nexthop 127.0.0.2 aspath 2 med 0"

    coord("peer1 expect %s" % epacket)
    coord("peer2 expect %s" % epacket)

    coord("peer2 send %s" % spacket)

    delay(10)

    coord("peer1 assert established")
    coord("peer2 assert established")

    coord("peer1 assert queue 0")
    coord("peer2 assert queue 1")

    return True

def test1_15_A():
    """
    ATOMIC_AGGREGATE Attribute
    """

    coord("reset")

    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("target 127.0.0.1 10002")
    coord("initialise attach peer2")

    coord("target 127.0.0.1 10003")
    coord("initialise attach peer3")

    delay(2)

    coord("peer1 establish AS 1 holdtime 0 id 10.0.0.1 keepalive false")
    coord("peer2 establish AS 2 holdtime 0 id 10.0.0.2 keepalive false")
    coord("peer3 establish AS 4 holdtime 0 id 10.0.0.3 keepalive false")
    
    packet1 =  "packet update \
    nexthop 127.0.0.2 \
    origin 0 \
    aspath %s \
    med 0 \
    nlri 192.0.0.0/8"

    packet2 =  "packet update \
    nexthop 127.0.0.2 \
    origin 0 \
    aspath %s \
    med 0 \
    nlri 192.1.0.0/16"

    spacket1 = packet1 % "1"
    spacket2 = packet2 % "2"

    epacket1 = packet1 % "3,1"
    epacket2 = packet2 % "3,2"

    coord("peer1 expect %s" % epacket2)
    coord("peer2 expect %s" % epacket1)
    coord("peer3 expect %s" % epacket1)
    coord("peer3 expect %s" % epacket2)

    coord("peer1 send %s" % spacket1)
    coord("peer2 send %s" % spacket2)

    delay(10)

    coord("peer1 assert queue 0")
    coord("peer2 assert queue 0")
    coord("peer3 assert queue 0")

    coord("peer1 assert established")
    coord("peer2 assert established")
    coord("peer3 assert established")
    
    return True

def test1_15_B():
    """
    ATOMIC_AGGREGATE Attribute
    """

    coord("reset")

    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("target 127.0.0.1 10002")
    coord("initialise attach peer2")

    coord("target 127.0.0.1 10003")
    coord("initialise attach peer3")

    delay(2)

    coord("peer1 establish AS 1 holdtime 0 id 10.0.0.1 keepalive false")
    coord("peer2 establish AS 2 holdtime 0 id 10.0.0.2 keepalive false")
    coord("peer3 establish AS 4 holdtime 0 id 10.0.0.3 keepalive false")
    
    packet1 =  "packet update \
    nexthop 127.0.0.2 \
    origin 0 \
    aspath %s \
    med 0 \
    nlri 192.0.0.0/8"

    packet2 =  "packet update \
    nexthop 127.0.0.2 \
    origin 0 \
    aspath %s \
    med 0 \
    nlri 192.1.0.0/16"

    spacket1 = packet1 % "1"
    spacket2 = packet2 % "2"

    #
    # XXX
    # This test is not complete the expect packets should check for:
    # Atomic Aggregate Attribute and Aggregator Attribute AS/3 10.0.0.1
    #

#    epacket1 = packet1 % "3,1"
#    epacket2 = packet2 % "3,2"

#    coord("peer1 expect %s" % epacket2)
#    coord("peer2 expect %s" % epacket1)
#    coord("peer3 expect %s" % epacket1)
#    coord("peer3 expect %s" % epacket2)

    coord("peer1 send %s" % spacket1)
    coord("peer2 send %s" % spacket2)

    delay(10)

    coord("peer1 trie recv lookup 192.1.0.0/8 aspath 3")
    coord("peer1 trie recv lookup 192.1.0.0/16 not")
    coord("peer2 trie recv lookup 192.1.0.0/8 aspath 3")
    coord("peer2 trie recv lookup 192.1.0.0/16 not")
    coord("peer3 trie recv lookup 192.1.0.0/8 aspath 3")
    coord("peer3 trie recv lookup 192.1.0.0/16 not")

#    coord("peer1 assert queue 0")
#    coord("peer2 assert queue 0")
#    coord("peer3 assert queue 0")

    coord("peer1 assert established")
    coord("peer2 assert established")
    coord("peer3 assert established")
    
    return True

def test3_8_A():
    """
    To verify that correct handling of NLRI field errors in UPDATE messages
    """

    print "Manual checks"
    print "1) Verify that the multicast NLRI generated an error message"
    print "2) Verify the route has not been installed"

    # configure the test peering.
    coord("reset")
    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("peer1 establish AS 65001 holdtime 0 id 1.2.3.4 keepalive false")
    
    delay(2)

    coord("peer1 assert established");

    bad_packet = "packet update \
    origin 0 \
    aspath 65001 \
    nexthop 127.0.0.1 \
    nlri 172.16.0.0/16 \
    nlri 224.0.0.5/16"

    coord("peer1 send %s" % bad_packet)

    delay(10)

    coord("peer1 assert established")

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

    if not config.conf_add_static_route4(builddir(1), "172.16.0.0/16"):
        return False

    delay(10)

    coord("peer1 assert established")
    coord("peer2 assert established")

    coord("peer1 assert queue 0")
    coord("peer2 assert queue 0")

    return True

def test4_8_A():
    """
    Verify correct operation of the multiprotocol extension
    """

    coord("reset")

    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("peer1 establish AS 1 holdtime 0 id 10.0.0.1 keepalive false")
    
    delay(2)

    coord("peer1 assert established");

    delay(2)

#    packet = "packet update \
#    nexthop 127.0.0.1 \
#    origin 0 \
#    aspath 65000 \
#    med 0 \
#    nlri 172.16.0.0/16 \
#    community NO_EXPORT"

#    coord("peer1 expect %s" % packet)

    if not config.conf_add_static_route4(builddir(1), "16.0.0.0/4"):
        return False

    delay(10)

    coord("peer1 assert established")

    coord("peer1 assert queue 0")

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
    # The nexthop is not re-written as it is on a common subnet.
    epacket = packet % ("127.0.0.2", "2,1")
    
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
    # The nexthop is not re-written as it is on a common subnet.
    epacket2 = packet % ("127.0.0.2", "2,1")
    epacket3 = packet % ("127.0.0.3", "2,1")
    
    coord("peer1 expect %s" % epacket3)
    coord("peer2 expect %s" % epacket3)
    coord("peer3 expect %s" % epacket3)

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

    coord("peer3 expect %s" % epacket2)
    coord("peer3 expect %s" % epacket3)

    coord("peer3 expect %s" % epacket2)
    coord("peer3 expect %s" % epacket3)

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
    coord("peer3 expect %s" % epacket2)
    
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

    # The nexthop is not re-written as it is on a common subnet.
    epacket1 = packet % ("127.0.0.3", "2,1,12")
    epacket2 = packet % ("127.0.0.3", "2,1,14")

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

test_main(TESTS, 'test_bgp_config', 'test_unh1')

# Local Variables:
# mode: python
# py-indent-offset: 4
# End:
