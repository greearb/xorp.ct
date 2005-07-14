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

#ident "$XORP$"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

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

#include "libxorp/eventloop.hh"

#include "libproto/spt.hh"

#include "libproto/spt.hh"

#include "ospf.hh"
#include "ls_database.hh"
#include "delay_queue.hh"
#include "area_router.hh"

template <typename A>
AreaRouter<A>::AreaRouter(Ospf<A>& ospf, OspfTypes::AreaID area,
			  OspfTypes::AreaType area_type, uint32_t options) 
    : _ospf(ospf), _area(area), _area_type(area_type), _options(options),
      _last_entry(0), _allocated_entries(0), _readers(0),
      _queue(ospf.get_eventloop(),
			  OspfTypes::MinLSInterval,
			  callback(this, &AreaRouter<A>::publish_all))
{
    // Never need to delete this as the ref_ptr will tidy up.
    RouterLsa *rlsa = new RouterLsa(_ospf.get_version());
    rlsa->set_self_originating(true);
    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    rlsa->record_creation_time(now);

    Lsa_header& header = rlsa->get_header();

    switch (ospf.get_version()) {
    case OspfTypes::V2:
	header.set_options(_options);
	break;
    case OspfTypes::V3:
	rlsa->set_options(_options);
	break;
    }
    
    // This is a router LSA so the link state ID is the Router ID.
    header.set_link_state_id(ntohl(_ospf.get_router_id().addr()));
    header.set_advertising_router(ntohl(_ospf.get_router_id().addr()));

    _router_lsa = Lsa::LsaRef(rlsa);
    add_lsa(_router_lsa);
//     _db.push_back(_router_lsa);
//     _last_entry = 1;
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

    if (update_router_links(psr)) {
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

    if (update_router_links(psr)) {
	// publish the router LSA.
	_queue.add(_router_lsa);
    }
		   
    return true;
}

template <typename A>
bool
AreaRouter<A>::add_router_link(PeerID peerid, RouterLink& router_link)
{
    debug_msg("PeerID %u %s\n", peerid, cstring(router_link));

    if (0 == _peers.count(peerid)) {
	XLOG_WARNING("Peer not found %u", peerid);
	return false;
    }

    // Update the router link.
    typename PeerMap::iterator i = _peers.find(peerid);
    PeerStateRef psr = i->second;
    list<RouterLink>::const_iterator r;
    for (r = psr->_router_links.begin(); r != psr->_router_links.end(); r++) {
	if (router_link == (*r))
	    return true;
    }
    psr->_router_links.push_back(router_link);

    if (update_router_links(psr)) {
	// publish the router LSA.
	_queue.add(_router_lsa);
    }
		   
    return true;
}

template <typename A>
bool
AreaRouter<A>::remove_router_link(PeerID peerid)
{
    debug_msg("PeerID %u\n", peerid);

    if (0 == _peers.count(peerid)) {
	XLOG_WARNING("Peer not found %u", peerid);
	return false;
    }

    // Mark the the router link as invalid.
    typename PeerMap::iterator i = _peers.find(peerid);
    PeerStateRef psr = i->second;
    psr->_router_links.clear();

    if (update_router_links(psr)) {
	// publish the router LSA.
	_queue.add(_router_lsa);
    }
		   
    return true;
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
			    list<Lsa::LsaRef>& lsas, bool backup, bool dr)
{
    debug_msg("PeerID %u NeighbourID %u %s backup %s dr %s\n", peerid, nid,
	      pp_lsas(lsas).c_str(),
	      backup ? "true" : "false",
	      dr ? "true" : "false");

    TimeVal now;
    _ospf.get_eventloop().current_time(now);

    list<Lsa_header> delayed_ack, direct_ack;
    
    // RFC 2328 Section 13. The Flooding Procedure
    // Validate the incoming LSAs.
    
    list<Lsa::LsaRef>::const_iterator i;
    for (i = lsas.begin(); i != lsas.end(); i++) {
	// (1) Validate the LSA's LS checksum. 
	// (2) Check that the LSA's LS type is known.
	// Both checks already performed in the packet/LSA decoding process.

	// (3) In stub areas discard AS-external-LSA's (LS type = 5, 0x4005).
	switch(_area_type) {
	case OspfTypes::BORDER:
	    break;
	case OspfTypes::STUB:
 	    if ((*i)->external())
 		continue;
	    break;
	case OspfTypes::NSSA:
	    XLOG_WARNING("TBD Check RFC 3101");
	    break;
	}
	const Lsa_header& lsah = (*i)->get_header();
	size_t index;
	LsaSearch search = compare_lsa(lsah, index);

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
	case NEWER:
	    // (5) New or newer LSA.
	    // (a) If this LSA is too recent drop it.
	    if (NEWER == search) {
		TimeVal then;
		_db[index]->get_creation_time(then);
		if ((now - then) < TimeVal(OspfTypes::MinLSArrival))
		    continue;
	    }
	    // (b) Flood this LSA to all of our neighbours.
	    // RFC 2328 Section 13.3. Next step in the flooding procedure

	    // If this is an AS-external-LSA send it to all area's
	    if ((*i)->external())
		flood_all_areas((*i));

	    publish(peerid, nid, (*i));

	    // (c) Remove the current copy from neighbours
	    // retransmission lists.
	    // (d) Install the new LSA.
	    if (NOMATCH == search)
		add_lsa((*i));
	    else
		update_lsa((*i), index);

	    XLOG_WARNING("TBD Section 13.2");
	    
	    // (e) Possibly acknowledge this LSA.
	    // RFC 2328 Section 13.5 Sending Link State Acknowledgment Packets

	    XLOG_WARNING("TBD Section 13.5");
	    
	    // (f) Self orignating LSAs 
	    // RFC 2328 Section 13.4. Receiving self-originated LSAs

	    XLOG_WARNING("TBD Section 13.4");
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
    XLOG_WARNING("TBD process direct and delayed acks");
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
AreaRouter<A>::delete_lsa(Lsa::LsaRef lsar, size_t index)
{
    Lsa_header& dblsah = _db[index]->get_header();
    XLOG_ASSERT(dblsah.get_ls_type() == lsar->get_header().get_ls_type());
    XLOG_ASSERT(dblsah.get_link_state_id() == 
		lsar->get_header().get_link_state_id());
    XLOG_ASSERT(dblsah.get_advertising_router() ==
		lsar->get_header().get_advertising_router());

    XLOG_ASSERT(_db[index]->valid());
    _db[index]->invalidate();
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
    if (0 == _readers) {
	_db[index]->invalidate();
	_db[index] = lsar;
    } else {
	delete_lsa(lsar, index);
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

/**
 * RFC 2328 Section 13.1 Determining which LSA is newer.
 */
template <typename A>
typename AreaRouter<A>::LsaSearch
AreaRouter<A>::compare_lsa(const Lsa_header& candidate,
			   const Lsa_header& current) const
{
    if (current.get_ls_sequence_number() > candidate.get_ls_sequence_number())
	return OLDER;
    if (current.get_ls_sequence_number() < candidate.get_ls_sequence_number())
	return NEWER;

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
			list<Lsa::LsaRef>& lsas) const
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
	_db[index]->update_age(now);
	lsas.push_back(_db[index]);
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
AreaRouter<A>::subsequent(DataBaseHandle& dbh) const
{
    bool another = false;
    for (size_t index = dbh.position(); index < dbh.last(); index++) {
	if (!_db[index]->valid() || !_db[index]->available())
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
    } while(!_db[position]->valid() || !_db[position]->available());

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
bool
AreaRouter<A>::update_router_links(PeerStateRef /*psr*/)
{
    // We know exactly which peer transitioned state an optimisation
    // would be use this information to add or delete the specific
    // router link. For the moment rebuild the whole list every time
    // there is a maximum of two router links per interface so we are
    // looking at a handful of entries. If this ever changes use psr
    // to do something more efficient.

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

    XLOG_WARNING("TBD: Set/Unset V,E and B bits");

    // A new router LSA should have been generated before MaxAge was hit.
    XLOG_ASSERT(router_lsa->get_header().get_ls_age() != OspfTypes::MaxAge);

    // If this LSA has been transmitted then its okay to bump the
    // sequence number.
    if (router_lsa->get_transmitted()) {
	router_lsa->set_transmitted(false);
	router_lsa->increment_sequence_number();
	router_lsa->get_header().set_ls_age(0);
	TimeVal now;
	_ospf.get_eventloop().current_time(now);
	router_lsa->record_creation_time(now);
    }

    router_lsa->encode();

    return true;
}

template <typename A>
void
AreaRouter<A>::publish(const PeerID peerid, const OspfTypes::NeighbourID nid,
		       Lsa::LsaRef lsar) const
{
    debug_msg("Publish: %s\n", cstring(*lsar));

    TimeVal now;
    _ospf.get_eventloop().current_time(now);

    // Update the age field unless its self originating.
    if (lsar->get_self_originating()) {
	if (OspfTypes::MaxSequenceNumber == lsar->get_ls_sequence_number())
	    XLOG_FATAL("TBD: Flush this LSA and generate a new LSA");
    } else {
	lsar->update_age(now);
    }

    typename PeerMap::const_iterator i;
    for(i = _peers.begin(); i != _peers.end(); i++) {
	PeerStateRef temp_psr = i->second;
	if (temp_psr->_up) {
	    if (!_ospf.get_peer_manager().
		queue_lsa(i->first, peerid, nid, lsar))
		XLOG_FATAL("Unable to queue LSA");
	}
    }
}

template <typename A>
void
AreaRouter<A>::publish_all(Lsa::LsaRef lsar)
{
    debug_msg("Publish: %s\n", cstring(*lsar));
    publish(ALLPEERS, OspfTypes::ALLNEIGHBOURS, lsar);

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
AreaRouter<A>::flood_all_areas(Lsa::LsaRef lsar)
{
    debug_msg("Flood all areas: %s\n", cstring(*lsar));

    XLOG_UNFINISHED();
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
    XLOG_UNFINISHED();
}

template <typename A>
bool
AreaRouter<A>::on_link_state_request_list(const PeerID /*peerid*/,
					  const OspfTypes::NeighbourID /*nid*/,
					  Lsa::LsaRef /*lsar*/) const
{
    XLOG_UNFINISHED();
}

template <typename A>
void
AreaRouter<A>::event_bad_link_state_request(const PeerID /*peerid*/,
				    const OspfTypes::NeighbourID/*nid*/) const
{
    XLOG_UNFINISHED();
}

template <typename A>
bool
AreaRouter<A>::send_lsa(const PeerID /*peerid*/,
			const OspfTypes::NeighbourID /*nid*/,
			Lsa::LsaRef /*lsar*/) const
{
    XLOG_UNFINISHED();
}

template class AreaRouter<IPv4>;
template class AreaRouter<IPv6>;
