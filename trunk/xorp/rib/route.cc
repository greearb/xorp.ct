// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2007 International Computer Science Institute
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

#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/c_format.hh"

#include "rib.hh"
#include "route.hh"

RouteEntry::RouteEntry(RibVif* vif, NextHop* nexthop, const Protocol& protocol,
		       uint16_t metric)
    : _vif(vif), _nexthop(nexthop), _protocol(protocol),
      _admin_distance(UNKNOWN_ADMIN_DISTANCE), _metric(metric)
{
    if (_vif != NULL)
	_vif->incr_usage_counter();
}

RouteEntry::~RouteEntry()
{
    if (_vif != NULL)
	_vif->decr_usage_counter();
}

template <class A>
string
IPRouteEntry<A>::str() const
{
    string dst = (_net.is_valid()) ? _net.str() : string("NULL");
    string vif = (_vif) ? string(_vif->name()) : string("NULL");
    return string("Dst: ") + dst + string(" Vif: ") + vif +
	string(" NextHop: ") + _nexthop->str() +
	string(" Metric: ") + c_format("%d", _metric) +
	string(" Protocol: ") + _protocol.name() +
	string(" PolicyTags: ") + _policytags.str();
}

template class IPRouteEntry<IPv4>;
template class IPRouteEntry<IPv6>;
