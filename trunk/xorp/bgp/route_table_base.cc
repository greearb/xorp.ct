// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/route_table_base.cc,v 1.4 2004/02/25 05:03:05 atanu Exp $"

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
BGPRouteTable<A>::route_dump(const InternalMessage<A> &rtmsg, 
			     BGPRouteTable<A> */*caller*/,
			     const PeerHandler *peer)
{
    XLOG_ASSERT(_next_table != NULL);
    return _next_table->route_dump(rtmsg, (BGPRouteTable<A>*)this, peer);
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

template class BGPRouteTable<IPv4>;
template class BGPRouteTable<IPv6>;
