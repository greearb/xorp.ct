// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/ospf/area_router.cc,v 1.74 2005/09/04 03:58:45 atanu Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME
#define PARANOIA

#include "config.h"
#include <map>
#include <list>
#include <set>
#include <deque>

#include "ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"

#include "libxorp/status_codes.h"
#include "libxorp/eventloop.hh"

#include "libproto/spt.hh"

#include "ospf.hh"
#include "delay_queue.hh"
#include "vertex.hh"
#include "area_router.hh"

template <typename A>
AreaRouter<A>::AreaRouter(Ospf<A>& ospf, OspfTypes::AreaID area,
			  OspfTypes::AreaType area_type) 
    : _ospf(ospf), _area(area), _area_type(area_type),
      _last_entry(0), _allocated_entries(0), _readers(0),
      _queue(ospf.get_eventloop(),
	     OspfTypes::MinLSInterval,
	     callback(this, &AreaRouter<A>::publish_all)),
      _TransitCapability(0),
      _routing_recompute_scheduled(false),
      _routing_recompute_delay(5)	// In seconds.
{
    // Never need to delete this as the ref_ptr will tidy up.
    // An entry to be placed in invalid slots.
    // When a LSA that was not self originated reaches MAXAGE it must
    // be flooded but we can't mark it as invalid. In all other cases
    // just marking an entry as invalid is good enough.
    _invalid_lsa = Lsa::LsaRef(new RouterLsa(_ospf.get_version()));
    _invalid_lsa->invalidate();

    // Never need to delete this as the ref_ptr will tidy up.
    RouterLsa *rlsa = new RouterLsa(_ospf.get_version());
    rlsa->set_self_originating(true);
    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    rlsa->record_creation_time(now);

    Lsa_header& header = rlsa->get_header();
    
    // This is a router LSA so the link state ID is the Router ID.
    header.set_link_state_id(_ospf.get_router_id());
    header.set_advertising_router(_ospf.get_router_id());

    _router_lsa = Lsa::LsaRef(rlsa);
    add_lsa(_router_lsa);
//     _db.push_back(_router_lsa);
//     _last_entry = 1;

#ifdef	UNFINISHED_INCREMENTAL_UPDATE
    // Add this router to the SPT table.
    Vertex v;
    RouterVertex(v);
    _spt.add_node(v);
    _spt.set_origin(v);
#endif
}

template <typename A>
void
AreaRouter<A>::add_peer(PeerID peerid)
{
    debug_msg("PeerID %u\n", peerid);
    // The peer starts in the down state.
    _peers[peerid] = PeerStateRef(new PeerState);
}

template <typename A>
void
AreaRouter<A>::delete_peer(PeerID peerid)
{
    debug_msg("PeerID %u\n", peerid);

    // The peer manager does not keep track of which peers belong to
    // which areas. So when a peer is deleted all areas are notified.
    if (0 == _peers.count(peerid))
	return;
    
    _peers.erase(_peers.find(peerid));
}

template <typename A>
bool
AreaRouter<A>::peer_up(PeerID peerid)
{
    debug_msg("PeerID %u\n", peerid);

    if (0 == _peers.count(peerid)) {
	XLOG_WARNING("Peer not found %u", peerid);
	return false;
    }

    // Mark the peer as UP
    typename PeerMap::iterator i = _peers.find(peerid);
    PeerStateRef psr = i->second;
    psr->_up = true;

    if (update_router_links()) {
	// publish the router LSA.
	_queue.add(_router_lsa);
    }

    return true;
}

template <typename A>
bool
AreaRouter<A>::peer_down(PeerID peerid)
{
    debug_msg("PeerID %u\n", peerid);

    if (0 == _peers.count(peerid)) {
	XLOG_WARNING("Peer not found %u", peerid);
	return false;
    }

    // Mark the peer as DOWN
    typename PeerMap::iterator i = _peers.find(peerid);
    PeerStateRef psr = i->second;
    psr->_up = false;

    if (update_router_links()) {
	// publish the router LSA.
	_queue.add(_router_lsa);
    }
		   
    return true;
}

template <typename A>
bool 
AreaRouter<A>::new_router_links(PeerID peerid,
				const list<RouterLink>& router_links)
{
    if (0 == _peers.count(peerid)) {
	XLOG_WARNING("Peer not found %u", peerid);
	return false;
    }

    typename PeerMap::iterator i = _peers.find(peerid);
    PeerStateRef psr = i->second;

    psr->_router_links.clear();
    psr->_router_links.insert(psr->_router_links.begin(),
			      router_links.begin(), router_links.end());

    if (update_router_links()) {
	// publish the router LSA.
	_queue.add(_router_lsa);
    }

    return true;
}

template <typename A>
bool 
AreaRouter<A>::generate_network_lsa(PeerID peerid,
				    OspfTypes::RouterID link_state_id,
				    list<OspfTypes::RouterID>& routers,
				    uint32_t network_mask)
{
    NetworkLsa *nlsa = new NetworkLsa(_ospf.get_version());
    nlsa->set_self_originating(true);
    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    nlsa->record_creation_time(now);

    Lsa_header& header = nlsa->get_header();
    header.set_link_state_id(link_state_id);
    header.set_advertising_router(_ospf.get_router_id());

    add_lsa(Lsa::LsaRef(nlsa));

    update_network_lsa(peerid, link_state_id, routers, network_mask);
    
    return true;
}

template <typename A>
bool 
AreaRouter<A>::update_network_lsa(PeerID peerid,
				  OspfTypes::RouterID link_state_id,
				  list<OspfTypes::RouterID>& routers,
				  uint32_t network_mask)
{
    OspfTypes::Version version = _ospf.get_version();
    Ls_request lsr(version, NetworkLsa(version).get_ls_type(),
		   link_state_id, _ospf.get_router_id());

    size_t index;
    if (!find_lsa(lsr, index)) {
	XLOG_FATAL("Couldn't find Network_lsa %s in LSA database",
		   cstring(lsr));
	return false;
    }

    NetworkLsa *nlsa = dynamic_cast<NetworkLsa *>(_db[index].get());
    XLOG_ASSERT(nlsa);

    list<OspfTypes::RouterID>& attached_routers = nlsa->get_attached_routers();
    if (&routers != &attached_routers) {
	attached_routers.clear();
	attached_routers.push_back(link_state_id); // Add this router.
	attached_routers.insert(attached_routers.begin(),
				routers.begin(), routers.end());
    }

    switch (version) {
    case OspfTypes::V2:
	nlsa->set_network_mask(network_mask);
	nlsa->get_header().set_options(get_options());
	break;
    case OspfTypes::V3:
	nlsa->set_options(get_options());
	break;
    }

    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    nlsa->update_age_and_seqno(now);

    // Prime this Network-LSA to be refreshed.
    nlsa->get_timer() = _ospf.get_eventloop().
	new_oneoff_after(TimeVal(OspfTypes::LSRefreshTime, 0),
			 callback(this, &AreaRouter<A>::refresh_network_lsa,
				  peerid,
				  _db[index]));

    publish_all(_db[index]);

    return true;
}

template <typename A>
bool 
AreaRouter<A>::withdraw_network_lsa(PeerID /*peerid*/,
				    OspfTypes::RouterID link_state_id)
{
    OspfTypes::Version version = _ospf.get_version();
    Ls_request lsr(version, NetworkLsa(version).get_ls_type(),
		   link_state_id, _ospf.get_router_id());

    size_t index;
    if (!find_lsa(lsr, index)) {
	XLOG_FATAL("Couldn't find Network_lsa %s in LSA database",
		   cstring(lsr));
	return false;
    }

    Lsa::LsaRef lsar = _db[index];
    lsar->set_maxage();
    premature_aging(lsar, index);

    return true;
}

template <typename A>
void
AreaRouter<A>::refresh_network_lsa(PeerID peerid, Lsa::LsaRef lsar)
{
    NetworkLsa *nlsa = dynamic_cast<NetworkLsa *>(lsar.get());
    XLOG_ASSERT(nlsa);
    XLOG_ASSERT(nlsa->valid());

    uint32_t network_mask = 0;

    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	network_mask = nlsa->get_network_mask();
	break;
    case OspfTypes::V3:
	break;
    }

    update_network_lsa(peerid,
		       nlsa->get_header().get_link_state_id(),
		       nlsa->get_attached_routers(),
		       network_mask);
}

inline
string
pp_lsas(const list<Lsa::LsaRef>& lsas)
{
    string output;

    list<Lsa::LsaRef>::const_iterator i;
    for (i = lsas.begin(); i != lsas.end(); i++) {
	output += "\n\t" + (*i)->str();
    }
    
    return output;
}

template <typename A>
void
AreaRouter<A>::receive_lsas(PeerID peerid,
			    OspfTypes::NeighbourID nid,
			    list<Lsa::LsaRef>& lsas, 
			    list<Lsa_header>& direct_ack,
			    list<Lsa_header>& delayed_ack,
			    bool backup, bool dr)
{
    debug_msg("PeerID %u NeighbourID %u %s\nbackup %s dr %s\n", peerid, nid,
	      pp_lsas(lsas).c_str(),
	      backup ? "true" : "false",
	      dr ? "true" : "false");

    TimeVal now;
    _ospf.get_eventloop().current_time(now);

    routing_begin();
    
    // RFC 2328 Section 13. The Flooding Procedure
    // Validate the incoming LSAs.
    
    list<Lsa::LsaRef>::const_iterator i;
    for (i = lsas.begin(); i != lsas.end(); i++) {
	// These LSAs came over the wire they must not be self originating.
	XLOG_ASSERT(!(*i)->get_self_originating());

	// Record the creation time and initial age.
	(*i)->record_creation_time(now);

	// (1) Validate the LSA's LS checksum. 
	// (2) Check that the LSA's LS type is known.
	// Both checks already performed in the packet/LSA decoding process.

	// (3) In stub areas discard AS-external-LSA's (LS type = 5, 0x4005).
	switch(_area_type) {
	case OspfTypes::BORDER:
	    break;
	case OspfTypes::STUB:
	case OspfTypes::NSSA:
 	    if ((*i)->external())
 		continue;
	    break;
	}
	const Lsa_header& lsah = (*i)->get_header();
	size_t index;
	LsaSearch search = compare_lsa(lsah, index);

#ifdef	DEBUG_LOGGING
	debug_msg("Searching for %s\n", cstring(*(*i)));
	switch(search) {
	case NOMATCH:
	    debug_msg("No match\n");
	    break;
	case NEWER:
	    debug_msg("is newer than \n%s\n", cstring(*_db[index]));
	    break;
	case OLDER:
	    debug_msg("is older than \n%s\n", cstring(*_db[index]));
	    break;
	case EQUIVALENT:
	    debug_msg("is equivalent to \n%s\n", cstring(*_db[index]));
	    break;
	}
#endif
	// (4) MaxAge
	if (OspfTypes::MaxAge == lsah.get_ls_age()) {
	    if (NOMATCH == search) {
		if (!neighbours_exchange_or_loading()) {
		    delayed_ack.push_back(lsah);
		    continue;
		}
	    }
	}
	
	switch(search) {
	case NOMATCH:
	case NEWER: {
	    // (5) New or newer LSA.
	    // (a) If this LSA is too recent drop it.
	    if (NEWER == search) {
		TimeVal then;
		_db[index]->get_creation_time(then);
		if ((now - then) < TimeVal(OspfTypes::MinLSArrival, 0)) {
		    debug_msg("Rejecting LSA last one arrived less than "
			      "%d second(s) ago\n %s\n%s",
			      OspfTypes::MinLSArrival,
			      cstring(*(*i)) , cstring(*(_db[index])));
		    XLOG_TRACE(_ospf.trace()._input_errors,
			       "Rejecting LSA last one arrived less than "
			       "%d second(s) ago\n%s\n%s",
			       OspfTypes::MinLSArrival,
			       cstring(*(*i)) , cstring(*(_db[index])));
		    continue;
		}
	    }
	    // This is out of sequence but doing it later makes no sense.
	    // (f) Self orignating LSAs 
	    // RFC 2328 Section 13.4. Receiving self-originated LSAs

	    bool match = false;
	    if (NEWER == search)
		match = _db[index]->get_self_originating();
	    if (self_originated((*i), match, index)) {
		bool multicast_on_peer;
		publish(peerid, nid, (*i), multicast_on_peer);
		continue;
	    }

	    // (b) Flood this LSA to all of our neighbours.
	    // RFC 2328 Section 13.3. Next step in the flooding procedure

	    // If this is an AS-external-LSA send it to all area's
	    if ((*i)->external())
		flood_all_areas((*i), true /* ADD */);

	    // Set to true if the LSA was multicast out of this
	    // interface. If it was, there is no requirement to send an
	    // ACK.
	    bool multicast_on_peer;
	    publish(peerid, nid, (*i), multicast_on_peer);

	    // (c) Remove the current copy from neighbours
	    // retransmission lists. If this is a new LSA then there
	    // can be no version of this LSA on the neighbours
	    // retransmission lists. If this is a newer version of an
	    // existing LSA then the old LSA will be invalidated in
	    // update LSA, therefore the neighbours will not try and
	    // transmit it.
	    // (d) Install the new LSA.
	    if (NOMATCH == search)
		add_lsa((*i));
	    else
		update_lsa((*i), index);
	    // Start aging this LSA.
	    age_lsa((*i));
	    routing_add(*i, NOMATCH != search);
	    
	    // (e) Possibly acknowledge this LSA.
	    // RFC 2328 Section 13.5 Sending Link State Acknowledgment Packets

	    if (!multicast_on_peer) {
		if ((backup && dr) || !backup)
		    delayed_ack.push_back(lsah);
	    }
	    
	    // Too late to do this now, its after (a).
	    // (f) Self orignating LSAs 
	    // RFC 2328 Section 13.4. Receiving self-originated LSAs

	}
	    break;
	case OLDER:
	    // XXX - Is this really where case (6) belongs?
	    // (6) If this LSA is on the sending neighbours request
	    // list something has gone wrong.

	    if (on_link_state_request_list(peerid, nid, (*i))) {
		event_bad_link_state_request(peerid, nid);
		goto out;
	    }

	    // (8) The database copy is more recent than the received LSA.
	    // The LSA's LS sequence number is wrapping.
	    if (_db[index]->get_header().get_ls_age() == OspfTypes::MaxAge &&
		_db[index]->get_header().get_ls_sequence_number() == 
		OspfTypes::MaxSequenceNumber)
		continue;
	    // We have a more up to date copy of this LSA than our
	    // neighbour so blast this LSA directly back to this neighbour.
	    send_lsa(peerid, nid, _db[index]);
	    break;
	case EQUIVALENT:
	    // (7) The LSAs are equivalent.
	    // (a) This might be an "implied acknowledgement".
	    if (_db[index]->exists_nack(nid)) { 

		_db[index]->remove_nack(nid);
		// (b) An "implied acknowledgement".
		if (backup && dr)
		    delayed_ack.push_back(lsah);
	    } else {
		// (b) Not an "implied acknowledgement".
		direct_ack.push_back(lsah);
	    }
	    break;
	}
    }

 out:
    push_lsas();
    routing_end();
}

template <typename A>
bool
AreaRouter<A>::age_lsa(Lsa::LsaRef lsar)
{
    size_t index;

    XLOG_ASSERT(!lsar->get_self_originating());

    if (!find_lsa(lsar, index)) {
	XLOG_WARNING("LSA not in database: %s", cstring(*lsar));
	return false;
    }

    lsar->get_timer() = _ospf.get_eventloop().
	new_oneoff_after(TimeVal(OspfTypes::MaxAge -
				 lsar->get_header().get_ls_age(), 0),
			 callback(this,
				  &AreaRouter<A>::maxage_reached, lsar,index));
    return true;
}

template <typename A>
void
AreaRouter<A>::maxage_reached(Lsa::LsaRef lsar, size_t i)
{
    size_t index;

    if (!find_lsa(lsar, index))
	XLOG_FATAL("LSA not in database: %s", cstring(*lsar));

    if (i != index)
	XLOG_FATAL("Indexes don't match %u != %u %s",  XORP_UINT_CAST(i),
		   XORP_UINT_CAST(index),
		   cstring(*_db[index]));

#ifdef PARANOIA
    if (!lsar->get_self_originating()) {
	TimeVal now;
	_ospf.get_eventloop().current_time(now);
	if (!lsar->maxage())
	    lsar->update_age(now);
    }
#endif
    if (OspfTypes::MaxAge != lsar->get_header().get_ls_age())
	XLOG_FATAL("LSA is not MaxAge %s", cstring(*lsar));
    
    delete_lsa(lsar, index, false /* Don't invalidate */);
    publish_all(lsar);

    if (lsar->external())
	flood_all_areas(lsar, false /* DELETE */);

    // Clear the timer otherwise there is a circular dependency.
    // The LSA contains a XorpTimer that points back to the LSA.
    lsar->get_timer().clear();
}

template <typename A>
void
AreaRouter<A>::premature_aging(Lsa::LsaRef lsar, size_t i)
{
    XLOG_ASSERT(lsar->get_self_originating());
    maxage_reached(lsar, i);
}

template <typename A>
bool
AreaRouter<A>::add_lsa(Lsa::LsaRef lsar)
{
    // If there are no readers we can put this LSA into an empty slot.
    if (0 == _readers && !_empty_slots.empty()) {
	_db[_empty_slots.front()] = lsar;
	_empty_slots.pop_front();
	return true;
    }

    if (_last_entry < _allocated_entries) {
	_db[_last_entry] = lsar;
    } else {
	_db.push_back(lsar);
	_allocated_entries++;
    }
    _last_entry++;

    return true;
}

template <typename A>
bool
AreaRouter<A>::delete_lsa(Lsa::LsaRef lsar, size_t index, bool invalidate)
{
    Lsa_header& dblsah = _db[index]->get_header();
    XLOG_ASSERT(dblsah.get_ls_type() == lsar->get_header().get_ls_type());
    XLOG_ASSERT(dblsah.get_link_state_id() == 
		lsar->get_header().get_link_state_id());
    XLOG_ASSERT(dblsah.get_advertising_router() ==
		lsar->get_header().get_advertising_router());

    XLOG_ASSERT(_db[index]->valid());

    // This LSA is being deleted remove it from the routing computation.
    routing_delete(lsar);

    if (invalidate)
	_db[index]->invalidate();
    _db[index] = _invalid_lsa;
    _empty_slots.push_back(index);

    // _last_entry points one past the last entry, if the deleted LSA
    // was at the end of the array then the _last_entry pointer can be
    // decreased.
    while(index + 1 == _last_entry && !_db[index]->valid() && 
	  0 != _last_entry) {
	_last_entry--;
	index--;
    }

    return true;
}

template <typename A>
bool
AreaRouter<A>::update_lsa(Lsa::LsaRef lsar, size_t index)
{
    Lsa_header& dblsah = _db[index]->get_header();
    XLOG_ASSERT(dblsah.get_ls_type() == lsar->get_header().get_ls_type());
    XLOG_ASSERT(dblsah.get_link_state_id() == 
		lsar->get_header().get_link_state_id());
    XLOG_ASSERT(dblsah.get_advertising_router() ==
		lsar->get_header().get_advertising_router());

    XLOG_ASSERT(_db[index]->valid());
    // A LSA arriving over the wire should never replace a
    // self originating LSA.
    XLOG_ASSERT(!_db[index]->get_self_originating());
    if (0 == _readers) {
	_db[index]->invalidate();
	_db[index] = lsar;
    } else {
	delete_lsa(lsar, index, true /* Mark the LSA as invalid */);
	add_lsa(lsar);
    }

    return true;
}

template <typename A>
bool
AreaRouter<A>::find_lsa(const Ls_request& lsr, size_t& index) const
{
    for(index = 0 ; index < _last_entry; index++) {
	if (!_db[index]->valid())
	    continue;
	Lsa_header& dblsah = _db[index]->get_header();
	if (dblsah.get_ls_type() != lsr.get_ls_type())
	    continue;

	if (dblsah.get_link_state_id() != lsr.get_link_state_id())
	    continue;

	if (dblsah.get_advertising_router() != lsr.get_advertising_router())
	    continue;

	return true;
    }

    return false;
}

template <typename A>
bool
AreaRouter<A>::find_lsa(Lsa::LsaRef lsar, size_t& index) const
{
    const Lsa_header lsah = lsar->get_header();
    Ls_request lsr(_ospf.get_version(), lsah.get_ls_type(),
		   lsah.get_link_state_id(), lsah.get_advertising_router());

    return find_lsa(lsr, index);
}

template <typename A>
bool
AreaRouter<A>::find_network_lsa(uint32_t link_state_id, size_t& index) const
{
    uint32_t ls_type = NetworkLsa(_ospf.get_version()).get_ls_type();

    for(index = 0 ; index < _last_entry; index++) {
	if (!_db[index]->valid())
	    continue;
	Lsa_header& dblsah = _db[index]->get_header();
	if (dblsah.get_ls_type() != ls_type)
	    continue;

	if (dblsah.get_link_state_id() != link_state_id)
	    continue;

	// Note we deliberately don't check for advertising router.

	return true;
    }

    return false;
}

/**
 * RFC 2328 Section 13.1 Determining which LSA is newer.
 */
template <typename A>
typename AreaRouter<A>::LsaSearch
AreaRouter<A>::compare_lsa(const Lsa_header& candidate,
			   const Lsa_header& current) const
{
    debug_msg("router id: %s\n", pr_id(_ospf.get_router_id()).c_str());
    debug_msg("\ncandidate: %s\ncurrent: %s\n", cstring(candidate),
	      cstring(current));

    const uint32_t candidate_seqno = candidate.get_ls_sequence_number();
    const uint32_t current_seqno = current.get_ls_sequence_number();

    if (current_seqno != candidate_seqno) {
	if (OspfTypes::InitialSequenceNumber == candidate_seqno)
	    return OLDER;
	if (OspfTypes::InitialSequenceNumber == current_seqno)
	    return NEWER;
	if (current_seqno > candidate_seqno)
	    return OLDER;
	if (current_seqno < candidate_seqno)
	    return NEWER;
    }

    if (current.get_ls_checksum() > candidate.get_ls_checksum())
	return OLDER;
    if (current.get_ls_checksum() < candidate.get_ls_checksum())
	return NEWER;

    if (current.get_ls_age() == OspfTypes::MaxAge)
	return OLDER;
    if (candidate.get_ls_age() == OspfTypes::MaxAge)
	return NEWER;
	
    if(abs(current.get_ls_age() - candidate.get_ls_age()) > 
       OspfTypes::MaxAgeDiff) {
	return candidate.get_ls_age() < current.get_ls_age() ? NEWER : OLDER;
    }
	
    // These two LSAs are identical.
    return EQUIVALENT;
}

template <typename A>
typename AreaRouter<A>::LsaSearch
AreaRouter<A>::compare_lsa(const Lsa_header& lsah, size_t& index) const
{
    Ls_request lsr(_ospf.get_version(), lsah.get_ls_type(),
		   lsah.get_link_state_id(), lsah.get_advertising_router());

    if (find_lsa(lsr, index)) {
	// Update the age before checking this field.
	TimeVal now;
	_ospf.get_eventloop().current_time(now);
	_db[index]->update_age(now);
	return compare_lsa(lsah, _db[index]->get_header());
    }

    return NOMATCH;
}

template <typename A>
typename AreaRouter<A>::LsaSearch
AreaRouter<A>::compare_lsa(const Lsa_header& lsah) const
{
    size_t index;
    return compare_lsa(lsah, index);
}

template <typename A>
bool
AreaRouter<A>::newer_lsa(const Lsa_header& lsah) const
{
    switch(compare_lsa(lsah)) {
    case NOMATCH:
    case NEWER:
	return true;
    case EQUIVALENT:
    case OLDER:
	return false;
    }

    XLOG_UNREACHABLE();
    return true;
}

template <typename A>
bool
AreaRouter<A>::get_lsas(const list<Ls_request>& reqs,
			list<Lsa::LsaRef>& lsas)
{
    TimeVal now;
    _ospf.get_eventloop().current_time(now);

    list<Ls_request>::const_iterator i;
    for(i = reqs.begin(); i != reqs.end(); i++) {
	size_t index;
	if (!find_lsa(*i, index)) {
	    XLOG_WARNING("Unable to find %s", cstring(*i));
	    return false;
	}
	Lsa::LsaRef lsar = _db[index];
	// Start the delay timer so we won't transmit any more self
	// originating LSAs until the appropriate time has passed.
	if (lsar->get_self_originating())
	    _queue.fire();
        lsar->update_age(now);
	lsas.push_back(lsar);
    }
    
    return true;
}

template <typename A>
DataBaseHandle
AreaRouter<A>::open_database(bool& empty)
{
    // While there are readers no entries can be add to the database
    // before the _last_entry.

    _readers++;

    // Snapshot the current last entry position. While the database is
    // open (_readers > 0) new entries will be added past _last_entry.

    DataBaseHandle dbh = DataBaseHandle(true, _last_entry);

    empty = !subsequent(dbh);

    return dbh;
}

template <typename A>
bool
AreaRouter<A>::valid_entry_database(size_t index) const
{
    // This LSA is not valid.
    if (!_db[index]->valid())
	return false;

    // There is no wire format for this LSA.
    if (!_db[index]->available())
	return false;

    return true;
}

template <typename A>
bool
AreaRouter<A>::subsequent(DataBaseHandle& dbh) const
{
    bool another = false;
    for (size_t index = dbh.position(); index < dbh.last(); index++) {
	if (!valid_entry_database(index))
	    continue;
	another = true;
	break;
    }

    return another;
}

template <typename A>
Lsa::LsaRef
AreaRouter<A>::get_entry_database(DataBaseHandle& dbh, bool& last)
{
    XLOG_ASSERT(dbh.valid());

    uint32_t position;

    do {
	position = dbh.position();

	if (position >= _db.size())
	    XLOG_FATAL("Index too far %d length %d", position,
		       XORP_INT_CAST(_db.size()));

	dbh.advance(last);
    } while(!valid_entry_database(position));

    // If this is not the last entry make sure there is a subsequent
    // valid entry.
    if (!last)
	last = !subsequent(dbh);

    return _db[position];
}

template <typename A>
void
AreaRouter<A>::close_database(DataBaseHandle& dbh)
{
    XLOG_ASSERT(dbh.valid());
    XLOG_ASSERT(0 != _readers);
    _readers--;

    if (subsequent(dbh))
	XLOG_WARNING("Database closed with entries remaining");

    dbh.invalidate();
}

template <typename A>
void
AreaRouter<A>:: print_link_state_database() const
{
  for (size_t index = 0 ; index < _last_entry; index++) {
	Lsa::LsaRef lsar = _db[index];
	if (!lsar->valid())
	    continue;
	// Please leave this as a printf its for debugging only.
	printf("%s database %s\n",
	       pr_id(_ospf.get_router_id()).c_str(),
	       cstring(*lsar));
  }
}

template <typename A>
bool
AreaRouter<A>::update_router_links()
{
    RouterLsa *router_lsa = dynamic_cast<RouterLsa *>(_router_lsa.get());
    XLOG_ASSERT(router_lsa);
    bool empty = router_lsa->get_router_links().empty();
    router_lsa->get_router_links().clear();
    typename PeerMap::iterator i;
    for(i = _peers.begin(); i != _peers.end(); i++) {
	PeerStateRef temp_psr = i->second;
	if (temp_psr->_up) {
	    typename list<RouterLink>::iterator j = 
		temp_psr->_router_links.begin();
	    for(; j != temp_psr->_router_links.end(); j++)
		router_lsa->get_router_links().push_back(*j);
	}
    }

    // If we weren't advertising and we still aren't return.
    if (empty && router_lsa->get_router_links().empty())
	return false;

    PeerManager<A>& pm = _ospf.get_peer_manager();
    router_lsa->set_v_bit(pm.virtual_link_endpoint(_area));
    router_lsa->set_e_bit(pm.as_boundary_router_p());
    router_lsa->set_b_bit(pm.area_border_router_p());

    switch (_ospf.get_version()) {
    case OspfTypes::V2:
	router_lsa->get_header().set_options(get_options());
	break;
    case OspfTypes::V3:
	router_lsa->set_options(get_options());
	break;
    }

    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    router_lsa->update_age_and_seqno(now);

#if	0
    // Prime this Router-LSA to be refreshed.
    router_lsa->get_timer() = _ospf.get_eventloop().
	new_oneoff_after(TimeVal(OspfTypes::LSRefreshTime, 0),
			 callback(this, &AreaRouter<A>::refresh_router_lsa));
#endif

    return true;
}

template <typename A>
void
AreaRouter<A>::refresh_router_lsa()
{
    if (update_router_links()) {
	// publish the router LSA.
	_queue.add(_router_lsa);
    }
}

template <typename A>
void
AreaRouter<A>::publish(const PeerID peerid, const OspfTypes::NeighbourID nid,
		       Lsa::LsaRef lsar, bool &multicast_on_peer) const
{
    debug_msg("Publish: %s\n", cstring(*lsar));

    TimeVal now;
    _ospf.get_eventloop().current_time(now);

    // Update the age field unless its self originating.
    if (lsar->get_self_originating()) {
	if (OspfTypes::MaxSequenceNumber == lsar->get_ls_sequence_number())
	    XLOG_FATAL("TBD: Flush this LSA and generate a new LSA");
    } else {
	if (!lsar->maxage())
	    lsar->update_age(now);
    }

    typename PeerMap::const_iterator i;
    for(i = _peers.begin(); i != _peers.end(); i++) {
	PeerStateRef temp_psr = i->second;
	if (temp_psr->_up) {
	    bool multicast;
	    if (!_ospf.get_peer_manager().
		queue_lsa(i->first, peerid, nid, lsar, multicast))
		XLOG_FATAL("Unable to queue LSA");
	    // Did this LSA get broadcast/multicast on the
	    // peer/interface that it came in on.
	    if (peerid == i->first)
		multicast_on_peer = multicast;
	}
    }
}

template <typename A>
void
AreaRouter<A>::publish_all(Lsa::LsaRef lsar)
{
    debug_msg("Publish: %s\n", cstring(*lsar));
    bool multicast_on_peer;
    publish(ALLPEERS, OspfTypes::ALLNEIGHBOURS, lsar, multicast_on_peer);

    push_lsas();	// NOTE: a push after every LSA.
}

template <typename A>
void
AreaRouter<A>::push_lsas()
{
    typename PeerMap::iterator i;
    for(i = _peers.begin(); i != _peers.end(); i++) {
	PeerStateRef temp_psr = i->second;
	if (temp_psr->_up) {
	    if (!_ospf.get_peer_manager().push_lsas(i->first))
		XLOG_FATAL("Unable to push LSAs");
	}
    }
}

template <typename A>
void
AreaRouter<A>::flood_all_areas(Lsa::LsaRef lsar, bool add)
{
    debug_msg("Flood all areas %s : %s\n", add ? "add" : "delete",
	      cstring(*lsar));

    XLOG_WARNING("TBD send external-LSAs to other areas");
}

template <typename A>
void
AreaRouter<A>::push_all_areas()
{
    XLOG_WARNING("TBD push all areas");
}

template <typename A>
bool
AreaRouter<A>::neighbours_exchange_or_loading() const
{
    typename PeerMap::const_iterator i;
    for(i = _peers.begin(); i != _peers.end(); i++) {
	if (_ospf.get_peer_manager().
	    neighbours_exchange_or_loading(i->first, _area))
	    return true;
    }

    return false;
}

template <typename A>
bool
AreaRouter<A>::on_link_state_request_list(const PeerID peerid,
					  const OspfTypes::NeighbourID nid,
					  Lsa::LsaRef lsar) const
{
    return _ospf.get_peer_manager().
	on_link_state_request_list(peerid, _area, nid, lsar);
}

template <typename A>
bool
AreaRouter<A>::event_bad_link_state_request(const PeerID peerid,
				    const OspfTypes::NeighbourID nid) const
{
    return _ospf.get_peer_manager().
	event_bad_link_state_request(peerid, _area, nid);
}

template <typename A>
bool
AreaRouter<A>::send_lsa(const PeerID peerid,
			const OspfTypes::NeighbourID nid,
			Lsa::LsaRef lsar) const
{
    return _ospf.get_peer_manager().send_lsa(peerid, _area, nid, lsar);
}

template <>
bool
AreaRouter<IPv4>::self_originated_by_interface(Lsa::LsaRef lsar, IPv4) const
{
    if (0 == dynamic_cast<NetworkLsa *>(lsar.get()))
	return false;

    IPv4 address(lsar->get_header().get_link_state_id());
    return _ospf.get_peer_manager().known_interface_address(address);
}

template <>
bool
AreaRouter<IPv6>::self_originated_by_interface(Lsa::LsaRef, IPv6) const
{
    XLOG_FATAL("Not required for OSPFv3");
    return false;
}

template <typename A>
bool
AreaRouter<A>::self_originated(Lsa::LsaRef lsar, bool lsa_exists, size_t index)
{
    // RFC 2328 Section 13.4. Receiving self-originated LSAs

    debug_msg("lsar: %s\noriginated: %s index: %u\n", cstring((*lsar)),
	      lsa_exists ? "true" : "false", XORP_UINT_CAST(index));
    if (lsa_exists)
	debug_msg("database copy: %s\n", cstring((*_db[index])));

    bool originated = lsa_exists;
    if (!originated) {
	if (lsar->get_header().get_advertising_router() ==
	    _ospf.get_router_id()) {
	    originated = true;
	} else {
	    switch (_ospf.get_version()) {
	    case OspfTypes::V2:
		if (self_originated_by_interface(lsar))
		    originated = true;
		break;
	    case OspfTypes::V3:
		break;
	    }
	}
    }

    if (!originated)
	return false;

    debug_msg("router id: %s\n", pr_id(_ospf.get_router_id()).c_str());
    debug_msg("self originated: %s\n", cstring((*lsar)));

    // If we got this far this is a self-originated LSA that needs to
    // be removed from the wild.

    // A database copy of this LSA exists. The new LSA that arrived is
    // newer than the database copy. Copy the sequence number from the
    // new LSA into the database LSA increment it and flood.
    if (lsa_exists) {
	_db[index]->set_ls_sequence_number(lsar->get_ls_sequence_number());
	lsar = _db[index];
	lsar->increment_sequence_number();
	return true;
    }

    // This is a spurious LSA that was probably originated by us in
    // the past just get rid of it by setting it to MaxAge.
    lsar->set_maxage();

    return true;
}

template <typename A>
void
AreaRouter<A>::RouterVertex(Vertex& v)
{
    v.set_version(_ospf.get_version());
    v.set_type(Vertex::Router);
    v.set_nodeid(_ospf.get_router_id());
}

template <typename A>
void
AreaRouter<A>::routing_begin()
{
}

template <typename A>
void
AreaRouter<A>::routing_add(Lsa::LsaRef lsar, bool known)
{
    debug_msg("%s known %s\n", cstring(*lsar), known ? "true" : "false");
}

template <typename A>
void
AreaRouter<A>::routing_delete(Lsa::LsaRef lsar)
{
    debug_msg("%s\n", cstring(*lsar));

    routing_schedule_total_recompute();
}

template <typename A>
void
AreaRouter<A>::routing_end()
{
    routing_schedule_total_recompute();
}

template <typename A>
void 
AreaRouter<A>::routing_schedule_total_recompute()
{
    if (_routing_recompute_scheduled)
	return;
    _routing_recompute_scheduled = true;
    _routing_recompute_timer = _ospf.get_eventloop().
	new_oneoff_after(TimeVal(_routing_recompute_delay),
			 callback(this, &AreaRouter<A>::routing_timer));
    
}

template <typename A>
void 
AreaRouter<A>::routing_timer()
{
    _routing_recompute_scheduled = false;
    routing_total_recompute();
}

template <typename A>
void 
AreaRouter<A>::routing_total_recompute()
{
//     print_link_state_database();

    Spt<Vertex> spt;
    bool transit_capability = false;

    // Add this router to the SPT table.
    Vertex rv;
    RouterVertex(rv);
    spt.add_node(rv);
    spt.set_origin(rv);

    for (size_t index = 0 ; index < _last_entry; index++) {
	Lsa::LsaRef lsar = _db[index];
	if (!lsar->valid() || lsar->maxage())
	    continue;

	RouterLsa *rlsa;
	
	if (0 != (rlsa = dynamic_cast<RouterLsa *>(lsar.get()))) {

	    if (rlsa->get_v_bit())
		transit_capability = true;
	    
	    Vertex v;

	    v.set_version(_ospf.get_version());
	    v.set_type(Vertex::Router);
	    v.set_nodeid(rlsa->get_header().get_link_state_id());

	    // Don't add this router back again.
	    if (spt.exists_node(v)) {
		debug_msg("%s Router %s\n",
		       pr_id(_ospf.get_router_id()).c_str(),
		       cstring(v));
	    } else {
		debug_msg("%s Add %s\n",
		       pr_id(_ospf.get_router_id()).c_str(),
		       cstring(v));
		spt.add_node(v);
	    }

	    debug_msg("%s Router-Lsa %s\n",
		   pr_id(_ospf.get_router_id()).c_str(),
		   cstring(*rlsa));

	    switch(_ospf.get_version()) {
	    case OspfTypes::V2:
		routing_router_lsaV2(spt, v, rlsa);
		break;
	    case OspfTypes::V3:
		routing_router_lsaV3(spt, v, rlsa);
		break;
	    }
	}
    }

    // Print the new routing table.
    list<RouteCmd<Vertex> > r;
    spt.compute(r);

    list<RouteCmd<Vertex> >::const_iterator ri;
    for(ri = r.begin(); ri != r.end(); ri++)
	XLOG_WARNING("TBD: Add route:\n%s %s",
		     pr_id(_ospf.get_router_id()).c_str(),
		     ri->str().c_str());
}

template <typename A>
bool 
AreaRouter<A>::bidirectional(const RouterLink& rl, NetworkLsa *nlsa)
{
    XLOG_ASSERT(0 != nlsa);
    XLOG_ASSERT(rl.get_type() == RouterLink::transit);

    // This is the edge from the Router-LSA to the Network-LSA.
    XLOG_ASSERT(rl.get_link_id() == nlsa->get_header().get_link_state_id());

    // Does the Network-LSA know about the Router-LSA.
    list<OspfTypes::RouterID>& routers = nlsa->get_attached_routers();
    list<OspfTypes::RouterID>::const_iterator i;
    for (i = routers.begin(); i != routers.end(); i++)
	if (rl.get_link_id() == *i)
	    return true;

    return false;
}

/**
 * XXX
 * This is temporary until SPT supports a single call to update an
 * edge adding the edge if necessary.
 *
 */
template <typename A>
inline
void
update_edge(Spt<A>& spt, const Vertex& src, int metric, const Vertex& dst)
{
    if (!spt.add_edge(src, metric, dst)) {
	// XXX
	// This warning should not appear during absolute calculation.
	// It may be normal for it to appear when doing incremental
	// updates when it should be removed.
	XLOG_WARNING("Edge exists between %s and %s", cstring(src),
		     cstring(dst));
	if (!spt.update_edge_weight(src, metric, dst))
	    XLOG_FATAL("Couldn't add edge between %s and %s",
		       cstring(src), cstring(dst));
    }
}

template <typename A>
void
AreaRouter<A>::routing_router_lsaV2(Spt<Vertex>& spt, const Vertex& src,
				    const RouterLsa *rlsa)
{
    debug_msg("Vertex %s \n%s\n", cstring(src), cstring(*rlsa));

    const list<RouterLink> &rl = 
	const_cast<RouterLsa *>(rlsa)->get_router_links();
    list<RouterLink>::const_iterator l = rl.begin();
    for(; l != rl.end(); l++) {
	Vertex dst;
	dst.set_version(_ospf.get_version());
	switch(l->get_type()) {
	case RouterLink::p2p:
	    XLOG_UNFINISHED();
	    break;
	case RouterLink::transit: {
	    size_t index;
	    if (find_network_lsa(l->get_link_id(), index)) {
		Lsa::LsaRef lsan = _db[index];
		// This can probably never happen
		if (lsan->maxage()) {
		    XLOG_WARNING("LSA in database MaxAge\n%s",
				 cstring(*lsan));
		    break;
		}
		// Both nodes exist check for
		// bi-directional connectivity.
		if (!bidirectional(*l, dynamic_cast<
				   NetworkLsa *>(lsan.get()))) {
				
		    break;
		}
		// Put both links back. If the network
		// vertex is not in the SPT put it in.
		// Create a destination node.
// 		printf("%s Cool %s\n", 
// 		       pr_id(_ospf.get_router_id()).c_str(),
// 		       cstring(*_db[index]));
		dst.set_type(Vertex::Network);
		dst.set_nodeid(lsan->get_header().
			       get_link_state_id());
		
		if (!spt.exists_node(dst)) {
		    spt.add_node(dst);
// 		    printf("%s Adding %s\n",
// 			   pr_id(_ospf.get_router_id()).c_str(),
// 			   cstring(dst));
		}
// 		printf("%s Edge %s %d %s\n",
// 		       pr_id(_ospf.get_router_id()).c_str(),
// 		       cstring(src),
// 		       l->get_metric(),
// 		       cstring(dst));
		update_edge(spt, src, l->get_metric(), dst);
		// Reverse edge
		update_edge(spt, dst, 0, src);
	    } else {
// 		printf("%s Network-Lsa not found for\n%s\n",
// 		       pr_id(_ospf.get_router_id()).c_str(),
// 		       cstring(*rlsa));
	    }
	}
	    break;
	case RouterLink::stub:
#if	0
	    dst.set_type(Vertex::Network);
	    dst.set_nodeid(l->get_link_id());
	    _spt.add_edge(v, l->get_metric(), dst);
#endif
	    break;
	case RouterLink::vlink:
	    XLOG_UNFINISHED();
	    break;
	}
	break;
    }
}

template <typename A>
void
AreaRouter<A>::routing_router_lsaV3(Spt<Vertex>& spt, const Vertex& v,
				    const RouterLsa *rlsa)
{
    debug_msg("Spt %s Vertex %s \n%s\n", cstring(spt), cstring(v),
	      cstring(*rlsa));

    XLOG_WARNING("TBD - add to SPT");
}


/*************************************************************************/

#ifdef	UNFINISHED_INCREMENTAL_UPDATE
template <typename A>
void
AreaRouter<A>::routing_begin()
{
    XLOG_ASSERT(_new_lsas.empty());

#ifdef  PARANOIA
    list<RouteCmd<Vertex> > r;
    _spt.compute(r);
    XLOG_ASSERT(r.empty());
#endif
    // Put back this routers interfaces.
    routing_add(_router_lsa, true);
}

template <typename A>
void
AreaRouter<A>::routing_add(Lsa::LsaRef lsar, bool known)
{
    debug_msg("%s\n", cstring(*lsar));

    // XXX - This lookup is currently expensive after TODO 28 it will
    // be fine.
    size_t index;
    if (!find_lsa(lsar, index))
	XLOG_FATAL("This LSA must be in the database\n%s\n", cstring(*lsar));

    _new_lsas.push_back(Bucket(index, known));
}

template <typename A>
void
AreaRouter<A>::routing_delete(Lsa::LsaRef lsar)
{
    debug_msg("%s\n", cstring(*lsar));

    RouterLsa *rlsa;
    if (0 != (rlsa = dynamic_cast<RouterLsa *>(lsar.get()))) {
	if (rlsa->get_v_bit())
	    _TransitCapability--;
    }

    XLOG_WARNING("TBD \n%s", cstring(*lsar));
}

template <typename A>
void
AreaRouter<A>::routing_end()
{
    typename list<Bucket>::iterator i;

    RouterLsa *rlsa;
    NetworkLsa *nlsa;

    // First part of the routing computation only consider Router-LSAs
    i = _new_lsas.begin();
    while (i != _new_lsas.end()) {
	Lsa::LsaRef lsar = _db[i->_index];
	if (0 != (rlsa = dynamic_cast<RouterLsa *>(lsar.get()))) {

	    if (rlsa->get_v_bit())
		_TransitCapability++;
	    
	    Vertex v;

	    v.set_version(_ospf.get_version());
	    v.set_type(Vertex::Router);
	    v.set_nodeid(rlsa->get_header().get_link_state_id());

	    // XXX 
	    // Really want to remove all the links but removing a node
	    // and putting it back has the same effect.
	    if (i->_known && _spt.exists_node(v)) {
		printf("%s Remove %s\n",
		       pr_id(_ospf.get_router_id()).c_str(),
		       cstring(v));
		_spt.remove_node(v);
	    } else {
		printf("%s New %s\n",
		       pr_id(_ospf.get_router_id()).c_str(),
		       cstring(v));
	    }

	    printf("%s Add %s\n",
		   pr_id(_ospf.get_router_id()).c_str(),
		   cstring(v));
	    _spt.add_node(v);

	    switch(_ospf.get_version()) {
		case OspfTypes::V2:
		    routing_router_lsaV2(v, rlsa);
		    break;
		case OspfTypes::V3:
		    routing_router_lsaV3(v, rlsa);
		    break;
	    }

	    _new_lsas.erase(i++);
	}  else if (0 != (nlsa = dynamic_cast<NetworkLsa *>(lsar.get()))) {
	    
	    printf("%s %s\n", pr_id(_ospf.get_router_id()).c_str(),
		   cstring(*nlsa));
	    i++;
	} else {
	    i++;
	}
    }

    i = _new_lsas.begin();
    while (i != _new_lsas.end()) {
	_new_lsas.erase(i++);
    }

    list<RouteCmd<Vertex> > r;
    _spt.compute(r);

    list<RouteCmd<Vertex> >::const_iterator ri;
    for(ri = r.begin(); ri != r.end(); ri++)
	XLOG_WARNING("TBD: Add route:\n%s %s",
		     pr_id(_ospf.get_router_id()).c_str(),
		     ri->str().c_str());
}

#endif

template class AreaRouter<IPv4>;
template class AreaRouter<IPv6>;
