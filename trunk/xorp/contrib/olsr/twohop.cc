// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP$"

#include "olsr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include "olsr.hh"
#include "link.hh"
#include "neighbor.hh"
#include "neighborhood.hh"
#include "twohop.hh"

TwoHopNeighbor::TwoHopNeighbor(EventLoop& ev, Neighborhood* parent,
			       const OlsrTypes::TwoHopNodeID id,
			       const IPv4& main_addr,
			       const OlsrTypes::TwoHopLinkID tlid)
 : _ev(ev),
   _parent(parent),
   _id(id),
   _main_addr(main_addr),
   _is_strict(false),
   _coverage(0),
   _reachability(0)
{
    add_twohop_link(tlid);
}

void
TwoHopNeighbor::add_twohop_link(const OlsrTypes::TwoHopLinkID tlid)
{
    debug_msg("TwoHopLinkID %u\n", XORP_UINT_CAST(tlid));

    // Invariant: Do not insert a link more than once.
    XLOG_ASSERT(0 == _twohop_links.count(tlid));

    _twohop_links.insert(tlid);
}

bool
TwoHopNeighbor::delete_twohop_link(const OlsrTypes::TwoHopLinkID tlid)
{
    debug_msg("TwoHopLinkID %u\n", XORP_UINT_CAST(tlid));

    // Invariant: Do not erase a non-existent link.
    XLOG_ASSERT(0 != _twohop_links.count(tlid));

    _twohop_links.erase(tlid);

    return _twohop_links.empty();
}

size_t
TwoHopNeighbor::delete_all_twohop_links()
{
    size_t deleted_count = 0;

    // Do not erase elements as Neighborhood will call back into
    // TwoHopNeighbor::delete_twohop_link(). Maintain a separate
    // iterator as jj will be invalidated by that method invocation.
    set<OlsrTypes::TwoHopLinkID>::iterator ii, jj;
    for (ii = _twohop_links.begin(); ii != _twohop_links.end(); ) {
	jj = ii++;
	_parent->delete_twohop_link((*jj));
	++deleted_count;
    }

    return deleted_count;
}

void
TwoHopNeighbor::add_covering_mpr(const OlsrTypes::NeighborID nid)
{
    // We need only update a counter if MPR state is computed
    // for N2(all I), rather than for N2(specific I).
    _coverage++;

    UNUSED(nid);
}

void
TwoHopNeighbor::withdraw_covering_mpr(const OlsrTypes::NeighborID nid)
{
    --_coverage;

    UNUSED(nid);
}

void
TwoHopNeighbor::reset_covering_mprs()
{
    _coverage = 0;
}

TwoHopLink::TwoHopLink(EventLoop& ev, Neighborhood* parent,
		       OlsrTypes::TwoHopLinkID tlid, Neighbor* nexthop,
		       const TimeVal& vtime)
 : _ev(ev), _parent(parent), _id(tlid), _nexthop(nexthop), _destination(0)
{
    update_timer(vtime);
}

void
TwoHopLink::update_timer(const TimeVal& vtime)
{
    if (_expiry_timer.scheduled())
	_expiry_timer.clear();

    _expiry_timer = _ev.new_oneoff_after(vtime,
	callback(this, &TwoHopLink::event_dead));
}

void
TwoHopLink::event_dead()
{
    debug_msg("called\n");

    _parent->event_twohop_link_dead_timer(id());
}
