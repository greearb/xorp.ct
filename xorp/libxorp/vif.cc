// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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


#include "xorp.h"
#include "vif.hh"

VifAddr::VifAddr(const IPvX& ipvx_addr)
    : _addr(ipvx_addr),
      _subnet_addr(ipvx_addr.af()),
      _broadcast_addr(ipvx_addr.af()),
      _peer_addr(ipvx_addr.af())
{
}

#ifdef XORP_USE_USTL
VifAddr::VifAddr()
    : _addr(AF_INET),
      _subnet_addr(AF_INET),
      _broadcast_addr(AF_INET),
      _peer_addr(AF_INET)
{
}
#endif

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
    return (_subnet_addr.contains(ipvxnet));
}

bool
VifAddr::is_same_subnet(const IPvX& ipvx_addr) const
{
    return (_subnet_addr.contains(ipvx_addr));
}

//
// Return C++ string representation of the VifAddr object in a
// human-friendy form.
//
string
VifAddr::str() const
{
    ostringstream oss;
    oss << "addr: " << _addr.str() << " subnet: " << _subnet_addr.str()
	<< " broadcast: " << _broadcast_addr.str() << " peer: " << _peer_addr.str();

    return oss.str();
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
    set_discard(false);
    set_unreachable(false);
    set_management(false);
    set_multicast_capable(false);
    set_broadcast_capable(false);
    set_underlying_vif_up(false);
    set_is_fake(false);
    set_mtu(0);
}

//
// Vif copy constructor
//
Vif::Vif(const Vif& vif) : BugCatcher(vif) {
    _name = vif.name();
    _ifname = vif.ifname();
    set_pif_index(vif.pif_index());
    set_vif_index(vif.vif_index());
    _addr_list = vif.addr_list();
    set_pim_register(vif.is_pim_register());
    set_p2p(vif.is_p2p());
    set_loopback(vif.is_loopback());
    set_discard(vif.is_discard());
    set_unreachable(vif.is_unreachable());
    set_management(vif.is_management());
    set_multicast_capable(vif.is_multicast_capable());
    set_broadcast_capable(vif.is_broadcast_capable());
    set_underlying_vif_up(vif.is_underlying_vif_up());
    set_is_fake(vif.is_fake());
    set_mtu(vif.mtu());
}

Vif& Vif::operator=(const Vif& vif) {
   _name = vif.name();
   _ifname = vif.ifname();
   set_pif_index(vif.pif_index());
   set_vif_index(vif.vif_index());
   _addr_list = vif.addr_list();
   set_pim_register(vif.is_pim_register());
   set_p2p(vif.is_p2p());
   set_loopback(vif.is_loopback());
   set_discard(vif.is_discard());
   set_unreachable(vif.is_unreachable());
   set_management(vif.is_management());
   set_multicast_capable(vif.is_multicast_capable());
   set_broadcast_capable(vif.is_broadcast_capable());
   set_underlying_vif_up(vif.is_underlying_vif_up());
   set_is_fake(vif.is_fake());
   set_mtu(vif.mtu());
   return *this;
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
	r += " ";
	r += iter->str();
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
    if (is_discard())
	r += " DISCARD";
    if (is_unreachable())
	r += " UNREACHABLE";
    if (is_management())
	r += " MANAGEMENT";
    if (is_underlying_vif_up())
	r += " UNDERLYING_VIF_UP";
    if (is_fake())
	r += " FAKE";
    r += c_format(" MTU: %u", XORP_UINT_CAST(mtu()));

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
	    && (is_discard() == other.is_discard())
	    && (is_unreachable() == other.is_unreachable())
	    && (is_management() == other.is_management())
	    && (is_multicast_capable() == other.is_multicast_capable())
	    && (is_broadcast_capable() == other.is_broadcast_capable())
	    && (is_underlying_vif_up() == other.is_underlying_vif_up())
	    && (is_fake() == other.is_fake())
	    && (mtu() == other.mtu()));
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
	if ((iter)->is_my_addr(ipvx_addr)) {
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
	if ((iter)->is_my_addr(ipvx_addr)) {
	    return &(*iter);
	}
    }

    return (NULL);
}

const VifAddr *
Vif::find_address(const IPvX& ipvx_addr) const
{
    list<VifAddr>::const_iterator iter;

    for (iter = _addr_list.begin(); iter != _addr_list.end(); ++iter) {
	if ((iter)->is_my_addr(ipvx_addr)) {
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
	if ((iter)->is_my_addr(ipvx_addr)) {
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

    if (is_pim_register())
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

    if (is_pim_register())
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
	if ((iter)->is_my_addr(ipvx_addr)
	    || ((iter)->peer_addr() == ipvx_addr)) {
	    return (true);
	}
    }

    return (false);
}
