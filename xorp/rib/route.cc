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
RouteEntry<A>::RouteEntry(RibVif<A>* vif, const Protocol* protocol,
		       uint32_t metric, const PolicyTags& policytags, const IPNet<A>& net, uint16_t admin_distance)
    : _vif(vif), _protocol(protocol),
      _admin_distance(admin_distance), _metric(metric),
      _policytags(new PolicyTags(policytags)), _net(net)
{
    if (_vif != NULL)
	_vif->incr_usage_counter();
}

template<class A>
RouteEntry<A>::RouteEntry(RibVif<A>* vif, const Protocol* protocol,
			uint32_t metric, const IPNet<A>& net, uint16_t admin_distance)
    : _vif(vif), _protocol(protocol),
      _admin_distance(admin_distance), _metric(metric),
      _policytags(new PolicyTags()), _net(net)
{
    if (_vif != NULL)
	_vif->incr_usage_counter();
}

template<class A>
RouteEntry<A>::RouteEntry(RibVif<A>* vif, const Protocol* protocol,
			uint32_t metric, smart_ptr<PolicyTags>& policytags,
			const IPNet<A>& net, uint16_t admin_distance)
    : _vif(vif), _protocol(protocol),
      _admin_distance(admin_distance), _metric(metric),
      _policytags(policytags), _net(net)
{
    if (_vif != NULL)
	_vif->incr_usage_counter();
}

template<class A>
RouteEntry<A>::RouteEntry(const RouteEntry& r) {
    _vif = r._vif;
    if (_vif)
	_vif->incr_usage_counter();
    _protocol = r._protocol;
    _admin_distance = r._admin_distance;
    _metric = r._metric;
    _policytags = r._policytags;
    _net = r._net;
}

template<class A>
RouteEntry<A>& RouteEntry<A>::operator=(const RouteEntry<A>& r) {
    if (this == &r)
	return *this;
    if (_vif)
	_vif->decr_usage_counter();
    _vif = r._vif;
    if (_vif)
	_vif->incr_usage_counter();
    _protocol = r._protocol;
    _admin_distance = r._admin_distance;
    _metric = r._metric;
    _policytags = r._policytags;
    _net = r._net;
    return *this;
}

template<class A>
RouteEntry<A>::~RouteEntry()
{
    if (_vif != NULL)
	_vif->decr_usage_counter();
}

template class RouteEntry<IPv4>;
template class RouteEntry<IPv6>;

template <class A>
string
IPRouteEntry<A>::str() const
{
    string dst = (RouteEntry<A>::_net.is_valid()) ? RouteEntry<A>::_net.str() : string("NULL");
    string vif = (RouteEntry<A>::_vif) ? string(RouteEntry<A>::_vif->name()) : string("NULL");
    return string("Dst: ") + dst + string(" Vif: ") + vif +
	string(" NextHop: ") + _nexthop->str() +
	string(" Metric: ") + c_format("%d", RouteEntry<A>::_metric) +
	string(" Protocol: ") + RouteEntry<A>::_protocol->name() +
	string(" PolicyTags: ") + RouteEntry<A>::_policytags->str();
}

template<class A>
void*
IPRouteEntry<A>::operator new(size_t/* size*/)
{
    return memory_pool().alloc();
}

template<class A>
void
IPRouteEntry<A>::operator delete(void* ptr)
{
    memory_pool().free(ptr);
}

template<class A>
inline
MemoryPool<IPRouteEntry<A> >&
IPRouteEntry<A>::memory_pool()
{
    static MemoryPool<IPRouteEntry<A> > mp;
    return mp;
}

template<class A>
void*
ResolvedIPRouteEntry<A>::operator new(size_t/* size*/)
{
    return memory_pool().alloc();
}

template<class A>
void
ResolvedIPRouteEntry<A>::operator delete(void* ptr)
{
    memory_pool().free(ptr);
}

template<class A>
inline
MemoryPool<ResolvedIPRouteEntry<A> >&
ResolvedIPRouteEntry<A>::memory_pool()
{
    static MemoryPool<ResolvedIPRouteEntry<A> > mp;
    return mp;
}

template<class A>
void*
UnresolvedIPRouteEntry<A>::operator new(size_t/* size*/)
{
    return memory_pool().alloc();
}

template<class A>
void
UnresolvedIPRouteEntry<A>::operator delete(void* ptr)
{
    memory_pool().free(ptr);
}

template<class A>
inline
MemoryPool<UnresolvedIPRouteEntry<A> >&
UnresolvedIPRouteEntry<A>::memory_pool()
{
    static MemoryPool<UnresolvedIPRouteEntry<A> > mp;
    return mp;
}


template<class A>
IPRouteEntry<A>::IPRouteEntry(const IPRouteEntry<A>& r) : RouteEntry<A>(r) {
    _nexthop = r._nexthop;
}

template<class A>
IPRouteEntry<A>& IPRouteEntry<A>::operator=(const IPRouteEntry<A>& r) {
    if (this == &r)
	return *this;
    RouteEntry<A>::operator=(r);
    _nexthop = r._nexthop;
    return *this;
}

template<class A>
ResolvedIPRouteEntry<A>::ResolvedIPRouteEntry(const ResolvedIPRouteEntry<A>& r) : IPRouteEntry<A>(r) {
    _resolving_parent = r._resolving_parent;
    _egp_parent = r._egp_parent;
    _backlink = r._backlink;
}

template<class A>
ResolvedIPRouteEntry<A>& ResolvedIPRouteEntry<A>::operator=(const ResolvedIPRouteEntry<A>& r) {
    if (this == &r)
	return *this;
    IPRouteEntry<A>::operator=(r);
    _resolving_parent = r._resolving_parent;
    _egp_parent = r._egp_parent;
    _backlink = r._backlink;
    return *this;
}


template class IPRouteEntry<IPv4>;
template class IPRouteEntry<IPv6>;

template class ResolvedIPRouteEntry<IPv4>;
template class ResolvedIPRouteEntry<IPv6>;

template class UnresolvedIPRouteEntry<IPv4>;
template class UnresolvedIPRouteEntry<IPv6>;

