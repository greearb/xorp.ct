// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2007-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
//
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/c_format.hh"

#include "rib.hh"
#include "route.hh"

template<class A>
RouteEntry<A>::RouteEntry(RibVif<A>* vif, const Protocol& protocol,
		       uint32_t metric, const PolicyTags& policytags, const IPNet<A>& net)
    : _vif(vif), _protocol(protocol),
      _admin_distance(UNKNOWN_ADMIN_DISTANCE), _metric(metric),
      _policytags(policytags), _net(net)
{
    if (_vif != NULL)
	_vif->incr_usage_counter();
}

template<class A>
RouteEntry<A>::RouteEntry(RibVif<A>* vif, const Protocol& protocol,
		       uint32_t metric, const IPNet<A>& net)
    : _vif(vif), _protocol(protocol),
      _admin_distance(UNKNOWN_ADMIN_DISTANCE), _metric(metric), _net(net)
{
    if (_vif != NULL)
	_vif->incr_usage_counter();
}

template<class A>
RouteEntry<A>::~RouteEntry()
{
    if (_vif != NULL)
	_vif->decr_usage_counter();
}

template <class A>
string
IPRouteEntry<A>::str() const
{
    string dst = (RouteEntry<A>::_net.is_valid()) ? RouteEntry<A>::_net.str() : string("NULL");
    string vif = (RouteEntry<A>::_vif) ? string(RouteEntry<A>::_vif->name()) : string("NULL");
    return string("Dst: ") + dst + string(" Vif: ") + vif +
	string(" NextHop: ") + _nexthop->str() +
	string(" Metric: ") + c_format("%d", RouteEntry<A>::_metric) +
	string(" Protocol: ") + RouteEntry<A>::_protocol.name() +
	string(" PolicyTags: ") + RouteEntry<A>::_policytags.str();
}

template class IPRouteEntry<IPv4>;
template class IPRouteEntry<IPv6>;

