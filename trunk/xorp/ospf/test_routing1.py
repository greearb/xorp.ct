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

# $XORP: xorp/ospf/test_routing1.py,v 1.5 2006/12/14 21:56:44 atanu Exp $

import sys
import os

TESTS=[
    # Fields:
    # 0: Test function
    # 1: True if this test works
    ['test1', True],
    ['test2', True],
    ['test3', True],
    ]

def test1():
    """
    Run the build lsa program with all the known LSAs and
    settings. Verifies that the program has been built and gives
    examples of how to define LSAs.
    """

    options='DC-bit EA-bit N/P-bit MC-bit E-bit'
    common_header='age 1800 %s lsid 1.2.3.4 adv 5.6.7.8 seqno 1 cksum 1' % \
                   options

    RouterLsa='RouterLsa %s bit-NT bit-V bit-E bit-B' % common_header
    RouterLsa=RouterLsa + ' p2p lsid 10.10.10.10 ldata 11.11.11.11 metric 42'

    NetworkLsa='NetworkLsa %s netmask 0xffffff00' % common_header

    SummaryNetworkLsa='SummaryNetworkLsa %s \
    netmask 0xffffff00 \
    metric 42' % common_header

    SummaryRouterLsa='SummaryRouterLsa %s \
    netmask 0xffffff00 \
    metric 42' % common_header

    ASExternalLsa='ASExternalLsa %s \
    netmask 0xffff0000 \
    bit-E metric 45 \
    forward4 9.10.11.12 \
    tag 0x40' %  common_header

    Type7Lsa='Type7Lsa %s \
    netmask 0xffff0000 \
    bit-E metric 45 \
    forward4 9.10.11.12 \
    tag 0x40' %  common_header

    lsas = [RouterLsa, NetworkLsa, SummaryNetworkLsa, SummaryRouterLsa, \
            ASExternalLsa, Type7Lsa]

    for i in lsas:
        if 0 != os.system('%s --OSPFv2 -l "%s"' % \
                          (os.path.abspath('test_build_lsa_main'), i)):
            return False

    return True

def test2():
    """
    Create an area and then destroy it.
    """

    fp = os.popen(os.path.abspath('test_routing_interactive') + ' -v', "w")

    command = """
create 0.0.0.0 normal
destroy 0.0.0.0
create 0.0.0.0 normal
destroy 0.0.0.0
    """

    print >>fp, command

    if not fp.close():
        return True
    else:
        return False

def test3():
    """
    Introduce a RouterLsa and a NetworkLsa
    """

    fp = os.popen(os.path.abspath('test_routing_interactive') + ' -v', "w")

    command = """
create 0.0.0.0 normal
select 0.0.0.0
replace RouterLsa
add NetworkLsa
compute 0.0.0.0
    """

    print >>fp, command

    if not fp.close():
        return True
    else:
        return False

def main():

    for i in TESTS:
        if i[1]:
            test = i[0] + '()'
            print test,
            if not eval(test):
                print "FAILED"
                sys.exit(-1)
            else:
                print "SUCEEDED"

    sys.exit(0)

main()

# Local Variables:
# mode: python
# py-indent-offset: 4
# End:
