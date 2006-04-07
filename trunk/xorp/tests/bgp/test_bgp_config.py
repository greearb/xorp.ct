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
create peer 127.0.0.1
edit peer 127.0.0.1
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

def conf_redist_static(builddir):
    """
    Redistribute static into BGP
    """

    # Configure the xorpsh
    xorpsh_commands = \
"""
configure

create protocols static
edit protocols static

top

create policy
edit policy

create policy-statement static
edit policy-statement static

create term export
edit term export

create from
edit from
set protocol static

up
create then
edit then
set origin 2

top

edit protocols bgp
set export static

commit
"""

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

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
set prefix-length 30

commit
"""

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True


def conf_add_static_route4(builddir, net):
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
set next-hop 127.0.0.1

commit
""" % (net, net)

    if not xorpsh(builddir, xorpsh_commands):
        return False

    return True

# Local Variables:
# mode: python
# py-indent-offset: 4
# End:
