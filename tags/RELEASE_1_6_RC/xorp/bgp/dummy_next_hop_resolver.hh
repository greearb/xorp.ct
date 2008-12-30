// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/bgp/dummy_next_hop_resolver.hh,v 1.13 2008/07/23 05:09:32 pavlin Exp $

#ifndef __BGP_DUMMY_NEXT_HOP_RESOLVER_HH__
#define __BGP_DUMMY_NEXT_HOP_RESOLVER_HH__

#include "next_hop_resolver.hh"


template<class A>
class DummyNextHopResolver : public NextHopResolver<A> {
public:
    DummyNextHopResolver(EventLoop& eventloop, BGPMain& bgp);

    virtual ~DummyNextHopResolver();

    /**
     * @short lookup next hop.
     *
     * @param nexthop. Next hop.
     * @param resolvable. Is this route resolvable.
     * @param metric. If this route is resolvable the metric of this
     * route.
     * @return True if this next hop is found.
     *
     */
    bool lookup(const A nexthop, bool& resolvable, uint32_t& metric) const;

    void set_nexthop_metric(const A nexthop, uint32_t metric);
    void unset_nexthop_metric(const A nexthop);
private:
    DecisionTable<A> *_decision;
    map <A, uint32_t> _metrics;
};

#endif // __BGP_DUMMY_NEXT_HOP_RESOLVER_HH__
