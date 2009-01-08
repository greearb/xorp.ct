// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#ident "$XORP: xorp/libfeaclient/ifmgr_atoms.cc,v 1.29 2008/10/02 21:57:15 bms Exp $"

#include "ifmgr_atoms.hh"


const IPv4 IfMgrIPv4Atom::_ZERO_ADDR = IPv4::ZERO();
const IPv6 IfMgrIPv6Atom::_ZERO_ADDR = IPv6::ZERO();

// ----------------------------------------------------------------------------
// IfMgrIfTree Methods

const IfMgrIfAtom*
IfMgrIfTree::find_interface(const string& ifname) const
{
    IfMgrIfTree::IfMap::const_iterator ii = interfaces().find(ifname);
    if (ii == interfaces().end())
	return (NULL);

    return (&ii->second);
}

IfMgrIfAtom*
IfMgrIfTree::find_interface(const string& ifname)
{
    IfMgrIfTree::IfMap::iterator ii = interfaces().find(ifname);
    if (ii == interfaces().end())
	return (NULL);

    return (&ii->second);
}

const IfMgrVifAtom*
IfMgrIfTree::find_vif(const string& ifname, const string& vifname) const
{
    const IfMgrIfAtom* ifa = find_interface(ifname);
    if (ifa == NULL)
	return (NULL);

    return (ifa->find_vif(vifname));
}

IfMgrVifAtom*
IfMgrIfTree::find_vif(const string& ifname, const string& vifname)
{
    IfMgrIfAtom* ifa = find_interface(ifname);
    if (ifa == NULL)
	return (NULL);

    return (ifa->find_vif(vifname));
}

const IfMgrIPv4Atom*
IfMgrIfTree::find_addr(const string& ifname,
		       const string& vifname,
		       const IPv4&   addr) const
{
    const IfMgrVifAtom* vifa = find_vif(ifname, vifname);
    if (vifa == NULL)
	return (NULL);

    return (vifa->find_addr(addr));
}

IfMgrIPv4Atom*
IfMgrIfTree::find_addr(const string& ifname,
		       const string& vifname,
		       const IPv4&   addr)
{
    IfMgrVifAtom* vifa = find_vif(ifname, vifname);
    if (vifa == NULL)
	return (NULL);

    return (vifa->find_addr(addr));
}

const IfMgrIPv6Atom*
IfMgrIfTree::find_addr(const string& ifname,
		       const string& vifname,
		       const IPv6&   addr) const
{
    const IfMgrVifAtom* vifa = find_vif(ifname, vifname);
    if (vifa == NULL)
	return (NULL);

    return (vifa->find_addr(addr));
}

IfMgrIPv6Atom*
IfMgrIfTree::find_addr(const string& ifname,
		       const string& vifname,
		       const IPv6&   addr)
{
    IfMgrVifAtom* vifa = find_vif(ifname, vifname);
    if (vifa == NULL)
	return (NULL);

    return (vifa->find_addr(addr));
}

bool
IfMgrIfTree::operator==(const IfMgrIfTree& o) const
{
    return (o.interfaces() == interfaces());
}

bool
IfMgrIfTree::is_my_addr(const IPv4& addr, string& ifname,
			string& vifname) const
{
    IfMgrIfTree::IfMap::const_iterator if_iter;

    for (if_iter = interfaces().begin(); if_iter != interfaces().end(); ++if_iter) {
	const IfMgrIfAtom& iface = if_iter->second;

	// Test if interface is enabled and the link state is up
	if ((! iface.enabled()) || iface.no_carrier())
	    continue;

	IfMgrIfAtom::VifMap::const_iterator vif_iter;
	for (vif_iter = iface.vifs().begin();
	     vif_iter != iface.vifs().end();
	     ++vif_iter) {
	    const IfMgrVifAtom& vif = vif_iter->second;

	    // Test if vif is enabled
	    if (! vif.enabled())
		continue;

	    // Test if there is matching IPv4 address
	    IfMgrVifAtom::IPv4Map::const_iterator a4_iter;

	    for (a4_iter = vif.ipv4addrs().begin();
		 a4_iter != vif.ipv4addrs().end();
		 ++a4_iter) {
		const IfMgrIPv4Atom& a4 = a4_iter->second;

		if (! a4.enabled())
		    continue;

		// Test if my own address
		if (a4.addr() == addr) {
		    ifname = iface.name();
		    vifname = vif.name();
		    return (true);
		}
	    }
	}
    }

    ifname = "";
    vifname = "";

    return (false);
}

bool
IfMgrIfTree::is_my_addr(const IPv6& addr, string& ifname,
			string& vifname) const
{
    IfMgrIfTree::IfMap::const_iterator if_iter;

    for (if_iter = interfaces().begin(); if_iter != interfaces().end(); ++if_iter) {
	const IfMgrIfAtom& iface = if_iter->second;

	// Test if interface is enabled and the link state is up
	if ((! iface.enabled()) || iface.no_carrier())
	    continue;

	IfMgrIfAtom::VifMap::const_iterator vif_iter;
	for (vif_iter = iface.vifs().begin();
	     vif_iter != iface.vifs().end();
	     ++vif_iter) {
	    const IfMgrVifAtom& vif = vif_iter->second;

	    // Test if vif is enabled
	    if (! vif.enabled())
		continue;

	    // Test if there is matching IPv6 address
	    IfMgrVifAtom::IPv6Map::const_iterator a6_iter;

	    for (a6_iter = vif.ipv6addrs().begin();
		 a6_iter != vif.ipv6addrs().end();
		 ++a6_iter) {
		const IfMgrIPv6Atom& a6 = a6_iter->second;

		if (! a6.enabled())
		    continue;

		// Test if my own address
		if (a6.addr() == addr) {
		    ifname = iface.name();
		    vifname = vif.name();
		    return (true);
		}
	    }
	}
    }

    ifname = "";
    vifname = "";

    return (false);
}

bool
IfMgrIfTree::is_my_addr(const IPvX& addr, string& ifname,
			string& vifname) const
{
    if (addr.is_ipv4()) {
	IPv4 addr4 = addr.get_ipv4();
	return (is_my_addr(addr4, ifname, vifname));
    }

    if (addr.is_ipv6()) {
	IPv6 addr6 = addr.get_ipv6();
	return (is_my_addr(addr6, ifname, vifname));
    }

    return (false);
}

bool
IfMgrIfTree::is_directly_connected(const IPv4& addr, string& ifname,
				   string& vifname) const
{
    IfMgrIfTree::IfMap::const_iterator if_iter;

    for (if_iter = interfaces().begin(); if_iter != interfaces().end(); ++if_iter) {
	const IfMgrIfAtom& iface = if_iter->second;

	// Test if interface is enabled and the link state is up
	if ((! iface.enabled()) || iface.no_carrier())
	    continue;

	IfMgrIfAtom::VifMap::const_iterator vif_iter;
	for (vif_iter = iface.vifs().begin();
	     vif_iter != iface.vifs().end();
	     ++vif_iter) {
	    const IfMgrVifAtom& vif = vif_iter->second;

	    // Test if vif is enabled
	    if (! vif.enabled())
		continue;

	    // Test if there is matching IPv4 address
	    IfMgrVifAtom::IPv4Map::const_iterator a4_iter;

	    for (a4_iter = vif.ipv4addrs().begin();
		 a4_iter != vif.ipv4addrs().end();
		 ++a4_iter) {
		const IfMgrIPv4Atom& a4 = a4_iter->second;

		if (! a4.enabled())
		    continue;

		// Test if my own address
		if (a4.addr() == addr) {
		    ifname = iface.name();
		    vifname = vif.name();
		    return (true);
		}

		// Test if p2p address
		if (a4.has_endpoint()) {
		    if (a4.endpoint_addr() == addr) {
			ifname = iface.name();
			vifname = vif.name();
			return (true);
		    }
		}

		// Test if same subnet
		if (IPv4Net(addr, a4.prefix_len())
		    == IPv4Net(a4.addr(), a4.prefix_len())) {
		    ifname = iface.name();
		    vifname = vif.name();
		    return (true);
		}
	    }
	}
    }

    ifname = "";
    vifname = "";

    return (false);
}

bool
IfMgrIfTree::is_directly_connected(const IPv6& addr, string& ifname,
				   string& vifname) const
{
    IfMgrIfTree::IfMap::const_iterator if_iter;

    for (if_iter = interfaces().begin(); if_iter != interfaces().end(); ++if_iter) {
	const IfMgrIfAtom& iface = if_iter->second;

	// Test if interface is enabled and the link state is up
	if ((! iface.enabled()) || iface.no_carrier())
	    continue;

	IfMgrIfAtom::VifMap::const_iterator vif_iter;
	for (vif_iter = iface.vifs().begin();
	     vif_iter != iface.vifs().end();
	     ++vif_iter) {
	    const IfMgrVifAtom& vif = vif_iter->second;

	    // Test if vif is enabled
	    if (! vif.enabled())
		continue;

	    // Test if there is matching IPv6 address
	    IfMgrVifAtom::IPv6Map::const_iterator a6_iter;

	    for (a6_iter = vif.ipv6addrs().begin();
		 a6_iter != vif.ipv6addrs().end();
		 ++a6_iter) {
		const IfMgrIPv6Atom& a6 = a6_iter->second;

		if (! a6.enabled())
		    continue;

		// Test if my own address
		if (a6.addr() == addr) {
		    ifname = iface.name();
		    vifname = vif.name();
		    return (true);
		}

		// Test if p2p address
		if (a6.has_endpoint()) {
		    if (a6.endpoint_addr() == addr) {
			ifname = iface.name();
			vifname = vif.name();
			return (true);
		    }
		}

		// Test if same subnet
		if (IPv6Net(addr, a6.prefix_len())
		    == IPv6Net(a6.addr(), a6.prefix_len())) {
		    ifname = iface.name();
		    vifname = vif.name();
		    return (true);
		}
	    }
	}
    }

    ifname = "";
    vifname = "";

    return (false);
}

bool
IfMgrIfTree::is_directly_connected(const IPvX& addr, string& ifname,
				   string& vifname) const
{
    if (addr.is_ipv4()) {
	IPv4 addr4 = addr.get_ipv4();
	return (is_directly_connected(addr4, ifname, vifname));
    }

    if (addr.is_ipv6()) {
	IPv6 addr6 = addr.get_ipv6();
	return (is_directly_connected(addr6, ifname, vifname));
    }

    return (false);
}


// ----------------------------------------------------------------------------
// IfMgrIfAtom methods

bool
IfMgrIfAtom::operator==(const IfMgrIfAtom& o) const
{
    return (
	    name()			== o.name()			&&
	    enabled()			== o.enabled()			&&
	    discard()			== o.discard()			&&
	    unreachable()		== o.unreachable()		&&
	    management()		== o.management()		&&
	    mtu()			== o.mtu()			&&
	    mac()			== o.mac()			&&
	    pif_index()			== o.pif_index()		&&
	    no_carrier()		== o.no_carrier()		&&
	    baudrate()			== o.baudrate()			&&
	    vifs()			== o.vifs()
	    );
}

const IfMgrVifAtom*
IfMgrIfAtom::find_vif(const string& vifname) const
{
    IfMgrIfAtom::VifMap::const_iterator vi = vifs().find(vifname);
    if (vi == vifs().end())
	return (NULL);

    return (&vi->second);
}

IfMgrVifAtom*
IfMgrIfAtom::find_vif(const string& vifname)
{
    IfMgrIfAtom::VifMap::iterator vi = vifs().find(vifname);
    if (vi == vifs().end())
	return (NULL);

    return (&vi->second);
}


// ----------------------------------------------------------------------------
// IfMgrVifAtom methods

bool
IfMgrVifAtom::operator==(const IfMgrVifAtom& o) const
{
    return (
	    name()			== o.name()			&&
	    enabled()			== o.enabled()			&&
	    multicast_capable()		== o.multicast_capable()	&&
	    broadcast_capable()	  	== o.broadcast_capable()	&&
	    p2p_capable()		== o.p2p_capable()		&&
	    loopback()			== o.loopback()			&&
	    pim_register()		== o.pim_register()		&&
	    pif_index()			== o.pif_index()		&&
	    vif_index()			== o.vif_index()		&&
	    is_vlan()			== o.is_vlan()			&&
	    vlan_id()			== o.vlan_id()			&&
	    ipv4addrs()			== o.ipv4addrs()		&&
	    ipv6addrs()			== o.ipv6addrs()
	    );
}

const IfMgrIPv4Atom*
IfMgrVifAtom::find_addr(const IPv4& addr) const
{
    IfMgrVifAtom::IPv4Map::const_iterator ai = ipv4addrs().find(addr);
    if (ai == ipv4addrs().end())
	return (NULL);

    return (&ai->second);
}

IfMgrIPv4Atom*
IfMgrVifAtom::find_addr(const IPv4& addr)
{
    IfMgrVifAtom::IPv4Map::iterator ai = ipv4addrs().find(addr);
    if (ai == ipv4addrs().end())
	return (NULL);

    return (&ai->second);
}

const IfMgrIPv6Atom*
IfMgrVifAtom::find_addr(const IPv6& addr) const
{
    IfMgrVifAtom::IPv6Map::const_iterator ai = ipv6addrs().find(addr);
    if (ai == ipv6addrs().end())
	return (NULL);

    return (&ai->second);
}

IfMgrIPv6Atom*
IfMgrVifAtom::find_addr(const IPv6& addr)
{
    IfMgrVifAtom::IPv6Map::iterator ai = ipv6addrs().find(addr);
    if (ai == ipv6addrs().end())
	return (NULL);

    return (&ai->second);
}


// ----------------------------------------------------------------------------
// IfMgrIPv4Atom methods

bool
IfMgrIPv4Atom::operator==(const IfMgrIPv4Atom& o) const
{
    return (
	    addr()			== o.addr()			&&
	    prefix_len()		== o.prefix_len()		&&
	    enabled()			== o.enabled()			&&
	    multicast_capable()		== o.multicast_capable()	&&
	    loopback()			== o.loopback()			&&
	    has_broadcast()		== o.has_broadcast()		&&
	    broadcast_addr()		== o.broadcast_addr()		&&
	    has_endpoint()		== o.has_endpoint()		&&
	    endpoint_addr()		== o.endpoint_addr()
	    );
}


// ----------------------------------------------------------------------------
// IfMgrIfIPv6Atom methods

bool
IfMgrIPv6Atom::operator==(const IfMgrIPv6Atom& o) const
{
    return (
	    addr()			== o.addr()			&&
	    prefix_len()		== o.prefix_len()		&&
	    enabled()			== o.enabled()			&&
	    multicast_capable()		== o.multicast_capable()	&&
	    loopback()			== o.loopback()			&&
	    has_endpoint()		== o.has_endpoint()		&&
	    endpoint_addr()		== o.endpoint_addr()
	    );
}
