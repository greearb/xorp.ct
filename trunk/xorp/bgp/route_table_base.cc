// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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



// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "route_table_base.hh"

#define DEBUG_ROUTE_TABLE

template<class A>
BGPRouteTable<A>::BGPRouteTable(string tablename, Safi safi)
    : _tablename(tablename), _safi(safi)
{
    _next_table = NULL;
    debug_msg("Creating table %s\n", _tablename.c_str());
}

template<class A>
BGPRouteTable<A>::~BGPRouteTable()
{}

template<class A>
bool
BGPRouteTable<A>::dump_next_route(DumpIterator<A>& dump_iter)
{
    XLOG_ASSERT(_parent != NULL);
    return _parent->dump_next_route(dump_iter);
}

template<class A>
int
BGPRouteTable<A>::route_dump(InternalMessage<A> &rtmsg, 
			     BGPRouteTable<A> */*caller*/,
			     const PeerHandler *peer)
{
    XLOG_ASSERT(_next_table != NULL);
    return _next_table->route_dump(rtmsg, (BGPRouteTable<A>*)this, peer);
}

template<class A>
void 
BGPRouteTable<A>::wakeup()
{
    XLOG_ASSERT(_next_table != NULL);
    _next_table->wakeup();
}

template<class A>
void
BGPRouteTable<A>::igp_nexthop_changed(const A& bgp_nexthop)
{
    XLOG_ASSERT(_parent != NULL);
    return _parent->igp_nexthop_changed(bgp_nexthop);
}

template<class A>
void
BGPRouteTable<A>::peering_went_down(const PeerHandler *peer, uint32_t genid,
				    BGPRouteTable<A> *caller)
{
    debug_msg("%s\n", _tablename.c_str());

    XLOG_ASSERT(_parent == caller);
    XLOG_ASSERT(_next_table != NULL);
    _next_table->peering_went_down(peer, genid, this);
}

template<class A>
void
BGPRouteTable<A>::peering_down_complete(const PeerHandler *peer, 
					uint32_t genid,
					BGPRouteTable<A> *caller) {
    XLOG_ASSERT(_parent == caller);
    XLOG_ASSERT(_next_table != NULL);
    _next_table->peering_down_complete(peer, genid, this);
}

template<class A>
void
BGPRouteTable<A>::peering_came_up(const PeerHandler *peer, 
					uint32_t genid,
					BGPRouteTable<A> *caller) {
    XLOG_ASSERT(_parent == caller);
    if (_next_table)
	_next_table->peering_came_up(peer, genid, this);
}

template class BGPRouteTable<IPv4>;
template class BGPRouteTable<IPv6>;
