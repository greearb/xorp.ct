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

#ident "$XORP$"

#include "vrrp_module.h"
#include "libxorp/xlog.h"
#include "vrrp_vif.hh"

VRRPVif::VRRPVif(const string& ifname, const string& vifname) 
	    : _ifname(ifname),
	      _vifname(vifname),
	      _ready(false)
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
    _ready = ready;

    for (VRRPS::iterator i = _vrrps.begin(); i != _vrrps.end(); ++i) {
	VRRP* v = i->second;

	if (_ready) {
	    v->check_ownership();
	    v->start();
	} else
	    v->stop();
    }
}

const IPv4&
VRRPVif::addr() const
{
    XLOG_ASSERT(_ips.size());

    // XXX we should use first configured address - not the lowest.
    return *(_ips.begin());
}
