// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

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

bool
IsLinkSymmetricPred::operator()(OlsrTypes::LogicalLinkID linkid)
{
    try {
	bool result = _nh->get_logical_link(linkid)->is_sym();
	debug_msg("link %u is %s\n",
		  XORP_UINT_CAST(linkid),
		  bool_c_str(result));
	return result;
    } catch (...) {
	return false;
    }
}

bool
IsTwoHopLinkStrictPred::operator()(OlsrTypes::TwoHopLinkID tlid)
{
    try {
	return _nh->get_twohop_link(tlid)->destination()->is_strict();
    } catch (...) {
	return false;
    }
}

Neighbor::Neighbor(EventLoop& ev,
		   Neighborhood* parent,
		   const OlsrTypes::NeighborID nid,
		   const IPv4& main_addr,
		   const OlsrTypes::LogicalLinkID linkid)
 : _eventloop(ev),
   _parent(parent),
   _id(nid),
   _main_addr(main_addr),
   _is_mpr(false),
   _is_sym(false),
   _willingness(OlsrTypes::WILL_NEVER),
   _degree(0),
   _is_advertised(false)
{
    // Invariant: A neighbor must be created with a newly created link.
    XLOG_ASSERT(OlsrTypes::UNUSED_NEIGHBOR_ID ==
		_parent->get_logical_link(linkid)->neighbor_id());

    update_link(linkid);

    // Ensure that the neighbor's initial MPR candidacy is updated.
    update_cand_mpr(false);
}

string Neighbor::toStringBrief() {
    ostringstream oss;
    oss << id() << "-(" << main_addr().str() << ")";
    return oss.str();
}


OlsrTypes::NeighborType
Neighbor::neighbor_type() const
{
    if (is_mpr())
	return OlsrTypes::MPR_NEIGH;
    else if (is_sym())
	return OlsrTypes::SYM_NEIGH;

    return OlsrTypes::NOT_NEIGH;
}

void
Neighbor::recount_degree()
{
    _degree = count_if(_twohop_links.begin(), _twohop_links.end(),
		       IsTwoHopLinkStrictPred(_parent));
}

bool
Neighbor::is_cand_mpr()
{
#if 0
    // A neighbor which will always forward MUST be an MPR, even
    // if it has no two-hop links.
    // XXX If this predicate always holds, we can't tell when a
    // change in degree would cause a persistent MPR to be considered
    // as an MPR candidate.
    if (_willingness == OlsrTypes::WILL_ALWAYS)
	return true;
#endif

    // A neighbor which will never forward MUST NOT be an MPR.
    if (_willingness == OlsrTypes::WILL_NEVER)
	return false;

    // A neighbor with degree > 0 is an MPR.
    return degree() > 0;
}

bool
Neighbor::update_cand_mpr(bool was_cand_mpr)
{
    // If willingness is neither WILL_ALWAYS nor WILL_NEVER,
    // a recount of degree is necessary.
    // Degree is never used for MPR decisions when these conditions hold,
    // so update it lazily.
    if (_willingness != OlsrTypes::WILL_ALWAYS ||
	_willingness != OlsrTypes::WILL_NEVER)
	recount_degree();

    // Reconsider node's MPR candidacy.
    bool is_now_cand_mpr = is_cand_mpr();

    // If there has been no change in MPR candidacy on a node
    // with normal willingness, there is no need to notify Neighborhood.
    if (!is_persistent_cand_mpr() && was_cand_mpr == is_now_cand_mpr)
	return is_now_cand_mpr;

    //
    // Trigger MPR recount in Neighborhood.
    //
    // 8.5: The MPR set MUST be recounted when a neighbor
    // appearance or loss is detected, or when a change in the 2-hop
    // neighborhood is detected.
    //
    // If a WILL_ALWAYS node is being updated we need to force an update
    // of this as it may have been left out during 
    //
    if (is_persistent_cand_mpr() ||
	(was_cand_mpr == false && is_cand_mpr() == true)) {
	// If a node is now a candidate MPR due to a change
	// in its state, it should be considered as an MPR
	// candidate for all interfaces from which it is reachable.
	_parent->add_cand_mpr(id());
    } else {
	// If a node is no longer a candidate MPR due to a change
	// in its state, it should be withdrawn as an MPR
	// candidate for all interfaces from which it is reachable.
	_parent->withdraw_cand_mpr(id());
    }

    return is_now_cand_mpr;
}

void
Neighbor::set_willingness(const OlsrTypes::WillType value)
{
    if (_willingness == value)
	return;

    // Consider node's MPR status before its change in willingness.
    bool was_cand_mpr = is_cand_mpr();

    _willingness = value;

    // Reconsider node's MPR status after a state change.
    update_cand_mpr(was_cand_mpr);
}

void
Neighbor::update_link(const OlsrTypes::LogicalLinkID linkid)
{
    debug_msg("LinkID %u\n", XORP_UINT_CAST(linkid));
    XLOG_ASSERT(OlsrTypes::UNUSED_LINK_ID != linkid);

    bool was_cand_mpr = is_cand_mpr();

    if (0 == _links.count(linkid)) {
	_links.insert(linkid);
    }

    //
    // 8.1 Neighbor Set Update: if any associated link tuple
    // indicates a symmetric link for the neighbor, set N_status to SYM.
    //
    // Update the link status regardless of whether this is a new link
    // or a link being touched as part of a HELLO message. Doing it from
    // Link itself may trigger an exception if the associated Neighbor
    // does not yet exist.
    //
    if (! is_sym())
	set_is_sym(_parent->get_logical_link(linkid)->is_sym());

    // We need to reconsider MPR candidacy as the neighbor
    // MAY now be reachable by another interface, or MAY now
    // provide a new path to two-hop neighbors which it did not before.
    update_cand_mpr(was_cand_mpr);
}

bool
Neighbor::delete_link(const OlsrTypes::LogicalLinkID linkid)
{
    debug_msg("LinkID %u\n", XORP_UINT_CAST(linkid));

    // Invariant: Non-existent links should not be deleted.
    XLOG_ASSERT(0 != _links.count(linkid));

    // Consider node's MPR status before its change in link state.
    bool was_cand_mpr = is_cand_mpr();

    _links.erase(linkid);
    bool is_empty = _links.empty();

    // 8.1 Re-evaluate N_status. A neighbor with no links is no longer
    // a symmetric neighbor; otherwise, check the rest of the links.
    // XXX TODO Only 'good' links should be considered when ETX
    // is implemented, see Neighborhood::is_better_link().
    set_is_sym(is_empty ? false :
	       find_if(_links.begin(), _links.end(),
			IsLinkSymmetricPred(_parent)) != _links.end());

    // Neighbor loses MPR status if we no longer have a symmetric link, or
    // indeed any links, to this neighbor.
    update_cand_mpr(was_cand_mpr);

    return is_empty;
}

bool
Neighbor::is_mpr_selector() const
{
    return _mpr_selector_timer.scheduled();
}

void
Neighbor::set_is_mpr_selector(bool value, const TimeVal& expiry_time)
{
    if (_mpr_selector_timer.scheduled())
	_mpr_selector_timer.clear();

    if (value == true) {
	debug_msg("scheduling %u's MPR selector expiry for %s\n",
	    XORP_UINT_CAST(id()), cstring(expiry_time));

	_mpr_selector_timer = _eventloop.new_oneoff_after(
	    expiry_time,
	    callback(this, &Neighbor::event_mpr_selector_expired));
    }
}

void
Neighbor::add_twohop_link(const OlsrTypes::TwoHopLinkID tlid)
{
    debug_msg("TwoHopLinkID %u\n", XORP_UINT_CAST(tlid));

    // Invariant: Two-hop links should not be added more than once.
    XLOG_ASSERT(0 == _twohop_links.count(tlid));

    // Consider node's MPR status before its change in link state.
    bool was_cand_mpr = is_cand_mpr();

    // Add the new two-hop link.
    _twohop_links.insert(tlid);

    // Reconsider node's MPR status after a state change.
    update_cand_mpr(was_cand_mpr);
}

bool
Neighbor::delete_twohop_link(const OlsrTypes::TwoHopLinkID tlid)
{
    debug_msg("TwoHopLinkID %u\n", XORP_UINT_CAST(tlid));

    // Invariant: Non-existent two-hop links should not be deleted.
    XLOG_ASSERT(0 != _twohop_links.count(tlid));

    // Consider node's MPR status before its change in link state.
    bool was_cand_mpr = is_cand_mpr();

    _twohop_links.erase(tlid);
    bool is_empty = _twohop_links.empty();

    // Reconsider node's MPR status after a state change.
    update_cand_mpr(was_cand_mpr);

    return is_empty;
}

size_t
Neighbor::delete_all_twohop_links()
{
    size_t deleted_count = 0;

    // Consider node's MPR status before its change in link state.
    bool was_cand_mpr = is_cand_mpr();

    set<OlsrTypes::TwoHopLinkID>::iterator ii, jj;
    ii = _twohop_links.begin();
    while (ii != _twohop_links.end()) {
	jj = ii++;
	_parent->delete_twohop_link((*jj));
	// Don't erase the element; delete_twohop_link() will
	// call back to do this.
	++deleted_count;
    }

    // Reconsider node's MPR status after a state change.
    update_cand_mpr(was_cand_mpr);

    return deleted_count;
}

void
Neighbor::event_mpr_selector_expired()
{
    _parent->event_mpr_selector_expired(id());
}
