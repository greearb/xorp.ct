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

# $XORP: xorp/tests/bgp/test_bgp_policy1.py,v 1.4 2008/07/23 05:11:50 pavlin Exp $

#
# Test bgp interactions with policy
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

#    ['test_import_med1', 'test_policy_med1', False, '',
#     ['conf_RUT_as2_TR1_as1_TR2_as2_TR3_as3', 'conf_interfaces',
#      'conf_import_med_change']],

    ['test_export_med1', 'test_policy_med1', True, '',
     ['conf_RUT_as2_TR1_as1_TR2_as1_TR3_as3', 'conf_interfaces',
      'conf_export_med_change']],

    ['test_import_origin1', 'test_policy_origin1', True, '',
     ['conf_RUT_as2_TR1_as1_TR2_as1_TR3_as3', 'conf_interfaces',
      'conf_import_origin_change']],

    ['test_export_origin1', 'test_policy_origin1', True, '',
     ['conf_RUT_as2_TR1_as1_TR2_as1_TR3_as3', 'conf_interfaces',
      'conf_export_origin_change']],

    ]

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
    # The nexthop is not re-written as it is on a common subnet.
    epacket = packet % ("127.0.0.2", "2,1", "42")
    
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
    # The nexthop is not re-written as it is on a common subnet.
    epacket = packet % ("127.0.0.2", "2", "2,1")
    
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

test_main(TESTS, 'test_bgp_config', 'test_bgp_policy1')

# Local Variables:
# mode: python
# py-indent-offset: 4
# End:
