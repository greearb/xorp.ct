// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/rip/rip_varrw.cc,v 1.14 2008/08/06 08:26:17 abittau Exp $"

#include "rip_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "policy/common/policy_utils.hh"
#include "rip_varrw.hh"

template <class A>
RIPVarRW<A>::RIPVarRW(RouteEntry<A>& route)
    : _route(route)
{
}

template <class A>
void
RIPVarRW<A>::start_read()
{
    initialize(VAR_POLICYTAGS, _route.policytags().element());

    read_route_nexthop(_route);

    initialize(VAR_METRIC, new ElemU32(_route.cost()));

    // XXX which tag wins?
    ElemU32* e = dynamic_cast<ElemU32*>(_route.policytags().element_tag());
    if (e->val())
	_route.set_tag(e->val());

    delete e;

    initialize(VAR_TAG, new ElemU32(_route.tag()));
}

template <class A>
Element*
RIPVarRW<A>::single_read(const Id& /* id */)
{
    XLOG_UNREACHABLE();

    return 0;
}

template <class A>
void
RIPVarRW<A>::single_write(const Id& id, const Element& e)
{
    if (id == VAR_POLICYTAGS) {
	_route.policytags().set_ptags(e);
	return;
    }

    if (write_nexthop(id, e))
	return;

    const ElemU32* u32 = NULL;
    if (e.type() == ElemU32::id) {
	u32 = dynamic_cast<const ElemU32*>(&e);
	XLOG_ASSERT(u32 != NULL);
    }

    if (id == VAR_METRIC) {
	XLOG_ASSERT(u32 != NULL);

	_route.set_cost(u32->val());
	return;
    }
    if (id == VAR_TAG) {
	XLOG_ASSERT(u32 != NULL);

	_route.set_tag(u32->val());
	_route.policytags().set_tag(e);
	return;
    }
}

#ifdef INSTANTIATE_IPV4

template <>
bool
RIPVarRW<IPv4>::write_nexthop(const Id& id, const Element& e)
{
    if (id == VAR_NEXTHOP4 && e.type() == ElemIPv4NextHop::id) {
	const ElemIPv4NextHop* v4 = dynamic_cast<const ElemIPv4NextHop*>(&e);

	XLOG_ASSERT(v4 != NULL);

	IPv4 nh(v4->val());

	_route.set_nexthop(nh);
	return true;
    }
    return false;
}

template <>
void
RIPVarRW<IPv4>::read_route_nexthop(RouteEntry<IPv4>& route)
{
    initialize(VAR_NETWORK4, new ElemIPv4Net(route.net()));
    initialize(VAR_NEXTHOP4, new ElemIPv4NextHop(route.nexthop()));
    
    initialize(VAR_NETWORK6, NULL);
    initialize(VAR_NEXTHOP6, NULL);
}

template class RIPVarRW<IPv4>;
#endif // INSTANTIATE_IPV4

#ifdef INSTANTIATE_IPV6
template <>
bool
RIPVarRW<IPv6>::write_nexthop(const Id& id, const Element& e)
{
    if (id == VAR_NEXTHOP6 && e.type() == ElemIPv6NextHop::id) {
	const ElemIPv6NextHop* v6 = dynamic_cast<const ElemIPv6NextHop*>(&e);

	XLOG_ASSERT(v6 != NULL);

	IPv6 nh(v6->val());

	_route.set_nexthop(nh);
	return true;
    }
    return false;
}

template <>
void
RIPVarRW<IPv6>::read_route_nexthop(RouteEntry<IPv6>& route)
{
    initialize(VAR_NETWORK6, new ElemIPv6Net(route.net()));
    initialize(VAR_NEXTHOP6, new ElemIPv6NextHop(route.nexthop()));
    
    initialize(VAR_NETWORK4, NULL);
    initialize(VAR_NEXTHOP4, NULL);
}

template class RIPVarRW<IPv6>;

#endif // INSTANTIATE_IPV6
