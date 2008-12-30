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

#ident "$XORP: xorp/rib/rt_tab_base.cc,v 1.14 2008/07/23 05:11:31 pavlin Exp $"

#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "rt_tab_base.hh"

template <typename A>
RouteTable<A>::~RouteTable()
{
}

template <typename A>
void
RouteTable<A>::set_next_table(RouteTable<A>* next_table)
{
    _next_table = next_table;
}

template <typename A>
void
RouteTable<A>::replace_policytags(const IPRouteEntry<A>& route,
				  const PolicyTags& prevtags,
				  RouteTable* caller)
{
    XLOG_ASSERT(_next_table);
    UNUSED(caller);
    _next_table->replace_policytags(route, prevtags, this);
}				  


template class RouteTable<IPv4>;
typedef RouteTable<IPv4> IPv4RouteTable;

template class RouteTable<IPv6>;
typedef RouteTable<IPv6> IPv6RouteTable;

template class RouteRange<IPv4>;
template class RouteRange<IPv6>;
