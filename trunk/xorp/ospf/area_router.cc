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

#include "ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv6net.hh"

#include "libxorp/eventloop.hh"

#include "libproto/spt.hh"

#include "ospf.hh"
#include "ls_database.hh"
#include "area_router.hh"

template <typename A>
AreaRouter<A>::AreaRouter(Ospf<A>& ospf, OspfTypes::AreaID area,
			  OspfTypes::AreaType area_type, uint32_t options) 
    : _ospf(ospf), _area(area), _area_type(area_type), _options(options),
      _readers(0)
{
    // Never need to delete this as the ref_ptr will tidy up.
    RouterLsa *rlsa = new RouterLsa(_ospf.get_version());
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
    _db.push_back(_router_lsa);
    _last_entry = 1;
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
	publish(_router_lsa);
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
	publish(_router_lsa);
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
    psr->_router_links.push_back(router_link);

    if (update_router_links(psr)) {
	// publish the router LSA.
	publish(_router_lsa);
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
	publish(_router_lsa);
    }
		   
    return true;
}

template <typename A>
bool
AreaRouter<A>::newer_lsa(const Lsa_header&) const
{
    XLOG_WARNING("TBD");

    return true;
}

template <typename A>
DataBaseHandle
AreaRouter<A>::open_database(bool& empty)
{
    _readers++;

    empty = 0 == _last_entry;

    // Snapshot the current last entry position. While the database is
    // open (_readers > 0) new entries will be added past _last_entry.

    return DataBaseHandle(true, _last_entry);
}

template <typename A>
Lsa::LsaRef
AreaRouter<A>::get_entry_database(DataBaseHandle& dbh, bool& last)
{
    XLOG_ASSERT(dbh.valid());

    uint32_t position = dbh.position();

    if (position >= _db.size())
	XLOG_FATAL("Index too far %d length %d", position, _db.size());

    dbh.advance(last);

    return _db[position];
}

template <typename A>
void
AreaRouter<A>::close_database(DataBaseHandle& dbh)
{
    XLOG_ASSERT(dbh.valid());
    XLOG_ASSERT(0 != _readers);
    _readers--;

    if (dbh.last() != dbh.position())
	XLOG_WARNING("Closing database prematurely: size = %d position = %d",
		     dbh.last(), dbh.position());

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
    // to do something more optimal.

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

    // If we weren't advertising and we still aren't
    if (empty && router_lsa->get_router_links().empty())
	return false;

    XLOG_WARNING("TBD: Set/Unset V,E and B bits");

    router_lsa->encode();

    XLOG_WARNING("Update LS age");
    XLOG_WARNING("Update sequence number");

    return true;
}

template <typename A>
void
AreaRouter<A>::publish(Lsa::LsaRef lsar)
{
    debug_msg("Publish: %s\n", cstring(*lsar));


    XLOG_WARNING("TBD");
}

template class AreaRouter<IPv4>;
template class AreaRouter<IPv6>;

