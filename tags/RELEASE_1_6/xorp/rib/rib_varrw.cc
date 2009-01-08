// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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

#ident "$XORP: xorp/rib/rib_varrw.cc,v 1.16 2008/10/02 21:58:11 bms Exp $"

#include "rib_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "rib_varrw.hh"

template <class A>
RIBVarRW<A>::RIBVarRW(IPRouteEntry<A>& route)
    : _route(route)
{
}

template <class A>
void
RIBVarRW<A>::start_read()
{
    initialize(_route.policytags());

    read_route_nexthop(_route);

    ostringstream oss;

    oss << _route.metric();

    initialize(VAR_METRIC, _ef.create(ElemU32::id, oss.str().c_str()));
}

template <>
void
RIBVarRW<IPv4>::read_route_nexthop(IPRouteEntry<IPv4>& route)
{
    initialize(VAR_NETWORK4,
	       _ef.create(ElemIPv4Net::id, route.net().str().c_str()));
    initialize(VAR_NEXTHOP4, _ef.create(ElemIPv4NextHop::id,
               route.nexthop_addr().str().c_str()));
    initialize(VAR_NETWORK6, NULL);
    initialize(VAR_NEXTHOP6, NULL);
}

template <>
void
RIBVarRW<IPv6>::read_route_nexthop(IPRouteEntry<IPv6>& route)
{
    initialize(VAR_NETWORK6,
	       _ef.create(ElemIPv6Net::id, route.net().str().c_str()));
    initialize(VAR_NEXTHOP6, _ef.create(ElemIPv6NextHop::id,
               route.nexthop_addr().str().c_str()));

    initialize(VAR_NETWORK4, NULL);
    initialize(VAR_NEXTHOP4, NULL);
}

template <class A>
void
RIBVarRW<A>::single_write(const Id& /* id */, const Element& /* e */)
{
}

template <class A>
Element*
RIBVarRW<A>::single_read(const Id& /* id */)
{
    XLOG_UNREACHABLE();

    return 0;
}

template class RIBVarRW<IPv4>;
template class RIBVarRW<IPv6>;
