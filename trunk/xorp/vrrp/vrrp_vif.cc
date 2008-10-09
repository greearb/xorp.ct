// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008 XORP, Inc.
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

#ident "$XORP: xorp/vrrp/vrrp_vif.cc,v 1.3 2008/10/09 17:47:49 abittau Exp $"

#include "vrrp_module.h"
#include "libxorp/xlog.h"
#include "vrrp_vif.hh"
#include "vrrp_target.hh"
#include "vrrp_exception.hh"

VRRPVif::VRRPVif(VRRPTarget& vt, const string& ifname, const string& vifname)
	    : _vt(vt),
	      _ifname(ifname),
	      _vifname(vifname),
	      _ready(false),
	      _join(0)
{
}

VRRPVif::~VRRPVif()
{
    for (VRRPS::iterator i = _vrrps.begin(); i != _vrrps.end(); ++i)
	delete i->second;
}

bool
VRRPVif::own(const IPv4& addr)
{
    return _ips.find(addr) != _ips.end();
}

VRRP*
VRRPVif::find_vrid(uint32_t vrid)
{
    VRRPS::iterator i = _vrrps.find(vrid);

    if (i == _vrrps.end())
	return NULL;

    return i->second;
}

void
VRRPVif::add_vrid(uint32_t vrid)
{
    XLOG_ASSERT(find_vrid(vrid) == NULL);

    _vrrps[vrid] = new VRRP(*this, vrid);
}

void
VRRPVif::delete_vrid(uint32_t vrid)
{
    VRRP* v = find_vrid(vrid);
    XLOG_ASSERT(v);

    _vrrps.erase(vrid);

    delete v;
}

bool
VRRPVif::ready() const
{
    return _ready;
}

void
VRRPVif::configure(const IfMgrIfTree& conf)
{
    // check interface
    const IfMgrIfAtom* ifa = conf.find_interface(_ifname);
    if (!is_enabled(ifa))
	return;

    // check vif
    const IfMgrVifAtom* vifa = ifa->find_vif(_vifname);
    if (!is_enabled(vifa))
	return;

    // check addresses
    _ips.clear();

    const IfMgrVifAtom::IPv4Map& addrs = vifa->ipv4addrs();
    for (IfMgrVifAtom::IPv4Map::const_iterator i = addrs.begin();
	 i != addrs.end(); ++i) {

	const IfMgrIPv4Atom& addr = i->second;

	if (addr.enabled())
	    _ips.insert(addr.addr());
    }

    if (_ips.empty()) {
	set_ready(false);
	return;
    }

    set_ready(true);
}

template <class T>
bool
VRRPVif::is_enabled(const T* obj)
{
    if (obj && obj->enabled())
	return true;

    set_ready(false);

    return false;
}

void
VRRPVif::set_ready(bool ready)
{
    if (ready)
	_ready = ready;

    for (VRRPS::iterator i = _vrrps.begin(); i != _vrrps.end(); ++i) {
	VRRP* v = i->second;

	if (ready) {
	    v->check_ownership();
	    v->start();
	} else
	    v->stop();
    }

    _ready = ready;
}

const IPv4&
VRRPVif::addr() const
{
    XLOG_ASSERT(_ips.size());

    // XXX we should use first configured address - not the lowest.
    return *(_ips.begin());
}

void
VRRPVif::send(const Mac& src, const Mac& dst, uint32_t ether,
	      const PAYLOAD& payload)
{
    XLOG_ASSERT(ready());

    _vt.send(_ifname, _vifname, src, dst, ether, payload);
}

void
VRRPVif::join_mcast()
{
    _join++;
    XLOG_ASSERT(_join);

    if (_join == 1)
	_vt.join_mcast(_ifname, _vifname);
}

void
VRRPVif::leave_mcast()
{
    XLOG_ASSERT(_join);
    _join--;

    if (_join > 0)
	return;

    _vt.leave_mcast(_ifname, _vifname);

    // paranoia
    int cnt = 0;
    for (VRRPS::iterator i = _vrrps.begin(); i != _vrrps.end(); ++i) {
	VRRP* v = i->second;

	if (v->running())
	    XLOG_ASSERT(++cnt == 1);
    }
}

void
VRRPVif::recv(const IPv4& from, const PAYLOAD& payload)
{
    try {
	const VRRPHeader& vh = VRRPHeader::assign(payload);

	VRRP* v = find_vrid(vh.vh_vrid);
	if (!v) {
	    XLOG_WARNING("Cannot find VRID %d", vh.vh_vrid);
	    return;
	}

	v->recv(from, vh);

    } catch (const VRRPException& e) {
	XLOG_WARNING("VRRP packet error: %s", e.str().c_str());
    }
}

void
VRRPVif::add_mac(const Mac& mac)
{
    // XXX mac operations are per physical interface.  We musn't have multiple
    // vifs tweaking the mac of the same physical inteface.  This is a
    // rudimentary (and very conservative) way to check this.  Now all of VRRP
    // works on a "per vif" basis, but that may be wrong - it might need to work
    // on a "per if" basis.  I still don't quite know the relationship, in XORP
    // terms, between if and vif.  (If it's like Linux's virtual IPs and eth0:X
    // notation, then a vif isn't much more than an extra IP, and we should be
    // working on a "per if" basis.)  -sorbo
    XLOG_ASSERT(_ifname == _vifname);

    _vt.add_mac(_ifname, mac);
}

void
VRRPVif::delete_mac(const Mac& mac)
{
    // XXX see add mac
    XLOG_ASSERT(_ifname == _vifname);

    _vt.delete_mac(_ifname, mac);
}
