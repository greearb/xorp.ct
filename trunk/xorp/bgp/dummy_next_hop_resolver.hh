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

// $XORP: xorp/bgp/dummy_next_hop_resolver.hh,v 1.4 2003/04/02 19:44:44 mjh Exp $

#ifndef __BGP_DUMMY_NEXT_HOP_RESOLVER_HH__
#define __BGP_DUMMY_NEXT_HOP_RESOLVER_HH__

#include "next_hop_resolver.hh"


template<class A>
class DummyNextHopResolver : public NextHopResolver<A> {
public:
    DummyNextHopResolver(EventLoop& event_loop);

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
