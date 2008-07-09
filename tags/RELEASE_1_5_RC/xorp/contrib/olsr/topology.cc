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

#ident "$XORP: xorp/contrib/olsr/topology.cc,v 1.1 2008/04/24 15:19:56 bms Exp $"

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

TopologyManager::TopologyManager(Olsr& olsr, EventLoop& eventloop,
    FaceManager& fm, Neighborhood& nh)
     : _olsr(olsr),
       _eventloop(eventloop),
       _fm(fm),
       _nh(nh),
       _rm(0),
       _next_mid_id(1),
       _next_tcid(1)
{
    _nh.set_topology_manager(this);

    _fm.add_message_cb(callback(this,
				&TopologyManager::event_receive_tc));
    _fm.add_message_cb(callback(this,
				&TopologyManager::event_receive_mid));
}

TopologyManager::~TopologyManager()
{
    _fm.delete_message_cb(callback(this,
				   &TopologyManager::event_receive_tc));
    _fm.delete_message_cb(callback(this,
				   &TopologyManager::event_receive_mid));

    clear_tc_entries();
    clear_mid_entries();

    XLOG_ASSERT(_topology.empty());
    XLOG_ASSERT(_mids.empty());
}

/*
 * TC entries.
 */

void
TopologyManager::update_tc_entry(const IPv4& dest_addr,
    const IPv4& origin_addr,
    const uint16_t distance,
    const uint16_t ansn,
    const TimeVal& vtime,
    bool& is_created)
    throw(BadTopologyEntry)
{
    debug_msg("DestAddr %s OriginAddr %s Distance %u ANSN %u Expiry %s\n",
	      cstring(dest_addr),
	      cstring(origin_addr),
	      XORP_UINT_CAST(distance),
	      XORP_UINT_CAST(ansn),
	      cstring(vtime));

    OlsrTypes::TopologyID tcid;
    bool is_found = false;

    TcDestMap::iterator ii = _tc_destinations.find(dest_addr);
    for (; ii != _tc_destinations.end(); ii++) {
	tcid = (*ii).second;
	if (_topology[tcid]->lasthop() == origin_addr) {
	    is_found = true;
	    break;
	}
    }

    TimeVal now;
    _eventloop.current_time(now);

    if (is_found) {
	TopologyEntry* tc = _topology[tcid];

	// 9.5, 4.1: Update holding time of the existing tuple in the
	// topology set.
	tc->update_timer(vtime);

	// Update the distance of this TC entry, as the hop-count may
	// have changed due to other changes in topology.
	update_tc_distance(tc, distance);
    } else {
	// 9.5, 4.2: Record a new tuple in the topology set.
	// May throw BadTopologyEntry exception.
	tcid = add_tc_entry(dest_addr, origin_addr, distance, ansn, vtime);
	debug_msg("%s: Added TC entry %u.\n",
		  cstring(_fm.get_main_addr()),
		  XORP_UINT_CAST(tcid));
    }

    is_created = !is_found;
}

OlsrTypes::TopologyID
TopologyManager::add_tc_entry(const IPv4& dest_addr,
    const IPv4& origin_addr,
    const uint16_t distance,
    const uint16_t ansn,
    const TimeVal& expiry_time)
    throw(BadTopologyEntry)
{
    debug_msg("DestAddr %s OriginAddr %s Distance %u ANSN %u Expiry %s\n",
	      cstring(dest_addr),
	      cstring(origin_addr),
	      XORP_UINT_CAST(distance),
	      XORP_UINT_CAST(ansn),
	      cstring(expiry_time));

    OlsrTypes::TopologyID tcid = _next_tcid++;

    // Throw an exception if we overflowed the TopologyID space.
    if (0 != _topology.count(tcid)) {
	xorp_throw(BadTopologyEntry,
		   c_format("Mapping for TopologyID %u already exists",
			    XORP_UINT_CAST(tcid)));
    }

    _topology[tcid] = new TopologyEntry(_eventloop, this, tcid,
					dest_addr, origin_addr,
					distance, ansn, expiry_time);

    // Add the new link to the multimaps.
    _tc_distances.insert(make_pair(distance, tcid));
    _tc_destinations.insert(make_pair(dest_addr, tcid));
    _tc_lasthops.insert(make_pair(origin_addr, tcid));

    return tcid;
}

bool
TopologyManager::delete_tc_entry(const OlsrTypes::TopologyID tcid)
{
    debug_msg("Tcid %u\n", XORP_UINT_CAST(tcid));

    map<OlsrTypes::TopologyID, TopologyEntry*>::iterator ii =
	_topology.find(tcid);
    if (ii == _topology.end())
	return false;

    TopologyEntry* t = (*ii).second;

    // Remove from destination map.
    pair<TcDestMap::iterator, TcDestMap::iterator> ra =
	_tc_destinations.equal_range(t->destination());
    for (TcDestMap::iterator jj = ra.first; jj != ra.second; ) {
	if ((*jj).second == tcid) {
	    _tc_destinations.erase(jj);
	    break;
	}
	jj++;
    }

    // Remove from lasthop map.
    pair<TcLasthopMap::iterator, TcLasthopMap::iterator> rb =
	_tc_lasthops.equal_range(t->lasthop());
    for (TcLasthopMap::iterator kk = rb.first; kk != rb.second; ) {
	if ((*kk).second == tcid) {
	    _tc_lasthops.erase(kk);
	    break;
	}
	kk++;
    }

    // Remove from distance map.
    pair<TcDistanceMap::iterator, TcDistanceMap::iterator> rc =
	_tc_distances.equal_range(t->distance());
    for (TcDistanceMap::iterator ll = rc.first; ll != rc.second; ) {
	if ((*ll).second == t->id()) {
	    _tc_distances.erase(ll);
	    break;
	}
	ll++;
    }

    // Remove from ID map.
    _topology.erase(ii);
    delete t;

    // May be called from destructor.
    if (_rm)
	_rm->schedule_route_update();

    return true;
}

// XXX: This is a definite candidate for a Boost multi_index container.

void
TopologyManager::clear_tc_entries()
{
    while (! _topology.empty())
	delete_tc_entry((*_topology.begin()).first);
}

bool
TopologyManager::apply_tc_ansn(const uint16_t ansn,
			       const IPv4& origin_addr)
{
    debug_msg("ANSN %u Origin %s\n",
	      XORP_UINT_CAST(ansn),
	      cstring(origin_addr));

    // The ANSN is regarded as valid until proven otherwise.
    bool is_valid = true;

    // Find the tuples, if any, recorded from the origin.
    TcLasthopMap::iterator ii, jj;
    ii = _tc_lasthops.find(origin_addr);
    while (is_valid && ii != _tc_lasthops.end()) {
	jj = ii++;
	OlsrTypes::TopologyID tcid = (*jj).second;

	if (is_seq_newer(_topology[tcid]->seqno(), ansn)) {
	    // 9.5, 2: If any tuple in the topology set has T_seq > ANSN,
	    // then the TC message was received out of order and MUST be
	    // silently discarded.
	    is_valid = false;
	} else {
	    // 9.5, 3: All tuples in the topology set where T_seq < ANSN
	    // MUST be removed from the topology set.
	    delete_tc_entry(tcid);
	}
    }

    return is_valid;
}

OlsrTypes::TopologyID
TopologyManager::get_topologyid(const IPv4& dest_addr,
				const IPv4& lasthop_addr)
    throw(BadTopologyEntry)
{
    debug_msg("DestAddr %s LasthopAddr %s\n",
	      cstring(dest_addr),
	      cstring(lasthop_addr));

    bool is_found = false;
    OlsrTypes::TopologyID tcid;

    multimap<IPv4, OlsrTypes::TopologyID>::const_iterator ii =
	_tc_destinations.find(dest_addr);
    while (ii != _tc_destinations.end()) {
	tcid = (*ii).second;
	if (_topology[tcid]->lasthop() == lasthop_addr) {
	    is_found = true;
	    break;
	}
	ii++;
    }

    if (!is_found) {
	xorp_throw(BadTopologyEntry,
		   c_format("No mapping for %s exists", cstring(dest_addr)));
    }

    return tcid;
}

const TopologyEntry*
TopologyManager::get_topology_entry_by_id(const OlsrTypes::TopologyID tcid)
    const throw(BadTopologyEntry)
{
    TcIdMap::const_iterator ii = _topology.find(tcid);

    if (ii == _topology.end()) {
	xorp_throw(BadTopologyEntry,
		   c_format("No mapping for %u exists", XORP_UINT_CAST(tcid)));
    }

    return (*ii).second;
}

void
TopologyManager::get_topology_list(list<OlsrTypes::TopologyID>& tclist) const
{
    TcIdMap::const_iterator ii;

    for (ii = _topology.begin(); ii != _topology.end(); ii++)
	tclist.push_back((*ii).first);
}

vector<IPv4>
TopologyManager::get_tc_neighbor_set(const IPv4& origin_addr, uint16_t& ansn)
    throw(BadTopologyEntry)
{
    debug_msg("MyMainAddr %s Origin %s\n",
	      cstring(_fm.get_main_addr()),
	      cstring(origin_addr));

    size_t found_tc_count = 0;
    vector<IPv4> addrs;

    pair<TcLasthopMap::iterator, TcLasthopMap::iterator> rl =
	_tc_lasthops.equal_range(origin_addr);
    for (TcLasthopMap::iterator ii = rl.first; ii != rl.second; ii++) {
	TopologyEntry* t = _topology[(*ii).second];
	XLOG_ASSERT(t != 0);	// paranoia

	// If this is the first match, record the ANSN.
	if (ii == rl.first)
	    ansn = t->seqno();

#ifdef DETAILED_DEBUG
	// Invariant: the ANSN of all entries for this peer must be the same.
	XLOG_ASSERT(t->seqno() == ansn);
#endif

	addrs.push_back(t->destination());
	++found_tc_count;
    }

    if (found_tc_count == 0) {
	// Check to see if this origin was recorded as having an empty
	// neighbor set.
	// OLSR nodes originating TC broadcasts MUST send an
	// empty TC message with a new ANSN to invalidate any existing
	// topology information when they stop being selected as MPRs.
	TcFinalSeqMap::const_iterator ii = _tc_final_seqnos.find(origin_addr);
	if (ii != _tc_final_seqnos.end()) {
	    ansn = (*ii).second;
	    debug_msg("%s: found final ansn %u for %s\n",
		      cstring(_fm.get_main_addr()),
		      XORP_UINT_CAST(ansn),
		      cstring(origin_addr));
	    return addrs;
	}
	// The origin is not known to TopologyManager.
	// This is a distinct condition from "there were no entries
	// for the given origin".
	xorp_throw(BadTopologyEntry,
		   c_format("No mapping for %s exists", cstring(origin_addr)));
    }

    return addrs;
}

uint16_t
TopologyManager::get_tc_distance(const IPv4& origin_addr,
				 const IPv4& dest_addr)
    throw(BadTopologyEntry)
{
    debug_msg("MyMainAddr %s Origin %s Dest %s\n",
	      cstring(_fm.get_main_addr()),
	      cstring(origin_addr),
	      cstring(dest_addr));

    bool is_found = false;
    uint16_t distance = 0;

    pair<TcLasthopMap::iterator, TcLasthopMap::iterator> rl =
	_tc_lasthops.equal_range(origin_addr);
    for (TcLasthopMap::iterator ii = rl.first; ii != rl.second; ii++) {
	TopologyEntry* t = _topology[(*ii).second];
	if (t->destination() == dest_addr) {
	    is_found = true;
	    distance = t->distance();
	    break;
	}
    }

    if (! is_found) {
	// No TC entry was found matching that origin and destination.
	xorp_throw(BadTopologyEntry,
		   c_format("No mapping for (%s, %s) exists",
			    cstring(origin_addr),
			    cstring(dest_addr)));
    }

    return distance;
}

size_t
TopologyManager::get_tc_lasthop_count_by_dest(const IPv4& dest_addr)
{
    debug_msg("MyMainAddr %s Dest %s\n",
	      cstring(_fm.get_main_addr()),
	      cstring(dest_addr));

    size_t lasthop_count = 0;

    pair<TcDestMap::iterator, TcDestMap::iterator> rl =
	_tc_destinations.equal_range(dest_addr);
    for (TcDestMap::iterator ii = rl.first; ii != rl.second; ii++) {
	++lasthop_count;
    }

    return lasthop_count;
}

size_t
TopologyManager::tc_node_count() const
{
    size_t unique_key_count = 0;

    // Count the number of unique keys (main addresses) in the lasthop map.
    TcLasthopMap::const_iterator ii;
    for (ii = _tc_lasthops.begin();
	 ii != _tc_lasthops.end();
	 ii = _tc_lasthops.upper_bound((*ii).first))
    {
	unique_key_count++;
    }

    return unique_key_count;
}

void
TopologyManager::update_tc_distance(TopologyEntry* tc, uint16_t distance)
{
    if (tc->distance() == distance)
	return;

    // If the TC entry is already recorded for this distance, remove it.
    pair<TcDistanceMap::iterator, TcDistanceMap::iterator> range =
	_tc_distances.equal_range(distance);
    for (TcDistanceMap::iterator ii = range.first; ii != range.second; ) {
	if ((*ii).second == tc->id()) {
	    _tc_distances.erase(ii);
	    break;
	}
	ii++;
    }

    // Update the TC's distance, and rewire it into the distance map.
    tc->set_distance(distance);
    _tc_distances.insert(make_pair(distance, tc->id()));

#ifdef DETAILED_DEBUG
    // Invariant: the ID of the topology entry must appear once and
    // only once in the distances map.
    assert_tc_distance_is_unique(tc->id());
#endif
}

void
TopologyManager::assert_tc_distance_is_unique(const OlsrTypes::TopologyID tcid)
    throw(BadTopologyEntry)
{
#ifdef DETAILED_DEBUG
    size_t id_seen_count = 0;

    multimap<uint16_t, OlsrTypes::TopologyID>::const_iterator ii;
    for (ii = _tc_distances.begin(); ii != _tc_distances.end(); ii++) {
	if (tcid == (*ii).second) {
	    id_seen_count++;
	}
    }

    if (id_seen_count != 1) {
	xorp_throw(BadTopologyEntry,
		   c_format("Duplicate TopologyID %u in _tc_distances: "
			    "appeared %u times.",
			    XORP_UINT_CAST(tcid),
			    XORP_UINT_CAST(id_seen_count)));
    }
#else
    UNUSED(tcid);
#endif // DETAILED_DEBUG
}

void
TopologyManager::assert_tc_ansn_is_identical(const IPv4& origin_addr)
    throw(BadTopologyEntry)
{
#ifdef DETAILED_DEBUG
    bool is_origin_found = false;
    bool is_seq_match_failed = false;
    uint16_t first_ansn = 0;

    TopologyEntry* t = 0;

    TcIdMap::const_iterator ii;
    for (ii = _topology.begin(); ii != _topology.end(); ii++) {
	t = (*ii).second;
	if (t->lasthop() == origin_addr) {
	    if (! is_origin_found) {
		// First match; record first sequence number seen
		// for this origin.
		first_ansn = t->seqno();
		is_origin_found = true;
	    } else {
		// We've seen at least one entry for this origin before.
		// Do the sequence numbers match, is the ANSN consistent?
		if (t->seqno() != first_ansn) {
		    is_seq_match_failed = true;
		    break;
		}
	    }
	}
    }

    if (is_origin_found && is_seq_match_failed) {
	xorp_throw(BadTopologyEntry,
		   c_format("Inconsistent ANSN (%s, %s, %u) in TopologyID %u",
			    cstring(t->lasthop()),
			    cstring(t->destination()),
			    XORP_UINT_CAST(t->seqno()),
			    XORP_UINT_CAST(t->id())));
    }

#else
    UNUSED(origin_addr);
#endif
}

/*
 * MID entries.
 */

void
TopologyManager::update_mid_entry(const IPv4& main_addr,
				  const IPv4& iface_addr,
				  const uint16_t distance,
				  const TimeVal& vtime,
				  bool& is_mid_created)
    throw(BadMidEntry)
{
    debug_msg("MyMainAddr %s MainAddr %s IfaceAddr %s Distance %u Vtime %s\n",
	      cstring(_fm.get_main_addr()),
	      cstring(main_addr),
	      cstring(iface_addr),
	      XORP_UINT_CAST(distance),
	      cstring(vtime));

    is_mid_created = false;

    // Do not add redundant MID entries.
    if (main_addr == iface_addr) {
	debug_msg("Rejecting MID entry from %s for its main address.\n",
		  cstring(main_addr));
	XLOG_TRACE(_olsr.trace()._input_errors,
		   "Rejecting MID entry from %s for its main address.",
		   cstring(main_addr));
	return;
    }

    bool is_found = false;
    MidEntry* mie = 0;

    // Look for MID entry's interface address in existing address map,
    // given origin address.
    pair<MidAddrMap::iterator, MidAddrMap::iterator> range =
	_mid_addr.equal_range(main_addr);
    for (MidAddrMap::iterator ii = range.first; ii != range.second; ii++) {
	mie = _mids[(*ii).second];
	if (mie->iface_addr() == iface_addr) {
	    is_found = true;
	    break;
	}
    }

    if (is_found) {
	// Section 5.4, 2.1: Update existing MID tuple.
	mie->update_timer(vtime);
	mie->set_distance(distance);
    } else {
	// Section 5.4, 2.2: Create new MID tuple.
	add_mid_entry(main_addr, iface_addr, distance, vtime);
	is_mid_created = true;
    }
}

void
TopologyManager::add_mid_entry(const IPv4& main_addr,
			       const IPv4& iface_addr,
			       const uint16_t distance,
			       const TimeVal& vtime)
    throw(BadMidEntry)
{
    debug_msg("MyMainAddr %s MainAddr %s IfaceAddr %s Distance %u Vtime %s\n",
	      cstring(_fm.get_main_addr()),
	      cstring(main_addr),
	      cstring(iface_addr),
	      XORP_UINT_CAST(distance),
	      cstring(vtime));

    // Section 5.4, 2.2: Create new MID tuple.
    OlsrTypes::MidEntryID mid_id = _next_mid_id++;

    // Throw an exception if we overflow the MID ID space.
    if (0 != _mids.count(mid_id)) {
	xorp_throw(BadMidEntry,
		   c_format("Mapping for %u already exists",
			    XORP_UINT_CAST(mid_id)));
    }

    _mids[mid_id] = new MidEntry(_eventloop, this, mid_id, iface_addr,
				 main_addr, distance, vtime);

    // Tot up another MID entry for this main address and MID entry combo.
    _mid_addr.insert(make_pair(main_addr, mid_id));

    debug_msg("new MidEntryID %u\n", XORP_UINT_CAST(mid_id));
}

bool
TopologyManager::delete_mid_entry(const OlsrTypes::MidEntryID mid_id)
{
    debug_msg("MyMainAddr %s MidEntryID %u\n",
	      cstring(_fm.get_main_addr()),
	      XORP_UINT_CAST(mid_id));

    bool is_deleted = false;

    map<OlsrTypes::MidEntryID, MidEntry*>::iterator ii = _mids.find(mid_id);
    if (ii != _mids.end()) {
	MidEntry* mie = (*ii).second;

	// Withdraw mid_id from multimap of MID IDs per primary address.
	pair<MidAddrMap::iterator, MidAddrMap::iterator> range =
	    _mid_addr.equal_range(mie->main_addr());
	for (MidAddrMap::iterator jj = range.first; jj != range.second; jj++) {
	    if ((*jj).second == mid_id) {
		// Only the first match by ID should be deleted.
		// Duplicates of the second key, mid_id, SHOULD NOT exist.
		_mid_addr.erase(jj);
		break;
	    }
	}

	delete mie;
	_mids.erase(ii);

	is_deleted = true;
	debug_msg("MidEntryID %u was deleted.\n", XORP_UINT_CAST(mid_id));
    }

    if (is_deleted) {
	// If any MID address entry was deleted, schedule a route
	// computation.
	// TODO: Find a way of optimizing the processing of MID entries
	// in this way, we should just be able to withdraw the MID-derived
	// routes for this entry in the routing table.
	if (_rm)
	    _rm->schedule_route_update();
    }

    return is_deleted;
}

void
TopologyManager::clear_mid_entries()
{
    map<OlsrTypes::MidEntryID, MidEntry*>::iterator ii, jj;
    for (ii = _mids.begin(); ii != _mids.end(); ) {
	jj = ii++;
	delete (*jj).second;
	_mids.erase(jj);
    }
}

vector<IPv4>
TopologyManager::get_mid_addresses(const IPv4& main_addr)
{
#if 0
    debug_msg("%s MyMainAddr %s MainAddr %s\n",
	      __func__,
	      cstring(_fm.get_main_addr())
	      cstring(main_addr));
#endif
    vector<IPv4> addrs;

    pair<MidAddrMap::const_iterator, MidAddrMap::const_iterator> range =
	_mid_addr.equal_range(main_addr);
    MidAddrMap::const_iterator ii;
    for (ii = range.first; ii != range.second; ii++) {
	MidEntry* mie = _mids[(*ii).second];
	addrs.push_back(mie->iface_addr());
    }

    return addrs;
}

// Only used by regression tests.

uint16_t
TopologyManager::get_mid_address_distance(const IPv4& main_addr,
					  const IPv4& iface_addr)
    throw(BadMidEntry)
{
    debug_msg("MyMainAddr %s MainAddr %s IfaceAddr %s\n",
	      cstring(_fm.get_main_addr()),
	      cstring(main_addr),
	      cstring(iface_addr));

    bool is_found = false;
    MidEntry* mie = 0;

    // Look up all matching interface addresses for main_addr,
    // and find the single matching MidEntry.
    pair<MidAddrMap::const_iterator, MidAddrMap::const_iterator> range =
	_mid_addr.equal_range(main_addr);
    MidAddrMap::const_iterator ii;
    for (ii = range.first; ii != range.second; ii++) {
	mie = _mids[(*ii).second];
	if (mie->iface_addr() == iface_addr) {
	    is_found = true;
	    break;
	}
    }

    if (! is_found) {
	xorp_throw(BadMidEntry,
		   c_format("No mapping for (%s, %s) exists",
			    cstring(main_addr),
			    cstring(iface_addr)));
    }

    return mie->distance();
}

IPv4
TopologyManager::get_main_addr_of_mid(const IPv4& mid_addr)
    throw(BadMidEntry)
{
    MidEntry* mie = 0;
    bool is_found = false;

    map<OlsrTypes::MidEntryID, MidEntry*>::const_iterator ii;
    for (ii = _mids.begin(); ii != _mids.end(); ii++) {
	mie = (*ii).second;
	if (mie->iface_addr() == mid_addr) {
	    is_found = true;
	    break;
	}
    }

    if (!is_found) {
	xorp_throw(BadMidEntry,
		   c_format("No mapping for %s exists", cstring(mid_addr)));
    }

    return mie->main_addr();
}

size_t
TopologyManager::mid_node_count() const
{
    size_t unique_key_count = 0;

    // Count the number of unique keys (main addresses) in the multimap.
    multimap<IPv4, OlsrTypes::MidEntryID>::const_iterator ii;
    for (ii = _mid_addr.begin();
	 ii != _mid_addr.end();
	 ii = _mid_addr.upper_bound((*ii).first))
    {
	unique_key_count++;
    }

    return unique_key_count;
}

const MidEntry*
TopologyManager::get_mid_entry_by_id(const OlsrTypes::MidEntryID midid) const
    throw(BadTopologyEntry)
{
    MidIdMap::const_iterator ii = _mids.find(midid);

    if (ii == _mids.end()) {
	xorp_throw(BadMidEntry,
		   c_format("No mapping for %u exists",
			    XORP_UINT_CAST(midid)));
    }

    return (*ii).second;
}

void
TopologyManager::get_mid_list(list<OlsrTypes::MidEntryID>& midlist) const
{
    MidIdMap::const_iterator ii;

    for (ii = _mids.begin(); ii != _mids.end(); ii++)
	midlist.push_back((*ii).first);
}


/*
 * RouteManager interaction.
 */

void
TopologyManager::push_topology()
{
    size_t tc_link_added_count = 0;

    XLOG_ASSERT(_rm != 0);

    // For all entries at each recorded radius in the topology set.
    TcDistanceMap::const_iterator ii, jj;
    for (ii = _tc_distances.begin(); ii != _tc_distances.end(); ) {
	// Look up the range of TC entries at the current radius.
	uint16_t hops = (*ii).first;

	// The following is equivalent to _tc_distances.upper_bound(hops);
	// equal_range() has a more efficient expansion.
	pair<TcDistanceMap::const_iterator,
	     TcDistanceMap::const_iterator> rd =
	    _tc_distances.equal_range(hops);
	ii = rd.second;

	// Skip neighbors with redundant TC information; that is, those
	// which SHOULD have been covered by link state information in
	// the Neighborhood as they are less than 2 hops away.
	// We can't ignore TCs from two-hop neighbors because they may
	// know about other nodes which our neighbors don't. In any event
	// the SPF calculation will have screened those out.
	// Print detailed information about such hops if debugging.
	if (hops < 2) {
	    debug_msg("%s: skipping %u redundant TC entries at distance %u\n",
		      cstring(_fm.get_main_addr()),
		      XORP_UINT_CAST(std::distance(rd.first, rd.second)),
		      XORP_UINT_CAST(hops));
	    continue;
	}

	// 10, 3: The execution will stop if no new TC entry is recorded
	// in an iteration.
	if (0 == std::distance(rd.first, rd.second))
	    break;

	// For each link in the topology set at the current radius.
	for (jj = rd.first; jj != rd.second; jj++) {
	    TopologyEntry* tc = _topology[(*jj).second];

	    // Notify RouteManager of the link's existence.
	    // Duplicates covered by N1/N2 will return false.
	    // If it cannot connect it with any node on the existing
	    // boundary of the SPT topology, add_tc_link() will reject it
	    // by returning false.
	    if (true == _rm->add_tc_link(tc))
		++tc_link_added_count;
	}
    }

    debug_msg("%u TC entries pushed to SPT.\n",
	      XORP_UINT_CAST(tc_link_added_count));
}

/*
 * Event handlers.
 */

bool
TopologyManager::event_receive_tc(
    Message* msg,
    const IPv4& remote_addr,
    const IPv4& local_addr)
{
    TcMessage* tc = dynamic_cast<TcMessage *>(msg);
    if (0 == tc)
	return false;	// not for me

    // Put this after the cast to skip false positives.
    debug_msg("[TC] MyMainAddr %s Src %s RecvIf %s\n",
	      cstring(_fm.get_main_addr()),
	      cstring(remote_addr),
	      cstring(local_addr));

    // 9.5.1 Sender of packet must be in symmetric 1-hop neighborhood.
    if (! _nh.is_sym_neighbor_addr(remote_addr)) {
	debug_msg("Rejecting TC message from %s via non-neighbor %s\n",
		  cstring(msg->origin()),
		  cstring(remote_addr));
	XLOG_TRACE(_olsr.trace()._input_errors,
		   "Rejecting TC message from %s via non-neighbor %s",
		   cstring(msg->origin()),
		   cstring(remote_addr));
	return true; // consumed but invalid.
    }

    // Invariant: The above call should mean I reject TC messages
    // looped back to myself.
    XLOG_ASSERT(tc->origin() != _fm.get_main_addr());

    // 9.5.1, 9.5.2 Evaluate ANSN.
    if (! apply_tc_ansn(tc->ansn(), tc->origin())) {
	debug_msg("Rejecting TC message from %s with old ANSN %u\n",
		  cstring(msg->origin()),
		  XORP_UINT_CAST(tc->ansn()));
	XLOG_TRACE(_olsr.trace()._input_errors,
		   "Rejecting TC message from %s with old ANSN %u",
		   cstring(msg->origin()),
		   XORP_UINT_CAST(tc->ansn()));
	return true;
    }

#ifdef DETAILED_DEBUG
    // Invariant: The ANSN of all TC entries from tc->origin() must
    // now be identical.
    assert_tc_ansn_is_identical(tc->origin());
#endif

    bool is_new_tc = false;

    // Determine distance of these TC entries.
    // TCs advertise neighbors which are 1 hop from the node which
    // originated the message. tc->hops() is not incremented until
    // forwarded, so add 1 here to take account of the actual distance.
    // However, the distance of the neighbors advertised in the TC is
    // one more than the distance of the neighbor itself, so add 2 hops.

    uint16_t distance = tc->hops() + 2;

    const vector<LinkAddrInfo>& addrs = tc->neighbors();
    vector<LinkAddrInfo>::const_iterator ii;
    for (ii = addrs.begin(); ii != addrs.end(); ii++) {
	update_tc_entry((*ii).remote_addr(), tc->origin(),
			distance, tc->ansn(),
			tc->expiry_time(), is_new_tc);
    }

    // Maintain a list of the final sequence numbers seen from origins
    // with empty neighbor sets.
    TcFinalSeqMap::iterator jj = _tc_final_seqnos.find(tc->origin());
    if (jj != _tc_final_seqnos.end())
	_tc_final_seqnos.erase(jj);
    if (addrs.empty()) {
	_tc_final_seqnos.insert(make_pair(tc->origin(), tc->ansn()));
    } else {
	// Invariant: There must be no 'final' entry recorded if the
	// origin advertised any neighbors.
	XLOG_ASSERT(0 == _tc_final_seqnos.count(tc->origin()));
    }

#ifdef DETAILED_DEBUG
    // Invariant: The ANSN of all TC entries from tc->origin() must
    // now be identical.
    assert_tc_ansn_is_identical(tc->origin());
#endif

    // A route computation will be necessary if apply_tc_ansn()
    // invalidated any entries or if any entries were added/updated.
    // It is possible to receive a TC w/o neighbors, so just assume
    // that the TC info changed if we get here.
    _rm->schedule_route_update();

    _fm.forward_message(remote_addr, msg);

    return true;    // consumed
}

void
TopologyManager::event_tc_dead(const OlsrTypes::TopologyID tcid)
{
    XLOG_ASSERT(0 != _topology.count(tcid));

    delete_tc_entry(tcid);
}

bool
TopologyManager::event_receive_mid(
    Message* msg,
    const IPv4& remote_addr,
    const IPv4& local_addr)
{
    MidMessage* mid = dynamic_cast<MidMessage *>(msg);
    if (0 == mid)
	return false;	// not for me

    // Put this after the cast to skip false positives.
    debug_msg("[MID] MyMainAddr %s Src %s RecvIf %s\n",
	      cstring(_fm.get_main_addr()),
	      cstring(remote_addr),
	      cstring(local_addr));

    // 5.4.1 Sender must be in symmetric 1-hop neighborhood.
    //  This condition can be triggered during startup if the
    //  protocol timers are set low or if we haven't seen a
    //  HELLO from remote_addr yet.
    if (! _nh.is_sym_neighbor_addr(remote_addr)) {
	debug_msg("Rejecting MID message from %s via non-neighbor %s\n",
		  cstring(msg->origin()),
		  cstring(remote_addr));
	XLOG_TRACE(_olsr.trace()._input_errors,
		   "Rejecting MID message from %s via non-neighbor %s",
		   cstring(msg->origin()),
		   cstring(remote_addr));
	return true; // consumed but invalid.
    }

    TimeVal now;
    _eventloop.current_time(now);

    // 5.4.2 Process each interface listed in MID message.
    size_t added_mid_count = 0;
    try {
	bool is_mid_created = false;

	// Message::hops() is not incremented for our last hop until
	// forwarded, so take account of this now when measuring distance.
	const vector<IPv4>& addrs = mid->interfaces();
	const uint16_t distance = mid->hops() + 1;

	vector<IPv4>::const_iterator ii;
	for (ii = addrs.begin(); ii != addrs.end(); ii++) {
	    update_mid_entry(mid->origin(), (*ii), distance,
			     mid->expiry_time(), is_mid_created);
	    if (is_mid_created)
		added_mid_count++;
	}
    } catch (...) {
	// If an exception is thrown, disregard the rest
	// of the MID entries in this message, as it is more than
	// likely we hit a hard limit. We can still forward the
	// message, we just can't process all the MID entries.
    }

    // Trigger a route computation if and only if we added new MID addresses.
    if (added_mid_count > 0)
	_rm->schedule_route_update();

    _fm.forward_message(remote_addr, msg);

    return true; // consumed
    UNUSED(local_addr);
}

void
TopologyManager::event_mid_dead(const OlsrTypes::MidEntryID mid_id)
{
    XLOG_ASSERT(0 != _mids.count(mid_id));

    delete_mid_entry(mid_id);
}

/*
 * Child classes.
 */

void
TopologyEntry::update_timer(const TimeVal& vtime)
{
    if (_expiry_timer.scheduled())
	_expiry_timer.clear();
    _expiry_timer = _ev.new_oneoff_after(vtime,
	callback(this, &TopologyEntry::event_dead));
}

void
TopologyEntry::event_dead()
{
    _parent->event_tc_dead(id());
}

void
MidEntry::update_timer(const TimeVal& vtime)
{
    if (_expiry_timer.scheduled())
	_expiry_timer.clear();

    _expiry_timer = _ev.new_oneoff_after(vtime,
	callback(this, &MidEntry::event_dead));
}

void
MidEntry::event_dead()
{
    _parent->event_mid_dead(id());
}
