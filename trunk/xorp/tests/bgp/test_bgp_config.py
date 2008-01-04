#!/usr/bin/env python

# Copyright (c) 2001-2008 International Computer Science Institute
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

# $XORP: xorp/tests/bgp/test_bgp_config.py,v 1.20 2007/12/12 02:03:54 atanu Exp $

import sys
sys.path.append("..")
from test_xorpsh import xorpsh

LOCALHOST="127.0.0.1"
AS=65000

# IBGP
AS_LOCAL_1=AS
AS_PEER_1=AS
PORT_LOCAL_1=10001
PORT_PEER_1=20001

def conf_interfaces(builddir):
    """
    Configure an interface
    """

    # Configure the xorpsh
    xorpsh_commands = \
"""
configure

create interfaces
edit interfaces

create interface fxp0
edit interface fxp0

create vif fxp0
edit vif fxp0

create address 127.0.0.1
edit address 127.0.0.1
set prefix-length 16

commit
"""

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_tracing_state(builddir):
    """
    Enable the tracing of state changes, assume that BGP is already configured.
    """

    # Configure the xorpsh
    xorpsh_commands = \
"""
configure

edit protocols bgp

create traceoptions
edit traceoptions

create flag
edit flag

create state-change

commit
"""

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_set_holdtime(builddir, peer, holdtime):
    """
    Set the holdtime on the specified peer.
    """

    # Configure the xorpsh
    xorpsh_commands = \
"""
configure

set protocols bgp peer %s holdtime %s

commit
""" % (peer, holdtime)

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_set_prefix_limit(builddir, peer, maximum):
    """
    Set the prefix limit on a peer
    """

    # Configure the xorpsh
    xorpsh_commands = \
"""
configure

create protocols bgp peer %s prefix-limit
edit  protocols bgp peer %s prefix-limit
set maximum %s

commit
""" % (peer, peer, maximum)

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_IBGP(builddir):
    """
    Configure ONE IBGP peering
    """

    # Configure the xorpsh
    xorpsh_commands = \
"""configure
load empty.boot
create protocol bgp
edit protocol bgp
set bgp-id 1.2.3.4
set local-as 65000

create peer peer1
edit peer peer1
set local-port 10001
set peer-port 20001
set next-hop 127.0.0.1
set local-ip 127.0.0.1
set as 65000
commit
"""

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_EBGP(builddir):
    """
    Configure ONE EBGP peering
    """

    # Configure the xorpsh
    xorpsh_commands = \
"""configure
load empty.boot
create protocol bgp
edit protocol bgp
set bgp-id 1.2.3.4
set local-as 65000

create peer peer1
edit peer peer1
set local-port 10001
set peer-port 20001
set next-hop 127.0.0.1
set local-ip 127.0.0.1
set as 65001
commit
"""

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_bug_360(builddir):
    """
    Configure ONE EBGP peering
    """

    # Configure the xorpsh
    xorpsh_commands = \
"""configure
load empty.boot
create protocol bgp
edit protocol bgp
set bgp-id 1.2.3.4
set local-as 65001

create peer peer1
edit peer peer1
set local-port 10001
set peer-port 20001
set next-hop 127.0.0.1
set local-ip 127.0.0.1
set as 75
commit
"""

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_EBGP_EBGP(builddir):
    """
    Configure two EBGP peerings
    """

    # Configure the xorpsh
    xorpsh_commands = \
"""configure
load empty.boot
create protocol bgp
edit protocol bgp
set bgp-id 1.2.3.4
set local-as 65000

create peer peer1
edit peer peer1
set local-port 10001
set peer-port 20001
set next-hop 127.0.0.1
set local-ip 127.0.0.1
set as 65001
up

create peer peer2
edit peer peer2
set local-port 10002
set peer-port 20002
set next-hop 127.0.0.1
set local-ip 127.0.0.1
set as 65002

commit
"""

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_test_dump_1(builddir):
    """
    Configure two EBGP peerings
    """

    # Configure the xorpsh
    xorpsh_commands = \
"""configure
load empty.boot
create protocol bgp
edit protocol bgp
set bgp-id 1.2.3.4
set local-as 65010

create peer peer1
edit peer peer1
set local-port 10001
set peer-port 20001
set next-hop 127.0.0.1
set local-ip 127.0.0.1
set as 65000
up

create peer peer2
edit peer peer2
set local-port 10002
set peer-port 20002
set next-hop 127.0.0.1
set local-ip 127.0.0.1
set as 65001
up

up
up

create protocols static
edit protocols static
create route 192.150.187.0/24
edit route 192.150.187.0/24
set next-hop 127.0.0.1
up

commit
"""

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_EBGP_IBGP_IBGP(builddir):
    """
    Configure One EBGP peering and two IBGP peerings
    """

    # Configure the xorpsh
    xorpsh_commands = \
"""configure
load empty.boot
create protocol bgp
edit protocol bgp
set bgp-id 1.2.3.4
set local-as 65000

create peer peer1
edit peer peer1
set local-port 10001
set peer-port 20001
set next-hop 127.0.0.1
set local-ip 127.0.0.1
set as 65001
up

create peer peer2
edit peer peer2
set local-port 10002
set peer-port 20002
set next-hop 127.0.0.1
set local-ip 127.0.0.1
set as 65000
up

create peer peer3
edit peer peer3
set local-port 10003
set peer-port 20003
set next-hop 127.0.0.1
set local-ip 127.0.0.1
set as 65000
up

commit
"""

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_RUT_as2_TR1_as1(builddir):
    """
    Configure One EBGP peering
    """

    # Configure the xorpsh
    xorpsh_commands = \
"""configure
load empty.boot
create protocol bgp
edit protocol bgp
set bgp-id 1.2.3.4
set local-as 2

create peer peer1
edit peer peer1
set local-port 10001
set peer-port 20001
set next-hop 127.0.0.1
set local-ip 127.0.0.1
set as 1
up

commit
"""

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_RUT_as2_TR1_as1_TR2_as2(builddir):
    """
    Configure One EBGP peering and one IBGP peerings
    """

    # Configure the xorpsh
    xorpsh_commands = \
"""configure
load empty.boot
create protocol bgp
edit protocol bgp
set bgp-id 1.2.3.4
set local-as 2

create peer peer1
edit peer peer1
set local-port 10001
set peer-port 20001
set next-hop 127.0.0.1
set local-ip 127.0.0.1
set as 1
up

create peer peer2
edit peer peer2
set local-port 10002
set peer-port 20002
set next-hop 127.0.0.1
set local-ip 127.0.0.1
set as 2
up

commit
"""

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_RUT_as2_TR1_as1_TR2_as1_TR3_as3(builddir):
    """
    Configure One EBGP peering and two IBGP peerings
    """

    # Configure the xorpsh
    xorpsh_commands = \
"""configure
load empty.boot
create protocol bgp
edit protocol bgp
set bgp-id 1.2.3.4
set local-as 2

create peer peer1
edit peer peer1
set local-port 10001
set peer-port 20001
set next-hop 127.0.0.1
set local-ip 127.0.0.1
set as 1
up

create peer peer2
edit peer peer2
set local-port 10002
set peer-port 20002
set next-hop 127.0.0.1
set local-ip 127.0.0.1
set as 1
up

create peer peer3
edit peer peer3
set local-port 10003
set peer-port 20003
set next-hop 127.0.0.1
set local-ip 127.0.0.1
set as 3
up

commit
"""

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_RUT_as2_TR1_as1_TR2_as3(builddir):
    """
    Configure One EBGP peering and two IBGP peerings
    """

    # Configure the xorpsh
    xorpsh_commands = \
"""configure
load empty.boot
create protocol bgp
edit protocol bgp
set bgp-id 1.2.3.4
set local-as 2

create peer peer1
edit peer peer1
set local-port 10001
set peer-port 20001
set next-hop 127.0.0.1
set local-ip 127.0.0.1
set as 1
up

create peer peer2
edit peer peer2
set local-port 10002
set peer-port 20002
set next-hop 127.0.0.1
set local-ip 127.0.0.1
set as 3

commit
"""

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_RUT_as3_TR1_as1_TR2_as2_TR3_as4(builddir):
    """
    Configure three EBGP peerings
    """

    # Configure the xorpsh
    xorpsh_commands = \
"""configure
load empty.boot
create protocol bgp
edit protocol bgp
set bgp-id 1.2.3.4
set local-as 3

create peer peer1
edit peer peer1
set local-port 10001
set peer-port 20001
set next-hop 127.0.0.1
set local-ip 127.0.0.1
set as 1
up

create peer peer2
edit peer peer2
set local-port 10002
set peer-port 20002
set next-hop 127.0.0.1
set local-ip 127.0.0.1
set as 2
up

create peer peer3
edit peer peer3
set local-port 10003
set peer-port 20003
set next-hop 127.0.0.1
set local-ip 127.0.0.1
set as 4

commit
"""

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True
    
def conf_redist_static(builddir, create_static = True):
    """
    Redistribute static into BGP
    """

    if create_static:
        create_static_command = \
"""
create protocols static
edit protocols static

top
"""
    else:
        create_static_command = ''

    # Configure the xorpsh
    xorpsh_commands = \
"""
configure

%s

create policy
edit policy

create policy-statement static
edit policy-statement static

create term 1
edit term 1

create from
edit from
set protocol static

top

edit protocols bgp
set export static

commit
""" % (create_static_command)

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_redist_static_incomplete(builddir):
    """
    Redistribute static into BGP and set the origin to INCOMPLETE
    """

    conf_redist_static(builddir)

    # Configure the xorpsh
    xorpsh_commands = \
"""
configure

edit policy policy-statement static term 1

create then
edit then
set origin 2

commit
"""

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_redist_static_no_export(builddir):
    """
    Redistribute static into BGP and set NO_EXPORT
    """

    conf_redist_static(builddir)

    # Configure the xorpsh
    xorpsh_commands = \
"""
configure

edit policy policy-statement static term 1

create then
edit then
set community NO_EXPORT

commit
"""

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_redist_static_med(builddir):
    """
    Redistribute static into BGP and set the MED to 42
    """

    conf_redist_static(builddir)

    # Configure the xorpsh
    xorpsh_commands = \
"""
configure

edit policy policy-statement static term 1

create then
edit then
set med 42
up
up

create term 2
edit term 2
create to
edit to
set as-path ^$
up

create then
edit then
set med-remove true

commit
"""

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_preference_TR1(builddir):
    """
    Configure TR1 to have a higher preference than TR2
    """

    # Configure the xorpsh
    xorpsh_commands = \
    """
configure

create policy policy-statement preference term 1
edit policy policy-statement preference term 1

create from
edit from
set nexthop4 127.0.0.2..127.0.0.2
up

create then
edit then
set localpref 200
top

edit protocols bgp
set import preference

commit
    """

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_import_med_change(builddir):
    """
    Set the med of all incoming packets to 42
    """

    # Configure the xorpsh
    xorpsh_commands = \
    """
configure

create policy policy-statement preference term 1
edit policy policy-statement preference term 1

create then
edit then
set med 42
top

edit protocols bgp
set import preference

commit
    """

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_export_med_change(builddir):
    """
    Set the med of all outgoing packets to 42
    """

    # Configure the xorpsh
    xorpsh_commands = \
    """
configure

create policy policy-statement preference term 1
edit policy policy-statement preference term 1

create then
edit then
set med 42
top

edit protocols bgp
set export preference

commit
    """

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_import_origin_change(builddir):
    """
    Set the origin of all incoming packets to incomplete
    """

    # Configure the xorpsh
    xorpsh_commands = \
    """
configure

create policy policy-statement preference term 1
edit policy policy-statement preference term 1

create then
edit then
set origin 2
top

edit protocols bgp
set import preference

commit
    """

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_export_origin_change(builddir):
    """
    Set the origin of all outgoing packets to incomplete
    """

    # Configure the xorpsh
    xorpsh_commands = \
    """
configure

create policy policy-statement preference term 1
edit policy policy-statement preference term 1

create then
edit then
set origin 2
top

edit protocols bgp
set export preference

commit
    """

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_damping(builddir):
    """
    Configure damping
    """

    # Configure the xorpsh
    xorpsh_commands = \
"""
configure

edit protocols bgp

create damping
edit damping
set suppress 2000
set reuse 800
set half-life 3
set max-suppress 5

commit
"""

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True


def conf_aggregate_brief(builddir):

    if not conf_aggregate(builddir, "true"):
        return False

    return True


def conf_aggregate_asset(builddir):

    if not conf_aggregate(builddir, "false"):
        return False

    return True


def conf_aggregate(builddir, brief_mode):
    """
    Configure aggregate 
    """

    # Configure the xorpsh
    xorpsh_commands = \
"""
configure

create policy
edit policy

create policy-statement aggregate
edit policy-statement aggregate

create term 1
edit term 1

create from
edit from

set network4 <= 192.0.0.0/8

up
create then
edit then
set aggregate-prefix-len 8
set aggregate-brief-mode %s

top
edit policy
create policy-statement drop-component
edit policy-statement drop-component

create term 1
edit term 1

create to
edit to

set was-aggregated true

up
create then
edit then

set reject

top
edit protocol bgp
set import aggregate
set export drop-component

commit
""" % (brief_mode)

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_create_protocol_static(builddir):
    """
    Create the static protocol
    """

    # Configure the xorpsh
    xorpsh_commands = \
"""
configure

create protocols static

commit
"""

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True


def conf_add_static_route4(builddir, net, next_hop = "127.0.0.1"):
    """
    Add a static route
    """

    # Configure the xorpsh
    xorpsh_commands = \
"""
configure

edit protocols static
create route %s
edit route %s
set next-hop %s

commit
""" % (net, net, next_hop)

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_delete_static_route4(builddir, net, next_hop = "127.0.0.1"):
    """
    Delete a static route
    """

    # Configure the xorpsh
    xorpsh_commands = \
"""
configure

delete protocols static route %s

commit
""" % (net)

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

def conf_multiprotocol(builddir):
    """
    Configure multiprotocol
    """

    return True

def show_bgp_routes(builddir):
    """
    Return the output of the show bgp routes command.
    """

    xorpsh_commands = \
"""
show bgp routes
"""

    result, output = xorpsh(builddir, xorpsh_commands)

    if not result:
        return False, output

    return True, output
    
    
# Local Variables:
# mode: python
# py-indent-offset: 4
# End:
