// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2011 XORP, Inc and Others
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

// #define DEBUG_LOGGING
// #define DEBUG_FUNCTION_NAME
// #define DETAILED_DEBUG

#include "olsr.hh"
#include "link.hh"
#include "neighbor.hh"
#include "neighborhood.hh"
#include "topology.hh"

bool
CandMprOrderPred::operator()(const Neighbor* lhs, const Neighbor* rhs) const
{
    if (lhs->willingness() == rhs->willingness()) {
	if (lhs->reachability() == rhs->reachability()) {
	    return lhs->degree() > rhs->degree();
	}
	return lhs->reachability() > rhs->reachability();
    }
    return lhs->willingness() > rhs->willingness();
}

bool
LinkOrderPred::operator()(const OlsrTypes::LogicalLinkID lhid,
			  const OlsrTypes::LogicalLinkID rhid)
{
    try {
	const LogicalLink* lhs = _nh->get_logical_link(lhid);
	const LogicalLink* rhs = _nh->get_logical_link(rhid);

	if (lhs->is_sym() == rhs->is_sym()) {
	    // TODO: ETX collation predicates go here.
	    if (lhs->sym_time_remaining() == rhs->sym_time_remaining()) {
		// Final collation tiebreak: Return if the link has a higher ID.
		return lhs->id() > rhs->id();
	    }
	    return lhs->sym_time_remaining() > rhs->sym_time_remaining();
	}
	return lhs->is_sym() > rhs->is_sym();

    } catch (...) {}

    return false;
}

bool
TwoHopLinkOrderPred::operator()(const OlsrTypes::TwoHopLinkID lhid,
				const OlsrTypes::TwoHopLinkID rhid)
{
    try {
	TwoHopLink* lhs = _nh->get_twohop_link(lhid);
	TwoHopLink* rhs = _nh->get_twohop_link(rhid);

	if (lhs->time_remaining() == rhs->time_remaining()) {
	    // Final collation tiebreak: Return if the link has a higher ID.
	    return lhs->id() > rhs->id();
	}
	return lhs->time_remaining() > rhs->time_remaining();
    } catch (...) {}

    return false;
}

Neighborhood::Neighborhood(Olsr& olsr, EventLoop& eventloop,
    FaceManager& fm)
     : _olsr(olsr),
       _eventloop(eventloop),
       _fm(fm),
       _tm(0),
       _link_order_pred(this),
       _twohop_link_order_pred(this),
       _next_linkid(1),
       _next_neighborid(1),
       _next_twohop_linkid(1),
       _next_twohop_nodeid(1),
       _enabled_face_count(0),
       _willingness(OlsrTypes::WILL_DEFAULT),
       _refresh_interval(TimeVal(OlsrTypes::DEFAULT_REFRESH_INTERVAL, 0)),
       _mpr_computation_enabled(true),
       _mpr_coverage(OlsrTypes::DEFAULT_MPR_COVERAGE),
       _tc_interval(TimeVal(OlsrTypes::DEFAULT_TC_INTERVAL, 0)),
       _tc_redundancy(OlsrTypes::DEFAULT_TC_REDUNDANCY),
       _tc_timer_state(TC_STOPPED),
       _tc_current_ansn(1),
       _tc_previous_ans_count(0),
       _loss_triggered_tc_enabled(true),
       _change_triggered_tc_enabled(false)
{
    _fm.add_message_cb(callback(this, &Neighborhood::event_receive_hello));

    // Create the MPR selection background task but make sure it is not
    // scheduled until we have neighbors to select as MPRs.
    // Weight this task to happen before route updates, as it SHOULD
    // happen before the state of our forwarding plane is changed.
    _mpr_recount_task = _eventloop.new_oneoff_task(
	callback(this, &Neighborhood::recount_mpr_set),
	XorpTask::PRIORITY_HIGH,
	XorpTask::WEIGHT_DEFAULT);

    // XXX does this cause a leak?
    _mpr_recount_task.unschedule();
}

Neighborhood::~Neighborhood()
{
    _mpr_recount_task.unschedule();

    // Must stop TC timer even if in FINISHING state.
    stop_tc_timer();

    _fm.delete_message_cb(callback(this, &Neighborhood::event_receive_hello));

    clear_links();

    XLOG_ASSERT(_twohop_links.empty());
    XLOG_ASSERT(_twohop_nodes.empty());
    XLOG_ASSERT(_links.empty());
    XLOG_ASSERT(_neighbors.empty());
}

/*
 * Protocol variable methods.
 */

bool
Neighborhood::set_mpr_coverage(const uint32_t coverage)
{
    if (_mpr_coverage != coverage) {
	_mpr_coverage = coverage;
	if (_enabled_face_count > 0)
	    schedule_mpr_recount();
    }

    return true;
}

void
Neighborhood::set_tc_interval(const TimeVal& interval)
{
    if (interval == _tc_interval)
	return;

    _tc_interval = interval;

    if (_tc_timer.scheduled()) {
	// Change period.
	reschedule_tc_timer();

	// Fire one off now.
	debug_msg("%s: Scheduling an immediate TC due to a change in "
		  "TC interval.\n", cstring(_fm.get_main_addr()));
	reschedule_immediate_tc_timer();
    }
}

bool
Neighborhood::set_tc_redundancy(const OlsrTypes::TcRedundancyType type)
{
    debug_msg("TcRedundancy %u\n", XORP_UINT_CAST(type));

    if (type == _tc_redundancy) {
	debug_msg("%s == %s, no-op.\n",
		  tcr_to_str(type),
		  tcr_to_str(_tc_redundancy));
	return true;
    }

    if (type > OlsrTypes::TCR_END) {
	XLOG_ERROR("Topology control mode %u out of range.",
		   XORP_UINT_CAST(type));
	return false;
    }

    if (willingness() == OlsrTypes::WILL_NEVER &&
	type != OlsrTypes::TCR_MPRS_IN) {
	XLOG_ERROR("Topology control mode %u invalid when configured not to "
		   "forward OLSR traffic.",
		   XORP_UINT_CAST(type));
	return false;
    }

    _tc_redundancy = type;

    if (is_mpr()) {
	debug_msg("%s: Scheduling an immediate TC due to a change in "
		  "TC redundancy.\n", cstring(_fm.get_main_addr()));
	reschedule_immediate_tc_timer();
    }

    XLOG_INFO("TC redundancy mode is set to %s.\n", tcr_to_str(type));

    return true;
}

void
Neighborhood::set_willingness(const OlsrTypes::WillType willingness)
{
    debug_msg("Willingness %u\n", XORP_UINT_CAST(willingness));

    if (willingness == _willingness)
	return;	// do nothing

    _willingness = willingness;
    XLOG_INFO("Willingness-to-forward is set to %s.\n",
	      will_to_str(willingness));

    // 15.1: A node with willingness equal to WILL_NEVER SHOULD have
    // TC_REDUNDANCY also equal to zero.
    if (willingness == OlsrTypes::WILL_NEVER) {
	debug_msg("Setting TC redundancy to %s to match willingness %s.\n",
		  tcr_to_str(OlsrTypes::TCR_MPRS_IN),
	          will_to_str(willingness));
	set_tc_redundancy(OlsrTypes::TCR_MPRS_IN);
    }
}

/*
 * Methods which interact with other major classes.
 */

size_t
Neighborhood::populate_hello(HelloMessage* hello)
{
#if 0
    debug_msg("MyMainAddr %s HelloMessage* %p\n",
	      cstring(_fm.get_main_addr()), hello);
#endif

    size_t populated_count = 0;

    XLOG_ASSERT(0 != hello);
    XLOG_ASSERT(hello->faceid() != OlsrTypes::UNUSED_FACE_ID);
    XLOG_ASSERT(hello->links().empty() == true);

    // Set message attributes that are derived from Neighborhood.
    hello->set_expiry_time(get_neighbor_hold_time());
    hello->set_willingness(willingness());

    map<OlsrTypes::LogicalLinkID, LogicalLink*>::iterator ii;
    for (ii = _links.begin(); ii != _links.end(); ii++) {
	const LogicalLink* l = (*ii).second;
	const Neighbor* n = l->destination();

	if (hello->faceid() == l->faceid()) {
	    LinkCode lc(n->neighbor_type(), l->link_type());

	    // The link is local to this interface, therefore we
	    // advertise it by its remote interface address.
	    hello->add_link(lc, l->remote_addr());
	    populated_count++;
	} else {
	    // We should not announce links for a disabled face; this
	    // check is implemented further up in the call path.
#if 0
	    debug_msg("hello->faceid() is %u, l->faceid() is %u\n",
		      XORP_UINT_CAST(hello->faceid()),
		      XORP_UINT_CAST(l->faceid()));
#endif
	    // Invariant: This path should only be reached if there is
	    // more than one OLSR interface configured administratively
	    // up in this process.
	    XLOG_ASSERT(_enabled_face_count > 1);

	    // The link is not local to this interface.
	    // From the point of view of any neighbors which see
	    // this HELLO message, the link we are advertising
	    // is to a two-hop neighbor -- therefore we advertise
	    // its main address.
	    LinkCode lc(n->neighbor_type(), OlsrTypes::UNSPEC_LINK);

	    hello->add_link(lc, n->main_addr());
	    populated_count++;
	}
    }

    return populated_count;
}

void
Neighborhood::push_topology()
{
    size_t n_pushed = 0;
    size_t n2_pushed = 0;

    map<OlsrTypes::NeighborID, Neighbor*>::iterator ii;
    for (ii = _neighbors.begin(); ii != _neighbors.end(); ii++) {
	if (true == push_neighbor((*ii).second)) {
	    n_pushed++;
	}
    }

    debug_msg("Pushed %u neighbors to SPT.\n", XORP_UINT_CAST(n_pushed));

    map<OlsrTypes::TwoHopNodeID, TwoHopNeighbor*>::iterator jj;
    for (jj = _twohop_nodes.begin(); jj != _twohop_nodes.end(); jj++) {
	if (true == push_twohop_neighbor((*jj).second)) {
	    n2_pushed++;
	}
    }

    debug_msg("Pushed %u two-hop neighbors to SPT.\n",
	      XORP_UINT_CAST(n2_pushed));
}

/*
 * Network interface methods.
 */

void
Neighborhood::add_face(const OlsrTypes::FaceID faceid)
{
    debug_msg("FaceID %u\n", XORP_UINT_CAST(faceid));
    _enabled_face_count++;
    UNUSED(faceid);
}

void
Neighborhood::delete_face(const OlsrTypes::FaceID faceid)
{
    debug_msg("FaceID %u\n", XORP_UINT_CAST(faceid));

    size_t deleted_link_count = 0;

    // If we are taking an interface administratively down,
    // pull all link-state information associated with the
    // address(es) configured on this interface.
    map<OlsrTypes::LogicalLinkID, LogicalLink*>::iterator ii, jj;
    for (ii = _links.begin(); ii != _links.end(); ) {
	jj = ii++;
	LogicalLink* l = (*jj).second;
	if (l->faceid() == faceid) {
	    delete_link(l->id());
	    deleted_link_count++;
	}
    }

    --_enabled_face_count;
    if (_enabled_face_count == 0) {
	// If no interfaces remaining, ensure TC timer has stopped.
	stop_tc_timer();
    } else {
	// If there were any links on the interface we have just
	// removed, we SHOULD schedule an MPR recount AND a route
	// computation.
	// [If we use per-interface granular MPR selection then we should
	//  probably been notified of this already as part of the
	//  delete_link() call graph above.]
	if (deleted_link_count > 0) {
	    schedule_mpr_recount();
	    if (_rm)
		_rm->schedule_route_update();
	}
    }
}

/*
 * One-hop link methods.
 */

OlsrTypes::LogicalLinkID
Neighborhood::add_link(const TimeVal& vtime,
			  const IPv4& remote_addr,
			  const IPv4& local_addr)
    throw(BadLogicalLink)
{
    debug_msg("MyMainAddr %s RemoteAddr %s LocalAddr %s\n",
	      cstring(_fm.get_main_addr()),
	      cstring(remote_addr), cstring(local_addr));

    OlsrTypes::LogicalLinkID linkid = _next_linkid++;

    // Throw an exception if we overflowed the LogicalLinkID space.
    if (_links.find(linkid) != _links.end()) {
	xorp_throw(BadLogicalLink,
		   c_format("Mapping for LogicalLinkID %u already exists",
			    XORP_UINT_CAST(linkid)));
    }

    _links[linkid] = new LogicalLink(this, _eventloop, linkid, vtime,
				     remote_addr, local_addr);
    _link_addr[make_pair(remote_addr, local_addr)] = linkid;

    debug_msg("New link: %s -> %s\n",
	      cstring(local_addr), cstring(remote_addr));
    XLOG_TRACE(_olsr.trace()._neighbor_events,
	       "New link: %s -> %s\n",
		cstring(local_addr), cstring(remote_addr));

    return linkid;
}

bool
Neighborhood::delete_link(OlsrTypes::LogicalLinkID linkid)
{
    debug_msg("MyMainAddr %s LinkID %u\n",
	      cstring(_fm.get_main_addr()), XORP_UINT_CAST(linkid));

    // Find link tuple by ID.
    map<OlsrTypes::LogicalLinkID, LogicalLink*>::iterator ii =
	_links.find(linkid);
    if (ii == _links.end())
	return false;

    LogicalLink* l = (*ii).second;

    // Find address-to-link in map, erase entry if found.
    map<pair<IPv4, IPv4>, OlsrTypes::LogicalLinkID>::iterator jj =
	_link_addr.find(make_pair(l->remote_addr(), l->local_addr()));
    if (jj != _link_addr.end()) {
	_link_addr.erase(jj);
    }

    XLOG_ASSERT(l->faceid() != OlsrTypes::UNUSED_FACE_ID);

    // 8.1: Each time a link is deleted, the associated neighbor tuple
    // MUST be removed if it has no longer any associated link tuples.
    //
    // We must check if the neighbor ID is initialized as this method
    // may be called if we are unable to create the neighbor.
    if (l->neighbor_id() != OlsrTypes::UNUSED_NEIGHBOR_ID) {
	bool is_unlinked = l->destination()->delete_link(linkid);
	if (is_unlinked) {
	    debug_msg("Deleting neighbor %u.\n", l->neighbor_id());
	    delete_neighbor(l->neighbor_id());
	}
    }

    debug_msg("Delete link: %s -> %s\n",
	      cstring(l->local_addr()), cstring(l->remote_addr()));
    XLOG_TRACE(_olsr.trace()._neighbor_events,
	       "Delete link: %s -> %s\n",
	       cstring(l->local_addr()), cstring(l->remote_addr()));

    // Destroy the link.
    _links.erase(ii);
    delete l;

    // Trigger a routing computation.
    if (_rm)
	_rm->schedule_route_update();

    return true;
}

void
Neighborhood::clear_links()
{
    map<OlsrTypes::LogicalLinkID, LogicalLink*>::iterator ii;
    for (ii = _links.begin(); ii != _links.end(); ii++) {
	delete_link((*ii).first);
    }
}

const LogicalLink*
Neighborhood::find_best_link(const Neighbor* n)
    throw(BadLinkCoverage)
{
    const set<OlsrTypes::LogicalLinkID>& links = n->links();

    if (links.empty()) {
	xorp_throw(BadLinkCoverage,
		   c_format("No suitable links to Neighbor %u.",
			    XORP_UINT_CAST(n->id())));
    }

    set<OlsrTypes::LogicalLinkID>::const_iterator ii =
	min_element(links.begin(), links.end(), _link_order_pred);

    // Catchall: the link which we arrive at must be OK.
    const LogicalLink* l = _links[(*ii)];

    if (! l->is_sym()) {
	xorp_throw(BadLinkCoverage,
		   c_format("No suitable links to Neighbor %u.",
			    XORP_UINT_CAST(n->id())));
    }

    return l;
}

OlsrTypes::LogicalLinkID
Neighborhood::get_linkid(const IPv4& remote_addr, const IPv4& local_addr)
    throw(BadLogicalLink)
{
    //debug_msg("RemoteAddr %s LocalAddr %s\n",
    //	cstring(remote_addr), cstring(local_addr));

    map<pair<IPv4, IPv4>, OlsrTypes::LogicalLinkID>::iterator ii =
	_link_addr.find(make_pair(remote_addr, local_addr));
    if (ii == _link_addr.end()) {
	xorp_throw(BadLogicalLink,
		   c_format("No mapping for %s:%s exists",
			    cstring(remote_addr),
			    cstring(local_addr)));
    }

    return (*ii).second;
}

OlsrTypes::LogicalLinkID
Neighborhood::update_link(const OlsrTypes::FaceID faceid,
			     const IPv4& remote_addr,
			     const IPv4& local_addr,
			     const TimeVal& vtime,
			     bool& is_created)
    throw(BadLogicalLink)
{
#if 0
    debug_msg("MyMainAddr %s FaceID %u RemoteAddr %s LocalAddr %s Vtime %s\n",
	      cstring(_fm.get_main_addr()),
	      XORP_UINT_CAST(faceid),
	      cstring(remote_addr),
	      cstring(local_addr),
	      cstring(vtime));
#endif

    OlsrTypes::LogicalLinkID linkid;

    try {
	linkid = get_linkid(remote_addr, local_addr);
	is_created = false;

	// Invariant: The Face which we see this link on MUST NOT
	// have changed. If an OLSR interface has been renumbered,
	// the face will have gone down and all its links with it.
	XLOG_ASSERT(faceid == _links[linkid]->faceid());

    } catch(BadLogicalLink& bl) {
	debug_msg("allocating new link entry.\n");

	// XXX: May throw an exception again, or even bad_alloc.
	linkid = add_link(vtime, remote_addr, local_addr);

	// Set back-reference to the Face where this link is terminated.
	// Inform FaceManager that the interface has a new link.
	_links[linkid]->set_faceid(faceid);

	is_created = true;
    }

    // A change in link state, or a link being added, means we
    // have to recompute the routing table.
    // TODO: Push this down further -- we should only trigger an
    // update when the link actually changed state.
    _rm->schedule_route_update();

    return linkid;
}

const LogicalLink*
Neighborhood::get_logical_link(const OlsrTypes::LogicalLinkID linkid)
    throw(BadLogicalLink)
{
    //debug_msg("LinkID %u\n", XORP_UINT_CAST(linkid));

    if (_links.find(linkid) == _links.end()) {
	xorp_throw(BadLogicalLink,
		   c_format("No mapping for %u exists",
			    XORP_UINT_CAST(linkid)));
    }

    return _links[linkid];
}

void
Neighborhood::get_logical_link_list(list<OlsrTypes::LogicalLinkID>& l1_list)
    const
{
    map<OlsrTypes::LogicalLinkID, LogicalLink*>::const_iterator ii;

    for (ii = _links.begin(); ii != _links.end(); ii++)
	l1_list.push_back((*ii).first);
}

/*
 * One-hop neighbor methods.
 */

OlsrTypes::NeighborID
Neighborhood::get_neighborid_by_main_addr(const IPv4& main_addr)
    throw(BadNeighbor)
{
    //debug_msg("MainAddr %s\n", cstring(main_addr));

    if (_neighbor_addr.find(main_addr) == _neighbor_addr.end()) {
	xorp_throw(BadNeighbor,
		   c_format("No mapping for %s exists", cstring(main_addr)));
    }

    return _neighbor_addr[main_addr];
}

OlsrTypes::NeighborID
Neighborhood::get_neighborid_by_remote_addr(const IPv4& remote_addr)
    throw(BadNeighbor)
{
    debug_msg("RemoteAddr %s\n", cstring(remote_addr));

    // Check for a main address match first.
    if (_neighbor_addr.find(remote_addr) != _neighbor_addr.end())
	return _neighbor_addr[remote_addr];

    //debug_msg("neighbor not found by main address, checking link database\n");

    OlsrTypes::NeighborID nid = OlsrTypes::UNUSED_NEIGHBOR_ID;
    bool is_found = false;

    // If not found by main address, we need to walk the one-hop link state.
    // We only have one end of the link so we must do a linear search.
    map<OlsrTypes::LogicalLinkID, LogicalLink*>::iterator ii;
    for (ii = _links.begin(); ii != _links.end(); ii++) {
	LogicalLink* l = (*ii).second;
	if (l->remote_addr() == remote_addr) {
	    nid = l->neighbor_id();
	    is_found = true;
	    break;
	}
    }

    if (is_found == false) {
	xorp_throw(BadNeighbor,
		   c_format("No mapping for %s exists", cstring(remote_addr)));
    }

    // Invariant: If a link is found, its neighbor ID
    // MUST have been set when the association between
    // LogicalLink and Neighbor was made.
    XLOG_ASSERT(nid != OlsrTypes::UNUSED_NEIGHBOR_ID);

    return nid;
}

OlsrTypes::NeighborID
Neighborhood::update_neighbor(const IPv4& main_addr,
    const OlsrTypes::LogicalLinkID linkid,
    const bool is_new_link,
    const OlsrTypes::WillType will,
    const bool is_mpr_selector,
    const TimeVal& mprs_expiry_time,
    bool& is_created)
    throw(BadNeighbor)
{
    debug_msg("MyMainAddr %s MainAddr %s LinkID %u new %s will %u "
	      "mprsel %s expiry %s\n",
	      cstring(_fm.get_main_addr()),
	      cstring(main_addr),
	      XORP_UINT_CAST(linkid),
	      bool_c_str(is_new_link),
	      XORP_UINT_CAST(will),
	      bool_c_str(is_mpr_selector),
	      cstring(mprs_expiry_time));

    OlsrTypes::NeighborID nid;
    Neighbor* n;

    try {
	nid = get_neighborid_by_main_addr(main_addr);
	n = _neighbors[nid];
	is_created = false;

	// Update neighbor's associated link, creating the association
	// if it does not already exist. N_status will be updated.
	n->update_link(linkid);

    } catch(BadNeighbor& bn) {
	// Create a new neighbor tuple.
	// XXX May throw BadNeighbor if a neighbor ID cannot be allocated,
	// i.e. we ran out of neighbor ID space.
	nid = add_neighbor(main_addr, linkid);
	n = _neighbors[nid];
	is_created = true;
    }

    if (is_new_link) {
	// Set back-reference from LogicalLink to Neighbor. Whilst a
	// Neighbor may have many LogicalLinks, a LogicalLink MUST be
	// associated with one and only one Neighbor.
	_links[linkid]->set_destination(n);
	_links[linkid]->set_neighbor_id(nid);
    }

    // Invariant: a neighbor cannot exist without at least one link.
    XLOG_ASSERT(! n->links().empty());

    // Section 8.1.1: Update neighbor willingness.
    n->set_willingness(will);

    // 8.4.1: Update the MPR selector set, if the neighbor selects
    // me as an MPR.  This *may* start the TC timer.
    if (is_mpr_selector)
	update_mpr_selector(nid, mprs_expiry_time);

    // Mark the neighbor as being advertised in TC broadcasts if
    // its advertisement state changed.
    schedule_ans_update(false);

    // Routing recomputation will be triggered by LogicalLink operations.

    return nid;
}

OlsrTypes::NeighborID
Neighborhood::add_neighbor(const IPv4& main_addr,
    OlsrTypes::LogicalLinkID linkid)
    throw(BadNeighbor)
{
    debug_msg("MyMainAddr %s MainAddr %s\n",
	      cstring(_fm.get_main_addr()),
	      cstring(main_addr));

    OlsrTypes::NeighborID neighborid = _next_neighborid++;

    // Throw an exception if we overflow the neighbor ID space.
    if (_neighbors.find(neighborid) != _neighbors.end()) {
	xorp_throw(BadNeighbor,
		   c_format("Mapping for NeighborID %u already exists",
			    XORP_UINT_CAST(neighborid)));
    }

    // Section 8.1: Associated neighbor MUST be created with the link
    // if it does not already exist.
    // We enforce this at language level with the constructor signature.
    Neighbor* n = new Neighbor(_eventloop, this, neighborid,
					  main_addr, linkid);
    _neighbors[neighborid] = n;

    XLOG_ASSERT(_neighbor_addr.find(main_addr) == _neighbor_addr.end());
    _neighbor_addr[main_addr] = neighborid;

    // Check and record if the new one-hop neighbor is already known to
    // us as a two-hop neighbor.
    // XXX Should we only do this when the neighbor is first considered
    // symmetric?
    try {
	OlsrTypes::TwoHopNodeID tnid =
	    get_twohop_nodeid_by_main_addr(main_addr);
	_twohop_nodes[tnid]->set_is_strict(false);
    } catch (...) {}

    debug_msg("New neighbor: %s\n", cstring(n->main_addr()));
    XLOG_TRACE(_olsr.trace()._neighbor_events,
	       "New neighbor: %s\n", cstring(n->main_addr()));

    // We defer associating the Neighbor with its first LogicalLink
    // fully until both have been created.

    return neighborid;
}

bool
Neighborhood::delete_neighbor(const OlsrTypes::NeighborID nid)
{
    debug_msg("MyMainAddr %s NeighborID %u\n",
	      cstring(_fm.get_main_addr()),
	      XORP_UINT_CAST(nid));

    XLOG_ASSERT(_neighbors.find(nid) != _neighbors.end());

    map<OlsrTypes::NeighborID, Neighbor*>::iterator ii = _neighbors.find(nid);
    if (ii == _neighbors.end())
	return false;

    Neighbor* n = (*ii).second;

    // Withdraw the neighbor from any pending TC broadcasts.
    // XXX NOTE WELL: This should only be done when the last good link
    // to a neighbor has been removed. Obviously ETX metrics are going
    // to mean this may be done earlier than neighbor deletion.
    schedule_ans_update(true);

    // Purge membership of MPR selector set.
    // 8.4.1: In case of neighbor loss, all MPR selector tuples with
    // MS_main_addr == Main Address of the neighbor MUST be deleted.
    // The timer will go out of scope when n is deleted.
    if (n->is_mpr_selector())
	delete_mpr_selector(nid);

    // 8.5: In case of neighbor loss, all 2-hop tuples with
    // nexthop() of neighbor's main address MUST be deleted.
    n->delete_all_twohop_links();

    // If the one-hop neighbor being lost is already known to us as a
    // two-hop neighbor, ensure that the 'N2 is not also an N' flag is cleared.
    try {
	OlsrTypes::TwoHopNodeID tnid =
	    get_twohop_nodeid_by_main_addr(n->main_addr());
	_twohop_nodes[tnid]->set_is_strict(true);
    } catch (...) {}

    // Purge membership of MPR sets, and trigger MPR set recount.
    withdraw_cand_mpr(nid);

    debug_msg("Delete neighbor: %s\n", cstring(n->main_addr()));
    XLOG_TRACE(_olsr.trace()._neighbor_events,
	       "Delete neighbor: %s\n", cstring(n->main_addr()));

    map<IPv4, OlsrTypes::NeighborID>::iterator jj =
	_neighbor_addr.find(n->main_addr());
    if (jj != _neighbor_addr.end()) {
	_neighbor_addr.erase(jj);
    }

    _neighbors.erase(ii);

    delete n;

    return true;
}

const Neighbor*
Neighborhood::get_neighbor(const OlsrTypes::NeighborID nid)
    throw(BadNeighbor)
{
    debug_msg("NeighborID %u\n", XORP_UINT_CAST(nid));

    if (_neighbors.find(nid) == _neighbors.end()) {
	xorp_throw(BadNeighbor,
		   c_format("No mapping for %u exists",
			    XORP_UINT_CAST(nid)));
    }

    return _neighbors[nid];
}

void
Neighborhood::get_neighbor_list(list<OlsrTypes::NeighborID>& n1_list) const
{
    map<OlsrTypes::NeighborID, Neighbor*>::const_iterator ii;

    for (ii = _neighbors.begin(); ii != _neighbors.end(); ii++)
	n1_list.push_back((*ii).first);
}

bool
Neighborhood::is_sym_neighbor_addr(const IPv4& remote_addr)
{
    debug_msg("%s: RemoteAddr %s\n",
	      cstring(_fm.get_main_addr()),
	      cstring(remote_addr));

    bool is_sym = false;
    try {
	OlsrTypes::NeighborID nid = get_neighborid_by_remote_addr(remote_addr);
	debug_msg("nid is %u\n", XORP_UINT_CAST(nid));
	is_sym = _neighbors[nid]->is_sym();
    } catch (BadNeighbor& bn) {
	debug_msg("threw exception: %s\n", cstring(bn));
    }

    return is_sym;
}

void
Neighborhood::schedule_ans_update(const bool is_deleted)
{
    // Force an update of the ANSN if a neighbor is deleted,
    // but only if the timer is not in FINISHING state.
    if (is_deleted && _tc_timer_state != TC_FINISHING)
	++_tc_current_ansn;

    // If we should be originating TC broadcasts, and we have neighbors
    // to advertise, restart the timer in RUNNING mode.
    if (! _mpr_selector_set.empty()) {
	if (_tc_timer_state != TC_RUNNING) {
	    debug_msg("%s: Restarting timer due to non-empty ANS.\n",
		      cstring(_fm.get_main_addr()));
	    start_tc_timer();
	}

	// If we are configured to send early TCs, and such a change
	// happened, and we are flooding non-empty TCs, schedule an early TC.
	if (_change_triggered_tc_enabled) {
	    debug_msg("%s: Scheduling an immediate TC due to a change in "
		      "the ANS.\n",
		      cstring(_fm.get_main_addr()));
	    reschedule_immediate_tc_timer();
	}
    }
}

bool
Neighborhood::push_neighbor(const Neighbor* n)
{
    // Reject non-symmetric neighbors.
    if (! n->is_sym())
	return false;

    // Choose the best link to this neighbor.
    const LogicalLink* l;
    try {
	l = find_best_link(n);
    } catch(BadLinkCoverage& blc) {
	// Don't add this neighbor -- there are no good links to it.
	return false;
    }

    // We have the best link to N; populate the SPT.
    _rm->add_onehop_link(l, n);

    return true;
}

/*
 * Two-hop link methods.
 */

OlsrTypes::TwoHopLinkID
Neighborhood::update_twohop_link(const LinkAddrInfo& node_info,
				 Neighbor& nexthop,
				 const OlsrTypes::FaceID faceid,
				 const TimeVal& vtime)
    throw(BadTwoHopLink)
{
    debug_msg("MyMainAddr %s NodeAddr %s Nexthop %s FaceID %u Vtime %s\n",
	      cstring(_fm.get_main_addr()),
	      cstring(node_info.remote_addr()),
	      cstring(nexthop.main_addr()),
	      XORP_UINT_CAST(faceid),
	      cstring(vtime));

    OlsrTypes::TwoHopLinkID tlid;
    bool is_new_l2 = false;

    // Check to see if the two-hop link exists.
    map<pair<IPv4, IPv4>, OlsrTypes::TwoHopLinkID>::iterator ii =
	_twohop_link_addrs.find(make_pair(nexthop.main_addr(),
					  node_info.remote_addr()));
    if (ii == _twohop_link_addrs.end()) {
	// If the two-hop link does not exist, attempt to create it.
	// May throw BadTwoHopLink.
	debug_msg("adding new two-hop link\n");
	tlid = add_twohop_link(&nexthop, node_info.remote_addr(), vtime);
	is_new_l2 = true;
    } else {
	// Update timer and status for existing two-hop link.
	debug_msg("updating two-hop link timer\n");
	tlid = (*ii).second;
	_twohop_links[tlid]->update_timer(vtime);
    }

    // Track the interface where this two-hop link was last advertised.
    TwoHopLink* tl = _twohop_links[tlid];
    tl->set_face_id(faceid);

    OlsrTypes::TwoHopNodeID tnid;
    bool is_new_n2 = false;
    try {
	// Update the associated TwoHopNeighbor, creating it if needed.
	tnid = update_twohop_node(node_info.remote_addr(),
				  tlid, is_new_l2, is_new_n2);
    } catch (BadTwoHopNode& btn) {
	// Re-throw exception with appropriate type for this method.
	xorp_throw(BadTwoHopLink,
		   c_format("Could not create TwoHopNode with "
			    "address %s",
			    cstring(node_info.remote_addr())));
    }

    // TODO: When ETX metrics are in use, we need only trigger a route
    // computation, or MPR computation, if the two-hop link is considered
    // good at this point.

    if (is_new_l2) {
	// Associate new TwoHopLink with new or existing TwoHopNeighbor.
	tl->set_destination(_twohop_nodes[tnid]);

	// Notify Neighbor that it has a two-hop link, now that both
	// TwoHopLink and TwoHopNeighbor exist and are associated together.
	// This must be done AFTER the two-hop neighborhood has been updated.
	// This MAY trigger an MPR computation.
	nexthop.add_twohop_link(tlid);
    }

    // Trigger a routing computation.
    _rm->schedule_route_update();

    return tlid;
}

OlsrTypes::TwoHopLinkID
Neighborhood::add_twohop_link(Neighbor* nexthop,
			      const IPv4& remote_addr,
			      const TimeVal& vtime)
    throw(BadTwoHopLink)
{
    debug_msg("MyMainAddr %s Neighbor %u Vtime %s\n",
	      cstring(_fm.get_main_addr()),
	      XORP_UINT_CAST(nexthop->id()),
	      cstring(vtime));

    // Neighbor must exist.
    XLOG_ASSERT(0 != nexthop);

    // Two-hop links may only be added when the strong association is
    // with a symmetric one-hop neighbor.
    XLOG_ASSERT(nexthop->is_sym() == true);

    OlsrTypes::TwoHopLinkID tlid = _next_twohop_linkid++;

    // Throw an exception if we overflowed the TwoHopLinkID space.
    if (_twohop_links.find(tlid) != _twohop_links.end()) {
	xorp_throw(BadTwoHopLink,
		   c_format("Mapping for TwoHopLinkID %u already exists",
			    XORP_UINT_CAST(tlid)));
    }

    _twohop_links[tlid] = new TwoHopLink(_eventloop, this,
					 tlid, nexthop, vtime);

    _twohop_link_addrs[make_pair(nexthop->main_addr(), remote_addr)] = tlid;

    // We defer notifying Neighbor until both TwoHopLink and
    // TwoHopNode exist; these objects are co-dependent, and Neighbor
    // needs to see the TwoHopNode for pre-computation of MPR state.
    // Routing table computation will be triggered by update_twohop_link().

    return tlid;
}

bool
Neighborhood::delete_twohop_link(const OlsrTypes::TwoHopLinkID tlid)
{
    debug_msg("MyMainAddr %s TwoHopLinkID %u\n",
	      cstring(_fm.get_main_addr()),
	      XORP_UINT_CAST(tlid));

    map<OlsrTypes::TwoHopLinkID, TwoHopLink*>::iterator ii =
	_twohop_links.find(tlid);
    if (ii == _twohop_links.end())
	return false;

    TwoHopLink* tl = (*ii).second;
    Neighbor* n = tl->nexthop();
    TwoHopNeighbor* n2 = tl->destination();

    map<pair<IPv4, IPv4>, OlsrTypes::TwoHopLinkID>::iterator jj =
	_twohop_link_addrs.find(make_pair(n->main_addr(), n2->main_addr()));
    XLOG_ASSERT(jj != _twohop_link_addrs.end());
    XLOG_ASSERT(tlid == (*jj).second);

    // Track two-hop link deletion in Neighbor.
    n->delete_twohop_link(tlid);

    // Track two-hop link deletion in TwoHopNeighbor.
    // If this is the last link to the TwoHopNeighbor, then we
    // will also delete the TwoHopNeighbor.
    // XXX: For LQ we must check if the two-hop is no longer 'reachable'.
    bool is_n2_unlinked = n2->delete_twohop_link(tlid);
    if (is_n2_unlinked) {
	debug_msg("Deleting two-hop neighbor %u.\n", XORP_UINT_CAST(n2->id()));
	delete_twohop_node(n2->id());
    }

    _twohop_link_addrs.erase(jj);
    _twohop_links.erase(ii);
    delete tl;

    // Trigger a routing computation.
    if (_rm)
	_rm->schedule_route_update();

    return is_n2_unlinked;
}

bool
Neighborhood::delete_twohop_link_by_addrs(const IPv4& nexthop_addr,
					  const IPv4& twohop_addr)
{
    debug_msg("NexthopAddr %s TwohopAddr %s\n",
	      cstring(nexthop_addr),
	      cstring(twohop_addr));

    map<pair<IPv4, IPv4>, OlsrTypes::TwoHopLinkID>::iterator ii =
	_twohop_link_addrs.find(make_pair(nexthop_addr, twohop_addr));
    if (ii == _twohop_link_addrs.end())
	return false;

    bool is_last = delete_twohop_link((*ii).second);

    return is_last;
}

TwoHopLink*
Neighborhood::get_twohop_link(const OlsrTypes::TwoHopLinkID tlid)
    throw(BadTwoHopLink)
{
    debug_msg("TwoHopLinkID %u\n", XORP_UINT_CAST(tlid));

    if (_twohop_links.find(tlid) == _twohop_links.end()) {
	xorp_throw(BadTwoHopLink,
		   c_format("No mapping for %u exists", XORP_UINT_CAST(tlid)));
    }

    return _twohop_links[tlid];
}

void
Neighborhood::get_twohop_link_list(list<OlsrTypes::TwoHopLinkID>& l2_list)
    const
{
    map<OlsrTypes::TwoHopLinkID, TwoHopLink*>::const_iterator ii;

    for (ii = _twohop_links.begin(); ii != _twohop_links.end(); ii++)
	l2_list.push_back((*ii).first);
}

const TwoHopLink*
Neighborhood::find_best_twohop_link(const TwoHopNeighbor* n2)
    throw(BadTwoHopCoverage)
{
    const set<OlsrTypes::TwoHopLinkID>& twohop_links = n2->twohop_links();

    if (twohop_links.empty()) {
	xorp_throw(BadTwoHopCoverage,
		   c_format("No suitable links to TwoHopNeighbor %u.",
			    XORP_UINT_CAST(n2->id())));
    }

    set<OlsrTypes::TwoHopLinkID>::const_iterator ii =
	min_element(twohop_links.begin(), twohop_links.end(),
			 _twohop_link_order_pred);

    // Catchall: the two-hop link which we select must be OK.
    TwoHopLink* l2 = _twohop_links[(*ii)];

#if 0
    // TODO: Catchall for ETX metrics.
    if (! l2->is_poor_etx()) {
	xorp_throw(BadTwoHopCoverage,
		   c_format("No suitable links to TwoHopNeighbor %u.",
			    XORP_UINT_CAST(n2->id())));
    }
#endif

    return l2;

}

/*
 * Two-hop neighbor methods.
 */

OlsrTypes::TwoHopNodeID
Neighborhood::update_twohop_node(const IPv4& main_addr,
    const OlsrTypes::TwoHopLinkID tlid,
    const bool is_new_l2,
    bool& is_n2_created)
    throw(BadTwoHopNode)
{
    debug_msg("MyMainAddr %s MainAddr %s TwoHopLinkID %u IsNewL2 %s\n",
	      cstring(_fm.get_main_addr()),
	      cstring(main_addr),
	      XORP_UINT_CAST(tlid),
	      bool_c_str(is_new_l2));

    // Check if the TwoHopNeighbor with main_addr already exists.
    OlsrTypes::TwoHopNodeID tnid;

    map<IPv4, OlsrTypes::TwoHopNodeID>::iterator ii =
	_twohop_node_addrs.find(main_addr);
    if (ii == _twohop_node_addrs.end()) {
	// A new TwoHopNeighbor needs to be created.
	// May throw BadTwoHopNode.
	tnid = add_twohop_node(main_addr, tlid);
	is_n2_created = true;
    } else {
	// We are updating the existing TwoHopNeighbor.
	tnid = (*ii).second;

	// Associate existing TwoHopNeighbor with new TwoHopLink.
	if (is_new_l2)
	    _twohop_nodes[tnid]->add_twohop_link(tlid);
    }

    // Check if the two-hop neighbor we are updating is already a
    // one-hop neighbor, and update the 'strict 2-hop' flag accordingly.
    try {
	OlsrTypes::NeighborID nid = get_neighborid_by_main_addr(main_addr);
	_twohop_nodes[tnid]->set_is_strict(false);
	UNUSED(nid);
    } catch (BadNeighbor& bn) {
	_twohop_nodes[tnid]->set_is_strict(true);
    }

    return tnid;
}

OlsrTypes::TwoHopNodeID
Neighborhood::add_twohop_node(const IPv4& main_addr,
			      const OlsrTypes::TwoHopLinkID tlid)
    throw(BadTwoHopNode)
{
    debug_msg("MyMainAddr %s MainAddr %s TwoHopLinkID %u\n",
	      cstring(_fm.get_main_addr()),
	      cstring(main_addr),
	      XORP_UINT_CAST(tlid));

#ifdef DETAILED_DEBUG
    if (_twohop_node_addrs.find(main_addr) != _twohop_node_addrs.end()) {
	xorp_throw(BadTwoHopNode,
		   c_format("Mapping for %s already exists",
			    cstring(main_addr)));
    }
#endif

    OlsrTypes::TwoHopNodeID tnid = _next_twohop_nodeid++;

    // Throw an exception if we overflowed the TwoHopNodeID space.
    if (_twohop_nodes.find(tnid) != _twohop_nodes.end()) {
	xorp_throw(BadTwoHopNode,
		   c_format("Mapping for TwoHopNodeID %u already exists",
			    XORP_UINT_CAST(tnid)));
    }

    _twohop_nodes[tnid] = new TwoHopNeighbor(_eventloop, this,
					      tnid, main_addr,
					      tlid);

    _twohop_node_addrs[main_addr] = tnid;

    return tnid;
}

bool
Neighborhood::delete_twohop_node(const OlsrTypes::TwoHopNodeID tnid)
{
    debug_msg("TwoHopNodeID %u\n", XORP_UINT_CAST(tnid));

    map<OlsrTypes::TwoHopNodeID, TwoHopNeighbor*>::iterator ii =
	_twohop_nodes.find(tnid);
    if (ii == _twohop_nodes.end())
	return false;

    TwoHopNeighbor* n2 = (*ii).second;

    map<IPv4, OlsrTypes::TwoHopNodeID>::iterator jj =
	_twohop_node_addrs.find(n2->main_addr());
    for (jj = _twohop_node_addrs.begin(); jj != _twohop_node_addrs.end();
	 jj++) {
	if ((*jj).second == tnid) {
	    _twohop_node_addrs.erase(jj);
	    break;
	}
    }

    // Delete all two-hop link entries which point to this two-hop neighbor.
    n2->delete_all_twohop_links();

    delete n2;
    _twohop_nodes.erase(ii);

    // 8.5 Schedule an MPR recount as the two-hop neighborhood has changed.
    schedule_mpr_recount();

    return true;
}

OlsrTypes::TwoHopNodeID
Neighborhood::get_twohop_nodeid_by_main_addr(const IPv4& main_addr)
    throw(BadTwoHopNode)
{
    debug_msg("MainAddr %s\n", cstring(main_addr));

    map<IPv4, OlsrTypes::TwoHopNodeID>::const_iterator ii =
	_twohop_node_addrs.find(main_addr);

    if (ii == _twohop_node_addrs.end()) {
	xorp_throw(BadTwoHopNode,
		   c_format("No mapping for %s exists", cstring(main_addr)));
    }

    return (*ii).second;
}

const TwoHopNeighbor*
Neighborhood::get_twohop_neighbor(const OlsrTypes::TwoHopNodeID tnid) const
    throw(BadTwoHopNode)
{
    map<OlsrTypes::TwoHopNodeID, TwoHopNeighbor*>::const_iterator ii = 
	_twohop_nodes.find(tnid);

    if (ii == _twohop_nodes.end()) {
	xorp_throw(BadTwoHopNode,
		   c_format("No mapping for %u exists", XORP_UINT_CAST(tnid)));
    }

    return (*ii).second;
}

void
Neighborhood::get_twohop_neighbor_list(list<OlsrTypes::TwoHopNodeID>& n2_list)
    const
{
    map<OlsrTypes::TwoHopNodeID, TwoHopNeighbor*>::const_iterator ii;

    for (ii = _twohop_nodes.begin(); ii != _twohop_nodes.end(); ii++)
	n2_list.push_back((*ii).first);
}

bool
Neighborhood::push_twohop_neighbor(TwoHopNeighbor* n2)
{
    debug_msg("called\n");

    // Reject two-hop neighbors which are also Neighbors, or have no links.
    if (! n2->is_strict()) {
	debug_msg("%s: rejected n2 %s due to !strict\n",
		  cstring(_fm.get_main_addr()),
		  cstring(n2->main_addr()));
	return false;
    }

    if (! n2->is_reachable()) {
	debug_msg("%s: rejected n2 %s due to !reachable\n",
		  cstring(_fm.get_main_addr()),
		  cstring(n2->main_addr()));
	return false;
    }

    const TwoHopLink* l2;
    try {
	l2 = find_best_twohop_link(n2);
    } catch (BadTwoHopCoverage& btc) {
	debug_msg("Caught BadTwoHopCoverage.\n");
	//xorp_print_standard_exceptions();
	return false;
    }

    _rm->add_twohop_link(l2->nexthop(), l2, n2);

    return true;
}

/*
 * MPR selector methods.
 */

void
Neighborhood::update_mpr_selector(const OlsrTypes::NeighborID nid,
				  const TimeVal& vtime)
{
    debug_msg("NeighborID %u Vtime %s\n",
	      XORP_UINT_CAST(nid),
	       cstring(vtime));

    _neighbors[nid]->set_is_mpr_selector(true, vtime);

    debug_msg("Added MPR selector %s\n",
	      cstring(_neighbors[nid]->main_addr()));
    XLOG_TRACE(_olsr.trace()._mpr_selection,
	       "Added MPR selector %s\n",
	       cstring(_neighbors[nid]->main_addr()));

    bool was_mpr = is_mpr();
    bool is_created = false;

    if (_mpr_selector_set.find(nid) == _mpr_selector_set.end()) {
	_mpr_selector_set.insert(nid);
	is_created = true;
    }

    // 9.3 Start originating TC broadcasts, if we have now become an MPR.
    if (!was_mpr && ! _mpr_selector_set.empty()) {
	if (is_created) {
	    debug_msg("%s now becoming an MPR node.\n",
		      cstring(_fm.get_main_addr()));
	}
	start_tc_timer();
    }

    XLOG_ASSERT(_mpr_selector_set.size() > 0);
}

void
Neighborhood::delete_mpr_selector(const OlsrTypes::NeighborID nid)
{
    debug_msg("NeighborID %u\n", XORP_UINT_CAST(nid));

    XLOG_ASSERT(_mpr_selector_set.find(nid) != _mpr_selector_set.end());

    _mpr_selector_set.erase(nid);
    _neighbors[nid]->set_is_mpr_selector(false, TimeVal());

    debug_msg("Expired MPR selector %s\n",
	      cstring(_neighbors[nid]->main_addr()));
    XLOG_TRACE(_olsr.trace()._mpr_selection,
	       "Expired MPR selector %s\n",
	       cstring(_neighbors[nid]->main_addr()));

    // 9.3 If our MPR selector set is now empty,
    // change the TC timer state to FINISHING.
    if (_mpr_selector_set.empty()) {
	debug_msg("%s now has no MPR selectors.\n",
		  cstring(_fm.get_main_addr()));
	finish_tc_timer();

	// 9.3 When a change to the MPR selector set is detected and
	// this change can be attributed to a link failure, an early
	// TC message SHOULD be transmitted.
	if (_loss_triggered_tc_enabled) {
	    debug_msg("%s: Scheduling an immediate TC due to the loss of "
		      "all MPR selectors.\n",
		      cstring(_fm.get_main_addr()));

	    reschedule_immediate_tc_timer();
	}
    }
}

bool
Neighborhood::is_mpr_selector_addr(const IPv4& remote_addr)
{
    debug_msg("RemoteAddr %s\n", cstring(remote_addr));

    bool is_mpr_selector = false;
    try {
	OlsrTypes::NeighborID nid = get_neighborid_by_remote_addr(remote_addr);
	is_mpr_selector = _neighbors[nid]->is_mpr_selector();
    } catch (BadNeighbor& bn) {
	debug_msg("threw exception: %s\n", cstring(bn));
    }

    return is_mpr_selector;
}


/*
 * MPR selection methods.
 */

void
Neighborhood::recount_mpr_set()
{
    //if (_neighbors.empty()
    //	return;
    ostringstream dbg;

    // Clear all existing MPR state for Neighbors.
    reset_onehop_mpr_state();

    // Clear all existing MPR state for TwoHopNeighbors.
    // Compute number of now uncovered reachable nodes at radius=2.
    const size_t reachable_n2_count = reset_twohop_mpr_state(dbg);
    size_t covered_n2_count = 0;

    set<OlsrTypes::NeighborID> new_mpr_set; // For debugging.

    if (_mpr_computation_enabled) {
	// 8.3.1, 1: Start with an MPR set made of all members of N with
	// willingness equal to WILL_ALWAYS.
	covered_n2_count += consider_persistent_cand_mprs(dbg);

	// 8.3.1, 3: If N2 is still not fully covered, ensure that for
	// all uncovered strict N2 reachable only via 1 edge, their
	// neighbor N is selected as an MPR.
	if (covered_n2_count < reachable_n2_count) {
	    covered_n2_count += consider_poorly_covered_twohops(dbg);
	}

	// 8.3.1, 4: If N2 is still not fully covered, consider
	// candidate MPRs in descending order of willingness, reachability
	// and degree.
	if (covered_n2_count < reachable_n2_count) {
	    consider_remaining_cand_mprs(reachable_n2_count, covered_n2_count, dbg);
	}

	if (covered_n2_count < reachable_n2_count) {
	    dbg << " covered_n2_count: " << covered_n2_count << " reachable_n2_count: "
		<< reachable_n2_count << endl;
	    XLOG_WARNING("%s", dbg.str().c_str());
	    // Invariant: All reachable N2 must now be covered by MPRs.
	    XLOG_ASSERT(covered_n2_count >= reachable_n2_count);
	}

	size_t removed_mpr_count = minimize_mpr_set(new_mpr_set);
	debug_msg("MPRs removed: %u\n", XORP_UINT_CAST(removed_mpr_count));

	// Invariant: All reachable N2 must remain covered by MPRs after
	// minimizing the set.
	XLOG_ASSERT(covered_n2_count >= reachable_n2_count);

    } else {
	// All one-hop neighbors with non-zero willingness are considered
	// MPRs when the MPR algorithm has been disabled.
	// Typically this is used when the topology is extremely dense.
	// XXX This path is UNTESTED!
	size_t added_mpr_count = mark_all_n1_as_mprs(new_mpr_set);
	UNUSED(added_mpr_count);
    }

    if (! (new_mpr_set == _mpr_set)) {
	// TODO: Print the changes to the MPR set just like olsrd does.
	debug_msg("%s: MPR set changed, now: \n",
		  cstring(_fm.get_main_addr()));
	set<OlsrTypes::NeighborID>::const_iterator ii;
	for (ii = new_mpr_set.begin(); ii != new_mpr_set.end(); ii++) {
	    debug_msg("\t%s\n",
		      cstring(get_neighbor((*ii))->main_addr()));
	}

        // TODO: Section 8.5: We MAY trigger an early HELLO when the MPR
	// set changes. The RFC does not specify if this early HELLO
	// should be sent on all interfaces, or only those from which the
	// newly selected MPRs are reachable.
    }

    // The protocol simulator needs to see the entire MPR set for
    // keeping invariants.
    _mpr_set = new_mpr_set;
}

size_t
Neighborhood::mark_all_n1_as_mprs(set<OlsrTypes::NeighborID>& final_mpr_set)
{
    size_t mpr_count = 0;

    map<OlsrTypes::NeighborID, Neighbor*>::iterator ii;
    for (ii = _neighbors.begin(); ii != _neighbors.end(); ii++) {
	Neighbor* n =(*ii).second;

	if (n->willingness() == OlsrTypes::WILL_NEVER)
	    continue;

	n->set_is_mpr(true);
	final_mpr_set.insert(n->id());
	++mpr_count;
    }

    return mpr_count;
}

size_t
Neighborhood::minimize_mpr_set(set<OlsrTypes::NeighborID>& final_mpr_set)
    throw(BadTwoHopCoverage)
{
    size_t withdrawn_mpr_count = 0;

    //UNUSED(final_mpr_set);

    // 8.3.1, 5: Select all N such that it has been selected as an MPR
    // by the current MPR recount, in ascending order of willingness, and
    // consider if it is essential for coverage of its 2-hop neighbors.
    // If not, remove it from the MPR set.
    OlsrTypes::WillType will;
    map<OlsrTypes::NeighborID, Neighbor*>::iterator ii;

#if 1
    // XXX: Make a pass to ensure nodes with WILL_ALWAYS are
    // added to the final set.
    // Such nodes are ALWAYS MPRs even if redundant, and SHOULD NOT
    // be withdrawn by the loop below.
    // We need to do this because the protocol simulator currently relies
    // on being able to perform an exact comparison with our final
    // MPR set for protocol validation.
    // It should not be needed for production, as n->is_mpr() is only
    // examined when a TC message is being issued, or ANSN is being updated.
    for (ii = _neighbors.begin(); ii != _neighbors.end(); ii++) {
	const Neighbor* n =(*ii).second;
	if (n->willingness() == OlsrTypes::WILL_ALWAYS)
	    final_mpr_set.insert(n->id());
    }
#endif

    for (will = OlsrTypes::WILL_LOW; will < OlsrTypes::WILL_ALWAYS; will++) {
	// For each previously selected MPR,
	// in reverse order of ID. XXX this may not be deterministic.
	for (ii = _neighbors.begin(); ii != _neighbors.end(); ii++) {
	    Neighbor* n =(*ii).second;
	    if (n->is_mpr() && n->willingness() == will) {
		//
		// If any N2 covered by N is poorly covered without N,
		// do not remove N from the MPR set.
		//
		if (! is_essential_mpr(n)) {
		    //
		    // N is non-essential. Withdraw N from each of its N2.
		    //
		    const set<OlsrTypes::TwoHopLinkID>& n2_links =
			n->twohop_links();

		    // For each N2 of N.
		    set<OlsrTypes::TwoHopLinkID>::const_iterator jj;
		    for (jj = n2_links.begin(); jj != n2_links.end(); jj++) {
			TwoHopLink* l2 = _twohop_links[(*jj)];
			TwoHopNeighbor* n2 = l2->destination();

			// Withdraw N from this N2.
			n2->withdraw_covering_mpr(n->id());
			n->set_is_mpr(false);

			// Invariant: N2 must remain covered.
			if (! n2->is_covered()) {
			    xorp_throw(BadTwoHopCoverage, c_format(
				       "OLSR node %s has uncovered "
				       "TwoHopNode %u (%sreachable "
				       "%u two-hop links)",
				       cstring(_fm.get_main_addr()),
				       XORP_UINT_CAST(n2->id()),
				       n2->is_reachable() ? "" : "un",
				       XORP_UINT_CAST(n2->reachability())));
			}
		    }
		    ++withdrawn_mpr_count;
		} else {
		    // N remains an MPR. Add N to the produced MPR set.
		    final_mpr_set.insert(n->id());
		}
	    }
	}
    }

    return withdrawn_mpr_count;
}

void
Neighborhood::add_cand_mpr(const OlsrTypes::NeighborID nid)
{
    schedule_mpr_recount();

    // The nid parameter is not needed if MPR selection is being
    // performed for the entire N2 set, which is the default behaviour.
    UNUSED(nid);
}

void
Neighborhood::withdraw_cand_mpr(const OlsrTypes::NeighborID nid)
{
    schedule_mpr_recount();

    // The nid parameter is not needed if MPR selection is being
    // performed for the entire N2 set, which is the default behaviour.
    UNUSED(nid);
}

bool
Neighborhood::is_essential_mpr(const Neighbor* n)
{
    bool is_essential = false;
    const set<OlsrTypes::TwoHopLinkID>& n2_links = n->twohop_links();

    // For each reachable strict N2 of N, consider if its MPR status
    // is essential to covering the strict N2.
    set<OlsrTypes::TwoHopLinkID>::const_iterator ii;
    for (ii = n2_links.begin(); ii != n2_links.end(); ii++) {
	const TwoHopLink* l2 = _twohop_links[(*ii)];
	const TwoHopNeighbor* n2 = l2->destination();
	if (n2->is_strict() && n2->coverage() <= mpr_coverage()) {
	    is_essential = true;
	    break;
	}
    }

    return is_essential;
}

void
Neighborhood::reset_onehop_mpr_state()
{
    map<OlsrTypes::NeighborID, Neighbor*>::iterator ii;
    for (ii = _neighbors.begin(); ii != _neighbors.end(); ii++) {
	Neighbor* n = (*ii).second;
	n->set_is_mpr(false);
    }
}

size_t
Neighborhood::reset_twohop_mpr_state(ostringstream& oss)
{
    size_t n2_count = 0;

    // Clear all existing MPR coverage counts.
    map<OlsrTypes::TwoHopNodeID, TwoHopNeighbor*>::iterator ii;
    for (ii = _twohop_nodes.begin(); ii != _twohop_nodes.end(); ii++) {
	TwoHopNeighbor* n2 = (*ii).second;

	// Reset N2's MPR coverage.
	n2->reset_covering_mprs();

	// Amortize the runtime cost of tree walks by precomputing N2's
	// reachability. We can't distribute it, as it relies on
	// data known at this point in time.
	// [Computing the degree of each N is already distributed, and
	//  therefore amortized to whenever the topology around N changes.]
	update_twohop_reachability(n2);

	// If N2 is strict and is reachable, count it as a member of N2.
	if (n2->is_strict() && n2->is_reachable()) {
	    oss << "Counting 2-hop neighbor, is strict and reachable: " << n2->reachability()
		<< ", n2: " << n2->toStringBrief() << endl;
	    n2_count++;
	}
    }

    return n2_count;
}

void
Neighborhood::update_onehop_reachability(Neighbor* n)
{
    size_t reachability = 0;
    const set<OlsrTypes::TwoHopLinkID>& n2_links = n->twohop_links();

    // Consider each edge between this N and each N2.
    // If N2 is uncovered, count it.
    set<OlsrTypes::TwoHopLinkID>::const_iterator ii;
    for (ii = n2_links.begin(); ii != n2_links.end(); ii++) {
	TwoHopLink* l2 = _twohop_links[(*ii)];
	if (! l2->destination()->is_covered())
	    ++reachability;
    }

    n->set_reachability(reachability);
}

void
Neighborhood::update_twohop_reachability(TwoHopNeighbor* n2)
{
    size_t reachability = 0;
    const set<OlsrTypes::TwoHopLinkID>& n_links = n2->twohop_links();

    // Consider each edge between each N and this N2.
    // If the N is an MPR candidate, the edge is considered OK for
    // transit and is a link by which the N2 reachable, so count it.
    set<OlsrTypes::TwoHopLinkID>::const_iterator ii;
    for (ii = n_links.begin(); ii != n_links.end(); ii++) {
	TwoHopLink* l2 = _twohop_links[(*ii)];

	// Persistent MPR candidates take priority, hence short-circuit OR.
	//
	// XXX: Should check if l2 is a 'good' link for ETX
	// to be treated correctly during MPR computation.
	if (l2->nexthop()->is_persistent_cand_mpr() ||
	    l2->nexthop()->is_cand_mpr()) {
	    debug_msg("%s: %s is reachable via cand-mpr %s\n",
		      cstring(_fm.get_main_addr()),
		      cstring(l2->destination()->main_addr()),
		      cstring(l2->nexthop()->main_addr()));
	    ++reachability;
	}
    }

    n2->set_reachability(reachability);
}

size_t
Neighborhood::consider_persistent_cand_mprs(ostringstream& dbg)
{
    size_t persistent_mpr_count = 0;

    // Mark each WILL_ALWAYS neighbor as an MPR in this pass.
    map<OlsrTypes::NeighborID, Neighbor*>::iterator ii;
    for (ii = _neighbors.begin(); ii != _neighbors.end(); ii++) {
	Neighbor* n = (*ii).second;
	if (n->willingness() != OlsrTypes::WILL_ALWAYS)
	    continue;
	n->set_is_mpr(true);
	++persistent_mpr_count;
    }

    debug_msg("%s: persistent_mpr_count is %u\n",
	      cstring(_fm.get_main_addr()),
	      XORP_UINT_CAST(persistent_mpr_count));

    size_t covered_n2_count = 0;

    // Maintain the coverage count on each strict reachable two-hop
    // neighbor, by considering each two-hop link.
    //
    // XXX: To maintain the "poorly covered" invariant on each n2 in
    // minimize_mpr_set(), it's necessary to update the coverage count
    // whilst  we are here.
    //
    // The invariant is there to ensure that the implementation of
    // MPR_COVERAGE is correct. It may be removed in future when the code
    // has been more rigorously tested, and then the need to mark each n2
    // as covered by a WILL_ALWAYS node will go away.
    //
    map<OlsrTypes::TwoHopLinkID, TwoHopLink*>::iterator jj;
    for (jj = _twohop_links.begin(); jj != _twohop_links.end(); jj++) {
	TwoHopLink* l2 = (*jj).second;
	Neighbor* n = l2->nexthop();
	TwoHopNeighbor* n2 = l2->destination();

	// If N is WILL_ALWAYS, mark its destination in N2 as covered by
	// the persistent MPR N.
	// Reachability of N2 is implied by the existence of the link
	// and the willingness of N, so checking for it is redundant here.
	if (n2->is_strict() &&
	    n->willingness() == OlsrTypes::WILL_ALWAYS) {
	    debug_msg("%s: adding WILL_ALWAYS MPR %s\n",
		      cstring(_fm.get_main_addr()),
		      cstring(n->main_addr()));

	    XLOG_ASSERT(n->is_mpr());

	    n2->add_covering_mpr(n->id());
	    dbg << "Covered n2: " << n2->toStringBrief() << " in consider_persistent.\n";
	    covered_n2_count++;
	}
	else {
	    dbg << "NOT covering n2: " << n2->toStringBrief() << " in consider_persistent, strict: "
		<< n2->is_strict() << "  n: " << n->toStringBrief() << " n->willingness: " << n->willingness() << endl;
	}
    }

    // The count may be 0 if the persistent MPRs have no 2-hop links.
    debug_msg("%s: covered_n2_count is %u\n",
	      cstring(_fm.get_main_addr()),
	      XORP_UINT_CAST(covered_n2_count));

    return covered_n2_count;
}

size_t
Neighborhood::consider_poorly_covered_twohops(ostringstream& dbg)
{
    size_t covered_n2_count = 0;

    map<OlsrTypes::TwoHopNodeID, TwoHopNeighbor*>::iterator ii;
    for (ii = _twohop_nodes.begin(); ii != _twohop_nodes.end(); ii++) {
	TwoHopNeighbor* n2 = (*ii).second;
	if (n2->is_strict() && n2->reachability() == 1 &&
	    !n2->is_covered()) {
	    //
	    // N2 is a strict, uncovered two-hop neighbor with reachability
	    // of 1, after ETX is taken into account. For brevity,
	    // re-use find_best_twohop_link().
	    //
	    // TODO: XXX: Whilst this considers ETX, it does not
	    // handle the case in which no link with favorable ETX
	    // was found, in which case an uncaught exception
	    // will be raised -- until this is implemented correctly
	    // in update_twohop_reachability().
	    //
	    const TwoHopLink* l2 = find_best_twohop_link(n2);
	    Neighbor* n = l2->nexthop();
	    //
	    // N2 is therefore covered by N.
	    // Mark N2 as covered by N. Mark N as MPR.
	    //
	    n2->add_covering_mpr(n->id());
	    n->set_is_mpr(true);
	    dbg << "Counting poorly_covered n2: " << n2->toStringBrief() << " n is set as mpr: "
		<< n->toStringBrief() << endl;
	    covered_n2_count++;
	}
	else {
	    dbg << "NOT Counting poorly_covered n2: " << n2->toStringBrief() << "  strict: " << n2->is_strict()
		<< "  reachability: " << n2->reachability() << "  n2-covered: " << n2->is_covered() << endl;
	}
    }

    return covered_n2_count;
}

void
Neighborhood::consider_remaining_cand_mprs(const size_t n2_count,
					   size_t& covered_n2_count, ostringstream& dbg)
{
    typedef set<Neighbor*, CandMprOrderPred> CandMprBag;
    UNUSED(n2_count);

    CandMprBag cand_mprs;

    // Construct the MPR candidate set.
    // Skip the ALWAYS neighbors, they have already been covered.
    map<OlsrTypes::NeighborID, Neighbor*>::iterator ii;
    for (ii = _neighbors.begin(); ii != _neighbors.end(); ii++) {
	Neighbor* n = (*ii).second;
	if (n->is_cand_mpr() &&
	    n->willingness() != OlsrTypes::WILL_ALWAYS) {
	    // 8.4.1, 4.1:
	    // Recompute n's reachability. We cannot amortize or
	    // distribute this computation as it depends on the
	    // state of the MPR set at this point in time.
	    update_onehop_reachability(n);

	    // Consider N as a candidate MPR if and only if it
	    // covers any uncovered N2. n will point to a valid
	    // object throughout this scope.
	    if (n->reachability() > 0)
		cand_mprs.insert(n); // Insertion sort.
	}
	else {
	    dbg << "Not using n: " << n->toStringBrief() << " as cand_mpr, willingness: "
		<< n->willingness() << "  is_cand_mpr: " << n->is_cand_mpr() << "  is_mpr: "
		<< n->is_mpr() << endl;
	}
    }

    // 8.3.1, 4.2: Select as an MPR the node with highest
    // willingness, reachability and degree.
    CandMprBag::iterator jj;
    for (jj = cand_mprs.begin(); jj != cand_mprs.end(); jj++) {
	Neighbor* n = (*jj);

	dbg << "Checking neighbour: " << n->toStringBrief() << "  link count: "
	    << n->twohop_links().size() << endl;

	// If all N2 are covered, we're done.
	// How do you know you haven't counted duplicates??  Comments indicate that redundant
	// MPRs will be cleaned up below....  --Ben
	//if (covered_n2_count >= n2_count)
	//    break;

	// Mark each N2 reachable from this N as covered,
	// and mark this N as a candidate MPR.
	// Redundant MPRs will be later eliminated by minimize_mpr_set().
	const set<OlsrTypes::TwoHopLinkID>& links = n->twohop_links();
	set<OlsrTypes::TwoHopLinkID>::const_iterator kk;
	for (kk = links.begin(); kk != links.end(); kk++) {
	    TwoHopLink* l2 = _twohop_links[(*kk)];
	    TwoHopNeighbor* n2 = l2->destination();
	    if (n2->is_strict()) {
		// Mark this strict N2 as covered by N. Mark N as MPR.
		dbg << "Adding covering_mpr: " << n->toStringBrief() << "  to n2: " << n2->toStringBrief() << endl;
		n2->add_covering_mpr(n->id());
		n->set_is_mpr(true);
		covered_n2_count++;
	    }
	    else {
		dbg << "n2: " << n2->toStringBrief() << "  is strict, skipping.\n";
	    }
	}
    }
}

/*
 * Callback methods, for event processing.
 */

void
Neighborhood::event_link_sym_timer(OlsrTypes::LogicalLinkID linkid)
{
    XLOG_ASSERT(_links.find(linkid) != _links.end());
    LogicalLink* l = _links[linkid];

    // The timer may have fired at the same time as
    // the ASYM timer.
    if (l->link_type() != OlsrTypes::SYM_LINK)
	return;

    // Transition: SYM -> ASYM.
    debug_msg("LinkID %u: sym -> asym.\n", XORP_UINT_CAST(linkid));

    XLOG_ASSERT(_neighbors.find(l->neighbor_id()) != _neighbors.end());
    Neighbor* n = l->destination();

    // Propagate status change to neighbor.
    // 8.5: This is "a change in the neighborhood"
    n->update_link(linkid);

    //_rm->schedule_route_update();
}

void
Neighborhood::event_link_asym_timer(OlsrTypes::LogicalLinkID linkid)
{
    // Transition: ASYM -> LOST.
    debug_msg("LinkID %u: asym -> lost.\n", XORP_UINT_CAST(linkid));

    XLOG_ASSERT(_links.find(linkid) != _links.end());
    LogicalLink* l = _links[linkid];

    // Propagate status change to neighbor.
    // 8.5: This is "a change in the neighborhood"
    // XXX: Should we delete the neighbor at this time?
    // Or withdraw it from interfaces?
    XLOG_ASSERT(_neighbors.find(l->neighbor_id()) != _neighbors.end());
    Neighbor* n = l->destination();

    n->update_link(linkid);

    // 8.5: When we lose a neighbor, we lose all links to the
    // two-hop neighbors which traverse that neighbor.
    // This will obviously trigger a route recomputation.
    n->delete_all_twohop_links();

    //_rm->schedule_route_update();
}

void
Neighborhood::event_link_lost_timer(OlsrTypes::LogicalLinkID linkid)
{
    XLOG_UNFINISHED();	    // XXX TODO
    UNUSED(linkid);
}

void
Neighborhood::event_link_dead_timer(OlsrTypes::LogicalLinkID linkid)
{
    // Transition: * -> DEAD.
    debug_msg("LinkID %u expired, deleting it.\n", XORP_UINT_CAST(linkid));
    delete_link(linkid);
}

void
Neighborhood::event_mpr_selector_expired(const OlsrTypes::NeighborID nid)
{
    // Transition: * -> DEAD.
    debug_msg("MPR selector for neighbor %u expired, deleting it.\n",
	      XORP_UINT_CAST(nid));

    // 8.4.1: Deletion of MPR selector tuples occurs in case of
    // expiration of the timer.
    delete_mpr_selector(nid);
}

void
Neighborhood::event_twohop_link_dead_timer(const OlsrTypes::TwoHopLinkID tlid)
{
    // Transition: * -> DEAD.
    debug_msg("TwoHopLinkID %u -> DEAD.\n", XORP_UINT_CAST(tlid));

    // 4.3.2: N_time specifies the time at which the 2-hop tuple
    // expires and *MUST* be removed.
    // If this is the last link to the two-hop neighbor, the two-hop
    // neighbor will now be deleted.
    delete_twohop_link(tlid);
}

bool
Neighborhood::event_receive_hello(
    Message* msg,
    const IPv4& remote_addr,
    const IPv4& local_addr)
{
    HelloMessage* hello = dynamic_cast<HelloMessage *>(msg);
    if (0 == hello)
	return false;	// I did not consume this message.

    if (hello->ttl() != 1 || hello->hops() != 0) {
	debug_msg("Rejecting HELLO with ttl %u and hop-count %u\n",
		  hello->ttl(), hello->hops());
	XLOG_TRACE(_olsr.trace()._input_errors,
		   "Rejecting HELLO with ttl %u and hop-count %u\n",
		    hello->ttl(), hello->hops());

	return true;	// I consumed this message.
    }

    // We should never see HELLO messages originating from our own
    // main address, unless they are being looped back for some reason.
    if (_fm.get_main_addr() == hello->origin()) {
	debug_msg("Rejecting self-originated HELLO\n");
	XLOG_TRACE(_olsr.trace()._input_errors,
		   "Rejecting self-originated HELLO\n");
	return true;
    }

    // Invariant: I should only receive HELLO messages from an
    // enabled interface.
    XLOG_ASSERT(true == _fm.get_face_enabled(hello->faceid()));

    bool i_was_mentioned = false;
    LinkCode mylc(OlsrTypes::UNSPEC_LINK, OlsrTypes::NOT_NEIGH);
    const HelloMessage::LinkBag& linkbag = hello->links();

    //
    // Make a first pass through the link state information to see
    // if our own address is listed there, as this affects how the
    // link-state timers are updated, as well as telling us if the
    // neighbor selects us as an MPR.
    //
    // Section 7.1.1, 2.2: If the node finds the address of
    // the interface which received the HELLO message among
    // the addresses listed in the link message, then the
    // rules for updating the link tuple change.
    //
    if (! hello->links().empty()) {
	HelloMessage::LinkBag::const_iterator ii;
	for (ii = linkbag.begin(); ii != linkbag.end(); ii++) {
	    const LinkAddrInfo& lai = (*ii).second;
	    if (local_addr == lai.remote_addr()) {
		mylc = (*ii).first;
		i_was_mentioned = true;
		break;
	    }
	}
    }

    // Update the link state tuple, creating it if needed.
    bool is_new_link = false;
    OlsrTypes::LogicalLinkID linkid;
    try {
	linkid = update_link(hello->faceid(), remote_addr, local_addr,
			     hello->expiry_time(), is_new_link);

	// 7.1.1: Update the link state timers.
	_links[linkid]->update_timers(hello->expiry_time(),
				      i_was_mentioned, mylc);

    } catch(BadLogicalLink& bl) {
	debug_msg("could not update link\n");
	return true;	// I consumed this message.
    }

    // Update the neighbor database tuple, creating it if needed.
    //  8.4.1: Update the MPR selector set, if the neighbor selects
    //  me as an MPR.
    bool is_new_neighbor = false;
    bool is_mpr_selector = i_was_mentioned && mylc.is_mpr_neighbor();
    OlsrTypes::NeighborID nid;
    try {
	nid = update_neighbor(hello->origin(),
			      linkid,
			      is_new_link,
			      hello->willingness(),
			      is_mpr_selector,
			      hello->expiry_time(),
			      is_new_neighbor);

    } catch(BadNeighbor& bn) {
	debug_msg("could not update neighbor\n");
	if (is_new_link)
	    delete_link(linkid);
	return true;	// I consumed this message.
    }

    Neighbor* n = _neighbors[nid];

    // Process two-hop neighbors, if link(s) with neighbor are now symmetric.
    if (n->is_sym()) {
	HelloMessage::LinkBag::const_iterator ii;
	for (ii = linkbag.begin(); ii != linkbag.end(); ii++) {
	    const LinkAddrInfo& lai = (*ii).second;
	    // 8.2.1, 1.1: A node cannot be its own two-hop neighbor.
	    if (_fm.is_local_addr(lai.remote_addr()))
		continue;
	    switch ((*ii).first.neighbortype()) {
	    case OlsrTypes::MPR_NEIGH:
	    case OlsrTypes::SYM_NEIGH:
		// 8.2.1, 1.2: Create or update two-hop neighbor tuple.
		// Interpreted as: add a link to a two-hop neighbor.
		// May throw BadTwoHopLink.
		update_twohop_link(lai, *n, hello->faceid(),
				   hello->expiry_time());
		break;
	    case OlsrTypes::NOT_NEIGH:
		// 8.2.1, 2: Delete two-hop neighbor tuple.
		// Interpreted as: delete a neighbor's link to a
		// two-hop neighbor.
		delete_twohop_link_by_addrs(hello->origin(),
					    lai.remote_addr());
		break;
	    default:
		break;
	    }
	}
    }

    return true;	// I consumed this message.
}

bool
Neighborhood::event_send_tc()
{
    debug_msg("%s: TC timer fired\n", cstring(_fm.get_main_addr()));

#ifdef DETAILED_DEBUG
    // 9.3 TC timer should only fire if running or finishing.
    XLOG_ASSERT(_tc_timer_state == TC_RUNNING ||
		_tc_timer_state == TC_FINISHING);
#endif

    TcMessage* tc = new TcMessage();

    tc->set_expiry_time(get_topology_hold_time());
    tc->set_origin(_fm.get_main_addr());
    tc->set_ttl(OlsrTypes::MAX_TTL);
    tc->set_hop_count(0);
    tc->set_seqno(_fm.get_msg_seqno());

    if (_tc_timer_state == TC_RUNNING) {
	size_t curr_ans_count = 0;
	size_t curr_ans_changes = 0;

	// Compute the Advertised Neighbor Set, according to the
	// current TC_REDUNDANCY mode.
	map<OlsrTypes::NeighborID, Neighbor*>::const_iterator ii;
	for (ii = _neighbors.begin(); ii != _neighbors.end(); ii++) {
	    Neighbor* n = (*ii).second;

	    // Maintain a count of the number of Neighbors which have
	    // changed state since the last TC broadcast.
	    bool was_advertised = n->is_advertised();
	    bool is_advertised = is_tc_advertised_neighbor(n);

	    if (was_advertised != is_advertised)
		++curr_ans_changes;

	    if (is_advertised) {
		++curr_ans_count;
		tc->add_neighbor(n->main_addr());
	    }

	    // Record the status of this neighbor in the current TC phase.
	    n->set_is_advertised(is_advertised);
	}

	// Deal with transitions to an empty ANS.
	if (curr_ans_count == 0) {
	    XLOG_ASSERT(tc->neighbors().empty());
	    if (_tc_previous_ans_count == 0) {
		// If the TC timer was started because we have a non-empty
		// MPR selector set, but the ANS set became empty before
		// the current timer phase, transition to STOPPED state.
		stop_tc_timer();
		return false;
	    } else {
		// If the ANS just computed was empty, transition to
		// FINISHING state to signal that we now have no neighbors
		// to advertise.
		finish_tc_timer();
	    }
	}

	// Deal with a non-empty ANS.
	// If there were any ANS changes, bump the ANSN for this TC.
	if (curr_ans_count > 0 && curr_ans_changes > 0)
	    ++_tc_current_ansn;

	// Maintain a count of ANS between TC timer phases for comparison.
	_tc_previous_ans_count = curr_ans_count;
    }

    tc->set_ansn(_tc_current_ansn);

    bool is_flooded = _fm.flood_message(tc);	// consumes tc
    UNUSED(is_flooded);

    // XXX no refcounting
    delete tc;

    // If TC timer is in 'finishing' state, and no further ticks are
    // pending, ensure that the TC timer is now unscheduled.
    if (_tc_timer_state == TC_FINISHING) {
	if (--_tc_timer_ticks_remaining == 0) {
	    _tc_timer_state = TC_STOPPED;
	    return false;
	}
    }

    return true;
}

/*
 * Timer manipulation methods.
 */

void
Neighborhood::stop_all_timers()
{
    stop_tc_timer();
}

void
Neighborhood::start_tc_timer()
{
    debug_msg("%s -> TC_RUNNING\n", cstring(_fm.get_main_addr()));

    _tc_timer_state = TC_RUNNING;
    _tc_timer = _eventloop.
	new_periodic(get_tc_interval(),
		     callback(this, &Neighborhood::event_send_tc));
}

void
Neighborhood::stop_tc_timer()
{
    debug_msg("%s -> TC_STOPPED\n", cstring(_fm.get_main_addr()));

    _tc_timer.clear();
    _tc_timer_state = TC_STOPPED;
}

void
Neighborhood::finish_tc_timer()
{
    XLOG_ASSERT(_tc_timer_state == TC_RUNNING ||
		_tc_timer_state == TC_FINISHING);

    if (_tc_timer_state == TC_RUNNING) {
	debug_msg("%s -> TC_FINISHING\n", cstring(_fm.get_main_addr()));
	_tc_timer_state = TC_FINISHING;

	// Bump the TC timer to make sure everyone notices.
	++_tc_current_ansn;

	// Run for at least another 3 ticks (TOP_HOLD_TIME / TOP_INTERVAL).
	_tc_timer_ticks_remaining = 3;
    }
}

void
Neighborhood::reschedule_immediate_tc_timer()
{
    XLOG_ASSERT(_tc_timer_state == TC_RUNNING ||
		_tc_timer_state == TC_FINISHING);

    _tc_timer.schedule_now();
}

void
Neighborhood::restart_tc_timer()
{
    reschedule_tc_timer();
}

void
Neighborhood::reschedule_tc_timer()
{
    XLOG_ASSERT(_tc_timer_state == TC_RUNNING ||
		_tc_timer_state == TC_FINISHING);

    _tc_timer.reschedule_after(get_tc_interval());
}
