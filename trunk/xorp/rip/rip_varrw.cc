// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/rip/rip_varrw.cc,v 1.11 2008/01/04 03:17:32 pavlin Exp $"

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
	_route.set_policytags(e);
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
	return;
    }
}

#ifdef INSTANTIATE_IPV4

template <>
bool
RIPVarRW<IPv4>::write_nexthop(const Id& id, const Element& e)
{
    if (id == VAR_NEXTHOP4 && e.type() == ElemIPv4::id) {
	const ElemIPv4* v4 = dynamic_cast<const ElemIPv4*>(&e);

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
    initialize(VAR_NEXTHOP4, new ElemIPv4(route.nexthop()));
    
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
    if (id == VAR_NEXTHOP6 && e.type() == ElemIPv6::id) {
	const ElemIPv6* v6 = dynamic_cast<const ElemIPv6*>(&e);

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
    initialize(VAR_NEXTHOP6, new ElemIPv6(route.nexthop()));
    
    initialize(VAR_NETWORK4, NULL);
    initialize(VAR_NEXTHOP4, NULL);
}

template class RIPVarRW<IPv6>;

#endif // INSTANTIATE_IPV6
