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

# $XORP: xorp/ospf/test_routing1.py,v 1.10 2006/12/15 09:12:09 atanu Exp $

import getopt
import sys
import os

TESTS=[
    # Fields:
    # 0: Test function
    # 1: True if this test works
    ['print_lsasV2', True],
    ['test1', True],
    ['test2', True],
    ['r1V2', True],
    ]

def print_lsasV2(verbose):
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

def test1(verbose):
    """
    Create an area and then destroy it.
    """

    if verbose:
        fp = os.popen(os.path.abspath('test_routing_interactive') + ' -v', "w")
    else:
        fp = os.popen(os.path.abspath('test_routing_interactive'), "w")

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

def test2(verbose):
    """
    Introduce a RouterLsa and a NetworkLsa
    """

    if verbose:
        fp = os.popen(os.path.abspath('test_routing_interactive') + ' -v', "w")
    else:
        fp = os.popen(os.path.abspath('test_routing_interactive'), "w")

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

def r1V2(verbose):
    """
    Some of the routers from Figure 2. in RFC 2328. Single area.
    This router is R6.
    """

    if verbose:
        fp = os.popen(os.path.abspath('test_routing_interactive') + ' -v', "w")
    else:
        fp = os.popen(os.path.abspath('test_routing_interactive'), "w")

    RT6 = "RouterLsa E-bit lsid 0.0.0.6 adv 0.0.0.6 \
    p2p lsid 0.0.0.3 ldata 0.0.0.4 metric 6 \
    p2p lsid 0.0.0.5 ldata 0.0.0.6 metric 6 \
    p2p lsid 0.0.0.10 ldata 0.0.0.11 metric 7 \
    "

    RT3 = "RouterLsa E-bit lsid 0.0.0.3 adv 0.0.0.3 \
    p2p lsid 0.0.0.6 ldata 0.0.0.7 metric 8 \
    stub lsid 0.4.0.0 ldata 255.255.0.0 metric 2 \
    "

    command = """
set_router_id 0.0.0.6
create 0.0.0.0 normal
select 0.0.0.0
replace %s
add %s
compute 0.0.0.0
verify_routing_table_size 1 verify_routing_entry 0.4.0.0/16 0.0.0.7 8 false false
""" % (RT6,RT3)

    print >>fp, command

    if not fp.close():
        return True
    else:
        return False

def main():
    def usage():
        us = \
           "usage: %s [-h|--help] [-v|--verbose] ][-t|--test] [-b|--bad]"
        print us % sys.argv[0]
        

    try:
        opts, args = getopt.getopt(sys.argv[1:], "hvt:b", \
                                   ["help", \
                                    "verbose", \
                                    "test=", \
                                    "bad", \
                                    ])
    except getopt.GetoptError:
        usage()
        sys.exit(1)


    bad = False
    tests = []
    verbose = False
    for o, a in opts:
	if o in ("-h", "--help"):
	    usage()
	    sys.exit()
	if o in ("-v", "--verbose"):
            verbose = True
        if o in ("-t", "--test"):
            tests.append(a)
        if o in ("-b", "--bad"):
            bad = True

    if not tests:
        for i in TESTS:
            if bad != i[1]:
                tests.append(i[0])

    print tests

    for i in tests:
        test = i + '(verbose)'
        print 'Running: ' + i,
        if not eval(test):
            print "FAILED"
            sys.exit(-1)
        else:
            print
        
    sys.exit(0)

main()

# Local Variables:
# mode: python
# py-indent-offset: 4
# End:
