// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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



// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include <map>
#include <list>
#include <set>
#include <queue>
#include <typeinfo>

// XXX
// The system header files are here to get the sizeof an ipv4 and ipv6
// header. Really should have a define somewhere in XORP for this.
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
#ifdef HAVE_NETINET_IP6_H
#include <netinet/ip6.h>
#endif
#ifdef HOST_OS_WINDOWS
#include "fea/ip.h"
#endif

#include "libproto/spt.hh"

#include "ospf.hh"
#include "delay_queue.hh"
#include "vertex.hh"
#include "area_router.hh"
#include "auth.hh"
#include "peer.hh"


/**
 * Oh which linktype should multicast be enabled.
 */
inline
bool
do_multicast(OspfTypes::LinkType linktype)
{
    switch(linktype) {
    case OspfTypes::PointToPoint:
    case OspfTypes::BROADCAST:
	return true;
	break;
    case OspfTypes::NBMA:
    case OspfTypes::PointToMultiPoint:
    case OspfTypes::VirtualLink:
	return false;
	break;
    }

    XLOG_UNREACHABLE();

    return true;
}

template <typename A>
PeerOut<A>:: PeerOut(Ospf<A>& ospf, const string interface, const string vif, 
		     const OspfTypes::PeerID peerid,
		     const A interface_address,
		     OspfTypes::LinkType linktype, OspfTypes::AreaID area,
		     OspfTypes::AreaType area_type)
    : _ospf(ospf), _interface(interface), _vif(vif), _peerid(peerid),
      _interface_id(0),
      _interface_address(interface_address),
      _interface_prefix_length(0),
      _interface_mtu(0),
      _interface_cost(1), // Must be greater than 0.
      _inftransdelay(1),  // Must be greater than 0.
      _linktype(linktype), _running(false), _link_status(false),
      _status(false), _receiving(false)
{
    _areas[area] = new Peer<A>(ospf, *this, area, area_type);
}

template <typename A>
PeerOut<A>::~PeerOut()
{
    set_state(false);
    typename map<OspfTypes::AreaID, Peer<A> *>::iterator i;

    for(i = _areas.begin(); i != _areas.end(); i++)
	delete (*i).second;
}

template <>
bool 
PeerOut<IPv4>::match(IPv4 source, string& interface, string& vif)
{
    if (get_interface_address() == source) {
	interface = _interface;
	vif = _vif;
	return true;
    }
    return false;
}

template <>
bool 
PeerOut<IPv6>::match(IPv6 source, string& interface, string& vif)
{
    map<OspfTypes::AreaID, Peer<IPv6> *>::const_iterator i;
    for(i = _areas.begin(); i != _areas.end(); i++) {
	if ((*i).second->match(source)) {
	    interface = _interface;
	    vif = _vif;
	    return true;
	}
    }

    return false;
}

template <typename A>
void
PeerOut<A>::set_mask(Peer<A> *peer)
{
    if (typeid(A) != typeid(IPv4))
	return;
    peer->
	set_network_mask(ntohl(IPv4::make_prefix(get_interface_prefix_length())
			       .addr()));
}

template <typename A>
uint16_t
PeerOut<A>::get_frame_size() const
{
    uint16_t router_alert = 4 * sizeof(uint8_t);// Router Alert 
    uint16_t frame = get_interface_mtu() - router_alert;

    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	frame -= sizeof(struct ip);
	static_assert(20 == sizeof(struct ip));
	break;
    case OspfTypes::V3:
#ifdef HAVE_NETINET_IP6_H
	frame -= sizeof(struct ip6_hdr);
	static_assert(40 == sizeof(struct ip6_hdr));
#else
	frame -= 40;
#endif
	break;
    }

    return frame;
}

template <typename A>
bool
PeerOut<A>::get_areas(list<OspfTypes::AreaID> &areas) const
{
    typename map<OspfTypes::AreaID, Peer<A> *>::const_iterator i;

    for(i = _areas.begin(); i != _areas.end(); i++) {
	areas.push_back((*i).first);
    }

    return true;
}

template <typename A>
bool
PeerOut<A>::add_area(OspfTypes::AreaID area, OspfTypes::AreaType area_type)
{
    debug_msg("Area %s\n", pr_id(area).c_str());

    // Only OSPFv3 allows a peer to be connected to multiple areas.
    XLOG_ASSERT(OspfTypes::V3 == _ospf.get_version());

    Peer<A> *peer = _areas[area] = new Peer<A>(_ospf, *this, area, area_type);
    set_mask(peer);
    if (_running)
	peer->start();

    return true;
}

template <typename A>
bool
PeerOut<A>::change_area_router_type(OspfTypes::AreaID area,
				    OspfTypes::AreaType area_type)
{
    debug_msg("Area %s Type %s\n", pr_id(area).c_str(), 
	      pp_area_type(area_type).c_str());

    // All the peers are notified when an area type is changed
    if (0 == _areas.count(area)) {
	return false;
    }

    _areas[area]->change_area_router_type(area_type);

    return true;
}

template <typename A>
bool
PeerOut<A>::remove_area(OspfTypes::AreaID area)
{
    debug_msg("Area %s\n", pr_id(area).c_str());
    // All the peers are notified when an area is deleted.
    if (0 == _areas.count(area)) {
	return false;
    }
    
    delete _areas[area];
    _areas.erase(_areas.find(area));
    
    // If this peer is no longer serving any areas it can be deleted.
    if (_areas.empty())
	return true;
    else
	return false;
}

template <typename A>
bool
PeerOut<A>::add_neighbour(OspfTypes::AreaID area, A neighbour_address,
			  OspfTypes::RouterID rid)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->add_neighbour(neighbour_address, rid);
}

template <typename A>
bool
PeerOut<A>::remove_neighbour(OspfTypes::AreaID area, A neighbour_address,
			     OspfTypes::RouterID rid)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->remove_neighbour(neighbour_address, rid);
}

template <typename A>
void
PeerOut<A>::set_state(bool state)
{
    debug_msg("state %s\n", state ? "up" : "down");
    _status = state;
    peer_change();
}

template <typename A>
void
PeerOut<A>::set_link_status(bool state)
{
    debug_msg("state %s\n", state ? "up" : "down");
    _link_status = state;
    peer_change();
}

template <typename A>
void
PeerOut<A>::peer_change()
{
    switch (_running) {
    case true:
	if (false == _status || false == _link_status) {
	    take_down_peering();
	    _running = false;
	}
	break;
    case false:
	if (true == _status && true == _link_status) {
	    _running = true;
	    _running = bring_up_peering();
	}
	break;
    }
}

template <typename A>
bool
PeerOut<A>::transmit(typename Transmit<A>::TransmitRef tr)
{
    if (!_running) {
	XLOG_WARNING("Attempt to transmit while peer is not running");
	return false;
    }

    do {
	if (!tr->valid())
	    return true;
	size_t len;
	uint8_t *ptr = tr->generate(len);
	_ospf.get_peer_manager().transmit(_interface, _vif,
					  tr->destination(), tr->source(), 
					  ptr, len);
    } while(tr->multiple());

    return true;
}

template <typename A>
bool
PeerOut<A>::receive(A dst, A src, Packet *packet)
    throw(BadPeer)
{
    debug_msg("dst %s src %s %s\n", cstring(dst), cstring(src),
	      cstring(*packet));

    if (!_running) {
	// There is a window that may occasionally get hit.
	XLOG_WARNING("Packet arrived while peer is not running");
	return false;
    }

    OspfTypes::AreaID area = packet->get_area_id();
    // Does the area ID in the packet match any that are expecting.
    if (0 == _areas.count(area)) {
	if (OspfTypes::BACKBONE == area) {
	    return _ospf.get_peer_manager().
		receive_virtual_link(dst, src, packet);
	}
	xorp_throw(BadPeer, c_format("Area %s not handled by %s/%s",
				     pr_id(packet->get_area_id()).c_str(),
				     _interface.c_str(), _vif.c_str()));
    }

    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	// Non link-local addreses are only allowed for virtual links
	// which by definition are in the backbone.
	if (!src.is_linklocal_unicast() && OspfTypes::BACKBONE != area
	    && get_linktype() != OspfTypes::VirtualLink) {
	    typename map<OspfTypes::AreaID, Peer<A> *>::const_iterator i;

	    for(i = _areas.begin(); i != _areas.end(); i++)
		XLOG_WARNING("area %s:", pr_id((*i).first).c_str());

	    XLOG_WARNING("Packet has not been sent with a link-local address "
			 "%s %s", cstring(src), cstring(*packet));
	    return false;
	}
	break;
    }

    return _areas[area]->receive(dst, src, packet);
}

template <typename A>
bool 
PeerOut<A>::send_lsa(OspfTypes::AreaID area, const OspfTypes::NeighbourID nid,
		     Lsa::LsaRef lsar)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->send_lsa(nid, lsar);
}

template <typename A>
bool
PeerOut<A>::queue_lsa(OspfTypes::PeerID peerid, OspfTypes::NeighbourID nid,
		      Lsa::LsaRef lsar, bool &multicast_on_peer) const
{
    typename map<OspfTypes::AreaID, Peer<A> *>::const_iterator i;

    for(i = _areas.begin(); i != _areas.end(); i++) {
	if (!(*i).second->queue_lsa(peerid, nid, lsar, multicast_on_peer))
	    return false;
    }

    return true;
}

template <typename A>
bool
PeerOut<A>::push_lsas()
{
    typename map<OspfTypes::AreaID, Peer<A> *>::iterator i;

    for(i = _areas.begin(); i != _areas.end(); i++) {
	if (!(*i).second->push_lsas())
	    return false;
    }

    return true;
}

template <typename A>
bool 
PeerOut<A>::neighbours_exchange_or_loading(OspfTypes::AreaID area)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->neighbours_exchange_or_loading();
}

template <typename A>
bool 
PeerOut<A>::neighbour_at_least_two_way(OspfTypes::AreaID area,
				       OspfTypes::RouterID rid,
				       bool& twoway)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->neighbour_at_least_two_way(rid, twoway);
}

template <typename A>
bool 
PeerOut<A>::get_neighbour_address(OspfTypes::AreaID area,
				  OspfTypes::RouterID rid,
				  uint32_t interface_id,
				  A& neighbour_address)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->
	get_neighbour_address(rid, interface_id, neighbour_address);
}

template <typename A>
bool 
PeerOut<A>::on_link_state_request_list(OspfTypes::AreaID area,
				       const OspfTypes::NeighbourID nid,
				       Lsa::LsaRef lsar)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->on_link_state_request_list(nid, lsar);
}

template <typename A>
bool 
PeerOut<A>::event_bad_link_state_request(OspfTypes::AreaID area,
					 const OspfTypes::NeighbourID nid)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->event_bad_link_state_request(nid);
}

template <typename A>
bool 
PeerOut<A>::virtual_link_endpoint(OspfTypes::AreaID area)
{
    if (0 == _areas.count(area)) {
	// Can be call opportunistically
//   	XLOG_ERROR("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->virtual_link_endpoint();
}

template <typename A>
bool
PeerOut<A>::bring_up_peering()
{
    uint32_t interface_id = 0;
    switch (_ospf.get_version()) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	// Get the interface ID.
	if (!_ospf.get_interface_id(_interface, _vif, interface_id)) {
	    XLOG_ERROR("Unable to get interface ID for %s",
		       _interface.c_str());
	    return false;
	}
	set_interface_id(interface_id);

	// Get the link-local address to use for transmission unless
	// this is a virtual link.
	if (OspfTypes::VirtualLink != get_linktype()) {
	    A link_local_address;
	    if (!_ospf.get_link_local_address(_interface, _vif,
					      link_local_address)) {
		XLOG_ERROR("Unable to get link local address for %s/%s",
			   _interface.c_str(), _vif.c_str());
		return false;
	    }
	    set_interface_address(link_local_address);
	}
	break;
    }

    // Get the prefix length.
    A source = get_interface_address();
    if (!_ospf.get_prefix_length(_interface, _vif, source,
				 _interface_prefix_length)) {
	XLOG_ERROR("Unable to get prefix length for %s/%s/%s",
		   _interface.c_str(), _vif.c_str(),
		   cstring(source));
	return false;
    }

    // Get the MTU.
    _interface_mtu = _ospf.get_mtu(_interface);
    if (0 == _interface_mtu) {
	XLOG_ERROR("Unable to get MTU for %s", _interface.c_str());
	return false;
    }

    start_receiving_packets();

    typename map<OspfTypes::AreaID, Peer<A> *>::iterator i;

    for(i = _areas.begin(); i != _areas.end(); i++) {
	set_mask((*i).second);
	(*i).second->start();
	AreaRouter<A> *area_router = 
	    _ospf.get_peer_manager().get_area_router((*i).first);
	XLOG_ASSERT(area_router);
	area_router->peer_up(_peerid);
    }

    return true;
}

template <typename A>
void
PeerOut<A>::take_down_peering()
{
    typename map<OspfTypes::AreaID, Peer<A> *>::iterator i;

    for(i = _areas.begin(); i != _areas.end(); i++) {
	(*i).second->stop();
	AreaRouter<A> *area_router = 
	    _ospf.get_peer_manager().get_area_router((*i).first);
	XLOG_ASSERT(area_router);
	area_router->peer_down(_peerid);
    }

    stop_receiving_packets();
}

template <typename A>
bool
PeerOut<A>::get_passive()
{
    typename map<OspfTypes::AreaID, Peer<A> *>::iterator i;

    // If all the peers are passive we will consider the peerout passive.
    // This is used to determine if we should or should not receive packets. 
    // If in the future we allow multiple peers to be configured then
    // passive will have to be handled by dropping packets in software
    // in the peer input routines.
    for(i = _areas.begin(); i != _areas.end(); i++) {
	if (!(*i).second->get_passive())
	    return false;
    }
    
    return true;
}

template <typename A>
void
PeerOut<A>::start_receiving_packets()
{
    if (_receiving)
	return;
	
    if (!_running)
	return;

    if (get_passive())
	return;

    // Start receiving packets on this peering.
    _ospf.enable_interface_vif(_interface, _vif);

    if (do_multicast(get_linktype()))
	join_multicast_group(A::OSPFIGP_ROUTERS());

    _receiving = true;
}

template <typename A>
void
PeerOut<A>::stop_receiving_packets()
{
    if (!_receiving)
	return;

    // Stop receiving packets on this peering.
    if (do_multicast(get_linktype()))
	leave_multicast_group(A::OSPFIGP_ROUTERS());
    _ospf.disable_interface_vif(_interface, _vif);

    _receiving = false;
}

template <typename A> 
void
PeerOut<A>::router_id_changing()
{
    typename map<OspfTypes::AreaID, Peer<A> *>::const_iterator i;
    for(i = _areas.begin(); i != _areas.end(); i++) {
	(*i).second->router_id_changing();
    }
}

template <typename A> 
bool
PeerOut<A>::set_interface_id(uint32_t interface_id)
{
    _interface_id = interface_id;

    typename map<OspfTypes::AreaID, Peer<A> *>::const_iterator i;
    for(i = _areas.begin(); i != _areas.end(); i++) {
	(*i).second->set_interface_id(interface_id);
    }

    return true;
}

template <typename A>
bool
PeerOut<A>::get_attached_routers(OspfTypes::AreaID area,
				 list<RouterInfo>& routers)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->get_attached_routers(routers);
}

template <typename A>
bool
PeerOut<A>::add_advertise_net(OspfTypes::AreaID area, A addr, uint32_t prefix)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->add_advertise_net(addr, prefix);
}

template <typename A>
bool
PeerOut<A>::remove_all_nets(OspfTypes::AreaID area)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->remove_all_nets();
}

template <typename A>
bool
PeerOut<A>::update_nets(OspfTypes::AreaID area)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->update_nets();
}

template <typename A>
bool
PeerOut<A>::set_hello_interval(OspfTypes::AreaID area, uint16_t hello_interval)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->set_hello_interval(hello_interval);
}

template <typename A> 
bool
PeerOut<A>::set_options(OspfTypes::AreaID area,	uint32_t options)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->set_options(options);
}

template <typename A> 
bool
PeerOut<A>::set_retransmit_interval(OspfTypes::AreaID area,
				    uint16_t retransmit_interval)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->set_rxmt_interval(retransmit_interval);
}

template <typename A> 
bool
PeerOut<A>::set_router_priority(OspfTypes::AreaID area,	uint8_t priority)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->set_router_priority(priority);
}

template <typename A>
bool
PeerOut<A>::set_router_dead_interval(OspfTypes::AreaID area, 
				     uint32_t router_dead_interval)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->set_router_dead_interval(router_dead_interval);
}

template <typename A>
bool
PeerOut<A>::set_simple_authentication_key(OspfTypes::AreaID	area,
					  const string&		password,
					  string&		error_msg)
{
    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	XLOG_FATAL("OSPFv3 does not support authentication");
	break;
    }

    if (0 == _areas.count(area)) {
	error_msg = c_format("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->set_simple_authentication_key(password, error_msg);
}

template <typename A>
bool
PeerOut<A>::delete_simple_authentication_key(OspfTypes::AreaID	area,
					     string&		error_msg)
{
    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	XLOG_FATAL("OSPFv3 does not support authentication");
	break;
    }

    if (0 == _areas.count(area)) {
	error_msg = c_format("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->delete_simple_authentication_key(error_msg);
}

template <typename A>
bool
PeerOut<A>::set_md5_authentication_key(OspfTypes::AreaID area,
				       uint8_t		key_id,
				       const string&	password,
				       const TimeVal&	start_timeval,
				       const TimeVal&	end_timeval,
				       const TimeVal&	max_time_drift,
				       string&		error_msg)
{
    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	XLOG_FATAL("OSPFv3 does not support authentication");
	break;
    }

    if (0 == _areas.count(area)) {
	error_msg = c_format("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->set_md5_authentication_key(key_id, password,
						    start_timeval, end_timeval,
						    max_time_drift, error_msg);
}

template <typename A>
bool
PeerOut<A>::delete_md5_authentication_key(OspfTypes::AreaID	area,
					  uint8_t		key_id,
					  string&		error_msg)
{
    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	XLOG_FATAL("OSPFv3 does not support authentication");
	break;
    }

    if (0 == _areas.count(area)) {
	error_msg = c_format("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->delete_md5_authentication_key(key_id, error_msg);
}

template <typename A>
bool
PeerOut<A>::set_passive(OspfTypes::AreaID area, bool passive, bool host)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s", pr_id(area).c_str());
	return false;
    }

    return _areas[area]->set_passive(passive, host);
}

template <typename A>
 bool
PeerOut<A>::set_interface_cost(uint16_t interface_cost)
{
    _interface_cost = interface_cost;

    typename map<OspfTypes::AreaID, Peer<A> *>::iterator i;
    for(i = _areas.begin(); i != _areas.end(); i++)
	(*i).second->update_router_links();

    return true;
}

template <typename A>
bool
PeerOut<A>::get_neighbour_list(list<OspfTypes::NeighbourID>& neighbours) const
{
    typename map<OspfTypes::AreaID, Peer<A> *>::const_iterator i;
    for(i = _areas.begin(); i != _areas.end(); i++) {
	(*i).second->get_neighbour_list(neighbours);
    }

    return true;
}

template <typename A>
bool
PeerOut<A>::get_neighbour_info(OspfTypes::NeighbourID nid,
			       NeighbourInfo& ninfo) const
{
    typename map<OspfTypes::AreaID, Peer<A> *>::const_iterator i;
    for(i = _areas.begin(); i != _areas.end(); i++) {
	if ((*i).second->get_neighbour_info(nid, ninfo)) {
	    return true;
	}
    }

    return false;
}

template <typename A>
set<AddressInfo<A> >&
PeerOut<A>::get_address_info(OspfTypes::AreaID area)
{
    if (0 == _areas.count(area)) {
	XLOG_ERROR("Unknown Area %s unable to return address info",
		   pr_id(area).c_str());
	return _dummy;
    }

    return _areas[area]->get_address_info();
}

/****************************************/

template <typename A>
bool
Peer<A>::initV3()
{
    if (OspfTypes::VirtualLink == get_linktype())
	return true;

    // Never need to delete this as the ref_ptr will tidy up.
    LinkLsa *llsa = new LinkLsa(_ospf.get_version());
    llsa->set_self_originating(true);
    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    llsa->record_creation_time(now);
    llsa->set_peerid(get_peerid());
    _link_lsa = Lsa::LsaRef(llsa);    

    return true;
}

template <typename A>
bool
Peer<A>::goV3()
{
    if (OspfTypes::VirtualLink == get_linktype())
	return true;

    populate_link_lsa();

    get_area_router()->add_link_lsa(get_peerid(), _link_lsa);

    return true;
}

template <typename A>
bool
Peer<A>::shutdownV3()
{
    if (OspfTypes::VirtualLink == get_linktype())
	return true;

    get_area_router()->withdraw_link_lsa(get_peerid(), _link_lsa);

    return true;
}

template <>
void
Peer<IPv4>::populate_link_lsa()
{
}

template <>
void
Peer<IPv6>::populate_link_lsa()
{
    XLOG_ASSERT(OspfTypes::VirtualLink != get_linktype());

    LinkLsa *llsa = dynamic_cast<LinkLsa *>(_link_lsa.get());
    XLOG_ASSERT(llsa);
    llsa->get_header().set_link_state_id(get_interface_id());
    llsa->get_header().set_advertising_router(_ospf.get_router_id());
    // The router priority is set in the set_router_priority method.
    // The options are set in the set_options method.
    llsa->set_link_local_address(get_interface_address());
}

template <typename A>
bool
Peer<A>::match(IPv6 source) const
{
    if (OspfTypes::VirtualLink == get_linktype())
	return false;

    LinkLsa *llsa = dynamic_cast<LinkLsa *>(_link_lsa.get());
    XLOG_ASSERT(llsa);

    const list<IPv6Prefix>& link_prefixes = llsa->get_prefixes();
    list<IPv6Prefix>::const_iterator i;
    for (i = link_prefixes.begin(); i != link_prefixes.end(); i++)
	if (i->get_network().masked_addr() == source)
	    return true;

    return false;
}

template <typename A>
bool
Peer<A>::add_neighbour(A neighbour_address, OspfTypes::RouterID rid)
{
    switch(get_linktype()) {
    case OspfTypes::PointToPoint:
	if (0 != _neighbours.size()) {
	    XLOG_ERROR("A PointToPoint link should have only one neighbour");
	    return false;
	}
	break;
    case OspfTypes::BROADCAST:
	// Just allow it there isn't any harm.
	break;
    case OspfTypes::NBMA:
	XLOG_UNFINISHED();
	break;
    case OspfTypes::PointToMultiPoint:
	// Allow multiple neighbours to be added.
	break;
    case OspfTypes::VirtualLink:
	break;
    }

    Neighbour<A> *n = find_neighbour(neighbour_address, rid);

    if (0 == n) {
	n = new Neighbour<A>(_ospf, *this, rid, neighbour_address,
			     Neighbour<A>::_ticket++, get_linktype());
	_neighbours.push_back(n);
    } else {
	XLOG_ERROR("Neighbour exists %s", cstring(*n));
	return false;
    }

    update_router_links();

    return true;
}

template <typename A>
bool
Peer<A>::remove_neighbour(A neighbour_address, OspfTypes::RouterID rid)
{
    Neighbour<A> *n = find_neighbour(neighbour_address, rid);

    if (0 == n) {
	XLOG_ERROR("Neighbour not found Address: %s RouterID %s",
		   cstring(neighbour_address),
		   pr_id(rid).c_str());
	return false;
    }

    typename list<Neighbour<A> *>::iterator ni;
    for(ni = _neighbours.begin(); ni != _neighbours.end(); ni++) {
	if (*ni == n) {
	    (*ni)->event_kill_neighbour();
	    delete (*ni);
	    _neighbours.erase(ni);
	    update_router_links();
	    return true;
	}
    }

    return false;
}

template <>
bool
Peer<IPv4>::belongs(IPv4 addr) const
{
    return addr == get_interface_address();
}

template <>
bool
Peer<IPv6>::belongs(IPv6 addr) const
{
    if (addr == get_interface_address())
	return true;

    return match(addr);
}

template <typename A>
bool
Peer<A>::receive(A dst, A src, Packet *packet)
{
    // Please note that the return status from this method determines
    // if the packet is freed. A return status of false means free the
    // packet it hasn't been stored.

    debug_msg("dst %s src %s %s\n", cstring(dst), cstring(src),
	      cstring(*packet));

    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	if (packet->get_instance_id() != _ospf.get_instance_id()) {
	    XLOG_TRACE(_ospf.trace()._input_errors,
		       "Instance ID does not match %d\n%s",
		       _ospf.get_instance_id(), cstring(*packet));
	    return false;
	}
	break;
    }

    // RFC 2328 Section 8.2. Receiving protocol packets

    // As the packet has reached this far a bunch of the tests have
    // already been performed.

    if (!belongs(dst) && 
	dst != A::OSPFIGP_ROUTERS() &&
	dst != A::OSPFIGP_DESIGNATED_ROUTERS()) {
	XLOG_TRACE(_ospf.trace()._input_errors,
		   "Destination address not acceptable %s\n%s",
		   cstring(dst), cstring(*packet));
	return false;
    }

    if (src == get_interface_address() &&
	(dst == A::OSPFIGP_ROUTERS() ||
	 dst == A::OSPFIGP_DESIGNATED_ROUTERS())) {
	    XLOG_TRACE(_ospf.trace()._input_errors,
		       "Dropping self originated packet %s\n%s",
		       cstring(src), cstring(*packet));
	return false;
    }

    switch(get_linktype()) {
    case OspfTypes::BROADCAST:
    case OspfTypes::NBMA:
    case OspfTypes::PointToMultiPoint:
	switch(_ospf.get_version()) {
	case OspfTypes::V2: {
	    const uint16_t plen = get_interface_prefix_length();
	    if (IPNet<A>(get_interface_address(), plen) != 
		IPNet<A>(src, plen)) {
		XLOG_TRACE(_ospf.trace()._input_errors,
			   "Dropping packet from foreign network %s\n",
			   cstring(IPNet<A>(src, plen)));
		return false;
	    }
	}
	    break;
	case OspfTypes::V3:
	    break;
	}
	break;
    case OspfTypes::VirtualLink:
	break;
    case OspfTypes::PointToPoint:
	break;
    }

    if (dst == A::OSPFIGP_DESIGNATED_ROUTERS()) {
	switch(get_state()) {
	case Peer<A>::Down:
	case Peer<A>::Loopback:
	case Peer<A>::Waiting:
	case Peer<A>::Point2Point:
	case Peer<A>::DR_other:
	    XLOG_TRACE(_ospf.trace()._input_errors,
	       "Must be in state DR or backup to receive ALLDRouters\n");
	    return false;
	    break;
	case Peer<A>::Backup:
	    break;
	case Peer<A>::DR:
	    break;
	}
    }

    // Authenticate packet.
    Neighbour<A> *n = find_neighbour(src, packet->get_router_id());
    bool new_peer = true;
    if (0 == n)
	new_peer = true;
    else
	new_peer = false;
    if (! _auth_handler.verify(packet->get(), src, new_peer)) {
	XLOG_TRACE(_ospf.trace()._input_errors, "Authentication failed: %s",
		   _auth_handler.error().c_str());
	return false;
    }

    HelloPacket *hello;
    DataDescriptionPacket *dd;
    LinkStateRequestPacket *lsrp;
    LinkStateUpdatePacket *lsup;
    LinkStateAcknowledgementPacket *lsap;

    if (0 != (hello = dynamic_cast<HelloPacket *>(packet))) {
	return process_hello_packet(dst, src, hello);
    } else if(0 != (dd = dynamic_cast<DataDescriptionPacket *>(packet))) {
	return process_data_description_packet(dst, src, dd);
    } else if(0 != (lsrp = dynamic_cast<LinkStateRequestPacket *>(packet))) {
	return process_link_state_request_packet(dst, src, lsrp);
    } else if(0 != (lsup = dynamic_cast<LinkStateUpdatePacket *>(packet))) {
	return process_link_state_update_packet(dst, src, lsup);
    } else if(0 != (lsap = dynamic_cast<LinkStateAcknowledgementPacket *>
		    (packet))) {
	return process_link_state_acknowledgement_packet(dst, src, lsap);
    } else {
	// A packet of unknown type will only get this far if
	// the deocoder has recognised it. Packets with bad type
	// fields will not get here.
	XLOG_FATAL("Unknown packet type %u", packet->get_type());
    }

    return false;
}

template <typename A>
bool 
Peer<A>::accept_lsa(Lsa::LsaRef lsar) const
{
    switch(get_linktype()) {
    case OspfTypes::BROADCAST:
    case OspfTypes::NBMA:
    case OspfTypes::PointToMultiPoint:
    case OspfTypes::PointToPoint:
	break;
    case OspfTypes::VirtualLink:
	if (lsar->external())
	    return false;
    }

    return true;;
}

template <typename A>
bool 
Peer<A>::send_lsa(const OspfTypes::NeighbourID nid, Lsa::LsaRef lsar) const
{
    if (!accept_lsa(lsar))
	return true;

    typename list<Neighbour<A> *>::const_iterator n;
    for(n = _neighbours.begin(); n != _neighbours.end(); n++)
	if ((*n)->get_neighbour_id() == nid)
	    return (*n)->send_lsa(lsar);

    XLOG_UNREACHABLE();

    return false;
}

template <typename A>
bool
Peer<A>::queue_lsa(OspfTypes::PeerID peerid, OspfTypes::NeighbourID nid,
		   Lsa::LsaRef lsar, bool &multicast_on_peer) const
{
    debug_msg("lsa %s nid %d \n", cstring(*lsar), nid);

    if (!accept_lsa(lsar))
	return true;

    multicast_on_peer = false;
    typename list<Neighbour<A> *>::const_iterator n;
    for(n = _neighbours.begin(); n != _neighbours.end(); n++)
	if (!(*n)->queue_lsa(peerid, nid, lsar, multicast_on_peer))
	    return false;

    return true;
}

template <typename A>
bool
Peer<A>::push_lsas()
{
    typename list<Neighbour<A> *>::iterator n;
    for(n = _neighbours.begin(); n != _neighbours.end(); n++)
	if (!(*n)->push_lsas())
	    return false;

    return true;
}

template <typename A>
bool
Peer<A>::do_dr_or_bdr() const
{
    switch(get_linktype()) {
    case OspfTypes::BROADCAST:
    case OspfTypes::NBMA:
	return true;
	break;
    case OspfTypes::PointToMultiPoint:
    case OspfTypes::VirtualLink:
    case OspfTypes::PointToPoint:
	return false;
	break;
    }
    XLOG_UNREACHABLE();

    return false;
}

template <typename A>
bool
Peer<A>::is_DR() const
{
    XLOG_ASSERT(do_dr_or_bdr());

    if (DR == get_state()) {
	if(get_candidate_id() != get_designated_router())
	    XLOG_WARNING("State DR %s != %s Did the router ID change?",
			 pr_id(get_candidate_id()).c_str(),
			 pr_id(get_designated_router()).c_str());
	return true;
    }

    return false;
}

template <typename A>
bool
Peer<A>::is_BDR() const
{
    XLOG_ASSERT(do_dr_or_bdr());

    if (Backup == get_state()) {
	if (get_candidate_id() != get_backup_designated_router())
	    XLOG_WARNING("State Backup %s != %s Did the router ID change?",
			 pr_id(get_candidate_id()).c_str(),
			 pr_id(get_backup_designated_router()).c_str());
	return true;
    }

    return false;
}

template <typename A>
bool
Peer<A>::is_DR_or_BDR() const
{
    XLOG_ASSERT(do_dr_or_bdr());
    XLOG_ASSERT(!(is_DR() && is_BDR()));

    if (is_DR())
	return true;

    if (is_BDR())
	return true;

    return false;
}

template <typename A>
bool 
Peer<A>::neighbours_exchange_or_loading() const
{
    typename list<Neighbour<A> *>::const_iterator n;
    for(n = _neighbours.begin(); n != _neighbours.end(); n++) {
	typename Neighbour<A>::State state = (*n)->get_state();
	if (Neighbour<A>::Exchange == state ||
	    Neighbour<A>::Loading == state)
	    return true;
    }

    return false;
}

template <typename A>
bool 
Peer<A>::neighbour_at_least_two_way(OspfTypes::RouterID rid, bool& twoway)
{
    typename list<Neighbour<A> *>::const_iterator n;
    for(n = _neighbours.begin(); n != _neighbours.end(); n++)
	if ((*n)->get_router_id() == rid) {
	    twoway = (*n)->get_state() >= Neighbour<A>::TwoWay;
	    return true;
	}

    return false;
}

template <typename A>
bool 
Peer<A>::get_neighbour_address(OspfTypes::RouterID rid, uint32_t interface_id,
			       A& neighbour_address)
{
    typename list<Neighbour<A> *>::const_iterator n;
    for(n = _neighbours.begin(); n != _neighbours.end(); n++)
	if ((*n)->get_router_id() == rid) {
	    const HelloPacket *hello = (*n)->get_hello_packet();
	    if (0 == hello)
		return false;
	    if (hello->get_interface_id() == interface_id) {
		neighbour_address = (*n)->get_neighbour_address();
		return true;
	    }
	    return false;
	}

    return false;
}

template <typename A>
bool 
Peer<A>::on_link_state_request_list(const OspfTypes::NeighbourID nid,
				    Lsa::LsaRef lsar) const
{
    typename list<Neighbour<A> *>::const_iterator n;
    for(n = _neighbours.begin(); n != _neighbours.end(); n++)
	if ((*n)->get_neighbour_id() == nid)
	    return (*n)->on_link_state_request_list(lsar);

    XLOG_UNREACHABLE();

    return false;
}

template <typename A>
bool 
Peer<A>::event_bad_link_state_request(const OspfTypes::NeighbourID nid) const
{
    typename list<Neighbour<A> *>::const_iterator n;
    for(n = _neighbours.begin(); n != _neighbours.end(); n++)
	if ((*n)->get_neighbour_id() == nid) {
	    (*n)->event_bad_link_state_request();
	    return true;
	}

    XLOG_UNREACHABLE();

    return false;
}

template <typename A>
bool 
Peer<A>::virtual_link_endpoint() const
{
    typename list<Neighbour<A> *>::const_iterator n;
    for(n = _neighbours.begin(); n != _neighbours.end(); n++) {
	// If this peer is associated with a virtual link it should
	// have only one neighbour.
	if (OspfTypes::VirtualLink == (*n)->get_linktype() &&
	    Neighbour<A>::Full == (*n)->get_state())
	    return true;
    }

    return false;
}

template <typename A>
void
Peer<A>::send_direct_acks(OspfTypes::NeighbourID nid,
			  list<Lsa_header>& ack)
{
    // A direct ACK is only sent back to the neighbour that sent the
    // original LSA. 

    if (ack.empty())
	return;

    bool multicast_on_peer;
    typename list<Neighbour<A> *>::const_iterator n;
    for(n = _neighbours.begin(); n != _neighbours.end(); n++)
	if ((*n)->get_neighbour_id() == nid) {
	    if (!(*n)->send_ack(ack, /* direct */true, multicast_on_peer))
		XLOG_WARNING("Failed to send ACK");
	    XLOG_ASSERT(!multicast_on_peer);
	    return;
	}

    XLOG_UNREACHABLE();
}

template <typename A>
void
Peer<A>::send_delayed_acks(OspfTypes::NeighbourID /*nid*/,
			   list<Lsa_header>& ack)
{
    // A delayed ACK is sent to all neighbours.

    if (ack.empty())
	return;

    // XXX - The sending of these ACKs should be delayed.
    // One possible implementation would be to hold a delayed ack list
    // in the peer. If when this method is called the list is empty
    // the list is populated with acks that have been provided. Then a
    // timer is started that is less than the retransmit value. If the
    // list is non-empty when this method is called the provided acks
    // are added to the list suppressing duplicates. The timer will
    // fire and send the ACKs and empty the list. Currently ACKs are
    // generated for every update packet, so its not possible that the
    // ACKs will not fit into a single packet. If a list with a delay
    // is used it is possible that all the ACKS will not fit into a
    // single packet.
    // XXX

    bool multicast_on_peer;
    typename list<Neighbour<A> *>::const_iterator n;
    for(n = _neighbours.begin(); n != _neighbours.end(); n++) {
	(*n)->send_ack(ack, /* direct */false, multicast_on_peer);
	// If this is broadcast peer it is only necessary to send the
	// packet once.
	if (multicast_on_peer)
	    return;
    }
}

template <typename A>
Neighbour<A> *
Peer<A>::find_neighbour(A src, OspfTypes::RouterID rid)
{
    typename list<Neighbour<A> *>::iterator n;
    switch(get_linktype()) {
    case OspfTypes::BROADCAST:
    case OspfTypes::NBMA:
    case OspfTypes::PointToMultiPoint:
	for(n = _neighbours.begin(); n != _neighbours.end(); n++)
	    if ((*n)->get_neighbour_address() == src)
		return *n;
	break;
    case OspfTypes::VirtualLink:
    case OspfTypes::PointToPoint:
	for(n = _neighbours.begin(); n != _neighbours.end(); n++)
	    if ((*n)->get_router_id() == rid)
		return *n;
	break;
    }

    return 0;
}

template <typename A>
bool
Peer<A>::is_neighbour_DR_or_BDR(OspfTypes::NeighbourID nid) const
{
    XLOG_ASSERT(do_dr_or_bdr());

    typename list<Neighbour<A> *>::const_iterator n;
    for(n = _neighbours.begin(); n != _neighbours.end(); n++)
	if ((*n)->get_neighbour_id() == nid)
	    return (*n)->is_neighbour_DR_or_BDR();

    XLOG_UNREACHABLE();

    return false;
}

template <typename A>
bool
Peer<A>::process_hello_packet(A dst, A src, HelloPacket *hello)
{
    debug_msg("dst %s src %s %s\n",cstring(dst),cstring(src),cstring(*hello));

    // Sanity check this hello packet.

    // Check the network masks - OSPFv2 only.
    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	if (OspfTypes::PointToPoint == get_linktype() ||
	    OspfTypes::VirtualLink == get_linktype())
	    break;
	if (_hello_packet.get_network_mask() !=
	    hello->get_network_mask()) {
	    XLOG_TRACE(_ospf.trace()._input_errors,
		       "Network masks don't match %#x %s",
		       _hello_packet.get_network_mask(),
		       hello->str().c_str());
	    return false;
	}
	break;
    case OspfTypes::V3:
	break;
    }

    // Check the hello interval.
    if (_hello_packet.get_hello_interval() != 
	hello->get_hello_interval()) {
	XLOG_TRACE(_ospf.trace()._input_errors,
		   "Hello intervals don't match %d %s",
		   _hello_packet.get_hello_interval(),
		   hello->str().c_str());
	return false;
    }

    // Check the router dead interval.
    if (_hello_packet.get_router_dead_interval() != 
	hello->get_router_dead_interval()) {
	XLOG_TRACE(_ospf.trace()._input_errors,
		   "Router dead intervals don't match %d %s",
		   _hello_packet.get_router_dead_interval(),
		   hello->str().c_str());
	return false;
    }

    // Compare our and the received E-Bit they must match.
    if ((_hello_packet.get_options() & Options::E_bit) !=
	(hello->get_options() & Options::E_bit)) {
	    XLOG_TRACE(_ospf.trace()._input_errors,
		       "E-bit does not match %s",
		       hello->str().c_str());
	    return false;
    }

    // Compare our and the received N-Bit they must match.
    if ((_hello_packet.get_options() & Options::N_bit) !=
	(hello->get_options() & Options::N_bit)) {
	    XLOG_TRACE(_ospf.trace()._input_errors,
		       "N-bit does not match %s",
		       hello->str().c_str());
	    return false;
    }

    Neighbour<A> *n = find_neighbour(src, hello->get_router_id());

    if (0 == n) {
	// If this isn't a BROADCAST interface don't just make friends.
	if (OspfTypes::BROADCAST != _peerout.get_linktype())
	    return false;
	n = new Neighbour<A>(_ospf, *this, hello->get_router_id(), src,
			     Neighbour<A>::_ticket++, get_linktype());
	_neighbours.push_back(n);
    }

    n->event_hello_received(hello);

    return true;
}

template <typename A>
bool
Peer<A>::process_data_description_packet(A dst,
					 A src,
					 DataDescriptionPacket *dd)
{
    debug_msg("dst %s src %s %s\n",cstring(dst),cstring(src),cstring(*dd));

    Neighbour<A> *n = find_neighbour(src, dd->get_router_id());

    if (0 == n) {
	XLOG_TRACE(_ospf.trace()._input_errors,
		   "No matching neighbour found source %s %s",
		   cstring(src),
		   cstring(*dd));
	
	return false;
    }

    // Perform the MTU check.
    if (dd->get_interface_mtu() > get_interface_mtu()) {
	XLOG_TRACE(_ospf.trace()._input_errors,
		   "Received MTU larger than %d %s",
		   get_interface_mtu(),
		   cstring(*dd));
	return false;
    }

    n->data_description_received(dd);

    return false;	// Never keep a copy of the packet.
}

template <typename A>
bool
Peer<A>::process_link_state_request_packet(A dst, A src,
					   LinkStateRequestPacket *lsrp)
{
    debug_msg("dst %s src %s %s\n",cstring(dst),cstring(src),cstring(*lsrp));

    Neighbour<A> *n = find_neighbour(src, lsrp->get_router_id());

    if (0 == n) {
	XLOG_TRACE(_ospf.trace()._input_errors,
		   "No matching neighbour found source %s %s",
		   cstring(src),
		   cstring(*lsrp));
	
	return false;
    }

    n->link_state_request_received(lsrp);
    
    return false;	// Never keep a copy of the packet.
}

template <typename A>
bool
Peer<A>::process_link_state_update_packet(A dst, A src,
					  LinkStateUpdatePacket *lsup)
{
    debug_msg("dst %s src %s %s\n",cstring(dst),cstring(src),cstring(*lsup));

    Neighbour<A> *n = find_neighbour(src, lsup->get_router_id());

    if (0 == n) {
	XLOG_TRACE(_ospf.trace()._input_errors,
		   "No matching neighbour found source %s %s",
		   cstring(src),
		   cstring(*lsup));
	
	return false;
    }

    n->link_state_update_received(lsup);
    
    return false;	// Never keep a copy of the packet.
}

template <typename A>
bool
Peer<A>::process_link_state_acknowledgement_packet(A dst, A src,
				     LinkStateAcknowledgementPacket *lsap)
{
    debug_msg("dst %s src %s %s\n",cstring(dst),cstring(src),cstring(*lsap));

    Neighbour<A> *n = find_neighbour(src, lsap->get_router_id());

    if (0 == n) {
	XLOG_TRACE(_ospf.trace()._input_errors,
		   "No matching neighbour found source %s %s",
		   cstring(src),
		   cstring(*lsap));
	
	return false;
    }

    n->link_state_acknowledgement_received(lsap);
    
    return false;	// Never keep a copy of the packet.
}

template <typename A>
void
Peer<A>::start()
{
    go();

    _enabled = true;
    //    _interface_state = Down;
    set_designated_router(set_id("0.0.0.0"));
    set_backup_designated_router(set_id("0.0.0.0"));
    if (_passive)
	event_loop_ind();
    else
	event_interface_up();
}

template <typename A>
void
Peer<A>::stop()
{
    _enabled = false;
    event_interface_down();

    shutdown();
}

template <typename A>
void
Peer<A>::change_area_router_type(OspfTypes::AreaType area_type)
{
    bool enabled = _enabled;

    if (enabled)
	stop();

    set_area_type(area_type);

    if (enabled)
	start();
}

template <typename A>
void
Peer<A>::event_interface_up()
{
    const char *event_name = "InterfaceUp";
    XLOG_TRACE(_ospf.trace()._interface_events,
	       "Event(%s) Interface(%s) State(%s) ", event_name,
	       get_if_name().c_str(), pp_interface_state(get_state()).c_str());

    XLOG_ASSERT(Down == get_state());

    switch(get_linktype()) {
    case OspfTypes::PointToPoint:
	change_state(Point2Point);
	start_hello_timer();
	break;
    case OspfTypes::BROADCAST:
	// Not eligible to be the designated router.
	if (0 == _hello_packet.get_router_priority()) {
	    change_state(DR_other);
	    start_hello_timer();
	} else {
	    change_state(Waiting);
	    start_hello_timer();
	    start_wait_timer();
	}
	break;
    case OspfTypes::NBMA:
	XLOG_UNFINISHED();
	break;
    case OspfTypes::PointToMultiPoint:
	change_state(Point2Point);
	start_hello_timer();
	break;
    case OspfTypes::VirtualLink:
	change_state(Point2Point);
	start_hello_timer();
	break;
    }

    update_router_links();

    XLOG_ASSERT(Down != get_state());
}

template <typename A>
void
Peer<A>::event_wait_timer()
{
    const char *event_name = "WaitTimer";
    XLOG_TRACE(_ospf.trace()._interface_events,
	       "Event(%s) Interface(%s) State(%s) ", event_name,
	       get_if_name().c_str(), pp_interface_state(get_state()).c_str());

    switch(get_state()) {
    case Down:
    case Loopback:
	XLOG_FATAL("Unexpected state %s",
		     pp_interface_state(get_state()).c_str());
	break;
    case Waiting:
	compute_designated_router_and_backup_designated_router();

	// The user has set the priority to zero while the router is in
	// state waiting.
	if (0 == _hello_packet.get_router_priority()) {
	    debug_msg("State(%s) Priority(%d)\n",
		      pp_interface_state(get_state()).c_str(),
		      _hello_packet.get_router_priority());
	    if (get_state() == Waiting)
		change_state(DR_other);
	}

	XLOG_ASSERT(get_state() == DR_other || get_state() == Backup ||
		    get_state() == DR);

	break;
    case Point2Point:
    case DR_other:
    case Backup:
    case DR:
	XLOG_FATAL("Unexpected state %s",
		   pp_interface_state(get_state()).c_str());
	break;
    }

    update_router_links();

    // Start sending hello packets.
    start_hello_timer();
}

template <typename A>
void
Peer<A>::event_backup_seen()
{
    const char *event_name = "BackupSeen";
    XLOG_TRACE(_ospf.trace()._interface_events,
	       "Event(%s) Interface(%s) State(%s) ", event_name,
	       get_if_name().c_str(), pp_interface_state(get_state()).c_str());

    switch(get_state()) {
    case Down:
    case Loopback:
	XLOG_FATAL("Unexpected state %s",
		     pp_interface_state(get_state()).c_str());
	break;
    case Waiting:
	stop_wait_timer();
	compute_designated_router_and_backup_designated_router();

	XLOG_ASSERT(get_state() == DR_other || get_state() == Backup ||
		    get_state() == DR);

	break;
    case Point2Point:
    case DR_other:
    case Backup:
    case DR:
	XLOG_FATAL("Unexpected state %s",
		   pp_interface_state(get_state()).c_str());
	break;
    }

    update_router_links();
}

template <typename A>
void
Peer<A>::event_neighbour_change()
{
    const char *event_name = "NeighborChange";
    XLOG_TRACE(_ospf.trace()._interface_events,
	       "Event(%s) Interface(%s) State(%s) ", event_name,
	       get_if_name().c_str(), pp_interface_state(get_state()).c_str());

    switch(get_state()) {
    case Down:
	break;
    case Waiting:
	break;
    case Loopback:
    case Point2Point:
	XLOG_WARNING("Unexpected state %s",
		     pp_interface_state(get_state()).c_str());
	break;
    case DR_other:
    case Backup:
    case DR:
	compute_designated_router_and_backup_designated_router();

	XLOG_ASSERT(get_state() == DR_other ||
		    get_state() == Backup ||
		    get_state() == DR);

	break;
    }

    update_router_links();
}

template <typename A>
void
Peer<A>::event_loop_ind()
{
    const char *event_name = "LoopInd";
    XLOG_TRACE(_ospf.trace()._interface_events,
	       "Event(%s) Interface(%s) State(%s) ", event_name,
	       get_if_name().c_str(), pp_interface_state(get_state()).c_str());

    change_state(Loopback);

    tear_down_state();
    update_router_links();

    remove_neighbour_state();

    _peerout.stop_receiving_packets();
}

template <typename A>
void
Peer<A>::event_unloop_ind()
{
    const char *event_name = "UnLoopInd";
    XLOG_TRACE(_ospf.trace()._interface_events,
	       "Event(%s) Interface(%s) State(%s) ", event_name,
	       get_if_name().c_str(), pp_interface_state(get_state()).c_str());

    switch(get_state()) {
    case Down:
	XLOG_WARNING("Unexpected state %s",
		     pp_interface_state(get_state()).c_str());
	break;
    case Loopback:
	change_state(Down);
	break;
    case Waiting:
    case Point2Point:
    case DR_other:
    case Backup:
    case DR:
	XLOG_WARNING("Unexpected state %s",
		     pp_interface_state(get_state()).c_str());

	break;
    }

    update_router_links();

    _peerout.start_receiving_packets();
}

template <typename A>
void
Peer<A>::event_interface_down()
{
    const char *event_name = "InterfaceDown";
    XLOG_TRACE(_ospf.trace()._interface_events,
	       "Event(%s) Interface(%s) State(%s) ", event_name,
	       get_if_name().c_str(), pp_interface_state(get_state()).c_str());

    change_state(Down);

    tear_down_state();
    update_router_links();

    remove_neighbour_state();
}

template <typename A>
void
Peer<A>::schedule_event(const char *event)
{
    if (_scheduled_events.empty()) {
	_event_timer = _ospf.get_eventloop().
	    new_oneoff_after_ms(0,
				callback(this,
					 &Peer<A>::process_scheduled_events));
    }

    _scheduled_events.push_back(event);
}

template <typename A>
void
Peer<A>::process_scheduled_events()
{
    struct event {
	string event_name;
	XorpCallback0<void>::RefPtr cb;
    } events[] = {
	{"NeighbourChange", callback(this, &Peer<A>::event_neighbour_change)},
 	{"BackupSeen", callback(this, &Peer<A>::event_backup_seen)},
    };

    _scheduled_events.unique();

    list<string>::const_iterator e;
    for(e = _scheduled_events.begin(); e != _scheduled_events.end(); e++) {
	bool found = false;
	for (size_t i = 0; i < sizeof(events) / sizeof(struct event); i++) {
	    if ((*e) == events[i].event_name) {
		events[i].cb->dispatch();
		found = true;
		break;
	    }
	}
	if (!found)
	    XLOG_FATAL("Unknown event %s", (*e).c_str());
    }
    _scheduled_events.clear();
}

template <typename A>
void
Peer<A>::start_hello_timer()
{
    // XXX - The hello packet should have all its parameters set.

    // Schedule one for the future.
    _hello_timer = _ospf.get_eventloop().
	new_periodic_ms(_hello_packet.get_hello_interval() * 1000,
			callback(this, &Peer<A>::send_hello_packet));

    // Send one immediately.
    send_hello_packet();

    // A more logical way of structuring the code may have been to get
    // send_hello_packet to schedule the next hello packet. The
    // problem with such an approach is that the interval between
    // transmissions would not be fixed. Any elasticity would allow
    // hello packets to become synchronized which is bad.
}

template <typename A>
void
Peer<A>::stop_hello_timer()
{
    _hello_timer.clear();
}

template <typename A>
void
Peer<A>::restart_hello_timer()
{
    if (_hello_timer.scheduled()) {
	stop_hello_timer();
	start_hello_timer();
    }
}

template <typename A>
void
Peer<A>::start_wait_timer()
{
    _wait_timer = _ospf.get_eventloop().
	new_oneoff_after(TimeVal(_hello_packet.get_router_dead_interval(), 0),
			 callback(this, &Peer<A>::event_wait_timer));
}

template <typename A>
void
Peer<A>::stop_wait_timer()
{
    _wait_timer.clear();
}

#if	0
template <typename A>
uint32_t
Peer<A>::send_options()
{
    // Set/UnSet E-Bit.
    Options options(_ospf.get_version(), 0);
    switch(_area_type) {
    case OspfTypes::NORMAL:
	options.set_e_bit(true);
	break;
    case OspfTypes::STUB:
    case OspfTypes::NSSA:
	options.set_e_bit(false);
	break;
    }

    return options.get_options();
}
#endif

template <typename A>
void
Peer<A>::populate_common_header(Packet& packet)
{
    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	packet.set_instance_id(_ospf.get_instance_id());
	break;
    }

    // Fetch the router ID.
    packet.set_router_id(_ospf.get_router_id());

    // Set the Area ID
    packet.set_area_id(get_area_id());
}

template <typename A>
bool
Peer<A>::send_hello_packet()
{
    vector<uint8_t> pkt;

    // Fetch the router ID.
    _hello_packet.set_router_id(_ospf.get_router_id());

#if	0
    // Options.
    _hello_packet.set_options(send_options());
#endif

    // Put the neighbours into the hello packet.
    _hello_packet.get_neighbours().clear();
    typename list<Neighbour<A> *>::iterator n;
    for(n = _neighbours.begin(); n != _neighbours.end(); n++) {
	if ((*n)->announce_in_hello_packet())
	    _hello_packet.get_neighbours().push_back((*n)->get_router_id());
    }

    _hello_packet.encode(pkt);
    get_auth_handler().generate(pkt);
    
    SimpleTransmit<A> *transmit = 0;

    switch(get_linktype()) {
    case OspfTypes::PointToPoint:
    case OspfTypes::BROADCAST:
	transmit = new SimpleTransmit<A>(pkt, A::OSPFIGP_ROUTERS(), 
					 _peerout.get_interface_address());
	break;
    case OspfTypes::NBMA:
	XLOG_UNFINISHED();
	break;
    case OspfTypes::PointToMultiPoint:
	// At the time of writing virtual links had only one neighbour.
    case OspfTypes::VirtualLink: 
	for(n = _neighbours.begin(); n != _neighbours.end(); n++) {
	    transmit = new SimpleTransmit<A>(pkt,
					     (*n)->get_neighbour_address(), 
					     _peerout.get_interface_address());
	    typename Transmit<A>::TransmitRef tr(transmit);
	    _peerout.transmit(tr);
	}
	return true;
	break;
    }

    typename Transmit<A>::TransmitRef tr(transmit);

    _peerout.transmit(tr);

    return true;
}

template <>
OspfTypes::RouterID
Peer<IPv4>::get_candidate_id(IPv4 source_address, OspfTypes::RouterID)
{
    return ntohl(source_address.addr());
}

template <>
OspfTypes::RouterID
Peer<IPv6>::get_candidate_id(IPv6, OspfTypes::RouterID router_id)
{
    return router_id;
}

template <>
OspfTypes::RouterID
Peer<IPv4>::get_candidate_id(IPv4) const
{
    return ntohl(_peerout.get_interface_address().addr());
}

template <>
OspfTypes::RouterID
Peer<IPv6>::get_candidate_id(IPv6) const
{
    return _ospf.get_router_id();
}

template <typename A>
OspfTypes::RouterID
Peer<A>::backup_designated_router(list<Candidate>& candidates) const
{
    XLOG_ASSERT(do_dr_or_bdr());

    // Step (2)
    // Calculate the the new backup designated router.
    // Look for routers that do not consider themselves to be the DR
    // but do consider themselves to the the BDR.
    Candidate c(set_id("0.0.0.0"), set_id("0.0.0.0"), set_id("0.0.0.0"),
		set_id("0.0.0.0"), 0);
    typename list<Candidate>::const_iterator i;
    for(i = candidates.begin(); i != candidates.end(); i++) {
	XLOG_TRACE(_ospf.trace()._election, "Candidate: %s ", cstring((*i)));
	if ((*i)._candidate_id != (*i)._dr &&
	    (*i)._candidate_id == (*i)._bdr) {
	    if ((*i)._router_priority > c._router_priority)
		c = *i;
	    else if ((*i)._router_priority == c._router_priority &&
		     (*i)._router_id > c._router_id)
		c = *i;
		
	}
    }

    // It is possible that no router was selected because no router
    // had itself as BDR.
    if (0 == c._router_priority) {
	for(i = candidates.begin(); i != candidates.end(); i++) {
	    if ((*i)._candidate_id != (*i)._dr) {
		if ((*i)._router_priority > c._router_priority)
		    c = *i;
		else if ((*i)._router_priority == c._router_priority &&
			 (*i)._router_id > c._router_id)
		    c = *i;
		
	    }
	}
    }

    XLOG_TRACE(_ospf.trace()._election, "New BDR %s",
	       pr_id(c._candidate_id).c_str());

    return c._candidate_id;
}

template <typename A>
OspfTypes::RouterID
Peer<A>::designated_router(list<Candidate>& candidates,
			   OspfTypes::RouterID backup_designated_router) const
{
    XLOG_ASSERT(do_dr_or_bdr());

    // Step (3)
    // Calculate the designated router.
    Candidate c(set_id("0.0.0.0"), set_id("0.0.0.0"), set_id("0.0.0.0"),
		set_id("0.0.0.0"), 0);
    typename list<Candidate>::const_iterator i;
    for(i = candidates.begin(); i != candidates.end(); i++) {
	XLOG_TRACE(_ospf.trace()._election, "Candidate: %s ", cstring((*i)));
	if ((*i)._candidate_id == (*i)._dr) {
	    if ((*i)._router_priority > c._router_priority)
		c = *i;
	    else if ((*i)._router_priority == c._router_priority &&
		     (*i)._router_id > c._router_id)
		c = *i;
		
	}
    }

    // It is possible that no router was selected because no router
    // had itself as DR. Therefore just select the backup designated router.
    if (0 == c._router_priority) {
	XLOG_TRACE(_ospf.trace()._election,"New DR chose BDR %s",
		  pr_id(backup_designated_router).c_str());
	return backup_designated_router;
    }

    XLOG_TRACE(_ospf.trace()._election, "New DR %s",
	      pr_id(c._candidate_id).c_str());

    return c._candidate_id;
}

template <typename A>
void
Peer<A>::compute_designated_router_and_backup_designated_router()
{
    XLOG_ASSERT(do_dr_or_bdr());
    XLOG_TRACE(_ospf.trace()._election, "Start election: DR %s BDR %s",
	       pr_id(get_designated_router()).c_str(),
	       pr_id(get_backup_designated_router()).c_str());

    list<Candidate> candidates;

    // Is this router a candidate?
    if (0 != _hello_packet.get_router_priority()) {
	candidates.
	    push_back(Candidate(get_candidate_id(),
				_ospf.get_router_id(),
				_hello_packet.get_designated_router(),
				_hello_packet.get_backup_designated_router(),
				_hello_packet.get_router_priority()));
					
    }

    // Go through the neighbours and pick possible candidates.
    typename list<Neighbour<A> *>::const_iterator n;
    for (n = _neighbours.begin(); n != _neighbours.end(); n++) {
	const HelloPacket *hello = (*n)->get_hello_packet();
	if (0 == hello)
	    continue;
	// A priority of 0 means a router is not a candidate.
	if (0 != hello->get_router_priority() &&
	    Neighbour<A>::TwoWay <= (*n)->get_state()) {
	    candidates.
		push_back(Candidate((*n)->get_candidate_id(),
				    (*n)->get_router_id(),
				    hello->get_designated_router(),
				    hello->get_backup_designated_router(),
				    hello->get_router_priority()));
	}
    }
    
    // Step (2)
    // Calculate the the new backup designated router.
    OspfTypes::RouterID bdr = backup_designated_router(candidates);

    // Step (3)
    // Calculate the designated router.
    OspfTypes::RouterID dr = designated_router(candidates, bdr);

    // Step (4)
    // If the router has become the DR or BDR or it was the DR or BDR
    // and no longer is then steps (2) and (3) need to be repeated.

    // If nothing has changed out of here.
    if (_hello_packet.get_designated_router() == dr &&
	_hello_packet.get_backup_designated_router() == bdr)
	if (Waiting != get_state()) {
	    XLOG_TRACE(_ospf.trace()._election, "End election: No change");
	    return;
	}

    bool recompute = false;
    // Has this router just become the DR or BDR
    if (get_candidate_id() == dr && 
	_hello_packet.get_designated_router() != dr)
	recompute = true;
    if (get_candidate_id() == bdr && 
	_hello_packet.get_backup_designated_router() != bdr)
	recompute = true;

    // Was this router the DR or BDR
    if (get_candidate_id() != dr && 
	_hello_packet.get_designated_router() == get_candidate_id())
	recompute = true;
    if (get_candidate_id() != bdr && 
	_hello_packet.get_backup_designated_router() == get_candidate_id())
	recompute = true;

    if (recompute) {
	// If this router was the DR or BDR and the priority was set
	// to 0 we can get here.
	if (0 != _hello_packet.get_router_priority()) {
	    typename list<Candidate>::iterator i = candidates.begin();
	    // Verify that the first entry in the candidate list is
	    // this router.
	    XLOG_ASSERT((*i)._candidate_id == get_candidate_id());
	    // Update the DR and BDR
	    (*i)._dr = dr;
	    (*i)._bdr = bdr;
	}
	// Repeat steps (2) and (3).
	bdr = backup_designated_router(candidates);
	dr = designated_router(candidates, bdr);
    }
    
    XLOG_TRACE(_ospf.trace()._election, "End election: DR %s BDR %s",
	       pr_id(dr).c_str(), pr_id(bdr).c_str());

    // Step(5)
    set_designated_router(dr);
    set_backup_designated_router(bdr);

    if (get_candidate_id() == dr)
	change_state(DR);
    else if (get_candidate_id() == bdr)
	change_state(Backup);
    else
	change_state(DR_other);

    // Step(6)
    if(OspfTypes::NBMA == get_linktype())
	XLOG_UNFINISHED();
	
    // Step(7)
    // Need to send AdjOK to all neighbours that are least 2-Way.
    for(n = _neighbours.begin(); n != _neighbours.end(); n++)
	if (Neighbour<A>::TwoWay <= (*n)->get_state())
	    (*n)->event_adj_ok();
}

#if	0
/**
 * Utility function to extract a 32 quantity, basically an IPv4
 * address. The code uses templates so sometimes code is generated to
 * extract a 32 bit quantity from an IPv6 address.
 *
 */
template <typename A> uint32_t get32(A a);

template <>
inline
uint32_t
get32<IPv4>(IPv4 a)
{
    return a.addr();
}

template <>
inline
uint32_t
get32<IPv6>(IPv6)
{
    XLOG_FATAL("Only IPv4 not IPv6");
    return 0;
}
#endif

template <typename A>
void
Peer<A>::update_router_links()
{
    debug_msg("Interface(%s) State(%s) Linktype(%s)\n",
	      get_if_name().c_str(),
	      pp_interface_state(get_state()).c_str(),
	      pp_link_type(get_linktype()).c_str());

    OspfTypes::Version version = _ospf.get_version();

    // Save the old router links so that its possible to tell if the
    // router links have changed.
    list<RouterLink> router_links;
    router_links.insert(router_links.begin(),
			_router_links.begin(), _router_links.end());
    _router_links.clear();

    switch(version) {
    case OspfTypes::V2:
	update_router_linksV2(_router_links);
	break;
    case OspfTypes::V3:
	if (0 != _neighbours.size())
	    update_router_linksV3(_router_links);
	break;
    }

    list<RouterLink>::iterator i, j;
    bool equal = false;
    if (router_links.size() == _router_links.size()) {
	for (i = router_links.begin(); i != router_links.end(); i++) {
	    equal = false;
	    for (j = _router_links.begin(); j != _router_links.end(); j++) {
		if(*i == *j) {
		    equal = true;
		    break;
		}
	    }
	    if (equal == false)
		break;
	}
    }
    if (equal == false) {
	get_area_router()->new_router_links(get_peerid(), _router_links);
    }
}

template <>
uint32_t
Peer<IPv4>::get_designated_router_interface_id(IPv4) const
{
    XLOG_ASSERT(do_dr_or_bdr());

    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	XLOG_FATAL("OSPFv3 Only");
	break;
    case OspfTypes::V3:
	break;
    }

    XLOG_UNREACHABLE();

    return 0;
}

template <>
uint32_t
Peer<IPv6>::get_designated_router_interface_id(IPv6) const
{
    XLOG_ASSERT(do_dr_or_bdr());

    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	XLOG_FATAL("OSPFv3 Only");
	break;
    case OspfTypes::V3:
	break;
    }

    switch(get_state()) {
    case Down:
    case Loopback:
    case Waiting:
    case Point2Point:
	break;
    case Backup:
    case DR_other: {
	// Scan through the neighbours until we find the DR.
	list<Neighbour<IPv6> *>::const_iterator n;
	for(n = _neighbours.begin(); n != _neighbours.end(); n++)
	    if (get_designated_router() == (*n)->get_router_id()) {
		XLOG_ASSERT((*n)->get_hello_packet());
		return (*n)->get_hello_packet()->get_interface_id();
		break;
	    }
	XLOG_FATAL("Designated router not found");
    }
	break;
    case DR:
	// If this is the DR simple.
	return get_interface_id();
	break;
    }
    XLOG_FATAL("Designated router interface ID "
	       "available in states DR, DR Other and Backup not %s",
	       pp_interface_state(get_state()).c_str());

    return 0;
}

template <>
void
Peer<IPv4>::update_router_linksV2(list<RouterLink>& router_links)
{
    OspfTypes::Version version = _ospf.get_version();
    RouterLink router_link(version);

    switch(get_state()) {
    case Down:
	// No links
	return;
	break;
    case Loopback: {
	// XXX - We should be checking to see if this is p2p unnumbered.
	uint16_t prefix_length;
	if (_passive_host)
	    prefix_length = IPv4::ADDR_BITLEN;
	else {
	    prefix_length = get_interface_prefix_length();
	}
	IPNet<IPv4> net(get_interface_address(), prefix_length);
	router_link.set_type(RouterLink::stub);
	router_link.set_link_id(ntohl(net.masked_addr().addr()));
	router_link.set_link_data(ntohl(net.netmask().addr()));
	router_link.set_metric(0);
	router_links.push_back(router_link);
    }
	return;
	break;
    case Waiting:
	break;
    case Point2Point:
	// Dealt with below in PointToPoint case.
	break;
    case DR_other:
	break;
    case Backup:
	break;
    case DR:
	break;
    }
    
    switch(get_linktype()) {
    case OspfTypes::PointToPoint: {
	// RFC 2328 Section 12.4.1.1. Describing point-to-point interfaces
	// There should be a single neighbour configured
	if (1 != _neighbours.size()) {
	    XLOG_ERROR("A single neighbour should be configured");
	    return;
	}
	list<Neighbour<IPv4> *>::const_iterator n = _neighbours.begin();
	XLOG_ASSERT(n != _neighbours.end());

	if (Neighbour<IPv4>::Full == (*n)->get_state()) {
	    router_link.set_type(RouterLink::p2p);
	    router_link.set_link_id((*n)->get_router_id());
	    // We are not dealing with the unnumbered interface case.
	    router_link.set_link_data(ntohl(get_interface_address().addr()));
	    router_link.set_metric(_peerout.get_interface_cost());
	    router_links.push_back(router_link);
	}

	switch(get_state()) {
	case Down:
	    break;
	case Loopback:
	    break;
	case Waiting:
	    break;
	case Point2Point:
	    // If a prefix length has been set to 32
	    // (IPv4::ADDR_BITLEN) no network has been configured,
	    // announce a host route, otherwise advertise the network.
	    if (IPv4::ADDR_BITLEN == _peerout.get_interface_prefix_length()) {
		// Option 1
		router_link.set_type(RouterLink::stub);
		router_link.
		    set_link_id(ntohl((*n)->get_neighbour_address().addr()));
		router_link.set_link_data(0xffffffff);
		router_link.set_metric(_peerout.get_interface_cost());
		router_links.push_back(router_link);
	    } else {
		// Option 2
		router_link.set_type(RouterLink::stub);
		IPNet<IPv4> net(get_interface_address(),
				_peerout.get_interface_prefix_length());
		router_link.set_link_id(ntohl(net.masked_addr().addr()));
		router_link.set_link_data(ntohl(net.netmask().addr()));
		router_link.set_metric(_peerout.get_interface_cost());
		router_links.push_back(router_link);
	    }
	    break;
	case DR_other:
	    break;
	case Backup:
	    break;
	case DR:
	    break;
	}
    }
	break;
    case OspfTypes::BROADCAST:
    case OspfTypes::NBMA: {
	// RFC 2328 Section 12.4.1.2.Describing broadcast and NBMA interfaces
	bool adjacent = false;
	switch(get_state()) {
	case Down:
	    break;
	case Loopback:
	    break;
	case Waiting: {
	    router_link.set_type(RouterLink::stub);
	    IPNet<IPv4> net(get_interface_address(),
			    _peerout.get_interface_prefix_length());
	    router_link.set_link_id(ntohl(net.masked_addr().addr()));
	    router_link.set_link_data(ntohl(net.netmask().addr()));
	    router_link.set_metric(_peerout.get_interface_cost());
	    router_links.push_back(router_link);
	}
	    break;
	case Point2Point:
	    XLOG_UNFINISHED();
	    break;
	case Backup:	// XXX - Is this correct for the backup state?
	case DR_other: {
	    // Do we have an adjacency with the DR?
	    list<Neighbour<IPv4> *>::iterator n;
	    for(n = _neighbours.begin(); n != _neighbours.end(); n++)
		if (get_designated_router() == (*n)->get_candidate_id()) {
		    break;
		}
	    if (n == _neighbours.end())
		return;
	    if (Neighbour<IPv4>::Full == (*n)->get_state())
		adjacent = true;
	}
	    /* FALLTHROUGH */
	case DR: {
	    // Try and find any adjacency.
	    list<Neighbour<IPv4> *>::iterator n;
	    for(n = _neighbours.begin(); n != _neighbours.end(); n++)
		if (Neighbour<IPv4>::Full == (*n)->get_state()) {
		    adjacent = true;
		    break;
		}
	    if (adjacent) {
		router_link.set_type(RouterLink::transit);

		router_link.set_link_id(get_designated_router());
		router_link.set_link_data(ntohl(get_interface_address().
						addr()));

	    } else {
		router_link.set_type(RouterLink::stub);
		IPNet<IPv4> net(get_interface_address(),
			     _peerout.get_interface_prefix_length());
		router_link.set_link_id(ntohl(net.masked_addr().addr()));
		router_link.set_link_data(ntohl(net.netmask().addr()));

	    }
	    router_link.set_metric(_peerout.get_interface_cost());
	    router_links.push_back(router_link);
	}
	    break;
	}
    }
    
	break;
    case OspfTypes::PointToMultiPoint: {
	// RFC 2328 Section 12.4.1.4. Describing Point-to-MultiPoint interfaces
	router_link.set_type(RouterLink::stub);
	router_link.set_link_id(ntohl(get_interface_address().addr()));
	router_link.set_link_data(0xffffffff);
	router_link.set_metric(0);
	router_links.push_back(router_link);
	list<Neighbour<IPv4> *>::iterator n;
	for(n = _neighbours.begin(); n != _neighbours.end(); n++) {
	    if (Neighbour<IPv4>::Full == (*n)->get_state()) {
		router_link.set_type(RouterLink::p2p);
		router_link.set_link_id((*n)->get_router_id());
		router_link.
		    set_link_data(ntohl(get_interface_address().addr()));
		router_link.set_metric(_peerout.get_interface_cost());
		router_links.push_back(router_link);
	    }
	}
    }
	break;
    case OspfTypes::VirtualLink:
	// At the time of writing virtual links had only one neighbour.
	list<Neighbour<IPv4> *>::iterator n;
	for(n = _neighbours.begin(); n != _neighbours.end(); n++) {
	    if (Neighbour<IPv4>::Full == (*n)->get_state()) {
		router_link.set_type(RouterLink::vlink);
		router_link.set_link_id((*n)->get_router_id());
		router_link.
		    set_link_data(ntohl(get_interface_address().addr()));
		router_link.set_metric(_peerout.get_interface_cost());
		router_links.push_back(router_link);
	    }
	}
	break;
    }
}

template <>
void
Peer<IPv6>::update_router_linksV2(list<RouterLink>&)
{
    XLOG_FATAL("OSPFv2 can't carry IPv6 addresses");
}

template <>
void
Peer<IPv4>::update_router_linksV3(list<RouterLink>&)
{
    XLOG_FATAL("OSPFv3 can't carry IPv4 addresses yet!");
}

template <>
void
Peer<IPv6>::update_router_linksV3(list<RouterLink>& router_links)
{
    RouterLink router_link(OspfTypes::V3);

    switch(get_state()) {
    case Down:
    case Loopback:
	// No links
	return;
	break;
    case Waiting:
	break;
    case Point2Point:
	break;
    case DR_other:
	break;
    case Backup:
	break;
    case DR:
	break;
    }

    router_link.set_interface_id(get_interface_id());
    router_link.set_metric(_peerout.get_interface_cost());

    switch(get_linktype()) {
    case OspfTypes::PointToPoint: {
	// Find the neighbour. If the neighbour is fully adjacent then
	// configure a router link.
	list<Neighbour<IPv6> *>::iterator n = _neighbours.begin();
	if (n != _neighbours.end() &&
	    Neighbour<IPv6>::Full == (*n)->get_state()) {
	    router_link.set_type(RouterLink::p2p);
	    XLOG_ASSERT((*n)->get_hello_packet());
	    router_link.set_neighbour_interface_id((*n)->get_hello_packet()->
						   get_interface_id());
	    router_link.set_neighbour_router_id((*n)->get_router_id());
	    router_links.push_back(router_link);
	}
	
    }
	break;
    case OspfTypes::BROADCAST:
    case OspfTypes::NBMA: {
	bool adjacent = false;
	switch(get_state()) {
	case Down:
	    break;
	case Loopback:
	    break;
	case Waiting:
	    break;
	case Point2Point:
	    break;
	case Backup:	// XXX - Is this correct for the backup state?
	case DR_other: {
	    // Do we have an adjacency with the DR?
	    list<Neighbour<IPv6> *>::iterator n;
	    for(n = _neighbours.begin(); n != _neighbours.end(); n++)
		if (get_designated_router() == (*n)->get_candidate_id()) {
		    break;
		}
	    if (n == _neighbours.end())
		return;
	    if (Neighbour<IPv6>::Full == (*n)->get_state())
		adjacent = true;
	}
	    /* FALLTHROUGH */
	case DR: {
	    // Try and find any adjacency.
	    list<Neighbour<IPv6> *>::iterator n;
	    for(n = _neighbours.begin(); n != _neighbours.end(); n++)
		if (Neighbour<IPv6>::Full == (*n)->get_state()) {
		    adjacent = true;
		    break;
		}
	    if (adjacent) {
		router_link.set_type(RouterLink::transit);
		// DR interface ID.
		router_link.
		    set_neighbour_interface_id(
				       get_designated_router_interface_id());
		// DR router ID.
		router_link.set_neighbour_router_id(get_designated_router());

// 		get_area_router()->add_router_link(get_peerid(), router_link);
		router_links.push_back(router_link);
	    }
	}
	    break;
	}
    }
	
    	break;
    case OspfTypes::PointToMultiPoint: {
	list<Neighbour<IPv6> *>::iterator n;
	for(n = _neighbours.begin(); n != _neighbours.end(); n++) {
	    if (Neighbour<IPv6>::Full == (*n)->get_state()) {
		router_link.set_type(RouterLink::p2p);
		router_link.
		    set_neighbour_interface_id((*n)->get_hello_packet()->
					       get_interface_id());
		router_link.set_neighbour_router_id((*n)->get_router_id());
		router_links.push_back(router_link);
	    }
	}
    }
	break;
    case OspfTypes::VirtualLink:
	// At the time of writing virtual links had only one neighbour.
	list<Neighbour<IPv6> *>::iterator n;
	for(n = _neighbours.begin(); n != _neighbours.end(); n++) {
	    if (Neighbour<IPv6>::Full == (*n)->get_state()) {
		router_link.set_type(RouterLink::vlink);
		router_link.
		    set_neighbour_interface_id((*n)->get_hello_packet()->
					       get_interface_id());
		router_link.set_neighbour_router_id((*n)->get_router_id());
		router_links.push_back(router_link);
	    }
	}
	break;
    }
}

template <typename A>
void
Peer<A>::designated_router_changed(bool yes)
{
    list<RouterInfo> routers;

    get_attached_routers(routers);
    if (routers.empty())
	return;

    uint32_t network_mask = 0;
    uint32_t link_state_id = 0;

    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	network_mask = get_network_mask();
	link_state_id = get_candidate_id();
	break;
    case OspfTypes::V3:
	link_state_id = get_interface_id();
	break;
    }

    // Yipee we just became the DR.
    if (yes) {
	get_area_router()->generate_network_lsa(get_peerid(), link_state_id,
						routers, network_mask);
    } else {
	get_area_router()->withdraw_network_lsa(get_peerid(), link_state_id);
    }
}

template <typename A>
void
Peer<A>::adjacency_change(bool up)
{
    XLOG_ASSERT(do_dr_or_bdr());
    XLOG_ASSERT(is_DR());

    list<RouterInfo> routers;
    uint32_t network_mask = 0;
    uint32_t link_state_id = 0;

    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	network_mask = get_network_mask();
	link_state_id = get_candidate_id();
	break;
    case OspfTypes::V3:
	link_state_id = get_interface_id();
	break;
    }

    get_attached_routers(routers);

    if (up) {
	if (1 == routers.size()) {
	    get_area_router()->generate_network_lsa(get_peerid(),
						    link_state_id,
						    routers,
						    network_mask);
	} else {
	    get_area_router()->update_network_lsa(get_peerid(),
						  link_state_id,
						  routers,
						  network_mask);
	}
    } else {
	if (routers.empty()) {
	    get_area_router()->withdraw_network_lsa(get_peerid(),
						    link_state_id);
	} else {
	    get_area_router()->update_network_lsa(get_peerid(),
						  link_state_id,
						  routers,
						  network_mask);
	}
    }
}

template <typename A>
bool
Peer<A>::get_neighbour_list(list<OspfTypes::NeighbourID>& neighbours) const
{
    typename list<Neighbour<A> *>::const_iterator n;
    for(n = _neighbours.begin(); n != _neighbours.end(); n++)
	neighbours.push_back((*n)->get_neighbour_id());

    return true;
}

template <typename A>
bool
Peer<A>::get_neighbour_info(OspfTypes::NeighbourID nid, NeighbourInfo& ninfo)
    const
{
    typename list<Neighbour<A> *>::const_iterator n;
    for(n = _neighbours.begin(); n != _neighbours.end(); n++) {
	if ((*n)->get_neighbour_id() == nid) {
	    return (*n)->get_neighbour_info(ninfo);
	}
    }

    return false;
}

template <typename A>
set<AddressInfo<A> >&
Peer<A>::get_address_info()
{
    return _address_info;
}

template <typename A>
bool
Peer<A>::get_attached_routers(list<RouterInfo>& routers)
{
    typename list<Neighbour<A> *>::const_iterator n;
    for(n = _neighbours.begin(); n != _neighbours.end(); n++)
	if (Neighbour<A>::Full == (*n)->get_state()) {
	    switch(_ospf.get_version()) {
	    case OspfTypes::V2:
		routers.push_back(RouterInfo((*n)->get_router_id()));
		break;
	    case OspfTypes::V3:
		routers.push_back(RouterInfo((*n)->get_router_id(),
					     (*n)->get_hello_packet()->
					     get_interface_id()));
		break;
	    }
	}

    return true;
}

template <typename A>
void
Peer<A>::tear_down_state()
{
    stop_hello_timer();
    stop_wait_timer();
}

template <typename A>
void
Peer<A>::remove_neighbour_state()
{
    typename list<Neighbour<A> *>::iterator n = _neighbours.begin();
    while (n != _neighbours.end()) {
	(*n)->event_kill_neighbour();
	// The assumption here is that only a linktype of BROADCAST
	// can have been dynamically created. So its acceptable to
	// delete it.
	if (OspfTypes::BROADCAST == (*n)->get_linktype()) {
	    delete (*n);
	    _neighbours.erase(n++);
	} else {
	    n++;
	}
    }
    _scheduled_events.clear();
}

template <typename A>
string
Peer<A>::pp_interface_state(InterfaceState is)
{
    switch(is) {
    case Peer<A>::Down:
	return "Down";
    case Peer<A>::Loopback:
	return "Loopback";
    case Peer<A>::Waiting:
	return "Waiting";
    case Peer<A>::Point2Point:
	return "Point-to-point";
    case Peer<A>::DR_other:
	return "DR Other";
    case Peer<A>::Backup:
	return "Backup";
    case Peer<A>::DR:
	return "DR";
    }
    XLOG_UNREACHABLE();
}

template <typename A>
void
Peer<A>::router_id_changing()
{
    // RFC 2328 Section 12.4.2. Network-LSAs
    // The router ID is about to change so flush out any Network-LSAs
    // originated by this router.
    if (Peer<A>::DR == get_state()) {
	list<RouterInfo> routers;
	get_attached_routers(routers);
	if (routers.empty())
	    return;

	uint32_t link_state_id = 0;

	switch(_ospf.get_version()) {
	case OspfTypes::V2:
	    link_state_id = get_candidate_id();
	    break;
	case OspfTypes::V3:
	    link_state_id = get_interface_id();
	    break;
	}

	get_area_router()->withdraw_network_lsa(get_peerid(), link_state_id);
    }
}

template <typename A>
bool
Peer<A>::set_network_mask(uint32_t network_mask)
{
    _hello_packet.set_network_mask(network_mask);

    return true;
}

template <typename A>
uint32_t
Peer<A>::get_network_mask() const
{
    return _hello_packet.get_network_mask();
}

template <typename A>
bool
Peer<A>::set_interface_id(uint32_t interface_id)
{
    _hello_packet.set_interface_id(interface_id);

    return true;
}

template <typename A>
uint32_t
Peer<A>::get_interface_id() const
{
    return _hello_packet.get_interface_id();
}

template <>
bool
Peer<IPv4>::add_advertise_net(IPv4 /*addr*/, uint32_t /*prefix_length*/)
{
    XLOG_FATAL("Only IPv6 not IPv4");

    return true;
}

template <>
bool
Peer<IPv6>::add_advertise_net(IPv6 addr, uint32_t prefix_length)
{
    XLOG_ASSERT(OspfTypes::VirtualLink != get_linktype());

    LinkLsa *llsa = dynamic_cast<LinkLsa *>(_link_lsa.get());
    XLOG_ASSERT(llsa);

    if (addr.is_linklocal_unicast())
	return false;

    IPv6Prefix prefix(_ospf.get_version());
    prefix.set_network(IPNet<IPv6>(addr, prefix_length));
    llsa->get_prefixes().push_back(prefix);

    // Add a host route that can be used if necessary to advertise a
    // virtual link endpoint.
    IPv6Prefix host_prefix(_ospf.get_version());
    host_prefix.set_network(IPNet<IPv6>(addr, IPv6::ADDR_BITLEN));
    host_prefix.set_la_bit(true);
    llsa->get_prefixes().push_back(host_prefix);

    return true;
}

template <>
bool
Peer<IPv4>::remove_all_nets()
{
    XLOG_FATAL("Only IPv6 not IPv4");

    return true;
}

template <>
bool
Peer<IPv6>::remove_all_nets()
{
    XLOG_ASSERT(OspfTypes::VirtualLink != get_linktype());

    LinkLsa *llsa = dynamic_cast<LinkLsa *>(_link_lsa.get());
    XLOG_ASSERT(llsa);

    llsa->get_prefixes().clear();

    return true;
}

template <>
bool
Peer<IPv4>::update_nets()
{
    XLOG_FATAL("Only IPv6 not IPv4");

    return true;
}

template <>
bool
Peer<IPv6>::update_nets()
{
    bool status = get_area_router()->update_link_lsa(get_peerid(), _link_lsa);
    if (do_dr_or_bdr() && is_DR())
	get_area_router()->update_intra_area_prefix_lsa(get_peerid());

    return status;
}

template <typename A>
bool
Peer<A>::set_hello_interval(uint16_t hello_interval)
{
    _hello_packet.set_hello_interval(hello_interval);

    restart_hello_timer();

    return true;
}

template <typename A>
bool
Peer<A>::set_options(uint32_t options)
{
    _hello_packet.set_options(options);

    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	if (OspfTypes::VirtualLink != get_linktype()) {
	    LinkLsa *llsa = dynamic_cast<LinkLsa *>(_link_lsa.get());
	    XLOG_ASSERT(llsa);
	    llsa->set_options(options);
	    get_area_router()->update_link_lsa(get_peerid(), _link_lsa);
	}
	break;
    }

    return true;
}

template <typename A>
uint32_t
Peer<A>::get_options() const
{
    return _hello_packet.get_options();
}

template <typename A>
bool
Peer<A>::set_router_priority(uint8_t priority)
{
    _hello_packet.set_router_priority(priority);

    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	if (OspfTypes::VirtualLink != get_linktype()) {
	    LinkLsa *llsa = dynamic_cast<LinkLsa *>(_link_lsa.get());
	    XLOG_ASSERT(llsa);
	    llsa->set_rtr_priority(priority);
	    get_area_router()->update_link_lsa(get_peerid(), _link_lsa);
	}
	break;
    }

    switch(get_state()) {
    case Down:
    case Loopback:
    case Waiting:
    case Point2Point:
	break;
    case DR_other:
    case Backup:
    case DR:
	compute_designated_router_and_backup_designated_router();
	break;
    }

    return true;
}

template <typename A>
bool
Peer<A>::set_router_dead_interval(uint32_t router_dead_interval)
{
    _hello_packet.set_router_dead_interval(router_dead_interval);

    return true;
}

template <typename A>
uint32_t
Peer<A>::get_router_dead_interval() const
{
    return _hello_packet.get_router_dead_interval();
}

template <typename A>
bool
Peer<A>::set_simple_authentication_key(const string&	password,
				       string&		error_msg)
{
    return get_auth_handler().set_simple_authentication_key(password,
							    error_msg);
}

template <typename A>
bool
Peer<A>::delete_simple_authentication_key(string&	error_msg)
{
    return get_auth_handler().delete_simple_authentication_key(error_msg);
}

template <typename A>
bool
Peer<A>::set_md5_authentication_key(uint8_t		key_id,
				    const string&	password,
				    const TimeVal&	start_timeval,
				    const TimeVal&	end_timeval,
				    const TimeVal&	max_time_drift,
				    string&		error_msg)
{
    return get_auth_handler().set_md5_authentication_key(key_id, password,
							 start_timeval,
							 end_timeval,
							 max_time_drift,
							 error_msg);
}

template <typename A>
bool
Peer<A>::delete_md5_authentication_key(uint8_t		key_id,
				       string&		error_msg)
{
    return get_auth_handler().delete_md5_authentication_key(key_id, error_msg);
}

template <typename A>
bool
Peer<A>::set_passive(bool passive, bool host)
{
    if (_passive == passive && _passive_host == host)
	return true;

    bool change_state = !(_passive == passive);

    _passive = passive;
    _passive_host = host;
    if (!_enabled)
	return true;

    if (change_state) {
	if (passive) {
	    event_loop_ind();
	} else {
	    event_unloop_ind();	
	    event_interface_up();
	}
    } else {
	update_router_links();
    }

    return true;
}

template <typename A>
bool
Peer<A>::get_passive() const
{
    return _passive;
}

template <typename A>
bool
Peer<A>::set_rxmt_interval(uint32_t rxmt_interval)
{
    _rxmt_interval = rxmt_interval;

    return true;
}

template <typename A>
uint32_t
Peer<A>::get_rxmt_interval()
{
    return _rxmt_interval;
}

template <typename A>
void
Peer<A>::change_state(InterfaceState state)
{
    InterfaceState previous_state = get_state();
    set_state(state);

    if (previous_state == state)
	return;

    if (Peer<A>::DR == state)
	designated_router_changed(true);
    if (Peer<A>::DR == previous_state)
	designated_router_changed(false);

    bool was_dr_or_bdr = Peer<A>::DR == previous_state || 
	Peer<A>::Backup == previous_state;
    bool is_dr_or_bdr = Peer<A>::DR == state || Peer<A>::Backup == state;
    if (is_dr_or_bdr != was_dr_or_bdr) {
	if (is_dr_or_bdr) {
	    _peerout.join_multicast_group(A::OSPFIGP_DESIGNATED_ROUTERS());
	} else {
	    _peerout.leave_multicast_group(A::OSPFIGP_DESIGNATED_ROUTERS());
	}
    }
}

template <typename A>
bool
Peer<A>::set_designated_router(OspfTypes::RouterID dr)
{
    _hello_packet.set_designated_router(dr);

    return true;
}

template <typename A>
OspfTypes::RouterID
Peer<A>::get_designated_router() const
{
    XLOG_ASSERT(do_dr_or_bdr());

    return _hello_packet.get_designated_router();
}

template <typename A>
bool
Peer<A>::set_backup_designated_router(OspfTypes::RouterID dr)
{
    _hello_packet.set_backup_designated_router(dr);

    return true;
}

template <typename A>
OspfTypes::RouterID
Peer<A>::get_backup_designated_router() const
{
    XLOG_ASSERT(do_dr_or_bdr());

    return _hello_packet.get_backup_designated_router();
}

/****************************************/
/**
 * Initialise the static variables that are used to generate unique
 * NeighbourIDs.
 */
template <> OspfTypes::NeighbourID Neighbour<IPv4>::_ticket = 
    OspfTypes::ALLNEIGHBOURS + 1;
template <> OspfTypes::NeighbourID Neighbour<IPv6>::_ticket =
    OspfTypes::ALLNEIGHBOURS + 1;

template <typename A>
string
Neighbour<A>::pp_state(State ns)
{
    switch(ns) {
    case Neighbour<A>::Down:
	return "Down";
    case Neighbour<A>::Attempt:
	return "Attempt";
    case Neighbour<A>::Init:
	return "Init";
    case Neighbour<A>::TwoWay:
	return "TwoWay";
    case Neighbour<A>::ExStart:
	return "ExStart";
    case Neighbour<A>::Exchange:
	return "Exchange";
    case Neighbour<A>::Loading:
	return "Loading";
    case Neighbour<A>::Full:
	return "Full";
    }
    XLOG_UNREACHABLE();
}

/**
 * RFC 2328 Section 10.4. Whether to become adjacent
 */
template <typename A>
bool
Neighbour<A>::establish_adjacency_p() const
{
    bool become_adjacent = false;

    switch(get_linktype()) {
    case OspfTypes::BROADCAST:
    case OspfTypes::NBMA:
	// The router itself is the Designated Router
	// The router itself is the Backup Designated Router
	// The neighboring router is the Designated Router
	// The neighboring router is the Backup Designated Router
	if (is_DR_or_BDR() || is_neighbour_DR_or_BDR())
	    become_adjacent = true;
	break;
    case OspfTypes::PointToPoint:
    case OspfTypes::PointToMultiPoint:
    case OspfTypes::VirtualLink:
	become_adjacent = true;
	break;
    }

    return become_adjacent;
}

template <typename A>
bool
Neighbour<A>::is_DR() const
{
    XLOG_ASSERT(_peer.do_dr_or_bdr());

    return _peer.is_DR();
}

template <typename A>
bool
Neighbour<A>::is_BDR() const
{
    XLOG_ASSERT(_peer.do_dr_or_bdr());

    return _peer.is_BDR();
}

template <typename A>
bool
Neighbour<A>::is_DR_or_BDR() const
{
    XLOG_ASSERT(_peer.do_dr_or_bdr());

    return _peer.is_DR_or_BDR();
}

template <typename A>
bool
Neighbour<A>::is_neighbour_DR() const
{
    XLOG_ASSERT(_peer.do_dr_or_bdr());

    if (get_candidate_id() == _peer.get_designated_router())
	return true;

    return false;
}

template <typename A>
bool
Neighbour<A>::is_neighbour_DR_or_BDR() const
{
    XLOG_ASSERT(_peer.do_dr_or_bdr());

    if (get_candidate_id() == _peer.get_designated_router())
	return true;

    if (get_candidate_id() == _peer.get_backup_designated_router())
	return true;

    return false;
}

template <typename A>
void
Neighbour<A>::start_inactivity_timer()
{
    _inactivity_timer = _ospf.get_eventloop().
	new_oneoff_after(TimeVal(_peer.get_router_dead_interval(), 0),
			 callback(this, &Neighbour::event_inactivity_timer));
}

template <typename A>
void
Neighbour<A>::stop_inactivity_timer()
{
    _inactivity_timer.unschedule();
}

/**
 * Simple wrapper for retransmission timers that simplified debugging.
 */
class RxmtWrapper {
 public:
    RxmtWrapper() {}
    RxmtWrapper(Neighbour<IPv4>::RxmtCallback rcb, const char *diagnostic)
	: _rcb(rcb), _diagnostic(diagnostic)
    {}

    RxmtWrapper(const RxmtWrapper& rhs) {
	_rcb = rhs._rcb;
	_diagnostic = rhs._diagnostic;
    }
	
    RxmtWrapper operator=(const RxmtWrapper& rhs) {
	if(&rhs == this)
	    return *this;
	_rcb = rhs._rcb;
	_diagnostic = rhs._diagnostic;
	return *this;
    }

    bool doit() {
	debug_msg("retransmission: %s\n", str().c_str());
	return  _rcb->dispatch();

    }

    string str() {
	return _diagnostic;
    }

 private:
    Neighbour<IPv4>::RxmtCallback _rcb;
    string _diagnostic;
};

template <typename A>
void
Neighbour<A>::start_rxmt_timer(uint32_t index, RxmtCallback rcb,
			       bool immediate, 
			       const char *comment)
{
    debug_msg("start_rxmt_timer: %p %s %s\n", this, 
	      _peer.get_if_name().c_str(),
	      comment);
    XLOG_ASSERT(index < TIMERS);

    // Any outstanding timers should already have been cancelled.
    XLOG_ASSERT(0 == _rxmt_wrapper[index]);

    _rxmt_wrapper[index] = new RxmtWrapper(rcb, 
				    c_format("%s %s",
					     _peer.get_if_name().c_str(),
					     comment).c_str());

    _rxmt_timer[index] = _ospf.get_eventloop().
 	new_periodic_ms(_peer.get_rxmt_interval() * 1000,
			callback(_rxmt_wrapper[index], &RxmtWrapper::doit));

    // Send one immediately. Do this last so all state is set.
    if (immediate)
	rcb->dispatch();
}

template <typename A>
void
Neighbour<A>::stop_rxmt_timer(uint32_t index, const char *comment)
{
    debug_msg("stop_rxmt_timer: %p %s %s\n", this,
	      _peer.get_if_name().c_str(), comment);
    XLOG_ASSERT(index < TIMERS);
    if (_rxmt_wrapper[index]) {
	debug_msg("\tstopping %s\n", cstring(*_rxmt_wrapper[index]));
	delete _rxmt_wrapper[index];
	_rxmt_wrapper[index] = 0;
    }

    _rxmt_timer[index].unschedule();
}

template <typename A>
void
Neighbour<A>::restart_retransmitter()
{
    if (_rxmt_wrapper[FULL])
	stop_rxmt_timer(FULL, "restart retransmitter");

    start_rxmt_timer(FULL, callback(this, &Neighbour<A>::retransmitter),
		     false,
		     "restart retransmitter");
}

// template <typename A>
// void
// Neighbour<A>::stop_retransmitter()
// {
//     stop_rxmt_timer("stop retransmitter");
// }

template <typename A>
bool
Neighbour<A>::retransmitter()
{
    // When there is nothing left to retransmit stop the timer 
    bool more = false;

    if (!_ls_request_list.empty()) {

	LinkStateRequestPacket lsrp(_ospf.get_version());

	size_t lsr_len = 0;
	list<Lsa_header>::iterator i;
	for (i = _ls_request_list.begin(); i != _ls_request_list.end(); i++) {
	    if (lsrp.get_standard_header_length() +
		Ls_request::length() + lsr_len < 
		_peer.get_frame_size()) {
		lsr_len += Ls_request::length();
		lsrp.get_ls_request().
		    push_back(Ls_request(i->get_version(),
					 i->get_ls_type(),
					 i->get_link_state_id(),
					 i->get_advertising_router()));
	    } else {
		send_link_state_request_packet(lsrp);
		lsrp.get_ls_request().clear();
		lsr_len = 0;
		// RFC 2328 Section 10.9
		// There should be at most one Link State Request packet
		// outstanding at any one time.
		break;
	    }
	}

	if (!lsrp.get_ls_request().empty())
	    send_link_state_request_packet(lsrp);
	
	more = true;
    }

    if (!_lsa_rxmt.empty()) {
	TimeVal now;
	_ospf.get_eventloop().current_time(now);

	LinkStateUpdatePacket lsup(_ospf.get_version(),
				   _ospf.get_lsa_decoder());
	size_t lsas_len = 0;
	list<Lsa::LsaRef>::iterator i = _lsa_rxmt.begin();
	while (i != _lsa_rxmt.end()) {
	    if ((*i)->valid() && (*i)->exists_nack(_neighbourid)) {
		if (!(*i)->maxage())
		    (*i)->update_age(now);
		size_t len;
		(*i)->lsa(len);
		if (lsup.get_standard_header_length() + len + lsas_len < 
		    _peer.get_frame_size()) {
		    lsas_len += len;
		    lsup.get_lsas().push_back(*i);
		} else {
		    XLOG_TRACE(_ospf.trace()._retransmit,
			       "retransmit: %s %s", str().c_str(),
			       cstring(lsup));
		    send_link_state_update_packet(lsup, true /* direct */);
		    lsup.get_lsas().clear();
		    lsas_len = 0;
		}
		i++;
	    } else {
		_lsa_rxmt.erase(i++);
	    }
	}

	if (!lsup.get_lsas().empty()) {
	    XLOG_TRACE(_ospf.trace()._retransmit,
		       "retransmit: %s %s", str().c_str(), cstring(lsup));
	    send_link_state_update_packet(lsup, true /* direct */);
	}

	more = true;
    }

    return more;
}

template <typename A>
void
Neighbour<A>::build_data_description_packet()
{
    // Clear out previous LSA headers.
    _data_description_packet.get_lsa_headers().clear();

    if (_all_headers_sent)
	return;

    // Open the database if the handle is invalid.
    if (!_database_handle.valid()) {
	bool empty;
	_database_handle = get_area_router()->open_database(_peer.get_peerid(),
							    empty);
	if (empty)
	    goto out;
    } else {
	// Make sure there are LSAs left in the database.
	if (!get_area_router()->subsequent(_database_handle))
	    goto out;
    }

    bool last;
    do {
	Lsa::LsaRef lsa = get_area_router()->
	    get_entry_database(_database_handle, last);

	// Don't summarize AS-External-LSAs over virtual adjacencies.
	if (!(OspfTypes::VirtualLink == get_linktype() && lsa->external())) {
	    _data_description_packet.get_lsa_headers().
		push_back(lsa->get_header());

	    // XXX - We are testing to see if there is space left in the
	    // packet by repeatedly encoding, this is very inefficient. We
	    // should encode to find the base size of the packet and then
	    // compare against the constant LSA header length each time.
	    vector<uint8_t> pkt;
	    _data_description_packet.encode(pkt);
	    if (pkt.size() + Lsa_header::length() >= _peer.get_frame_size())
		return;
	}
    } while(last == false);

 out:
    // No more headers.
    _data_description_packet.set_m_bit(false);
    
    get_area_router()->close_database(_database_handle);

    _all_headers_sent = true;
}

template <typename A>
bool
Neighbour<A>::send_data_description_packet()
{
    _peer.populate_common_header(_data_description_packet);
    switch(get_linktype()) {
    case OspfTypes::PointToPoint:
    case OspfTypes::BROADCAST:
    case OspfTypes::NBMA:
    case OspfTypes::PointToMultiPoint:
	_data_description_packet.set_interface_mtu(_peer.get_interface_mtu());
	break;
    case OspfTypes::VirtualLink:
	_data_description_packet.set_interface_mtu(0);
	break;
    }
    _data_description_packet.set_options(_peer.get_options());
    
    vector<uint8_t> pkt;
    _data_description_packet.encode(pkt);
    get_auth_handler().generate(pkt);

    SimpleTransmit<A> *transmit = 0;

    switch(get_linktype()) {
    case OspfTypes::PointToPoint:
	transmit = new SimpleTransmit<A>(pkt,
					 A::OSPFIGP_ROUTERS(), 
					 _peer.get_interface_address());
	break;
    case OspfTypes::BROADCAST:
    case OspfTypes::NBMA:
    case OspfTypes::PointToMultiPoint:
	transmit = new SimpleTransmit<A>(pkt,
					 get_neighbour_address(),
					 _peer.get_interface_address());
	break;
    case OspfTypes::VirtualLink:
	transmit = new SimpleTransmit<A>(pkt,
					 get_neighbour_address(),
					 _peer.get_interface_address());
	break;
    }

    typename Transmit<A>::TransmitRef tr(transmit);

    _peer.transmit(tr);
    
    return true;
}

/**
 * RFC 2328 Section 10.8. Sending Database Description Packets. (ExStart)
 */
template <typename A>
void
Neighbour<A>::start_sending_data_description_packets(const char *event_name,
						     bool immediate)
{
    XLOG_ASSERT(ExStart == get_state());

    // Clear out the request list.
    _ls_request_list.clear();

    uint32_t seqno = _data_description_packet.get_dd_seqno();
    _data_description_packet.set_dd_seqno(++seqno);
    _data_description_packet.set_i_bit(true);
    _data_description_packet.set_m_bit(true);
    _data_description_packet.set_ms_bit(true);
    _data_description_packet.get_lsa_headers().clear();

    start_rxmt_timer(INITIAL,
		     callback(this,
			      &Neighbour<A>::send_data_description_packet),
		     immediate,
		     c_format("send_data_description from %s",
			      event_name).c_str());
}

template <typename A>
bool
Neighbour<A>::send_link_state_request_packet(LinkStateRequestPacket& lsrp)
{
    _peer.populate_common_header(lsrp);
    
    vector<uint8_t> pkt;
    lsrp.encode(pkt);
    get_auth_handler().generate(pkt);

    SimpleTransmit<A> *transmit = 0;

    switch(get_linktype()) {
    case OspfTypes::PointToPoint:
	transmit = new SimpleTransmit<A>(pkt,
					 A::OSPFIGP_ROUTERS(), 
					 _peer.get_interface_address());
	break;
    case OspfTypes::BROADCAST:
    case OspfTypes::NBMA:
    case OspfTypes::PointToMultiPoint:
    case OspfTypes::VirtualLink:
	transmit = new SimpleTransmit<A>(pkt,
					 get_neighbour_address(),
					 _peer.get_interface_address());
	break;
    }

    typename Transmit<A>::TransmitRef tr(transmit);

    _peer.transmit(tr);
    
    return true;
}

template <typename A>
bool
Neighbour<A>::send_link_state_update_packet(LinkStateUpdatePacket& lsup, 
					    bool direct)
{
    _peer.populate_common_header(lsup);
    
    vector<uint8_t> pkt;
    lsup.encode(pkt, _peer.get_inftransdelay());
    get_auth_handler().generate(pkt);

    SimpleTransmit<A> *transmit = 0;

    switch(get_linktype()) {
    case OspfTypes::PointToPoint:
	transmit = new SimpleTransmit<A>(pkt,
					 A::OSPFIGP_ROUTERS(), 
					 _peer.get_interface_address());
	break;
    case OspfTypes::BROADCAST: {
	A dest;
	if (direct) {
	    dest = get_neighbour_address();
	} else {
	    if (is_DR_or_BDR()) {
		dest = A::OSPFIGP_ROUTERS();
	    } else {
		dest = A::OSPFIGP_DESIGNATED_ROUTERS();
	    }
	}
	transmit = new SimpleTransmit<A>(pkt,
					 dest, 
					 _peer.get_interface_address());
    }
	break;
    case OspfTypes::NBMA:
    case OspfTypes::PointToMultiPoint:
    case OspfTypes::VirtualLink:
	transmit = new SimpleTransmit<A>(pkt,
					 get_neighbour_address(),
					 _peer.get_interface_address());
	break;
    }

    typename Transmit<A>::TransmitRef tr(transmit);

    _peer.transmit(tr);
    
    return true;
}

template <typename A>
bool
Neighbour<A>::send_link_state_ack_packet(LinkStateAcknowledgementPacket& lsap,
					 bool direct,
					 bool& multicast_on_peer)
{
    _peer.populate_common_header(lsap);
    
    vector<uint8_t> pkt;
    lsap.encode(pkt);
    get_auth_handler().generate(pkt);

    SimpleTransmit<A> *transmit = 0;

    multicast_on_peer = false;

    switch(get_linktype()) {
    case OspfTypes::PointToPoint:
	transmit = new SimpleTransmit<A>(pkt,
					 A::OSPFIGP_ROUTERS(), 
					 _peer.get_interface_address());
	break;
    case OspfTypes::BROADCAST: {
	A dest;
	if (direct) {
	    dest = get_neighbour_address();
	} else {
	    multicast_on_peer = true;
	    if (is_DR_or_BDR()) {
		dest = A::OSPFIGP_ROUTERS();
	    } else {
		dest = A::OSPFIGP_DESIGNATED_ROUTERS();
	    }
	}
	transmit = new SimpleTransmit<A>(pkt,
					 dest, 
					 _peer.get_interface_address());
    }
	break;
    case OspfTypes::NBMA:
    case OspfTypes::PointToMultiPoint:
    case OspfTypes::VirtualLink:
	transmit = new SimpleTransmit<A>(pkt,
					 get_neighbour_address(),
					 _peer.get_interface_address());
	break;
    }

    typename Transmit<A>::TransmitRef tr(transmit);

    _peer.transmit(tr);
    
    return true;
}

template <typename A>
void
Neighbour<A>::tear_down_state(State previous_state)
{
    stop_inactivity_timer();
    for (uint32_t i = 0; i < TIMERS; i++)
	stop_rxmt_timer(i, "Tear Down State");
    _all_headers_sent = false;
    if (_database_handle.valid())
	get_area_router()->close_database(_database_handle);
    _ls_request_list.clear();
    // If this assertion ever occurs then the nack list on the LSAs on
    // the queue must be cleared. The top of push_lsas shows how this
    // can be done. If the current state is less than exchange just
    // calling push_lsas will do the trick.
    XLOG_ASSERT(_lsa_queue.empty());

    list<Lsa::LsaRef>::iterator i = _lsa_rxmt.begin();
    for (i = _lsa_rxmt.begin(); i != _lsa_rxmt.end(); i++)
	(*i)->remove_nack(_neighbourid);
    _lsa_rxmt.clear();

    if (_peer.do_dr_or_bdr() && is_DR() && Full == previous_state)
	_peer.adjacency_change(false);

    if (TwoWay <= previous_state) {
	if (_peer.do_dr_or_bdr())
	    _peer.schedule_event("NeighbourChange");
	else
	    _peer.update_router_links();
    }
}

/**
 * RFC 2328 Section 10.5 Receiving Hello Packets, neighbour component.
 */
template <typename A>
void
Neighbour<A>::event_hello_received(HelloPacket *hello)
{
    const char *event_name = "HelloReceived";
    XLOG_TRACE(_ospf.trace()._neighbour_events, 
	       "Event(%s) Interface(%s) Neighbour(%s) DR (%s) BDR (%s) "
	       "State(%s)",
	       event_name,
	       _peer.get_if_name().c_str(),
	       pr_id(get_candidate_id()).c_str(),
	       pr_id(hello->get_designated_router()).c_str(),
	       pr_id(hello->get_backup_designated_router()).c_str(),
	       pp_state(get_state()).c_str());

    debug_msg("ID = %s interface state <%s> neighbour state <%s> %s\n",
	      pr_id(get_candidate_id()).c_str(),
	      Peer<A>::pp_interface_state(_peer.get_state()).c_str(),
	      pp_state(get_state()).c_str(),
	      cstring(*hello));

    switch(get_state()) {
    case Down:
	// It is legal to delete 0!
	delete _hello_packet;
	_hello_packet = 0;
	change_state(Init);
	break;
    case Attempt:
    case Init:
    case TwoWay:
    case ExStart:
    case Exchange:
    case Loading:
    case Full:
	break;
    }

    bool first = 0 ==_hello_packet;
    uint8_t previous_router_priority = 0;
    OspfTypes::RouterID previous_dr = 0;
    OspfTypes::RouterID previous_bdr = 0;
    if (first) {
	XLOG_ASSERT(!_inactivity_timer.scheduled());
	if (_peer.do_dr_or_bdr()) {
	    previous_router_priority = hello->get_router_priority();
	    previous_dr = hello->get_designated_router();
	    previous_bdr = hello->get_backup_designated_router();
	}
    } else {
	if (hello->get_router_id() != _hello_packet->get_router_id()) {
	    XLOG_INFO("Router ID changed from %s to %s",
		      pr_id(_hello_packet->get_router_id()).c_str(),
		      pr_id(hello->get_router_id()).c_str());
	}
	if (_peer.do_dr_or_bdr()) {
	    previous_router_priority = _hello_packet->get_router_priority();
	    previous_dr = _hello_packet->get_designated_router();
	    previous_bdr = _hello_packet->get_backup_designated_router();
	}
	delete _hello_packet;
    }
    _hello_packet = hello;

    start_inactivity_timer();

    // Search for this router in the neighbour list.
    list<OspfTypes::RouterID> li = hello->get_neighbours();
    list<OspfTypes::RouterID>::iterator i = li.begin();
    for(; i != li.end(); i++) {
	if ((*i) == _ospf.get_router_id())
	    break;
    }

    if (i == li.end()) {
	// This neighbour has no knowledge of this router.
	event_1_way_received();
	return;
    }

    event_2_way_received();

    // Don't attempt to compute the DR or BDR if this is not BROADCAST or NBMA.
    if (!_peer.do_dr_or_bdr())
	return;

    if (previous_router_priority != hello->get_router_priority())
	_peer.schedule_event("NeighbourChange");

    // Everything below here it trying to figure out if a
    // "NeighbourChange" event should be scheduled. Which is not
    // allowed in state Waiting.
    if (Peer<A>::Waiting == _peer.get_state()) {
	if ((get_candidate_id() == hello->get_designated_router() &&
	    set_id("0.0.0.0") == hello->get_backup_designated_router()) ||
	    get_candidate_id() == hello->get_backup_designated_router()) {
	    _peer.schedule_event("BackupSeen");
	}
    }

    // If the neighbour is declaring itself the designated router and
    // it had not previously.
    if ((get_candidate_id() == hello->get_designated_router() &&
	 previous_dr != hello->get_designated_router())) {
	_peer.schedule_event("NeighbourChange");
    }

    // If the neighbour is not declaring itself the designated router and
    // it had previously.
    if ((get_candidate_id() == previous_dr &&
	 previous_dr != hello->get_designated_router())) {
	_peer.schedule_event("NeighbourChange");
    }

    // If the neighbour is declaring itself the designated backup router and
    // it had not previously.
    if ((get_candidate_id() == hello->get_backup_designated_router() &&
	 previous_bdr != hello->get_backup_designated_router())) {
	_peer.schedule_event("NeighbourChange");
    }

    // If the neighbour is not declaring itself the designated backup
    // router and it had previously.
    if ((get_candidate_id() == previous_bdr &&
	 previous_bdr != hello->get_backup_designated_router())) {
	_peer.schedule_event("NeighbourChange");
    }

    if (OspfTypes::NBMA == get_linktype())
	XLOG_WARNING("TBD");
}

template <typename A>
void
Neighbour<A>::event_1_way_received()
{
    const char *event_name = "1-WayReceived";
    XLOG_TRACE(_ospf.trace()._neighbour_events, 
	       "Event(%s) Interface(%s) Neighbour(%s) State(%s)",
	       event_name,
	       _peer.get_if_name().c_str(),
	       pr_id(get_candidate_id()).c_str(),
	       pp_state(get_state()).c_str());

    switch(get_state()) {
    case Down:
    case Attempt:
	XLOG_WARNING("Unexpected state %s", pp_state(get_state()).c_str());
	break;
    case Init:
	// No change
	break;
    case TwoWay:
    case ExStart:
    case Exchange:
    case Loading:
    case Full:
	change_state(Init);
	break;
    }
}

template <typename A>
void
Neighbour<A>::event_2_way_received()
{
    const char *event_name = "2-WayReceived";
    XLOG_TRACE(_ospf.trace()._neighbour_events, 
	       "Event(%s) Interface(%s) Neighbour(%s) State(%s)",
	       event_name,
	       _peer.get_if_name().c_str(),
	       pr_id(get_candidate_id()).c_str(),
	       pp_state(get_state()).c_str());

    switch(get_state()) {
    case Down:
	XLOG_WARNING("Unhandled state %s", pp_state(get_state()).c_str());
	break;
    case Attempt:
	XLOG_ASSERT(get_linktype() == OspfTypes::NBMA);
	break;
    case Init:
	if (establish_adjacency_p()) {
	    change_state(ExStart);

	    start_sending_data_description_packets(event_name);
	} else {
	    change_state(TwoWay);
	}
	if (_peer.do_dr_or_bdr())
	    _peer.schedule_event("NeighbourChange");
	break;
    case TwoWay:
    case ExStart:
    case Exchange:
    case Loading:
    case Full:
	// Cool nothing to do.
	break;
    }
}

/**
 * Save specific fields (not all) in Database Description Packets:
 * initialize(I), more(M), master(MS), options, DD sequence number.
 * Please turn this into "operator=".
 */
inline
void
assign(DataDescriptionPacket& lhs, const DataDescriptionPacket& rhs)
{
    if (&lhs == &rhs)
	return;

    lhs.set_i_bit(rhs.get_i_bit());
    lhs.set_m_bit(rhs.get_m_bit());
    lhs.set_ms_bit(rhs.get_ms_bit());
    lhs.set_options(rhs.get_options());
    lhs.set_dd_seqno(rhs.get_dd_seqno());
}

/**
 * Compare specific fields (not all) in Database Description Packets:
 * initialize(I), more(M), master(MS), options, DD sequence number.
 */
inline
bool
operator==(const DataDescriptionPacket& lhs, const DataDescriptionPacket& rhs)
{
    if (&lhs == &rhs)
	return true;

    if (lhs.get_i_bit() != rhs.get_i_bit())
	return false;

    if (lhs.get_m_bit() != rhs.get_m_bit())
	return false;

    if (lhs.get_ms_bit() != rhs.get_ms_bit())
	return false;

    if (lhs.get_options() != rhs.get_options())
	return false;

    if (lhs.get_dd_seqno() != rhs.get_dd_seqno())
	return false;

    return true;
}

/**
 * RFC 2328 Section 10.6 Receiving Database Description Packets,
 * neighbour component.
 */
template <typename A>
void
Neighbour<A>::data_description_received(DataDescriptionPacket *dd)
{
    const char *event_name = "DataDescriptionReceived-pseudo-event";
    XLOG_TRACE(_ospf.trace()._neighbour_events, 
	       "Event(%s) Interface(%s) Neighbour(%s) State(%s)",
	       event_name,
	       _peer.get_if_name().c_str(),
	       pr_id(get_candidate_id()).c_str(),
	       pp_state(get_state()).c_str());

    debug_msg("ID = %s interface state <%s> neighbour state <%s> %s\n",
	      pr_id(get_candidate_id()).c_str(),
	      Peer<A>::pp_interface_state(_peer.get_state()).c_str(),
	      pp_state(get_state()).c_str(),
	      cstring(*dd));

    switch(get_state()) {
    case Down:
	// Reject Packet
	break;
    case Attempt:
	// Reject Packet
	break;
    case Init:
	event_2_way_received();
	if (ExStart == get_state())
	    ;// FALLTHROUGH
	else
	    break;
    case ExStart: {
	bool negotiation_done = false;

	// Save some fields for later duplicate detection.
	assign(_last_dd, *dd);

	_all_headers_sent = false;

	if (dd->get_i_bit() && dd->get_m_bit() && dd->get_ms_bit() && 
	    dd->get_lsa_headers().empty() && 
	    dd->get_router_id() > _ospf.get_router_id()) { // Router is slave
	    // Use this sequence number to respond to the poll
	    _data_description_packet.set_dd_seqno(dd->get_dd_seqno());
	    // This is the slave switch off the master slave bit.
	    _data_description_packet.set_ms_bit(false);
	    negotiation_done = true;
	}

	if (!dd->get_i_bit() && !dd->get_ms_bit() &&
	    _data_description_packet.get_dd_seqno() == dd->get_dd_seqno() && 
	    dd->get_router_id() < _ospf.get_router_id()) {  // Router is master
	    // Bump the sequence number of the master.
	    _data_description_packet.
		set_dd_seqno(_data_description_packet.get_dd_seqno() + 1);
	    if (!extract_lsa_headers(dd))
		return;
	    negotiation_done = true;
	}

	if (negotiation_done) {
	    event_negotiation_done();
	}
    }
	break;
    case TwoWay:
	// Ignore Packet
	break;
    case Exchange: {
	// Check for duplicates
	if (_last_dd == *dd) {
	    if (_last_dd.get_ms_bit()) {// Router is slave
		send_data_description_packet();
	    } else {			// Router is master
		// Discard
	    }
	    return;
	}

	// Make sure the saved value is the same as the incoming.
	if (_last_dd.get_ms_bit() != dd->get_ms_bit()) {
	    XLOG_TRACE(_ospf.trace()._neighbour_events,
		       "Neighbour(%s) sequence mismatch: "
		       "MS expected %s got %s",
		       pr_id(get_candidate_id()).c_str(),
		       bool_c_str(_last_dd.get_ms_bit()),
		       bool_c_str(dd->get_ms_bit()));
	    event_sequence_number_mismatch();
	    break;
	}
	
	if (dd->get_i_bit())  {
	    XLOG_TRACE(_ospf.trace()._neighbour_events,
		       "Neighbour(%s) sequence mismatch: I-Bit set",
		       pr_id(get_candidate_id()).c_str());
	    event_sequence_number_mismatch();
	    break;
	}

	if (dd->get_options() != _last_dd.get_options())  {
	    XLOG_TRACE(_ospf.trace()._neighbour_events,
		       "Neighbour(%s) sequence mismatch: (options)",
		       pr_id(get_candidate_id()).c_str());
	    event_sequence_number_mismatch();
	    break;
	}
	
	bool in_sequence = false;
	if (!_data_description_packet.get_ms_bit()) { // Router is slave
	    if (_data_description_packet.get_dd_seqno() + 1 == 
		dd->get_dd_seqno())
		in_sequence = true;
	} else {		     // Router is master
	    if (_data_description_packet.get_dd_seqno() == dd->get_dd_seqno())
		in_sequence = true;
	}

	if (!in_sequence)  {
	    XLOG_TRACE(_ospf.trace()._neighbour_events,
		       "Neighbour(%s) sequence mismatch: Out of sequence",
		       pr_id(get_candidate_id()).c_str());
	    event_sequence_number_mismatch();
	    break;
	}

	if (!extract_lsa_headers(dd))
	    return;

	if (!_data_description_packet.get_ms_bit()) { // Router is slave
	    _data_description_packet.set_dd_seqno(dd->get_dd_seqno());
	    build_data_description_packet();
	    if (!_data_description_packet.get_m_bit() &&
		!dd->get_m_bit()) {
		event_exchange_done();
	    }
	    send_data_description_packet();
	} else {		     // Router is master
	    if (_all_headers_sent && !dd->get_m_bit()) {
		event_exchange_done();
	    } else {
		_data_description_packet.
		    set_dd_seqno(_data_description_packet.get_dd_seqno() + 1);
		build_data_description_packet();
		send_data_description_packet();
	    }
	}
	    
	// Save some fields for later duplicate detection.
	assign(_last_dd, *dd);
    }
	break;
    case Loading:
    case Full:
	// Check for duplicates
	if (_last_dd == *dd) {
	    if (_last_dd.get_ms_bit()) { // Router is slave
		send_data_description_packet();
	    } else {		     // Router is master
		// Discard
	    }
	} else {
	    event_sequence_number_mismatch();
	}
	break;
    }
}

template <typename A>
bool 
Neighbour<A>::extract_lsa_headers(DataDescriptionPacket *dd)
{
    list<Lsa_header> li = dd->get_lsa_headers();
    list<Lsa_header>::const_iterator i;
    for (i = li.begin(); i != li.end(); i++) {
	uint16_t ls_type = i->get_ls_type();

	// Do we recognise this LS type?
	// If the LSA decoder knows about about the LSA then we
	// must be good.
	if (!_ospf.get_lsa_decoder().validate(ls_type)) {
	    XLOG_TRACE(_ospf.trace()._input_errors,
		       "Unknown LS type %u %s", ls_type, cstring(*dd));
	    event_sequence_number_mismatch();
	    return false;
	    break;
	}

	// Deal with AS-external-LSA's (LS type = 5, 0x4005).
	switch(_peer.get_area_type()) {
	case OspfTypes::NORMAL:
	    break;
	case OspfTypes::STUB:
	case OspfTypes::NSSA:
	    if (_ospf.get_lsa_decoder().external(ls_type)) {
		XLOG_TRACE(_ospf.trace()._input_errors,
			   "AS-external-LSA not allowed in %s area %s",
			   pp_area_type(_peer.get_area_type()).c_str(),
			   cstring(*dd));
		event_sequence_number_mismatch();
		return false;
	    }
	    break;
	}
	    
	// Check to see if this is a newer LSA.
	if (get_area_router()->newer_lsa(*i))
	    _ls_request_list.push_back((*i));
    }

    return true;
}

/**
 * RFC 2328 Section 10.7. Receiving Link State Request Packets
 */
template <typename A>
void
Neighbour<A>::link_state_request_received(LinkStateRequestPacket *lsrp)
{
    const char *event_name = "LinkStateRequestReceived-pseudo-event";
    XLOG_TRACE(_ospf.trace()._neighbour_events, 
	       "Event(%s) Interface(%s) Neighbour(%s) State(%s)",
	       event_name,
	       _peer.get_if_name().c_str(),
	       pr_id(get_candidate_id()).c_str(),
	       pp_state(get_state()).c_str());

    debug_msg("ID = %s interface state <%s> neighbour state <%s> %s\n",
	      pr_id(get_candidate_id()).c_str(),
	      Peer<A>::pp_interface_state(_peer.get_state()).c_str(),
	      pp_state(get_state()).c_str(),
	      cstring(*lsrp));

    switch(get_state()) {
    case Down:
    case Attempt:
    case Init:
    case TwoWay:
    case ExStart:
	// Ignore
	return;
    case Exchange:
    case Loading:
    case Full:
	break;
    }

    list<Lsa::LsaRef> lsas;
    if (!get_area_router()->get_lsas(lsrp->get_ls_request(), lsas)) {
	event_bad_link_state_request();
	return;
    }

    LinkStateUpdatePacket lsup(_ospf.get_version(), _ospf.get_lsa_decoder());
    size_t lsas_len = 0;
    list<Lsa::LsaRef>::iterator i;
    for (i = lsas.begin(); i != lsas.end(); i++) {
	XLOG_ASSERT((*i)->valid());
	size_t len;
	(*i)->lsa(len);
	(*i)->set_transmitted(true);
	if (lsup.get_standard_header_length() + len + lsas_len < 
	    _peer.get_frame_size()) {
	    lsas_len += len;
	    lsup.get_lsas().push_back(*i);
	} else {
	    send_link_state_update_packet(lsup);
	    lsup.get_lsas().clear();
	    lsas_len = 0;
	}
    }

    if (!lsup.get_lsas().empty())
	send_link_state_update_packet(lsup);
}

template <typename A>
void
Neighbour<A>::link_state_update_received(LinkStateUpdatePacket *lsup)
{
    const char *event_name = "LinkStateUpdateReceived-pseudo-event";
    XLOG_TRACE(_ospf.trace()._neighbour_events, 
	       "Event(%s) Interface(%s) Neighbour(%s) State(%s)",
	       event_name,
	       _peer.get_if_name().c_str(),
	       pr_id(get_candidate_id()).c_str(),
	       pp_state(get_state()).c_str());

    debug_msg("ID = %s interface state <%s> neighbour state <%s> %s\n",
	      pr_id(get_candidate_id()).c_str(),
	      Peer<A>::pp_interface_state(_peer.get_state()).c_str(),
	      pp_state(get_state()).c_str(),
	      cstring(*lsup));

    switch(get_state()) {
    case Down:
    case Attempt:
    case Init:
    case TwoWay:
    case ExStart:
	// Ignore
	return;
    case Exchange:
    case Loading:
    case Full:
	break;
    }

    list<Lsa_header> direct_ack, delayed_ack;
    bool is_router_dr = false;
    bool is_router_bdr = false;
    bool is_neighbour_dr = false;
    if (_peer.do_dr_or_bdr()) {
 	is_router_dr = is_DR();
	is_router_bdr = is_BDR();
	is_neighbour_dr = is_neighbour_DR();
    }
    
    get_area_router()->
	receive_lsas(_peer.get_peerid(),
		     _neighbourid,
		     lsup->get_lsas(),
		     direct_ack,
		     delayed_ack,
		     is_router_dr,
		     is_router_bdr,
		     is_neighbour_dr);

    // A more efficient way of sending the direct ack.
//     bool multicast_on_peer;
//     send_ack(direct_ack, /*direct*/true, multicast_on_peer);
    _peer.send_direct_acks(get_neighbour_id(), direct_ack);
    _peer.send_delayed_acks(get_neighbour_id(), delayed_ack);

#ifndef	MAX_AGE_IN_DATABASE
    // MaxAge LSAs are in the retransmission list with no connection
    // to the database. The LSAs can either be removed due to an ACK
    // or because of a new LSA. If an incoming LSA matches a MaxAge
    // LSA remove the MaxAge LSA. An incoming LSA can be rewritten and
    // placed on the retransmission list, leave these alone.
    // 
 again:
    for (list<Lsa::LsaRef>::iterator i = _lsa_rxmt.begin();
	 i != _lsa_rxmt.end(); i++) {
	if (!(*i)->maxage())
	    continue;
	if ((*i)->max_sequence_number())
	    continue;
	list<Lsa::LsaRef>& lsas = lsup->get_lsas();
	list<Lsa::LsaRef>::const_iterator j;
	for (j = lsas.begin(); j != lsas.end(); j++) {
	    // Possibly rewritten
	    if (*i == *j) {
		continue;
	    }
	    if ((*i).get()->get_header() == (*j).get()->get_header()) {
// 		XLOG_INFO("Same LSA\n%s\n%s", cstring(*(*i)), cstring(*(*j)));
		_lsa_rxmt.erase(i);
		goto again;
	    }
	}
    }
#endif

    // Have any of the update packets satisfied outstanding requests?
    if (_ls_request_list.empty())
	return;
    
    list<Lsa::LsaRef>& lsas = lsup->get_lsas();
    list<Lsa::LsaRef>::const_iterator i;
    list<Lsa_header>::iterator j;

    for (i = lsas.begin(); i != lsas.end(); i++) {
	for (j = _ls_request_list.begin(); j != _ls_request_list.end(); j++) {
	    if ((*j) == (*i)->get_header()) {
		_ls_request_list.erase(j);
		break;
	    }
	}
    }
    if (_ls_request_list.empty())
	event_loading_done();
}

template <typename A>
void
Neighbour<A>::
link_state_acknowledgement_received(LinkStateAcknowledgementPacket *lsap)
{
    const char *event_name = "LinkStateAcknowledgementReceived-pseudo-event";
    XLOG_TRACE(_ospf.trace()._neighbour_events, 
	       "Event(%s) Interface(%s) Neighbour(%s) State(%s)",
	       event_name,
	       _peer.get_if_name().c_str(),
	       pr_id(get_candidate_id()).c_str(),
	       pp_state(get_state()).c_str());

    debug_msg("ID = %s interface state <%s> neighbour state <%s> %s\n",
	      pr_id(get_candidate_id()).c_str(),
	      Peer<A>::pp_interface_state(_peer.get_state()).c_str(),
	      pp_state(get_state()).c_str(),
	      cstring(*lsap));

    switch(get_state()) {
    case Down:
    case Attempt:
    case Init:
    case TwoWay:
    case ExStart:
	// Ignore
	return;
    case Exchange:
    case Loading:
    case Full:
	break;
    }

    // Find the LSA on the retransmission list.
    // 1) Remove the NACK state.
    // 2) Remove from the retransmisson list.

    list<Lsa_header>& headers = lsap->get_lsa_headers();
    list<Lsa_header>::iterator i;
    list<Lsa::LsaRef>::iterator j;
    for (i = headers.begin(); i != headers.end(); i++) {
	// Neither the found or partial variable are required they
	// exist solely for monitoring. The double lookup is also
	// unnecessary. A call to compare_all_header_fields is all
	// that is required.
	bool found = false;
	bool partial = false;
	for (j = _lsa_rxmt.begin(); j != _lsa_rxmt.end(); j++) {
	    if ((*i) == (*j)->get_header()) {
		partial = true;
		if (compare_all_header_fields((*i),(*j)->get_header())) {
		    found = true;
		    (*j)->remove_nack(get_neighbour_id());
		    _lsa_rxmt.erase(j);
		    break;
		}
	    }
	}
	// Its probably going to be a common occurence that an ACK
	// arrives for a LSA that has since been updated. A LSA that
	// we don't know about at all is more interesting.
#if	0
	if (!found && !partial) {
	    XLOG_TRACE(_ospf.trace()._input_errors,
		       "Ack for LSA not in retransmission list.\n%s\n%s",
		       cstring(*i), cstring(*lsap));
	    // Print the retransmission list.
	    list<Lsa::LsaRef>::iterator k;
	    for (k = _lsa_rxmt.begin(); k != _lsa_rxmt.end(); k++) {
		XLOG_TRACE(_ospf.trace()._input_errors,
			   "Retransmit entry %s", cstring(*(*k)));
	    }
	}
#endif
    }
}

template <typename A>
bool
Neighbour<A>::send_lsa(Lsa::LsaRef lsar)
{
    LinkStateUpdatePacket lsup(_ospf.get_version(), _ospf.get_lsa_decoder());
    lsup.get_lsas().push_back(lsar);

    _peer.populate_common_header(lsup);
    
    vector<uint8_t> pkt;
    lsup.encode(pkt, _peer.get_inftransdelay());
    get_auth_handler().generate(pkt);
    
    SimpleTransmit<A> *transmit;

    transmit = new SimpleTransmit<A>(pkt,
				     get_neighbour_address(),
				     _peer.get_interface_address());

    typename Transmit<A>::TransmitRef tr(transmit);

    _peer.transmit(tr);

    return true;
}

template <typename A>
bool
Neighbour<A>::queue_lsa(OspfTypes::PeerID peerid, OspfTypes::NeighbourID nid,
			Lsa::LsaRef lsar, bool& multicast_on_peer)
{
    // RFC 2328 Section 13.3.  Next step in the flooding procedure

    XLOG_TRACE(lsar->tracing(), "Attempting to queue %s", cstring(*lsar));

    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	if (lsar->link_local_scope()) {
	    if (lsar->get_peerid() != _peer.get_peerid()) {
		XLOG_TRACE(lsar->tracing(), "Not queued Link-local %s",
			   cstring(*lsar));
		return true;
	    }
	}
	break;
    }

    // (1) 
    switch(get_state()) {
    case Down:
    case Attempt:
    case Init:
    case TwoWay:
    case ExStart:
	// (a) Neighbour is in too low a state so return.
	XLOG_TRACE(lsar->tracing(), "Not queued state too low %s",
		   cstring(*lsar));
	return true;
    case Exchange:
    case Loading: {
	// (b) See if this LSA is on the link state request list.
	Lsa_header& lsah = lsar->get_header();
	list<Lsa_header>::iterator i = find(_ls_request_list.begin(),
					    _ls_request_list.end(),
					    lsah);
	if (i != _ls_request_list.end()) {
	    switch(get_area_router()->compare_lsa(lsah, *i)) {
	    case AreaRouter<A>::NOMATCH:
		XLOG_UNREACHABLE();
		break;
	    case AreaRouter<A>::EQUIVALENT:
		_ls_request_list.erase(i);
		if (_ls_request_list.empty())
		    event_loading_done();
		return true;
		break;
	    case AreaRouter<A>::NEWER:
		_ls_request_list.erase(i);
		if (_ls_request_list.empty())
		    event_loading_done();
		break;
	    case AreaRouter<A>::OLDER:
		return true;
		break;
	    }
	}
    }
	break;
    case Full:

	break;
    }
    
    // (c) If the neighbour IDs match then this is the neighbour that this
    // LSA was originally received on.
    if (_neighbourid == nid) {
	XLOG_TRACE(lsar->tracing(), "LSA came from this neighbour %s",
		   cstring(*lsar));
	return true;
    }

#ifndef	MAX_AGE_IN_DATABASE
    // Look for the LSA by <Type,ID,ADV> tuple. In all but the case
    // described below it is sufficient to compare LSA pointers as
    // below to decide if two LSAs are equivalent. Principally because
    // all LSAs are in the database and two LSAs cannot exist in the
    // database with the same tuple. If an LSA reaches MaxAge and is
    // being withdrawn it is removed from the database. There is a
    // window where the old LSA exists only in the retransmission list
    // and a new LSA is inserted into the database and an attempt is
    // made to flood it. A common instance of this is that after a
    // crash a neighbour may send the router its own LSA which it
    // attempts to flush by setting the MaxAge and then moments later
    // it puts the same LSA in the database.

    // XXX
    // Once we are happy with this fix then combine this with the loop
    // below and do away with this define.

    list<Lsa::LsaRef>::iterator i;
    for (i = _lsa_rxmt.begin(); i != _lsa_rxmt.end(); i++) {
	if (lsar != (*i) &&
	    (*i).get()->get_header() == lsar.get()->get_header()) {
// 	    XLOG_ASSERT((*i)->maxage());
// 	    XLOG_INFO("Same LSA\n%s\n%s", cstring(*(*i)), cstring(*lsar));
	    _lsa_rxmt.erase(i);
	    break;
	}
    }
#endif

    // (d) If this LSA isn't already on the retransmit queue add it.
    if (find(_lsa_rxmt.begin(), _lsa_rxmt.end(), lsar) == _lsa_rxmt.end())
	_lsa_rxmt.push_back(lsar);

    // Add this neighbour ID to the set of unacknowledged neighbours.
    lsar->add_nack(_neighbourid);

    // (2) By now we should be thinking about sending this LSA out.

    // Did this LSA arrive on this peer (interface).
    if (_peer.get_peerid() == peerid ) {
	// (3) If this LSA arrived from the designated router or the
	// backup designated router. Chances are high that our
	// neighbours have received this LSA already.
	if (_peer.do_dr_or_bdr() && _peer.is_neighbour_DR_or_BDR(nid)) {
	    XLOG_TRACE(lsar->tracing(), "Peers neighbour is DR or BDR %s",
		       cstring(*lsar));
	    return true;
	}

	// (4) If this peer (interface) is in state Backup then out of
	// here.
	if (_peer.get_state() == Peer<A>::Backup) {
	    XLOG_TRACE(lsar->tracing(), "Peer state is backup%s",
		       cstring(*lsar));
	    return true;
	}
    }

    // (5) This LSA should be flooded now. If it hasn't already been
    // multicast out of this peer/inteface.

    switch(get_linktype()) {
    case OspfTypes::BROADCAST:
	if (multicast_on_peer) {
	    XLOG_TRACE(lsar->tracing(), "LSA has already been multicast %s",
		       cstring(*lsar));
	    return true;
	} else
	    multicast_on_peer = true;
	break;
    case OspfTypes::NBMA:
    case OspfTypes::PointToMultiPoint:
    case OspfTypes::VirtualLink:
    case OspfTypes::PointToPoint:
	break;
    }

    // Increment LSA's LS age by InfTransDelay performed in
    // send_link_state_update_packet.

    _lsa_queue.push_back(lsar);

    XLOG_TRACE(lsar->tracing(), "Queued successful %s", cstring(*lsar));

    return true;
}

template <typename A>
bool
Neighbour<A>::push_lsas()
{
    // Typically push_lsas will be called immediately after one or
    // more calls to queue_lsa, it therefore shouldn't be possible for
    // the state to change. If the state was less than exchange then
    // queue_lsa shouldn't have queued anything.
    if (get_state() < Exchange) {
	list<Lsa::LsaRef>::iterator i;
	for (i = _lsa_queue.begin(); i != _lsa_queue.end(); i++)
	    (*i)->remove_nack(_neighbourid);
	_lsa_queue.clear();
	return true;
    }

    LinkStateUpdatePacket lsup(_ospf.get_version(), _ospf.get_lsa_decoder());
    size_t lsas_len = 0;
    list<Lsa::LsaRef>::iterator i;
    for (i = _lsa_queue.begin(); i != _lsa_queue.end(); i++) {
	if ((*i)->valid() && (*i)->exists_nack(_neighbourid)) {
	    size_t len;
	    (*i)->lsa(len);
	    (*i)->set_transmitted(true);
	    if (lsup.get_standard_header_length() + len + lsas_len < 
		_peer.get_frame_size()) {
		lsas_len += len;
		lsup.get_lsas().push_back(*i);
	    } else {
		send_link_state_update_packet(lsup);
		lsup.get_lsas().clear();
		lsas_len = 0;
	    }
	}
    }

    if (!lsup.get_lsas().empty())
	send_link_state_update_packet(lsup);

    // All the LSAs on the queue should now be scheduled for
    // sending. Zap the queue.
    _lsa_queue.clear();

    restart_retransmitter();

    return true;
}

template <typename A>
bool
Neighbour<A>::on_link_state_request_list(Lsa::LsaRef lsar) const
{
    if (_ls_request_list.end() != find(_ls_request_list.begin(),
				       _ls_request_list.end(),
				       lsar->get_header()))
	return true;
				       
    return false;
}

template <typename A>
bool
Neighbour<A>::send_ack(list<Lsa_header>& ack, bool direct,
		       bool& multicast_on_peer)
{
    switch(get_state()) {
    case Down:
    case Attempt:
    case Init:
    case TwoWay:
    case ExStart:
	multicast_on_peer = false;
	return false;
    case Exchange:
    case Loading:
    case Full:
	break;
    }

    LinkStateAcknowledgementPacket lsap(_ospf.get_version());

    list<Lsa_header>& l = lsap.get_lsa_headers();
    l.insert(l.begin(), ack.begin(), ack.end());

    return send_link_state_ack_packet(lsap, direct, multicast_on_peer);
}

template <typename A>
void
Neighbour<A>::change_state(State state)
{
    State previous_state = get_state();
    set_state(state);

    if (Full == state || Full == previous_state)
	_ospf.get_peer_manager().
	    adjacency_changed(_peer.get_peerid(), get_router_id(),
			      Full == state);

    if (Full == state)
	_ospf.get_eventloop().current_time(_adjacency_time);

    // If we are dropping down states tear down any higher level state.
    if (previous_state > state)
	tear_down_state(previous_state);

    if (Down == state)
	get_auth_handler().reset();
}

template <typename A>
void
Neighbour<A>::event_negotiation_done()
{
    const char *event_name = "NegotiationDone";
    XLOG_TRACE(_ospf.trace()._neighbour_events, 
	       "Event(%s) Interface(%s) Neighbour(%s) State(%s)",
	       event_name,
	       _peer.get_if_name().c_str(),
	       pr_id(get_candidate_id()).c_str(),
	       pp_state(get_state()).c_str());

    debug_msg("ID = %s interface state <%s> neighbour state <%s>\n",
	      pr_id(get_candidate_id()).c_str(),
	      Peer<A>::pp_interface_state(_peer.get_state()).c_str(),
	      pp_state(get_state()).c_str());

    switch(get_state()) {
    case Down:
	break;
    case Attempt:
	break;
    case Init:
	break;
    case TwoWay:
	break;
    case ExStart:
	change_state(Exchange);
	// Inferred from the specification.
	_data_description_packet.set_i_bit(false);
	build_data_description_packet();
	// If we are the master start sending description packets.
	if (!_last_dd.get_ms_bit()) {
	    stop_rxmt_timer(INITIAL, "NegotiationDone (master)");
	    start_rxmt_timer(INITIAL, callback(this,
				      &Neighbour<A>::
				      send_data_description_packet),
			     true,
			     "send_data_description from NegotiationDone");
	} else {
	    // We have now agreed we are the slave so stop retransmitting.
	    stop_rxmt_timer(INITIAL, "NegotiationDone (slave)");
	    // Send a response to the poll
	    send_data_description_packet();
	}
	break;
    case Exchange:
	break;
    case Loading:
	break;
    case Full:
	break;
    }
}

template <typename A>
void
Neighbour<A>::event_sequence_number_mismatch()
{
    const char *event_name = "SequenceNumberMismatch";
    event_SequenceNumberMismatch_or_BadLSReq(event_name);
}

template <typename A>
void
Neighbour<A>::event_exchange_done()
{
    const char *event_name = "ExchangeDone";
    XLOG_TRACE(_ospf.trace()._neighbour_events, 
	       "Event(%s) Interface(%s) Neighbour(%s) State(%s)",
	       event_name,
	       _peer.get_if_name().c_str(),
	       pr_id(get_candidate_id()).c_str(),
	       pp_state(get_state()).c_str());

    debug_msg("ID = %s interface state <%s> neighbour state <%s>\n",
	      pr_id(get_candidate_id()).c_str(),
	      Peer<A>::pp_interface_state(_peer.get_state()).c_str(),
	      pp_state(get_state()).c_str());

    switch(get_state()) {
    case Down:
	break;
    case Attempt:
	break;
    case Init:
	break;
    case TwoWay:
	break;
    case ExStart:
	break;
    case Exchange:
	change_state(Loading);
	// Stop any retransmissions, only the master should have any running.
	if (!_last_dd.get_ms_bit())
	    stop_rxmt_timer(INITIAL, "ExchangeDone");

	// The protocol allows link state request packets to be sent
	// before the database exchange has taken place. For the time
	// being wait until the exchange is over before sending
	// requests. Currently we only have a single retransmit timer
	// although this isn't really an issue.
	if (_ls_request_list.empty()) {
	    event_loading_done();
	    return;
	}
	restart_retransmitter();
	debug_msg("link state request list count: %d\n",
		  XORP_INT_CAST(_ls_request_list.size()));
	break;
    case Loading:
	break;
    case Full:
	break;
    }
}

template <typename A>
void
Neighbour<A>::event_bad_link_state_request()
{
    const char *event_name = "BadLSReq";
    event_SequenceNumberMismatch_or_BadLSReq(event_name);
}

template <typename A>
void
Neighbour<A>::event_loading_done()
{
    const char *event_name = "LoadingDone";
    XLOG_TRACE(_ospf.trace()._neighbour_events, 
	       "Event(%s) Interface(%s) Neighbour(%s) State(%s)",
	       event_name,
	       _peer.get_if_name().c_str(),
	       pr_id(get_candidate_id()).c_str(),
	       pp_state(get_state()).c_str());

    debug_msg("ID = %s interface state <%s> neighbour state <%s>\n",
	      pr_id(get_candidate_id()).c_str(),
	      Peer<A>::pp_interface_state(_peer.get_state()).c_str(),
	      pp_state(get_state()).c_str());

    switch(get_state()) {
    case Down:
	break;
    case Attempt:
	break;
    case Init:
	break;
    case TwoWay:
	break;
    case ExStart:
	break;
    case Exchange:
	break;
    case Loading:
	change_state(Full);
	_peer.update_router_links();
	if (_peer.do_dr_or_bdr() && is_DR())
	    _peer.adjacency_change(true);
	break;
    case Full:
	break;
    }
}

template <typename A>
void
Neighbour<A>::event_kill_neighbour()
{
    const char *event_name = "KillNbr";
    XLOG_TRACE(_ospf.trace()._neighbour_events, 
	       "Event(%s) Interface(%s) Neighbour(%s) State(%s)",
	       event_name,
	       _peer.get_if_name().c_str(),
	       pr_id(get_candidate_id()).c_str(),
	       pp_state(get_state()).c_str());

    debug_msg("ID = %s interface state <%s> neighbour state <%s>\n",
	      pr_id(get_candidate_id()).c_str(),
	      Peer<A>::pp_interface_state(_peer.get_state()).c_str(),
	      pp_state(get_state()).c_str());

    switch(get_state()) {
    case Down:
	break;
    case Attempt:
    case Init:
    case TwoWay:
    case ExStart:
    case Exchange:
    case Loading:
    case Full:
	change_state(Down);
	break;
    }
}

template <typename A>
void
Neighbour<A>::event_adj_ok()
{
    const char *event_name = "AdjOK?";
    XLOG_TRACE(_ospf.trace()._neighbour_events, 
	       "Event(%s) Interface(%s) Neighbour(%s) State(%s)",
	       event_name,
	       _peer.get_if_name().c_str(),
	       pr_id(get_candidate_id()).c_str(),
	       pp_state(get_state()).c_str());

    debug_msg("ID = %s interface state <%s> neighbour state <%s>\n",
	      pr_id(get_candidate_id()).c_str(),
	      Peer<A>::pp_interface_state(_peer.get_state()).c_str(),
	      pp_state(get_state()).c_str());

    switch(get_state()) {
    case Down:
    case Attempt:
    case Init:
	// Nothing
	break;
    case TwoWay:
	if (establish_adjacency_p()) {
	    change_state(ExStart);

	    start_sending_data_description_packets(event_name);
	}
	break;
    case ExStart:
    case Exchange:
    case Loading:
    case Full:
	if (!establish_adjacency_p())
	    change_state(TwoWay);
	break;
    }
}

template <typename A>
void
Neighbour<A>::event_inactivity_timer()
{
    const char *event_name = "InactivityTimer";
    XLOG_TRACE(_ospf.trace()._neighbour_events, 
	       "Event(%s) Interface(%s) Neighbour(%s) State(%s)",
	       event_name,
	       _peer.get_if_name().c_str(),
	       pr_id(get_candidate_id()).c_str(),
	       pp_state(get_state()).c_str());

    debug_msg("ID = %s interface state <%s> neighbour state <%s>\n",
	      pr_id(get_candidate_id()).c_str(),
	      Peer<A>::pp_interface_state(_peer.get_state()).c_str(),
	      pp_state(get_state()).c_str());

    change_state(Down);
    // The saved hello packet is no longer required as it has timed
    // out. Use the presence of a hello packet to decide if this
    // neighbour should be included in this routers hello packets.
    delete _hello_packet;
    _hello_packet = 0;
}

template <typename A>
void
Neighbour<A>::event_SequenceNumberMismatch_or_BadLSReq(const char *event_name)
{
    XLOG_TRACE(_ospf.trace()._neighbour_events, 
	       "Event(%s) Interface(%s) Neighbour(%s) State(%s)",
	       event_name,
	       _peer.get_if_name().c_str(),
	       pr_id(get_candidate_id()).c_str(),
	       pp_state(get_state()).c_str());

    debug_msg("ID = %s interface state <%s> neighbour state <%s>\n",
	      pr_id(get_candidate_id()).c_str(),
	      Peer<A>::pp_interface_state(_peer.get_state()).c_str(),
	      pp_state(get_state()).c_str());

    switch(get_state()) {
    case Down:
    case Attempt:
    case Init:
    case TwoWay:
    case ExStart:
	XLOG_WARNING("Event %s in state %s not possible",
		     event_name,
		     pp_state(get_state()).c_str());
	break;
    case Exchange:
    case Loading:
    case Full:
	change_state(ExStart);
	// Don't send this packet immediately wait for the retransmit interval.
	start_sending_data_description_packets(event_name, false);
	break;
    }
}

template <typename A>
bool
Neighbour<A>::get_neighbour_info(NeighbourInfo& ninfo) const
{
    uint32_t priority = 0;
    uint32_t options = 0;
    uint32_t dr = 0;
    uint32_t bdr = 0;
    if (_hello_packet) {
	priority = _hello_packet->get_router_priority();
	options = _hello_packet->get_options();
	dr = _hello_packet->get_designated_router();
	bdr = _hello_packet->get_backup_designated_router();
    }
    TimeVal remain;
    if (!_inactivity_timer.time_remaining(remain))
	remain = TimeVal(0,0);
    ninfo._address = get_neighbour_address().str();
    ninfo._interface = _peer.get_if_name();
    ninfo._state = pp_state(get_state());
    ninfo._rid = IPv4(htonl(get_router_id()));
    ninfo._priority = priority;
    ninfo._deadtime = remain.sec();
    ninfo._area = IPv4(htonl(_peer.get_area_id()));
    ninfo._opt = options;
    ninfo._dr = IPv4(htonl(dr));
    ninfo._bdr = IPv4(htonl(bdr));

    TimeVal now, diff;
    _ospf.get_eventloop().current_time(now);
    diff = now - _creation_time;
    ninfo._up = diff.sec();
    if (Full == get_state()) {
	diff = now - _adjacency_time;
	ninfo._adjacent = diff.sec();
    } else {
	ninfo._adjacent = 0;
    }

    return true;
}

template class PeerOut<IPv4>;
template class PeerOut<IPv6>;
