// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/rip/rip_varrw.cc,v 1.2 2004/09/18 00:00:31 pavlin Exp $"

#include "rip_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "policy/common/policy_utils.hh"

#include "rip_varrw.hh"

template <class A>
RIPVarRW<A>::RIPVarRW(RouteEntry<A>& route)
    : _route(route)
{
    initialize("policytags", _route.policytags().element());

    read_route_nexthop(route);

    initialize("metric", new ElemU32(route.cost()));
    initialize("tag", new ElemU32(route.tag()));
}

template <class A>
void
RIPVarRW<A>::single_start()
{
}

template <class A>
void
RIPVarRW<A>::single_write(const string& id, const Element& e)
{
    if (id == "policytags") {
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

    if (id == "metric") {
	XLOG_ASSERT(u32 != NULL);

	_route.set_cost(u32->val());
	return;
    }
    if (id == "tag") {
	XLOG_ASSERT(u32 != NULL);

	_route.set_tag(u32->val());
	return;
    }
}

template <class A>
void
RIPVarRW<A>::single_end()
{
}

#ifdef INSTANTIATE_IPV4

template <>
bool
RIPVarRW<IPv4>::write_nexthop(const string& id, const Element& e)
{
    if (id == "nexthop4" && e.type() == ElemIPv4::id) {
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
    initialize("network4", new ElemIPv4Net(route.net()));
    initialize("nexthop4", new ElemIPv4(route.nexthop()));
    
    initialize("network6", NULL);
    initialize("nexthop6", NULL);
}

template class RIPVarRW<IPv4>;
#endif // INSTANTIATE_IPV4


#ifdef INSTANTIATE_IPV6
template <>
bool
RIPVarRW<IPv6>::write_nexthop(const string& id, const Element& e)
{
    if (id == "nexthop6" && e.type() == ElemIPv6::id) {
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
    initialize("network6", new ElemIPv6Net(route.net()));
    initialize("nexthop6", new ElemIPv6(route.nexthop()));
    
    initialize("network4", NULL);
    initialize("nexthop4", NULL);
}


template class RIPVarRW<IPv6>;

#endif // INSTANTIATE_IPV6
