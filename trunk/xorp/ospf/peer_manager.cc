// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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
#include <queue>

#include "ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#include "ospf.hh"
#include "packet.hh"
#include "peer.hh"
#include "area_router.hh"
#include "peer_manager.hh"

template <typename A>
PeerManager<A>::~PeerManager()
{
    XLOG_UNFINISHED();
}

template <typename A>
bool
PeerManager<A>::create_area_router(OspfTypes::AreaID area,
				   OspfTypes::AreaType area_type)
{
    debug_msg("Area %s Type %s\n", area.str().c_str(), 
	      pp_area_type(area_type).c_str());

    // Check this area doesn't already exist.
    if (0 != _areas.count(area)) {
	XLOG_ERROR("Area %s already exists\n", area.str().c_str());
	return false;
    }

    _areas[area] = new AreaRouter<A>(_ospf, area, area_type);
    
    return true;
}

template <typename A>
AreaRouter<A> *
PeerManager<A>::get_area_router(OspfTypes::AreaID area)
{
    debug_msg("Area %s\n", area.str().c_str());

    // Check this area exists.
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Area %s doesn't exist\n", area.str().c_str());
	return 0;
    }

    return _areas[area];
}

template <typename A>
bool
PeerManager<A>::destroy_area_router(OspfTypes::AreaID area)
{
    debug_msg("Area %s\n", area.str().c_str());

    // Check this area doesn't already exist.
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Area %s doesn't exist\n", area.str().c_str());
	return false;
    }

    // Notify the peers that this area is being removed. If this is
    // the only area that the peer belonged to the peer can signify
    // this and the peer can be removed.
    typename map<PeerID, Peer<A> *>::iterator i;
    for(i = _peers.begin(); i != _peers.end(); i++)
	if ((*i).second->remove_area(area))
	    delete_peer((*i).first);

    delete _areas[area];
    _areas.erase(_areas.find(area));

    return true;
}

template <typename A>
PeerID
PeerManager<A>::create_peerid(const string& interface, const string& vif)
    throw(BadPeer)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());
    string concat = interface + "/" + vif;

    if (0 != _pmap.count(concat))
	xorp_throw(BadPeer,
		   c_format("Mapping for %s already exists", concat.c_str()));
			    
    PeerID peerid = _next_peerid++;
    _pmap[concat] = peerid;

    return peerid;
}

template <typename A>
PeerID
PeerManager<A>::get_peerid(const string& interface, const string& vif)
    throw(BadPeer)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());
    string concat = interface + "/" + vif;

    if (0 == _pmap.count(concat))
	xorp_throw(BadPeer, 
		   c_format("No mapping for %s exists", concat.c_str()));
			    
    return _pmap[concat];
}

template <typename A>
void
PeerManager<A>::destroy_peerid(const string& interface, const string& vif)
    throw(BadPeer)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());
    string concat = interface + "/" + vif;

    if (0 == _pmap.count(concat))
	xorp_throw(BadPeer, 
		   c_format("No mapping for %s exists", concat.c_str()));
			    
    _pmap.erase(_pmap.find(concat));
}

template <typename A>
bool
PeerManager<A>::create_peer(const string& interface, const string& vif,
    OspfTypes::LinkType linktype, OspfTypes::AreaID area)
    throw(BadPeer)
{
    debug_msg("Interface %s Vif %s linktype %u area %s\n",
	      interface.c_str(), vif.c_str(), linktype, area.str().c_str());

    AreaRouter<A> *area_router = get_area_router(area);

    // Verify that we know about this area.
    if (0 == area_router)
	return false;

    PeerID peerid = create_peerid(interface, vif);

    // If we got this far create_peerid did not throw an exception so
    // this interface/vif is unique.

    _peers[peerid] = new Peer<A>(_ospf, interface, vif, linktype, area);

    area_router->add_peer(peerid);

    return true;
}

template <typename A>
bool
PeerManager<A>::delete_peer(const PeerID peerid)
{
    debug_msg("PeerID %u\n", peerid);

    if (0 == _peers.count(peerid)) {
	XLOG_ERROR("Unknown PeerID %u", peerid);
	return false;
    }

    // Tell *all* area routers that this peer is being deleted.
    // It is simpler to do this than hold the reverse mappings.
    typename map<OspfTypes::AreaID, AreaRouter<A> *>::iterator i;
    for(i = _areas.begin(); i != _areas.end(); i++)
	(*i).second->delete_peer(peerid);

    delete _peers[peerid];
    _peers.erase(_peers.find(peerid));

    return true;
}

template <typename A>
void
PeerManager<A>::incoming_packet(const string& interface, const string& vif,
				Packet *packet)
{
    debug_msg("Interface %s Vif %s %s\n", interface.c_str(),
	      vif.c_str(), cstring((*packet)));
    XLOG_WARNING("TBD");
}

template class PeerManager<IPv4>;
template class PeerManager<IPv6>;
