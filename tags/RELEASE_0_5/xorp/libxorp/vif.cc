// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/libxorp/vif.cc,v 1.7 2003/07/04 19:57:24 pavlin Exp $"

#include <functional>
#include <string>
#include <algorithm>
#include <vector>

#include "xorp.h"
#include "vif.hh"

VifAddr::VifAddr(const IPvX& ipvx_addr)
    : _addr(ipvx_addr),
      _subnet_addr(ipvx_addr.af()),
      _broadcast_addr(ipvx_addr.af()),
      _peer_addr(ipvx_addr.af())
{
}

VifAddr::VifAddr(const IPvX& ipvx_addr, const IPvXNet& ipvxnet_subnet_addr,
		 const IPvX& ipvx_broadcast_addr, const IPvX& ipvx_peer_addr)
    : _addr(ipvx_addr),
      _subnet_addr(ipvxnet_subnet_addr),
      _broadcast_addr(ipvx_broadcast_addr),
      _peer_addr(ipvx_peer_addr)
{
}

bool
VifAddr::is_same_subnet(const IPvXNet& ipvxnet) const
{
    return (_subnet_addr == ipvxnet);
}

bool
VifAddr::is_same_subnet(const IPvX& ipvx_addr) const
{
    IPvXNet ipvx_net(ipvx_addr, subnet_addr().prefix_len());
    
    return (ipvx_net.masked_addr() == subnet_addr().masked_addr());
}

//
// Return C++ string representation of the VifAddr object in a
// human-friendy form.
//
string
VifAddr::str() const
{
    string s = "";
    
    s += "addr: " + _addr.str();
    s += " subnet: " + _subnet_addr.str();
    s += " broadcast: " + _broadcast_addr.str();
    s += " peer: " + _peer_addr.str();
    
    return s;
}

bool
VifAddr::operator==(const VifAddr& other) const
{
    return ((addr() == other.addr())
	    && (subnet_addr() == other.subnet_addr())
	    && (broadcast_addr() == other.broadcast_addr())
	    && (peer_addr() == other.peer_addr()));
}

//
// Vif constructor
//
Vif::Vif(const string& vifname, const string& ifname)
    : _name(vifname), _ifname(ifname)
{
    set_pif_index(0);
    set_vif_index(0);
    set_pim_register(false);
    set_p2p(false);
    set_loopback(false);
    set_multicast_capable(false);
    set_broadcast_capable(false);
    set_underlying_vif_up(false);
}

//
// Vif copy constructor
//
Vif::Vif(const Vif& vif)
{
    _name = vif.name();
    set_pif_index(vif.pif_index());
    set_vif_index(vif.vif_index());
    _addr_list = vif.addr_list();
    set_pim_register(vif.is_pim_register());
    set_p2p(vif.is_p2p());
    set_loopback(vif.is_loopback());
    set_multicast_capable(vif.is_multicast_capable());
    set_broadcast_capable(vif.is_broadcast_capable());
    set_underlying_vif_up(vif.is_underlying_vif_up());
}

//
// Vif destructor
//
Vif::~Vif()
{
    
}

//
// Return C++ string representation of the Vif object in a
// human-friendy form.
//
string
Vif::str() const
{
    string r;
    
    // The vif name
    r += "Vif[";
    r += _name;
    r += "]";

    // The physical and virtual indexes
    r += " pif_index: ";
    r += c_format("%d", pif_index());
    r += " vif_index: ";
    r += c_format("%d", vif_index());
    
    // The list of addresses
    list<VifAddr>::const_iterator iter;
    for (iter = _addr_list.begin(); iter != _addr_list.end(); ++iter) {
	r += " " + iter->str();
    }
    
    // The flags
    r += " Flags:";
    if (is_p2p())
	r += " P2P";
    if (is_pim_register())
	r += " PIM_REGISTER";
    if (is_multicast_capable())
	r += " MULTICAST";
    if (is_broadcast_capable())
	r += " BROADCAST";
    if (is_loopback())
	r += " LOOPBACK";
    if (is_underlying_vif_up())
	r += " UNDERLYING_VIF_UP";
    
    return r;
}

bool
Vif::operator==(const Vif& other) const
{
    return ((name() == other.name())
	    && (pif_index() == other.pif_index())
	    && (vif_index() == other.vif_index())
	    && (addr_list() == other.addr_list())
	    && (is_pim_register() == other.is_pim_register())
	    && (is_p2p() == other.is_p2p())
	    && (is_loopback() == other.is_loopback())
	    && (is_multicast_capable() == other.is_multicast_capable())
	    && (is_broadcast_capable() == other.is_broadcast_capable())
	    && (is_underlying_vif_up() == other.is_underlying_vif_up()));
}

const IPvX *
Vif::addr_ptr() const
{
    list<VifAddr>::const_iterator iter;
    
    for (iter = _addr_list.begin(); iter != _addr_list.end(); ++iter) {
	const VifAddr *vif_addr = &(*iter);
	if (vif_addr->addr().is_unicast())
	    return (&vif_addr->addr());
    }
    
    return (NULL);
}

int
Vif::add_address(const VifAddr& vif_addr)
{
    if (is_my_vif_addr(vif_addr))
	return (XORP_ERROR);
    
    _addr_list.push_back(vif_addr);
    return (XORP_OK);
}

int
Vif::add_address(const IPvX& ipvx_addr, const IPvXNet& ipvxnet_subnet_addr,
		 const IPvX& ipvx_broadcast_addr, const IPvX& ipvx_peer_addr)
{
    const VifAddr vif_addr(ipvx_addr, ipvxnet_subnet_addr,
			   ipvx_broadcast_addr, ipvx_peer_addr);
    return add_address(vif_addr);
}

int
Vif::add_address(const IPvX& ipvx_addr)
{
    const VifAddr vif_addr(ipvx_addr);
    return add_address(vif_addr);
}

int
Vif::delete_address(const IPvX& ipvx_addr)
{
    list<VifAddr>::iterator iter;
    
    for (iter = _addr_list.begin(); iter != _addr_list.end(); ++iter) {
	if ((iter)->is_same_addr(ipvx_addr)) {
	    _addr_list.erase(iter);
	    return (XORP_OK);
	}
    }
    
    return (XORP_ERROR);
}

VifAddr *
Vif::find_address(const IPvX& ipvx_addr)
{
    list<VifAddr>::iterator iter;
    
    for (iter = _addr_list.begin(); iter != _addr_list.end(); ++iter) {
	if ((iter)->is_same_addr(ipvx_addr)) {
	    return &(*iter);
	}
    }
    
    return (NULL);
}

bool
Vif::is_my_addr(const IPvX& ipvx_addr) const
{
    list<VifAddr>::const_iterator iter;
    
    for (iter = _addr_list.begin(); iter != _addr_list.end(); ++iter) {
	if ((iter)->is_same_addr(ipvx_addr)) {
	    return (true);
	}
    }
    
    return (false);
}

bool
Vif::is_my_vif_addr(const VifAddr& vif_addr) const
{
    list<VifAddr>::const_iterator iter;
    
    for (iter = _addr_list.begin(); iter != _addr_list.end(); ++iter) {
	const VifAddr& tmp_vif_addr = *iter;
	if (tmp_vif_addr == vif_addr)
	    return (true);
    }
    
    return (false);
}

bool
Vif::is_same_subnet(const IPvXNet& ipvxnet) const
{
    list<VifAddr>::const_iterator iter;
    
    if (is_pim_register() || is_p2p())
	return (false);
    
    for (iter = _addr_list.begin(); iter != _addr_list.end(); ++iter) {
	if ((iter)->is_same_subnet(ipvxnet)) {
	    return (true);
	}
    }
    
    return (false);
}

bool
Vif::is_same_subnet(const IPvX& ipvx_addr) const
{
    list<VifAddr>::const_iterator iter;
    
    if (is_pim_register() || is_p2p())
	return (false);
    
    for (iter = _addr_list.begin(); iter != _addr_list.end(); ++iter) {
	if ((iter)->is_same_subnet(ipvx_addr)) {
	    return (true);
	}
    }
    
    return (false);
}

bool
Vif::is_same_p2p(const IPvX& ipvx_addr) const
{
    list<VifAddr>::const_iterator iter;
    
    if (is_pim_register() || (! is_p2p()))
	return (false);
    
    for (iter = _addr_list.begin(); iter != _addr_list.end(); ++iter) {
	if ((iter)->is_same_addr(ipvx_addr)
	    || ((iter)->peer_addr() == ipvx_addr)) {
	    return (true);
	}
    }
    
    return (false);
}
