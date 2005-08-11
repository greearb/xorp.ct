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
#include "libxorp/ipnet.hh"

#include "libxorp/eventloop.hh"

#include "libproto/spt.hh"

#include "ospf.hh"
#include "ls_database.hh"
#include "packet.hh"
#include "delay_queue.hh"
#include "vertex.hh"
#include "area_router.hh"
#include "peer.hh"
#include "peer_manager.hh"

template <typename A>
PeerManager<A>::~PeerManager()
{
    // Remove all the areas, this should cause all the peers to be
    // removed. Every call to destroy_area_router will change _areas,
    // so call begin again each time.
    for(;;) {
	typename map<OspfTypes::AreaID, AreaRouter<A> *>::iterator i;
	i = _areas.begin();
	if (i == _areas.end())
	    break;
	destroy_area_router((*i).first);
    }
    XLOG_ASSERT(_pmap.empty());
    XLOG_ASSERT(_peers.empty());
    XLOG_ASSERT(_areas.empty());
}

template <typename A>
bool
PeerManager<A>::create_area_router(OspfTypes::AreaID area,
				   OspfTypes::AreaType area_type)
{
    debug_msg("Area %s Type %s\n", pr_id(area).c_str(), 
	      pp_area_type(area_type).c_str());

    // Check this area doesn't already exist.
    if (0 != _areas.count(area)) {
	XLOG_ERROR("Area %s already exists\n", pr_id(area).c_str());
	return false;
    }

    _areas[area] = new AreaRouter<A>(_ospf, area, area_type,
				     compute_options(area_type));
    
    return true;
}

template <typename A>
AreaRouter<A> *
PeerManager<A>::get_area_router(OspfTypes::AreaID area)
{
    debug_msg("Area %s\n", pr_id(area).c_str());

    // Check this area exists.
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Area %s doesn't exist\n", pr_id(area).c_str());
	return 0;
    }

    return _areas[area];
}

template <typename A>
bool
PeerManager<A>::destroy_area_router(OspfTypes::AreaID area)
{
    debug_msg("Area %s\n", pr_id(area).c_str());

    // Verify this area exists.
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Area %s doesn't exist\n", pr_id(area).c_str());
	return false;
    }

    // Notify the peers that this area is being removed. If this is
    // the only area that the peer belonged to the peer can signify
    // this and the peer can be removed.
    typename map<PeerID, PeerOut<A> *>::iterator i;
    for(i = _peers.begin(); i != _peers.end();)
	if ((*i).second->remove_area(area)) {
	    delete_peer((*i).first);
	    i = _peers.begin();
	} else
	    i++;

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
PeerID
PeerManager<A>::create_peer(const string& interface, const string& vif,
			    const A source,
			    uint16_t interface_prefix_length,
			    uint16_t interface_mtu,
			    OspfTypes::LinkType linktype, 
			    OspfTypes::AreaID area)
    throw(BadPeer)
{
    debug_msg("Interface %s Vif %s source net %s mtu %d linktype %u area %s\n",
	      interface.c_str(), vif.c_str(),
	      cstring(source),  interface_mtu,
	      linktype, pr_id(area).c_str());

    AreaRouter<A> *area_router = get_area_router(area);

    // Verify that this area is known.
    if (0 == area_router)
	xorp_throw(BadPeer, 
		   c_format("Unknown Area %s", pr_id(area).c_str()));

    PeerID peerid = create_peerid(interface, vif);

    // If we got this far create_peerid did not throw an exception so
    // this interface/vif is unique.

    _peers[peerid] = new PeerOut<A>(_ospf, interface, vif, peerid,
				    source, interface_prefix_length,
				    interface_mtu, linktype,
				    area, area_router->get_area_type());

    // Pass in the option to be sent by the hello packet.
    _peers[peerid]->set_options(area,
				compute_options(area_router->get_area_type()));

    area_router->add_peer(peerid);

    return peerid;
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

    // Remove the interface/vif to PeerID mapping
    typename map<string, PeerID>::iterator pi;
    for(pi = _pmap.begin(); pi != _pmap.end(); pi++)
	if ((*pi).second == peerid) {
	    _pmap.erase(pi);
	    break;
	}

    return true;
}

template <typename A>
bool
PeerManager<A>::set_state_peer(const PeerID peerid, bool state)
{
    debug_msg("PeerID %u\n", peerid);

    if (0 == _peers.count(peerid)) {
	XLOG_ERROR("Unknown PeerID %u", peerid);
	return false;
    }

    _peers[peerid]->set_state(state);

    return true;
}

template <typename A>
bool
PeerManager<A>::receive(const string& interface, const string& vif,
			A dst, A src, Packet *packet)
    throw(BadPeer)
{
    debug_msg("Interface %s Vif %s src %s dst %s %s\n", interface.c_str(),
	      vif.c_str(), cstring(dst), cstring(src), cstring((*packet)));

    PeerID peerid = get_peerid(interface, vif);
    XLOG_ASSERT(0 != _peers.count(peerid));
    return _peers[peerid]->receive(dst, src, packet);
}

template <typename A>
bool
PeerManager<A>::queue_lsa(const PeerID peerid, const PeerID peer,
			  OspfTypes::NeighbourID nid, Lsa::LsaRef lsar,
			  bool &multicast_on_peer)
{
    if (0 == _peers.count(peerid)) {
	XLOG_ERROR("Unknown PeerID %u", peerid);
	return false;
    }

    return _peers[peerid]->queue_lsa(peer, nid, lsar, multicast_on_peer);
}

template <typename A>
bool
PeerManager<A>::push_lsas(const PeerID peerid)
{
    if (0 == _peers.count(peerid)) {
	XLOG_ERROR("Unknown PeerID %u", peerid);
	return false;
    }

    return _peers[peerid]->push_lsas();    
}

template <typename A>
bool
PeerManager<A>::known_interface_address(const A address) const
{
    // XXX
    // Note we are only checking the interface addresses of the
    // configured peers. We should be checking the interface addresses
    // with the FEA.

    typename map<PeerID, PeerOut<A> *>::const_iterator i;
    for(i = _peers.begin(); i != _peers.end();)
	if ((*i).second->get_interface_address() == address) 
	    return true;

    return false;
}

template <typename A>
bool
PeerManager<A>::on_link_state_request_list(const PeerID peerid,
					   OspfTypes::AreaID area,
					   const OspfTypes::NeighbourID nid,
					   Lsa::LsaRef lsar)
{
    if (0 == _peers.count(peerid)) {
	XLOG_ERROR("Unknown PeerID %u", peerid);
	return false;
    }

    return _peers[peerid]->on_link_state_request_list(area, nid, lsar);
}

template <typename A> 
bool
PeerManager<A>::set_interface_id(const PeerID peerid, OspfTypes::AreaID area,
		 uint32_t interface_id)
{
    if (0 == _peers.count(peerid)) {
	XLOG_ERROR("Unknown PeerID %u", peerid);
	return false;
    }

    return _peers[peerid]->set_interface_id(area, interface_id);
}

template <typename A>
bool
PeerManager<A>::set_hello_interval(const PeerID peerid, OspfTypes::AreaID area,
				   uint16_t hello_interval)
{
    if (0 == _peers.count(peerid)) {
	XLOG_ERROR("Unknown PeerID %u", peerid);
	return false;
    }

    return _peers[peerid]->set_hello_interval(area, hello_interval);
}

#if	0
template <typename A> 
bool
PeerManager<A>::set_options(const PeerID peerid, OspfTypes::AreaID area,
			    uint32_t options)
{
    if (0 == _peers.count(peerid)) {
	XLOG_ERROR("Unknown PeerID %u", peerid);
	return false;
    }

    return _peers[peerid]->set_options(area, options);
}
#endif

template <typename A> 
uint32_t
PeerManager<A>::compute_options(OspfTypes::AreaType area_type)

{
    // Set/UnSet E-Bit.
    Options options(_ospf.get_version(), 0);
    switch(area_type) {
    case OspfTypes::BORDER:
	options.set_e_bit(true);
	break;
    case OspfTypes::STUB:
    case OspfTypes::NSSA:
	options.set_e_bit(false);
	break;
    }

    return options.get_options();
}

template <typename A> 
bool
PeerManager<A>::set_router_priority(const PeerID peerid,
				    OspfTypes::AreaID area,
				    uint8_t priority)
{
    if (0 == _peers.count(peerid)) {
	XLOG_ERROR("Unknown PeerID %u", peerid);
	return false;
    }

    return _peers[peerid]->set_router_priority(area, priority);
}

template <typename A>
bool
PeerManager<A>::set_router_dead_interval(const PeerID peerid,
					 OspfTypes::AreaID area, 
					 uint32_t router_dead_interval)
{
    if (0 == _peers.count(peerid)) {
	XLOG_ERROR("Unknown PeerID %u", peerid);
	return false;
    }

    return _peers[peerid]->
	set_router_dead_interval(area, router_dead_interval);
}

template <typename A>
bool
PeerManager<A>::set_interface_cost(const PeerID peerid, 
				   OspfTypes::AreaID /*area*/,
				   uint16_t interface_cost)
{
    if (0 == _peers.count(peerid)) {
	XLOG_ERROR("Unknown PeerID %u", peerid);
	return false;
    }

    return _peers[peerid]->set_interface_cost(interface_cost);
}

template <typename A>
bool
PeerManager<A>::set_inftransdelay(const PeerID peerid, 
				   OspfTypes::AreaID /*area*/,
				   uint16_t inftransdelay)
{
    if (0 == _peers.count(peerid)) {
	XLOG_ERROR("Unknown PeerID %u", peerid);
	return false;
    }

    return _peers[peerid]->set_inftransdelay(inftransdelay);
}

template <typename A>
bool
PeerManager<A>::internal_router_p() const
{
    // True if are connected to only one area.
    return 1 == _areas.size() ? true : false;
}

template <typename A>
bool
PeerManager<A>::area_border_router_p() const
{
    // True if this router is connected to multiple areas..
    return 1 < _areas.size() ? true : false;
}

const OspfTypes::AreaID BACKBONE = OspfTypes::BACKBONE;

template <typename A>
bool
PeerManager<A>::backbone_router_p() const
{
    // True if one of the areas the router is connected to is the
    // backbone area.
    // XXX - The line below should be OspfTypes::BACKBONE the gcc34
    // compiler rejected this hence the local declaration.
    return 1 == _areas.count(BACKBONE) ? true : false;
}

template <typename A>
bool
PeerManager<A>::as_boundary_router_p() const
{
    // XXX - By definition, we don't yet have a way of redisting
    // routes from another protocol.
    return false;
}

template class PeerManager<IPv4>;
template class PeerManager<IPv6>;
