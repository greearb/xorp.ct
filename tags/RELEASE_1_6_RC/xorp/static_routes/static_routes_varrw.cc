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

#ident "$XORP: xorp/static_routes/static_routes_varrw.cc,v 1.13 2008/08/06 08:26:17 abittau Exp $"

#include "static_routes_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "static_routes_varrw.hh"

StaticRoutesVarRW::StaticRoutesVarRW(StaticRoute& route)
    : _route(route), _is_ipv4(route.is_ipv4()), _is_ipv6(route.is_ipv6())
{
}

void
StaticRoutesVarRW::start_read()
{
    initialize(_route.policytags());

    if (_is_ipv4) {
	initialize(VAR_NETWORK4,
		   _ef.create(ElemIPv4Net::id,
			      _route.network().str().c_str()));
	initialize(VAR_NEXTHOP4,
		   _ef.create(ElemIPv4NextHop::id,
			      _route.nexthop().str().c_str()));
	
	initialize(VAR_NETWORK6, NULL);
	initialize(VAR_NEXTHOP6, NULL);
    }

    if (_is_ipv6) {
	initialize(VAR_NETWORK6,
		   _ef.create(ElemIPv6Net::id,
			      _route.network().str().c_str()));
	initialize(VAR_NEXTHOP6,
		   _ef.create(ElemIPv6NextHop::id,
			      _route.nexthop().str().c_str()));

	initialize(VAR_NETWORK4, NULL);
	initialize(VAR_NEXTHOP4, NULL);
    }

    ostringstream oss;

    oss << _route.metric();

    initialize(VAR_METRIC, _ef.create(ElemU32::id, oss.str().c_str()));
}

void
StaticRoutesVarRW::single_write(const Id& /* id */, const Element& /* e */)
{
}

Element*
StaticRoutesVarRW::single_read(const Id& /* id */)
{
    XLOG_UNREACHABLE();
}
