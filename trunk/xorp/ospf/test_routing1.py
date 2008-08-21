#!/usr/bin/env python

# Copyright (c) 2001-2008 XORP, Inc.
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

# $XORP: xorp/ospf/test_routing1.py,v 1.30 2008/08/20 01:34:03 atanu Exp $

import getopt
import sys
import os

TESTS=[
    # Fields:
    # 0: Test function
    # 1: True if this test works
    # 2: Protocol under test (optional), allowed values 'v2' or 'v3'
    ['print_lsasV2', True],
    ['print_lsasV3', True],
    ['test1', True, 'v2'],
    ['test2', True, 'v2'],
    ['r1V2', True, 'v2'],
    ['r1V3', True, 'v3'],
    ['r2V3', True, 'v3'],
    ['r3V3', True, 'v3'],
    ['r4V3', True, 'v3'],
    ['r5V3', True, 'v3'],
    ['r6V3', True, 'v3'],
    ['r7V3', True, 'v3'],
    ['r8V2', True, 'v2'],
    ['r9V2', True, 'v2'],
    ['r10V2', True, 'v2'],
    ['r11V3', True, 'v3'],
    ['r12V3', True, 'v3'],
    ['r13V3', True, 'v3'],
    ['r14V3', True, 'v3'],
    ]

def start_routing_interactive(verbose, protocol):
    """
    Helper function to start the test_routing_interactive program
    """

    if protocol == 'v2':
        flags = ' --OSPFv2'
    elif protocol == 'v3':
        flags = ' --OSPFv3'
    else:
        print "unknown protocol " + protocol
        sys.exit(-1)

    if verbose:
        flags = flags + ' -v'

    fp = os.popen(os.path.abspath('test_routing_interactive') + flags, "w")

    return fp
    
def print_lsasV2(verbose, protocol):
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
    NetworkLsa=NetworkLsa + ' add-router 1.2.3.4'

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

def print_lsasV3(verbose, protocol):
    """
    Run the build lsa program with all the known LSAs and
    settings. Verifies that the program has been built and gives
    examples of how to define LSAs.
    """

    common_header='age 1800 lsid 1.2.3.4 adv 5.6.7.8 seqno 1 cksum 1'
    options='DC-bit R-bit N-bit MC-bit E-bit V6-bit'

    RouterLsa='RouterLsa %s bit-NT bit-W bit-B bit-E bit-V' % common_header
    RouterLsa=RouterLsa + ' ' + options
    RouterLsa=RouterLsa + ' p2p iid 1 nid 2 nrid 0.0.0.3 metric 42'

    NetworkLsa='NetworkLsa %s' % common_header
    NetworkLsa=NetworkLsa + ' ' + options
    NetworkLsa=NetworkLsa + ' add-router 1.2.3.4'

    SummaryNetworkLsa='SummaryNetworkLsa %s metric 42 ' % common_header
    IPv6Prefix='IPv6Prefix 5f00:0000:c001::/48 DN-bit \
    P-bit MC-bit LA-bit NU-bit'
    SummaryNetworkLsa=SummaryNetworkLsa + ' ' + IPv6Prefix

    SummaryRouterLsa='SummaryRouterLsa %s' % common_header
    SummaryRouterLsa=SummaryRouterLsa + ' ' + options
    SummaryRouterLsa=SummaryRouterLsa + ' metric 42 drid 1.2.3.4'

    ASExternalLsa='ASExternalLsa %s' % common_header
    ASExternalLsa=ASExternalLsa + ' bit-E bit-F bit-T metric 45'
    ASExternalLsa=ASExternalLsa + ' ' + IPv6Prefix
    ASExternalLsa=ASExternalLsa + ' ' + 'rlstype 2'
    ASExternalLsa=ASExternalLsa + ' ' + 'forward6 5f00:0000:c001::00'
    ASExternalLsa=ASExternalLsa + ' ' + 'tag 0x40'
    ASExternalLsa=ASExternalLsa + ' ' + 'rlsid 1.2.3.4'

    Type7Lsa='Type7Lsa %s' % common_header
    Type7Lsa=Type7Lsa + ' bit-E bit-F bit-T metric 45'
    Type7Lsa=Type7Lsa + ' ' + IPv6Prefix
    Type7Lsa=Type7Lsa + ' ' + 'rlstype 2'
    Type7Lsa=Type7Lsa + ' ' + 'forward6 5f00:0000:c001::00'
    Type7Lsa=Type7Lsa + ' ' + 'tag 0x40'
    Type7Lsa=Type7Lsa + ' ' + 'rlsid 1.2.3.4'

    LinkLsa='LinkLsa %s' % common_header
    LinkLsa=LinkLsa + ' rtr-priority 42'
    LinkLsa=LinkLsa + ' ' + options
    LinkLsa=LinkLsa + ' link-local-address fe80:0001::'
    LinkLsa=LinkLsa + ' ' + IPv6Prefix
    LinkLsa=LinkLsa + ' ' + IPv6Prefix
    
    IntraAreaPrefixLsa='IntraAreaPrefixLsa %s' % common_header
    IntraAreaPrefixLsa=IntraAreaPrefixLsa + ' rlstype 0x2001'
    IntraAreaPrefixLsa=IntraAreaPrefixLsa + ' rlsid 1.2.3.4'
    IntraAreaPrefixLsa=IntraAreaPrefixLsa + ' radv 9.8.7.6'
    IntraAreaPrefixLsa=IntraAreaPrefixLsa + ' ' + IPv6Prefix + ' metric 1'
    IntraAreaPrefixLsa=IntraAreaPrefixLsa + ' ' + IPv6Prefix + ' metric 2'

    IntraAreaPrefixLsa_r='IntraAreaPrefixLsa %s' % common_header
    IntraAreaPrefixLsa_r=IntraAreaPrefixLsa_r + ' rlstype RouterLsa'
    IntraAreaPrefixLsa_r=IntraAreaPrefixLsa_r + ' rlsid 1.2.3.4'
    IntraAreaPrefixLsa_r=IntraAreaPrefixLsa_r + ' radv 9.8.7.6'
    IntraAreaPrefixLsa_r=IntraAreaPrefixLsa_r + ' ' + IPv6Prefix + ' metric 1'
    IntraAreaPrefixLsa_r=IntraAreaPrefixLsa_r + ' ' + IPv6Prefix + ' metric 2'

    IntraAreaPrefixLsa_n='IntraAreaPrefixLsa %s' % common_header
    IntraAreaPrefixLsa_n=IntraAreaPrefixLsa_n + ' rlstype NetworkLsa'
    IntraAreaPrefixLsa_n=IntraAreaPrefixLsa_n + ' rlsid 1.2.3.4'
    IntraAreaPrefixLsa_n=IntraAreaPrefixLsa_n + ' radv 9.8.7.6'
    IntraAreaPrefixLsa_n=IntraAreaPrefixLsa_n + ' ' + IPv6Prefix + ' metric 1'
    IntraAreaPrefixLsa_n=IntraAreaPrefixLsa_n + ' ' + IPv6Prefix + ' metric 2'

    IntraAreaPrefixLsa_u='IntraAreaPrefixLsa %s' % common_header
    IntraAreaPrefixLsa_u=IntraAreaPrefixLsa_u + ' rlstype 42'
    IntraAreaPrefixLsa_u=IntraAreaPrefixLsa_u + ' rlsid 1.2.3.4'
    IntraAreaPrefixLsa_u=IntraAreaPrefixLsa_u + ' radv 9.8.7.6'
    IntraAreaPrefixLsa_u=IntraAreaPrefixLsa_u + ' ' + IPv6Prefix + ' metric 1'
    IntraAreaPrefixLsa_u=IntraAreaPrefixLsa_u + ' ' + IPv6Prefix + ' metric 2'

    lsas = [RouterLsa, NetworkLsa, SummaryNetworkLsa, SummaryRouterLsa, \
            ASExternalLsa, Type7Lsa, LinkLsa, \
            IntraAreaPrefixLsa, IntraAreaPrefixLsa_r, IntraAreaPrefixLsa_n, \
            IntraAreaPrefixLsa_u]

    for i in lsas:
        if 0 != os.system('%s --OSPFv3 -l "%s"' % \
                          (os.path.abspath('test_build_lsa_main'), i)):
            return False

    return True

def test1(verbose, protocol):
    """
    Create an area and then destroy it.
    """

    fp = start_routing_interactive(verbose, protocol)

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

def test2(verbose, protocol):
    """
    Introduce a RouterLsa and a NetworkLsa
    """

    fp = start_routing_interactive(verbose, protocol)

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

def r1V2(verbose, protocol):
    """
    Some of the routers from Figure 2. in RFC 2328. Single area.
    This router is R6.
    """

    fp = start_routing_interactive(verbose, protocol)

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
verify_routing_table_size 1
verify_routing_entry 0.4.0.0/16 0.0.0.7 8 false false
""" % (RT6,RT3)

    print >>fp, command

    if not fp.close():
        return True
    else:
        return False

def r1V3(verbose, protocol):
    """
    This test is the OSPFv3 version of the r1V2 although the OSPF for
    IPV6 RFC does not have a point to point link in Figure 1.
    """

    fp = start_routing_interactive(verbose, protocol)

    RT6 = "RouterLsa E-bit lsid 0.0.0.1 adv 0.0.0.6 \
    p2p iid 1 nid 1 nrid 0.0.0.3 metric 6 \
    "

    RT3 = "RouterLsa R-bit V6-bit E-bit lsid 0.0.0.1 adv 0.0.0.3 \
    p2p iid 1 nid 1 nrid 0.0.0.6 metric 8 \
    "

    RT3_LINK = "LinkLsa lsid 0.0.0.1 adv 0.0.0.3 \
    link-local-address fe80:0001::3"

    RT3_INTRA = "IntraAreaPrefixLsa lsid 0.0.0.1 adv 0.0.0.3 \
    rlstype 0x2001 rlsid 0.0.0.0 radv 0.0.0.3 \
    IPv6Prefix 5f00:0000:c001:0200::/56 metric 3 \
    "
    command = """
set_router_id 0.0.0.6
create 0.0.0.0 normal
select 0.0.0.0
replace %s
add %s
add %s
add %s
compute 0.0.0.0
verify_routing_table_size 1
verify_routing_entry 5f00:0000:c001:0200::/56 fe80:0001::3 9 false false
""" % (RT6,RT3,RT3_LINK,RT3_INTRA)

    print >>fp, command

    if not fp.close():
        return True
    else:
        return False

def r2V3(verbose, protocol):
    """
    This test is based on Figure 1 in RFC2740.
    This router it RT1, RT1 is also the designated router for N3.
    """

    fp = start_routing_interactive(verbose, protocol)

    RT1 = "RouterLsa V6-bit E-bit R-bit lsid 1.0.0.1 adv 0.0.0.1 \
    transit iid 1 nid 1 nrid 0.0.0.1 metric 1 \
    "

    RT1_LINK = "LinkLsa lsid 0.0.0.1 adv 0.0.0.1 \
    link-local-address fe80:0002::1"

    # N1 - Not used as RT1 is this router.
    RT1_INTRA_R = "IntraAreaPrefixLsa lsid 0.0.0.1 adv 0.0.0.1 \
    rlstype RouterLsa rlsid 0.0.0.0 radv 0.0.0.1 \
    IPv6Prefix 5f00:0000:c001:0200::/56 metric 1 \
    "

    # N3 - Not used as RT1 is this router.
    RT1_INTRA_N = "IntraAreaPrefixLsa lsid 0.0.0.2 adv 0.0.0.1 \
    rlstype NetworkLsa rlsid 0.0.0.1 radv 0.0.0.1 \
    IPv6Prefix 5f00:0000:c001:0100::/56 metric 1 \
    "

    RT1_NETWORK = "NetworkLsa lsid 0.0.0.1 adv 0.0.0.1 \
    add-router 0.0.0.1 \
    add-router 0.0.0.2 \
    add-router 0.0.0.3 \
    add-router 0.0.0.4 \
    "

    RT2 = "RouterLsa V6-bit E-bit R-bit lsid 0.0.0.1 adv 0.0.0.2 \
    transit iid 2 nid 1 nrid 0.0.0.1 metric 1 \
    "

    RT2_LINK = "LinkLsa lsid 0.0.0.2 adv 0.0.0.2 \
    link-local-address fe80:0002::2"

    # N2
    RT2_INTRA = "IntraAreaPrefixLsa lsid 0.0.0.1 adv 0.0.0.2 \
    rlstype RouterLsa rlsid 0.0.0.0 radv 0.0.0.2 \
    IPv6Prefix 5f00:0000:c001:0300::/56 metric 3 \
    "

    RT3 = "RouterLsa V6-bit E-bit R-bit lsid 0.0.0.1 adv 0.0.0.3 \
    transit iid 1 nid 1 nrid 0.0.0.1 metric 1 \
    "

    RT3_LINK = "LinkLsa lsid 0.0.0.1 adv 0.0.0.3 \
    link-local-address fe80:0001::3"

    # N4
    RT3_INTRA = "IntraAreaPrefixLsa lsid 0.0.0.1 adv 0.0.0.3 \
    rlstype RouterLsa rlsid 0.0.0.0 radv 0.0.0.3 \
    IPv6Prefix 5f00:0000:c001:0400::/56 metric 2 \
    "

    RT4 = "RouterLsa V6-bit E-bit R-bit lsid 0.0.0.1 adv 0.0.0.4 \
    transit iid 1 nid 1 nrid 0.0.0.1 metric 1 \
    "

    RT4_LINK = "LinkLsa lsid 0.0.0.1 adv 0.0.0.4 \
    link-local-address fe80:0001::4"

    command = """
set_router_id 0.0.0.1
create 0.0.0.0 normal
select 0.0.0.0
replace %s
add %s
add %s
add %s
add %s
add %s
add %s
add %s
add %s
add %s
add %s
add %s
add %s
compute 0.0.0.0
verify_routing_table_size 2
verify_routing_entry 5f00:0000:c001:0300::/56 fe80:0002::2 4 false false
verify_routing_entry 5f00:0000:c001:0400::/56 fe80:0001::3 3 false false
""" % (RT1,RT1_LINK,RT1_INTRA_R,RT1_INTRA_N,RT1_NETWORK,
       RT2,RT2_LINK,RT2_INTRA,
       RT3,RT3_LINK,RT3_INTRA,
       RT4,RT4_LINK)

    print >>fp, command

    if not fp.close():
        return True
    else:
        return False

def r3V3(verbose, protocol):
    """
    Verify the correct processing of Network-LSAs from non-directly connected
    interface.
    """

    # RT1 is this router, RT2 in connected to RT1 via p2p, RT2 generates a
    # Network-LSA for N1. The LSAs from the other routers N1 are not required.

    #                            +
    #     +---+1       1+---+5   |
    #     |RT1|---------|RT2|----|N1
    #     +---+         +---+    |
    #                            +

    # Network   IPv6 prefix
    # -----------------------------------
    # N1	5f00:0000:c001:0200::/56

    # Router	Interface   Interface ID   link-local address
    # -------------------------------------------------------
    # RT1	to RT2	    1		   fe80:0001::1
    # RT2	to RT1	    1		   fe80:0001::2
    #           to N3	    20		   fe80:0002::2

    fp = start_routing_interactive(verbose, protocol)

    RT1 = "RouterLsa V6-bit E-bit R-bit lsid 42.0.0.1 adv 0.0.0.1 \
    p2p iid 1 nid 1 nrid 0.0.0.2 metric 1 \
    "
    RT1_LINK = "LinkLsa lsid 0.0.0.1 adv 0.0.0.1 \
    link-local-address fe80:0001::1"

    RT2 = "RouterLsa V6-bit E-bit R-bit lsid 42.0.0.1 adv 0.0.0.2 \
    p2p iid 1 nid 1 nrid 0.0.0.1 metric 1 \
    transit iid 20 nid 20 nrid 0.0.0.2 metric 5 \
    "
    RT2_LINK = "LinkLsa lsid 0.0.0.1 adv 0.0.0.2 \
    link-local-address fe80:0001::2"

    RT2_INTRA = "IntraAreaPrefixLsa lsid 42.0.0.2 adv 0.0.0.2 \
    rlstype NetworkLsa rlsid 0.0.0.20 radv 0.0.0.2 \
    IPv6Prefix 5f00:0000:c001:0200::/56 metric 1 \
    "

    RT2_NETWORK = "NetworkLsa lsid 0.0.0.20 adv 0.0.0.2 \
    add-router 0.0.0.2 \
    "

    command = """
set_router_id 0.0.0.1
create 0.0.0.0 normal
select 0.0.0.0
replace %s
add %s
add %s
add %s
add %s
add %s
compute 0.0.0.0
verify_routing_table_size 1
verify_routing_entry 5f00:0000:c001:0200::/56 fe80:0001::2 7 false false
""" % (RT1,RT1_LINK,
       RT2,RT2_LINK,RT2_INTRA,RT2_NETWORK)

    print >>fp, command

    if not fp.close():
        return True
    else:
        return False

def r4V3(verbose, protocol):
    """
    This test is based on Figure 1 in RFC2740.
    This router it RT1, RT2 is the designated router for N3.
    """

    fp = start_routing_interactive(verbose, protocol)

    RT1 = "RouterLsa V6-bit E-bit R-bit lsid 1.0.0.1 adv 0.0.0.1 \
    transit iid 1 nid 2 nrid 0.0.0.2 metric 1 \
    "

    RT1_LINK = "LinkLsa lsid 0.0.0.1 adv 0.0.0.1 \
    link-local-address fe80:0002::1"

    # N1
    RT1_INTRA = "IntraAreaPrefixLsa lsid 0.0.0.1 adv 0.0.0.1 \
    rlstype RouterLsa rlsid 0.0.0.0 radv 0.0.0.1 \
    IPv6Prefix 5f00:0000:c001:0200::/56 metric 1 \
    "

    RT2 = "RouterLsa V6-bit E-bit R-bit lsid 0.0.0.1 adv 0.0.0.2 \
    transit iid 2 nid 2 nrid 0.0.0.2 metric 1 \
    "

    RT2_LINK = "LinkLsa lsid 0.0.0.2 adv 0.0.0.2 \
    link-local-address fe80:0002::2"

    # N2
    RT2_INTRA_R = "IntraAreaPrefixLsa lsid 0.0.0.1 adv 0.0.0.2 \
    rlstype RouterLsa rlsid 0.0.0.0 radv 0.0.0.2 \
    IPv6Prefix 5f00:0000:c001:0300::/56 metric 3 \
    "
    # N3
    RT2_INTRA_N = "IntraAreaPrefixLsa lsid 0.0.0.2 adv 0.0.0.2 \
    rlstype NetworkLsa rlsid 0.0.0.2 radv 0.0.0.2 \
    IPv6Prefix 5f00:0000:c001:0100::/56 metric 1 \
    "

    RT2_NETWORK = "NetworkLsa lsid 0.0.0.2 adv 0.0.0.2 \
    add-router 0.0.0.1 \
    add-router 0.0.0.2 \
    add-router 0.0.0.3 \
    add-router 0.0.0.4 \
    "

    RT3 = "RouterLsa V6-bit E-bit R-bit lsid 0.0.0.1 adv 0.0.0.3 \
    transit iid 1 nid 2 nrid 0.0.0.2 metric 1 \
    "

    RT3_LINK = "LinkLsa lsid 0.0.0.1 adv 0.0.0.3 \
    link-local-address fe80:0001::3"

    # N4
    RT3_INTRA = "IntraAreaPrefixLsa lsid 0.0.0.1 adv 0.0.0.3 \
    rlstype RouterLsa rlsid 0.0.0.0 radv 0.0.0.3 \
    IPv6Prefix 5f00:0000:c001:0400::/56 metric 2 \
    "

    RT4 = "RouterLsa V6-bit E-bit R-bit lsid 0.0.0.1 adv 0.0.0.4 \
    transit iid 1 nid 2 nrid 0.0.0.2 metric 1 \
    "

    RT4_LINK = "LinkLsa lsid 0.0.0.1 adv 0.0.0.4 \
    link-local-address fe80:0001::4"

    command = """
set_router_id 0.0.0.1
create 0.0.0.0 normal
select 0.0.0.0
replace %s
add %s
add %s
add %s
add %s
add %s
add %s
add %s
add %s
add %s
add %s
add %s
add %s
compute 0.0.0.0
verify_routing_table_size 2
verify_routing_entry 5f00:0000:c001:0300::/56 fe80:0002::2 4 false false
verify_routing_entry 5f00:0000:c001:0400::/56 fe80:0001::3 3 false false
""" % (RT1,RT1_LINK,RT1_INTRA,
       RT2,RT2_LINK,RT2_INTRA_R,RT2_INTRA_N,RT2_NETWORK,
       RT3,RT3_LINK,RT3_INTRA,
       RT4,RT4_LINK)

    print >>fp, command

    if not fp.close():
        return True
    else:
        return False

def r5V3(verbose, protocol):
    """
    Verify the correct processing of Inter-Area-Prefix-LSAs.
    """

    # RT1 is this router.
    # RT2 is an area border router that generates an inter-area-prefix-LSAs.
    # RT1 is connected to RT2 via p2p.

    #                            +
    #     +---+1       1+---+5   |
    #     |RT1|---------|RT2|----| Other Area
    #     +---+         +---+    |
    #                            +

    # Network           IPv6 prefix
    # -----------------------------------
    # Other Area	5f00:0000:c001:0200::/56

    # Router	Interface   Interface ID   link-local address
    # -------------------------------------------------------
    # RT1	to RT2	    1		   fe80:0001::1
    # RT2	to RT1	    1		   fe80:0001::2
    #           to N3	    20		   fe80:0002::2

    fp = start_routing_interactive(verbose, protocol)

    RT1 = "RouterLsa V6-bit E-bit R-bit lsid 42.0.0.1 adv 0.0.0.1 \
    p2p iid 1 nid 1 nrid 0.0.0.2 metric 1 \
    "
    RT1_LINK = "LinkLsa lsid 0.0.0.1 adv 0.0.0.1 \
    link-local-address fe80:0001::1"

    RT2 = "RouterLsa bit-B V6-bit E-bit R-bit lsid 42.0.0.1 adv 0.0.0.2 \
    p2p iid 1 nid 1 nrid 0.0.0.1 metric 1 \
    transit iid 20 nid 20 nrid 0.0.0.2 metric 5 \
    "
    RT2_LINK = "LinkLsa lsid 0.0.0.1 adv 0.0.0.2 \
    link-local-address fe80:0001::2"

    RT2_INTER = "SummaryNetworkLsa lsid 42.0.0.2 adv 0.0.0.2 \
    metric 6 \
    IPv6Prefix 5f00:0000:c001:0200::/56 \
    "

    command = """
set_router_id 0.0.0.1
create 0.0.0.0 normal
select 0.0.0.0
replace %s
add %s
add %s
add %s
add %s
compute 0.0.0.0
verify_routing_table_size 1
verify_routing_entry 5f00:0000:c001:0200::/56 fe80:0001::2 7 false false
""" % (RT1,RT1_LINK,
       RT2,RT2_LINK,RT2_INTER)

    print >>fp, command

    if not fp.close():
        return True
    else:
        return False

def r6V3(verbose, protocol):
    """
    Verify the correct processing of AS-External-LSAs.
    """

    # RT1 is this router.
    # RT2 is an AS boundary router that generates an AS-External-LSA
    # RT1 is connected to RT2 via p2p.

    #                            
    #     +---+1       1+---+5   
    #     |RT1|---------|RT2|
    #     +---+         +---+    
    #                            

    # Network           IPv6 prefix
    # -----------------------------------
    # External address	5f00:0000:c001:0200::/56

    # Router	Interface   Interface ID   link-local address
    # -------------------------------------------------------
    # RT1	to RT2	    1		   fe80:0001::1
    # RT2	to RT1	    1		   fe80:0001::2


    fp = start_routing_interactive(verbose, protocol)

    RT1 = "RouterLsa V6-bit E-bit R-bit lsid 42.0.0.1 adv 0.0.0.1 \
    p2p iid 1 nid 1 nrid 0.0.0.2 metric 1 \
    "

    RT1_LINK = "LinkLsa lsid 0.0.0.1 adv 0.0.0.1 \
    link-local-address fe80:0001::1"

    RT2 = "RouterLsa bit-E V6-bit E-bit R-bit lsid 42.0.0.1 adv 0.0.0.2 \
    p2p iid 1 nid 1 nrid 0.0.0.1 metric 1 \
    "
    RT2_LINK = "LinkLsa lsid 0.0.0.1 adv 0.0.0.2 \
    link-local-address fe80:0001::2"

    RT2_INTER = "ASExternalLsa lsid 42.0.0.2 adv 0.0.0.2 \
    metric 6 \
    IPv6Prefix 5f00:0000:c001:0200::/56 \
    "

    command = """
set_router_id 0.0.0.1
create 0.0.0.0 normal
select 0.0.0.0
replace %s
add %s
add %s
add %s
add %s
compute 0.0.0.0
verify_routing_table_size 1
verify_routing_entry 5f00:0000:c001:0200::/56 fe80:0001::2 7 false false
""" % (RT1,RT1_LINK,
       RT2,RT2_LINK,RT2_INTER)

    print >>fp, command

    if not fp.close():
        return True
    else:
        return False

def r7V3(verbose, protocol):
    """
    Verify virtual links are correctly processed when the router is
    directly adjacent to the other router.
    """

    # RT1 is this router.
    # RT2 is a virtual neighbour.
    # RT1 is directly connected to RT2 via a virtual link.

    #                            
    #     +---+1       1+---+5   
    #     |RT1|---------|RT2|
    #     +---+         +---+    
    #                            

    # Network           IPv6 prefix
    # -----------------------------------
    # External address	5f00:0000:c001:0200::/56

    # Router	Interface   Interface ID   link-local address global address
    # ----------------------------------------------------------------------
    # RT1	to RT2	    1		   fe80:0001::1       5f00::0001
    # RT2	to RT1	    1		   fe80:0001::2       5f00::0002


    fp = start_routing_interactive(verbose, protocol)

    RT1 = "RouterLsa V6-bit E-bit R-bit lsid 42.0.0.1 adv 0.0.0.1 \
    vlink iid 1 nid 1 nrid 0.0.0.2 metric 1 \
    "

    RT1_LINK = "LinkLsa lsid 0.0.0.1 adv 0.0.0.1 \
    link-local-address fe80:0001::1"

    RT1_INTRA_R = "IntraAreaPrefixLsa lsid 0.0.0.0 adv 0.0.0.1 \
    rlstype RouterLsa rlsid 0.0.0.0 radv 0.0.0.1 \
    IPv6Prefix 5f00::0001/128 metric 1 LA-bit \
    "

    RT2 = "RouterLsa bit-E V6-bit E-bit R-bit lsid 42.0.0.1 adv 0.0.0.2 \
    vlink iid 1 nid 1 nrid 0.0.0.1 metric 1 \
    "
    RT2_LINK = "LinkLsa lsid 0.0.0.1 adv 0.0.0.2 \
    link-local-address fe80:0001::2"

    RT2_INTRA_R = "IntraAreaPrefixLsa lsid 0.0.0.0 adv 0.0.0.2 \
    rlstype RouterLsa rlsid 0.0.0.0 radv 0.0.0.2 \
    IPv6Prefix 5f00::0002/128 metric 1 LA-bit \
    IPv6Prefix 5f00:0000:c001:0200::/56 metric 1 \
    "

    command = """
set_router_id 0.0.0.1
create 0.0.0.0 normal
select 0.0.0.0
replace %s
add %s
add %s
add %s
add %s
add %s
compute 0.0.0.0
verify_routing_table_size 1
verify_routing_entry  5f00:0000:c001:0200::/56 5f00::0002 2 false false
""" % (RT1,RT1_LINK,RT1_INTRA_R,
       RT2,RT2_LINK,RT2_INTRA_R)

    print >>fp, command

    if not fp.close():
        return True
    else:
        return False

def r8V2(verbose, protocol):
    """
    Simple two router case the next two tests will add another link between
    RT1 and RT2.
    """

    # RT1 is this router
    # RT2 is its direct neighbour
    # A single route should be installed for 10.1.1.0/24 via 10.1.2.1
			       
    #            +-----+                     +-----+
    #   	 | RT1 |                     | RT2 |
    #   	 |     |                     |     |
    #   	 |     |                     |     |
    # 10.2.2.0/24|     |                     |     |10.1.1.0/24
    # -----------|     |                     |     |------------
    #   	 |     |     10.1.2.0/24     |     |         
    #   	 |     |---------------------|	   |
    #   	 |     |10.1.2.2     10.1.2.1|     |
    #   	 |     |     metric 1        |	   |
    #   	 +-----+        	     +-----+

    fp = start_routing_interactive(verbose, protocol)

    RT1 = "RouterLsa E-bit lsid 127.1.0.2 adv 127.1.0.2 \
    stub lsid 10.2.2.0 ldata  255.255.255.0 metric 1 \
    transit lsid 10.1.2.2 ldata 10.1.2.2 metric 1 \
    "

    RT2 = "RouterLsa E-bit lsid 127.1.0.1 adv 127.1.0.1 \
    stub lsid 10.1.1.0 ldata  255.255.255.0 metric 1 \
    transit lsid 10.1.2.2 ldata 10.1.2.1 metric 1 \
    "

    RT1_NETWORK1 = "NetworkLsa E-bit lsid 10.1.2.2 adv 127.1.0.2 \
    netmask 0xffffff00 \
    add-router 127.1.0.2 \
    add-router 127.1.0.1 \
    "

    RT1_NETWORK2 = "NetworkLsa E-bit lsid  99.1.1.2 adv 127.1.0.2 \
    netmask 0xffffff00 \
    add-router 127.1.0.2 \
    add-router 127.1.0.1 \
    "

    command = """
set_router_id 127.1.0.2
create 0.0.0.0 normal
select 0.0.0.0
replace %s
add %s
add %s
add %s
compute 0.0.0.0
verify_routing_table_size 1
verify_routing_entry 10.1.1.0/24 10.1.2.1 2 false false
""" % (RT1, RT2, RT1_NETWORK1, RT1_NETWORK2)

    print >>fp, command

    if not fp.close():
        return True
    else:
        return False

def r9V2(verbose, protocol):
    """
    Test that when there are two paths between two routers the one
    with the lowest metric is choosen. This test is the same as the
    previous test apart from the addition of the more expensive path.
    """

    # RT1 is this router
    # RT2 is its direct neighbour via two interfaces.
    # A single route should be installed for 10.1.1.0/24 via 10.1.2.1

    #            +-----+                     +-----+
    #   	 | RT1 |     99.1.2.0/24     | RT2 |
    #   	 |     |---------------------|     |
    #   	 |     |99.1.2.2     99.1.2.1|     |
    # 10.2.2.0/24|     |     metric 10       |     |10.1.1.0/24
    # -----------|     |                     |     |------------
    #   	 |     |     10.1.2.0/24     |     |         
    #   	 |     |---------------------|	   |
    #   	 |     |10.1.2.2     10.1.2.1|     |
    #   	 |     |     metric 1        |	   |
    #   	 +-----+                     +-----+
        	       	       
    fp = start_routing_interactive(verbose, protocol)

    RT1 = "RouterLsa E-bit lsid 127.1.0.2 adv 127.1.0.2 \
    age 1074 seqno 0x8000001a cksum 0x9976 \
    transit lsid 99.1.1.2 ldata 99.1.1.2 metric 10 \
    stub lsid 10.2.2.0 ldata  255.255.255.0 metric 1 \
    transit lsid 10.1.2.2 ldata 10.1.2.2 metric 1 \
    "

    RT2 = "RouterLsa E-bit lsid 127.1.0.1 adv 127.1.0.1 \
    age 1076 seqno 0x8000001a cksum 0x4cc9 \
    transit lsid 99.1.1.2 ldata 99.1.1.1 metric 10 \
    stub lsid 10.1.1.0 ldata  255.255.255.0 metric 1 \
    transit lsid 10.1.2.2 ldata 10.1.2.1 metric 1 \
    "
 
    RT1_NETWORK1 = "NetworkLsa E-bit lsid 10.1.2.2 adv 127.1.0.2 \
    age 1075 seqno 0x80000019 cksum 0x3676 \
    netmask 0xffffff00 \
    add-router 127.1.0.2 \
    add-router 127.1.0.1 \
    "

    RT1_NETWORK2 = "NetworkLsa E-bit lsid  99.1.1.2 adv 127.1.0.2 \
    age 1074 seqno 0x80000019 cksum 0xb79c \
    netmask 0xffffff00 \
    add-router 127.1.0.2 \
    add-router 127.1.0.1 \
    "

    command = """
set_router_id 127.1.0.2
create 0.0.0.0 normal
select 0.0.0.0
replace %s
add %s
add %s
add %s
compute 0.0.0.0
verify_routing_table_size 1
verify_routing_entry 10.1.1.0/24 10.1.2.1 2 false false
""" % (RT1, RT2, RT1_NETWORK1, RT1_NETWORK2)

    print >>fp, command

    if not fp.close():
        return True
    else:
        return False

def r10V2(verbose, protocol):
    """
    Test that when there are two paths between two routers the one
    with the lowest metric is choosen. This test is identical to the
    previous test apart from the order of the transit links in the
    Router-LSAs.
    """

    # RT1 is this router
    # RT2 is its direct neighbour via two interfaces.
    # A single route should be installed for 10.1.1.0/24 via 10.1.2.1

    #            +-----+                     +-----+
    #   	 | RT1 |     99.1.2.0/24     | RT2 |
    #   	 |     |---------------------|     |
    #   	 |     |99.1.2.2     99.1.2.1|     |
    # 10.2.2.0/24|     |     metric 10       |     |10.1.1.0/24
    # -----------|     |                     |     |------------
    #   	 |     |     10.1.2.0/24     |     |         
    #   	 |     |---------------------|	   |
    #   	 |     |10.1.2.2     10.1.2.1|     |
    #   	 |     |     metric 1        |	   |
    #   	 +-----+                     +-----+

    fp = start_routing_interactive(verbose, protocol)

    RT1 = "RouterLsa E-bit lsid 127.1.0.2 adv 127.1.0.2 \
    age 1074 seqno 0x8000001a cksum 0x9976 \
    transit lsid 10.1.2.2 ldata 10.1.2.2 metric 1 \
    stub lsid 10.2.2.0 ldata  255.255.255.0 metric 1 \
    transit lsid 99.1.1.2 ldata 99.1.1.2 metric 10 \
    "

    RT2 = "RouterLsa E-bit lsid 127.1.0.1 adv 127.1.0.1 \
    age 1076 seqno 0x8000001a cksum 0x4cc9 \
    transit lsid 10.1.2.2 ldata 10.1.2.1 metric 1 \
    stub lsid 10.1.1.0 ldata  255.255.255.0 metric 1 \
    transit lsid 99.1.1.2 ldata 99.1.1.1 metric 10 \
    "
 
    RT1_NETWORK1 = "NetworkLsa E-bit lsid 10.1.2.2 adv 127.1.0.2 \
    age 1075 seqno 0x80000019 cksum 0x3676 \
    netmask 0xffffff00 \
    add-router 127.1.0.2 \
    add-router 127.1.0.1 \
    "

    RT1_NETWORK2 = "NetworkLsa E-bit lsid  99.1.1.2 adv 127.1.0.2 \
    age 1074 seqno 0x80000019 cksum 0xb79c \
    netmask 0xffffff00 \
    add-router 127.1.0.2 \
    add-router 127.1.0.1 \
    "

    command = """
set_router_id 127.1.0.2
create 0.0.0.0 normal
select 0.0.0.0
replace %s
add %s
add %s
add %s
compute 0.0.0.0
verify_routing_table_size 1
verify_routing_entry 10.1.1.0/24 10.1.2.1 2 false false
""" % (RT1, RT2, RT1_NETWORK1, RT1_NETWORK2)

    print >>fp, command

    if not fp.close():
        return True
    else:
        return False

def r11database():
    """
    Return the database for test r11V3
    """

    # RT1 is this router.
    # RTC is an area border router that generates an Inter-Area-Prefix-LSA.
    # RT1 is connected to RTC via transit.

    #                            +
    #     +---+1       1+---+1   |
    #     |RT1|---------|RTC|----| Other Area
    #     +---+         +---+    |
    #                            +

    # Network           IPv6 prefix
    # -----------------------------------
    # Other Area	3ffe:4725:c404::/64

    # Router	Interface   Interface ID   link-local address
    # -------------------------------------------------------
    # RT1	to RTC	    4		   fe80::8:800:200c:4300
    # RTC	to RT1	    3		   fe80::8:800:200c:4350

    RT1 = "RouterLsa V6-bit E-bit R-bit lsid 0.0.0.0 adv 10.80.52.103 \
    transit iid 4 nid 4 nrid 10.80.52.103 metric 1 \
    "
    RT1_LINK = "LinkLsa lsid 0.0.0.4 adv 10.80.52.103 \
    link-local-address fe80::8:800:200c:4300"

    RT1_NETWORK = "NetworkLsa lsid 0.0.0.4 adv 10.80.52.103 \
    add-router 10.80.52.103 \
    add-router 10.80.52.107 \
    "

    RTC = "RouterLsa bit-B V6-bit E-bit R-bit lsid 0.0.0.1 adv 10.80.52.107 \
    transit iid 3 nid 4 nrid 10.80.52.103 metric 1 \
    "

    RTC_LINK = "LinkLsa lsid 0.0.0.3 adv 10.80.52.107 \
    link-local-address fe80::8:800:200c:4350"

    RTC_INTER = "SummaryNetworkLsa lsid 0.0.0.2 adv 10.80.52.107 \
    metric 1 \
    IPv6Prefix 3ffe:4725:c404::/64 \
    "

    database = """
replace %s
add %s
add %s
add %s
add %s
add %s
""" % (RT1,RT1_LINK,RT1_NETWORK,
       RTC,RTC_LINK,RTC_INTER)

    return database

def r11V3(verbose, protocol):
    """
    Verify the correct processing of Inter-Area-Prefix-LSAs, when this router
    is connected to the generating router by a Network-LSA.
    """

    fp = start_routing_interactive(verbose, protocol)

    command = """
set_router_id 10.80.52.103
create 0.0.0.0 normal
select 0.0.0.0
%s
compute 0.0.0.0
verify_routing_table_size 1
verify_routing_entry 3ffe:4725:c404::/64 fe80::8:800:200c:4350 2 false false
""" % r11database()

    print >>fp, command

    if not fp.close():
        return True
    else:
        return False

def r12database():
    """
    Return the database for test r12V3
    """

    # RT1 is this router.
    # RTB is an area border router that generates an Intra-Area-Prefix-LSA.
    # RT1 is connected to RTB via transit.

    #                            +
    #     +---+1       1+---+1   |
    #     |RT1|---------|RTB|----| Other Area
    #     +---+         +---+    |
    #                            +

    # Network           IPv6 prefix
    # -----------------------------------
    # Other Area	3ffe:4725:c404::/64

    # Router	Interface   Interface ID   link-local address
    # -------------------------------------------------------
    # RT1	to RTB	    5		   fe80::8:800:200c:4200
    # RTB	to RT1	    2		   fe80::8:800:200c:4250

    RT1 = "RouterLsa V6-bit E-bit R-bit lsid 0.0.0.0 adv 10.80.52.103 \
    transit iid 5 nid 5 nrid 10.80.52.103 metric 1 \
    "
    RT1_LINK = "LinkLsa lsid 0.0.0.5 adv 10.80.52.103 \
    link-local-address fe80::8:800:200c:4200"

    RT1_NETWORK = "NetworkLsa lsid 0.0.0.5 adv 10.80.52.103 \
    add-router 10.80.52.103 \
    add-router 10.80.52.106 \
    "

    RTB = "RouterLsa bit-B V6-bit E-bit R-bit lsid 0.0.0.1 adv 10.80.52.106 \
    transit iid 2 nid 5 nrid 10.80.52.103 metric 1 \
    "

    RTB_LINK = "LinkLsa lsid 0.0.0.2 adv 10.80.52.106 \
    link-local-address fe80::8:800:200c:4250"

    RTB_INTRA = "IntraAreaPrefixLsa lsid 0.0.0.2 adv 10.80.52.106 \
    rlstype 0x2001 rlsid 0.0.0.0 radv 10.80.52.106 \
    IPv6Prefix 3ffe:4725:c404::/64 metric 3 \
    "

    database = """
replace %s
add %s
add %s
add %s
add %s
add %s
""" % (RT1,RT1_LINK,RT1_NETWORK,
       RTB,RTB_LINK,RTB_INTRA)

    return database

def r12V3(verbose, protocol):
    """
    Verify the correct processing of Intra-Area-Prefix-LSAs.
    """

    fp = start_routing_interactive(verbose, protocol)

    command = """
set_router_id 10.80.52.103
create 36.0.0.0 normal
select 36.0.0.0
%s
compute 36.0.0.0
verify_routing_table_size 1
verify_routing_entry 3ffe:4725:c404::/64 fe80::8:800:200c:4250 4 false false
""" % r12database()

    print >>fp, command

    if not fp.close():
        return True
    else:
        return False

def r13V3(verbose, protocol):
    """
    Verify that Intra-Area-Prefix-LSAs always win over Inter-Area-Prefix-LSAs.
    The same route is introduced into thebackbone 0.0.0.0 and area 36.0.0.0,
    In the backbone 0.0.0.0 it is an Inter-Area-Prefix-LSA, in area 36.0.0.0
    it is an Intra-Area-Prefix-LSA.

    The two area databases are introduced in various orders and the
    computations run to make sure that however the LSAs are introduced
    the IntraAreaPrefixLsa always wins.
    """

    init = """
set_router_id 10.80.52.103
create 0.0.0.0 normal
create 36.0.0.0 normal
"""

    area0 = """
select 0.0.0.0
%s
""" % (r11database())

    verify0 = """
verify_routing_table_size 1
verify_routing_entry 3ffe:4725:c404::/64 fe80::8:800:200c:4350 2 false false
"""

    area36 = """
select 36.0.0.0
%s
""" % (r12database())

    verify36 = """
verify_routing_table_size 1
verify_routing_entry 3ffe:4725:c404::/64 fe80::8:800:200c:4250 4 false false
"""

    fp = start_routing_interactive(verbose, protocol)

    print >>fp, init
    print >>fp, area0
    print >>fp, "compute 0.0.0.0"
    print >>fp, verify0
    print >>fp, area36
    print >>fp, "compute 36.0.0.0"
    print >>fp, verify36

    if fp.close():
        return False

    fp = start_routing_interactive(verbose, protocol)

    print >>fp, init
    print >>fp, area36
    print >>fp, "compute 36.0.0.0"
    print >>fp, verify36
    print >>fp, area0
    print >>fp, "compute 0.0.0.0"
    print >>fp, verify36

    if fp.close():
        return False

    fp = start_routing_interactive(verbose, protocol)

    print >>fp, init
    print >>fp, area0
    print >>fp, area36
    print >>fp, "compute 0.0.0.0"
    print >>fp, verify0

    if fp.close():
        return False

    return True

# http://bugzilla.xorp.org/bugzilla/show_bug.cgi?id=770

def r14V3(verbose, protocol):
    """
    If two or more routers AS boundary or area border routers had the
    same link-local address then only one router would be installed in
    the routing table. If the router that did not get its routes
    installed was an AS boundary router then all AS-External-LSAs that it
    introduced would not be installed in the routing table.
    """

    # RT1 is this router.
    # RT2 is adjacent to RT1
    # RTC is an AS boundary router that generates an AS-External-LSA

    # external IPv6 connection
    # |
    # |
    # | RTC                  RT2                     RT1
    # Cisco --------------- XORP-1.5 -------------- XORP-1.5
    # 195.113.144.2       147.229.255.254          147.229.9.41
    #                  fe80::207:e9ff:fe1a:b2f<->fe80::230:48ff:fe87:d512

    # Network           IPv6 prefix
    # -----------------------------------
    # External address	2000::/3

    # Router	Interface   Interface ID   link-local address
    # -------------------------------------------------------
    # RT1	to RT2	    1		   fe80::230:48ff:fe87:d512
    # RT2	to RT1	    2		   fe80::207:e9ff:fe1a:b2f
    # RT2       to RTC      1              fe80:0001::1
    # RTC	to RT2	    1		   fe80:0001::2

    fp = start_routing_interactive(verbose, protocol)

    RT2_ID = "147.229.255.254"
#    RT2_ID = "247.229.255.254"
    
    RTC_LINK_LOCAL= "fe80:0001::2"

    RT1 = "RouterLsa V6-bit R-bit lsid 42.0.0.1 adv 147.229.9.41 \
    p2p iid 1 nid 2 nrid %s metric 1 \
    " % RT2_ID
    
    RT1_LINK = "LinkLsa lsid 0.0.0.1 adv 147.229.9.41 \
    link-local-address fe80::230:48ff:fe87:d512 \
    "

    RT2 = "RouterLsa bit-E V6-bit R-bit lsid 42.0.0.1 adv %s \
    p2p iid 1 nid 1 nrid 195.113.144.2 metric 1 \
    p2p iid 2 nid 1 nrid 147.229.9.41 metric 1 \
    " % RT2_ID

    RT2_LINK1 = "LinkLsa lsid 0.0.0.1 adv %s \
    link-local-address fe80:0001::2 \
    " % RT2_ID

    RT2_LINK2 = "LinkLsa lsid 0.0.0.2 adv %s \
    link-local-address fe80::207:e9ff:fe1a:b2f \
    " % RT2_ID

    RTC = "RouterLsa bit-E V6-bit R-bit E-bit lsid 42.0.0.1 adv 195.113.144.2 \
    p2p iid 1 nid 1 nrid %s metric 1 \
    " % RT2_ID

    RTC_LINK = "LinkLsa lsid 0.0.0.1 adv 195.113.144.2 \
    link-local-address %s \
    " % RTC_LINK_LOCAL

    RTC_AS_EXTERNAL = "ASExternalLsa lsid 42.0.0.2 adv 195.113.144.2 \
    metric 1 \
    IPv6Prefix 2000::/3 \
    "

    command = """
set_router_id  147.229.9.41  
create 0.0.0.0 normal
select 0.0.0.0
replace %s
add %s
add %s
add %s
add %s
add %s
add %s
add %s
compute 0.0.0.0
verify_routing_table_size 1
verify_routing_entry 2000::/3  fe80::207:e9ff:fe1a:b2f 3 false false
""" % (RT1,RT1_LINK,
       RT2,RT2_LINK1,RT2_LINK2,
       RTC,RTC_LINK,RTC_AS_EXTERNAL)

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
        protocol = 'unknown'
        for j in TESTS:
            if j[0] == i:
                if len(j) > 2:
                    protocol = j[2]
        test = i + '(verbose,protocol)'
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
