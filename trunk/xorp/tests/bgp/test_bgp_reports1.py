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

# $XORP: xorp/devnotes/template.py,v 1.2 2006/03/24 18:10:47 atanu Exp $

# Tests used to investigate bug reports.

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

    ['test_bug_360', 'test_bug_360', True, '',
     ['conf_bug_360', 'conf_interfaces',
      'conf_redist_static']],
    ]

def test_bug_360():
    """
    http://www.xorp.org/bugzilla/show_bug.cgi?id=360
    The bug report stated that the output from the bgp show route
    command can vary. This test populates the routing table with
    similar values and repeats the show command a number of times.
    NOTE: No problem was ever found.
    """

    coord("reset")

    coord("target 127.0.0.1 10001")
    coord("initialise attach peer1")

    coord("peer1 establish AS 75 holdtime 0 id 75.75.75.75 keepalive false")
    
    delay(2)

    coord("peer1 assert established");

    delay(2)

    packet = "packet update \
    nexthop 127.0.0.2 \
    origin %s \
    aspath %s \
    nlri %s"

    incomplete = "2"
    
    coord("peer1 send %s" % (packet % (incomplete,
                                       "75,50,25", "25.25.25.0/24")))
    coord("peer1 send %s" % (packet % (incomplete,
                                       "75,50,25", "25.25.0.0/16")))
    coord("peer1 send %s" % (packet % (incomplete,
                                       "75,50,25", "25.0.0.0/8")))
    coord("peer1 send %s" % (packet % (incomplete, "75", "75.75.75.0/24")))
    coord("peer1 send %s" % (packet % (incomplete, "75", "75.75.0.0/16")))
    coord("peer1 send %s" % (packet % (incomplete, "75", "75.70.0.0/8")))

    if not config.conf_add_static_route4(builddir(1), "100.100.100.0/24"):
        return False

    if not config.conf_add_static_route4(builddir(1), "100.100.0.0/16"):
        return False

    if not config.conf_add_static_route4(builddir(1), "100.0.0.0/8"):
        return False

    reference_output = \
"""*> 100.100.100.0/24      127.0.0.1                  0.0.0.0        i
*> 100.100.0.0/16        127.0.0.1                  0.0.0.0        i
*> 100.0.0.0/8           127.0.0.1                  0.0.0.0        i
*> 25.25.25.0/24         127.0.0.2                  75.75.75.75   75 50 25 ?
*> 25.25.0.0/16          127.0.0.2                  75.75.75.75   75 50 25 ?
*> 25.0.0.0/8            127.0.0.2                  75.75.75.75   75 50 25 ?
*> 75.75.75.0/24         127.0.0.2                  75.75.75.75   75 ?
*> 75.75.0.0/16          127.0.0.2                  75.75.75.75   75 ?
*> 75.0.0.0/8            127.0.0.2                  75.75.75.75   75 ?
"""

    for l in range(100):

        result, output = config.show_bgp_routes(builddir(1))

        import string

        # Looks like the xorpsh inserts carriage returns.
        output = output.replace('\r', '')
        # XXX
        # This might be a problem on windows, in later installations of python
        # we can use splitlines.
        lines = string.split(output, '\n')
        output = ""
        for i in lines:
            if i and i[0] == '*':
                output += i + '\n'
        
        if reference_output != output:
            reference = '/tmp/reference'
            actual = '/tmp/actual'
            print 'Output did now match check %s and %s' % (reference, actual)

            file = open(reference, 'w')
            file.write(reference_output)
            file.close()
    
            file = open(actual, 'w')
            file.write(output)
            file.close()
            
            print '<@' + reference_output + '@>'
            print "==="
            print '<@' + output + '@>'
            return False
    
    return True

test_main(TESTS, 'test_bgp_config', 'test_bgp_reports1')

# Local Variables:
# mode: python
# py-indent-offset: 4
# End:
