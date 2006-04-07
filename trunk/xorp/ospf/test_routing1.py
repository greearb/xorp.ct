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

# $XORP: xorp/ospf/test_routing1.py,v 1.1 2006/03/28 01:50:07 atanu Exp $

import sys
import os

TESTS_NOT_FIXED=[]
TESTS=['test1']
PATH_CHAR='/'

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
        if 0 != os.system('%s%stest_build_lsa_main -l "%s"' % (os.getcwd(), PATH_CHAR, i)):
            return -1

    return 0

def main():

    global TESTS, TESTS_NOT_FIXED, PATH_CHAR

    if sys.platform == 'win32':
        PATH_CHAR='\\'

    for i in TESTS:
        test = i + '()'
        if 0 != eval(test):
            print "TEST " + i + " FAILED"
            sys.exit(-1)

    sys.exit(0)

main()

# Local Variables:
# mode: python
# py-indent-offset: 4
# End:
