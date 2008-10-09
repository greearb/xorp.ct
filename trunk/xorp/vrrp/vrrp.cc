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

#include <sstream>

#include "vrrp_module.h"
#include "vrrp.hh"

VRRPKey::VRRPKey(const string& ifname, const string& vifname, uint32_t id)
	    : _ifname(ifname), _vifname(vifname), _vrid(id)
{
}

bool
VRRPKey::operator==(const VRRPKey& rhs) const
{
    return (_ifname == rhs._ifname)
           && (_vifname == rhs._vifname)
	   && (_vrid == rhs._vrid);
}

bool
VRRPKey::operator<(const VRRPKey& rhs) const
{
    if (_ifname < rhs._ifname)
	return true;
    else if (_ifname > rhs._ifname)
	return false;

    if (_vifname < rhs._vifname)
	return true;
    else if (_vifname > rhs._vifname)
	return false;

    return _vrid < rhs._vrid;
}

string
VRRPKey::str() const
{
    ostringstream oss;

    oss << "interface " << _ifname 
        << " vif " << _vifname 
	<< " vrid " << _vrid;

    return oss.str();
}

VRRP::VRRP(const VRRPKey& key) : _key(key),
				 _priority(100),
				 _interval(1),
				 _preempt(true),
				 _disable(true)
{
}

void
VRRP::set_priority(uint32_t priority)
{
    _priority = priority;
}

void
VRRP::set_interval(uint32_t interval)
{
    _interval = interval;
}

void
VRRP::set_preempt(bool preempt)
{
    _preempt = preempt;
}

void
VRRP::set_disable(bool disable)
{
    _disable = disable;
}

void
VRRP::add_ip(const IPv4& ip)
{
    _ips.insert(ip);
}

void
VRRP::delete_ip(const IPv4& ip)
{
    _ips.erase(ip);
}
