// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/ospf/ospf.cc,v 1.99 2008/07/23 05:11:08 pavlin Exp $"

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

#include "ospf.hh"

template <typename A>
Ospf<A>::Ospf(OspfTypes::Version version, EventLoop& eventloop, IO<A>* io)
    : _version(version), _eventloop(eventloop),
      _testing(false),
      _io(io), _reason("Waiting for IO"), _process_status(PROC_STARTUP),
      _lsa_decoder(version), _peer_manager(*this), _routing_table(*this),
      _instance_id(0), _router_id(0),
      _rfc1583_compatibility(false)
{
    // Register the LSAs and packets with the associated decoder.
    initialise_lsa_decoder(version, _lsa_decoder);
    initialise_packet_decoder(version, _packet_decoder, _lsa_decoder);

    // Now that all the packet decoders are in place register for
    // receiving packets.
    _io->register_receive(callback(this,&Ospf<A>::receive));

    // The peer manager will solicit packets from the various interfaces.

    // Make sure that this value can never be allocated as an interface ID.
    _iidmap[""] = OspfTypes::UNUSED_INTERFACE_ID;
}

/**
 * All packets for OSPF are received through this interface. All good
 * packets are sent to the peer manager which verifies that the packet
 * is expected and authenticates the packet if necessary. The peer
 * manager can choose to accept the packet in which case it becomes
 * the owner. If the packet is rejected this routine will delete the
 * packet.
 */
template <typename A>
void 
Ospf<A>::receive(const string& interface, const string& vif,
		 A dst, A src, uint8_t* data, uint32_t len)
{
    XLOG_TRACE(trace()._packets, 
	       "Interface %s Vif %s dst %s src %s data %p len %u\n",
	      interface.c_str(), vif.c_str(),
	      dst.str().c_str(), src.str().c_str(),
	      data, len);
    debug_msg("Interface %s Vif %s dst %s src %s data %p len %u\n",
	      interface.c_str(), vif.c_str(),
	      dst.str().c_str(), src.str().c_str(),
	      data, len);

    Packet *packet;
    try {
	// If the transport is IPv6 then the checksum verification has
	// to include the pseudo header. In the IPv4 case this
	// function is a noop.
	ipv6_checksum_verify<A>(src, dst, data, len, Packet::CHECKSUM_OFFSET,
				_io->get_ip_protocol_number());
	packet = _packet_decoder.decode(data, len);
    } catch(InvalidPacket& e) {
	XLOG_ERROR("%s", cstring(e));
	return;
    }

    XLOG_TRACE(trace()._packets, "%s\n", cstring(*packet));
    debug_msg("%s\n", cstring(*packet));
    // We have a packet and its good.

    bool packet_accepted = false;
    try {
	packet_accepted = _peer_manager.receive(interface, vif, dst, src,
						packet);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
    }

    if (!packet_accepted)
	delete packet;
}

template <typename A>
bool
Ospf<A>::enable_interface_vif(const string& interface, const string& vif)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    if (string(VLINK) == interface)
	return true;

    return _io->enable_interface_vif(interface, vif);
}

template <typename A>
bool
Ospf<A>::disable_interface_vif(const string& interface, const string& vif)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    if (string(VLINK) == interface)
	return true;

    return _io->disable_interface_vif(interface, vif);
}

template <typename A>
bool
Ospf<A>::enabled(const string& interface, const string& vif)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    return _io->is_vif_enabled(interface, vif);
}

template <typename A>
bool
Ospf<A>::enabled(const string& interface, const string& vif, A address)
{
    debug_msg("Interface %s Vif %s Address %s\n", interface.c_str(),
	      vif.c_str(), cstring(address));

    return _io->is_address_enabled(interface, vif, address);
}

template <typename A>
bool
Ospf<A>::get_addresses(const string& interface, const string& vif,
		       list<A>& addresses) const
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    return _io->get_addresses(interface, vif, addresses);
}

template <typename A>
bool
Ospf<A>::get_link_local_address(const string& interface, const string& vif,
				A& address)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    return _io->get_link_local_address(interface, vif, address);
}

template <typename A>
bool
Ospf<A>::get_interface_id(const string& interface, const string& vif, 
			  uint32_t& interface_id)
{
    debug_msg("Interface %s Vif %s\n", interface.c_str(), vif.c_str());

    string concat = interface + "/" + vif;

    if (0 == _iidmap.count(concat)) {
	if (string(VLINK) == interface)
	    interface_id = 100000;
	else
	    _io->get_interface_id(interface, interface_id);

	bool match;
	do {
	    match = false;
	    typename map<string, uint32_t>::iterator i;
	    for(i = _iidmap.begin(); i != _iidmap.end(); i++)
		if ((*i).second == interface_id) {
		    interface_id++;
		    match = true;
		    break;
		}
	} while(match);
	_iidmap[concat] = interface_id;
    }

    interface_id = _iidmap[concat];

    XLOG_ASSERT(OspfTypes::UNUSED_INTERFACE_ID != interface_id);

    debug_msg("Interface %s Vif %s ID = %u\n", interface.c_str(), vif.c_str(),
	      interface_id);

    _io->set_interface_mapping(interface_id, interface, vif);

    return true;
}

template <typename A>
bool
Ospf<A>::get_interface_vif_by_interface_id(uint32_t interface_id,
					   string& interface, string& vif)
{
    typename map<string, uint32_t>::iterator i;
    for(i = _iidmap.begin(); i != _iidmap.end(); i++) {
	if ((*i).second == interface_id) {
	    string concat = (*i).first;
	    interface = concat.substr(0, concat.find('/'));
	    vif = concat.substr(concat.find('/') + 1, concat.size() - 1);
// 	    fprintf(stderr, "interface <%s> vif <%s>\n", interface.c_str(),
// 		    vif.c_str());
	    return true;
	}
    }
    
    return false;
}

template <typename A>
bool
Ospf<A>::get_prefix_length(const string& interface, const string& vif,
			   A address, uint16_t& prefix_length)
{
    debug_msg("Interface %s Vif %s Address %s\n", interface.c_str(),
	      vif.c_str(), cstring(address));

    if (string(VLINK) == interface) {
	prefix_length = 0;
	return true;
    }

    prefix_length = _io->get_prefix_length(interface, vif, address);
    return 0 == prefix_length ? false : true;
}

template <typename A>
uint32_t
Ospf<A>::get_mtu(const string& interface)
{
    debug_msg("Interface %s\n", interface.c_str());

    if (string(VLINK) == interface)
	return VLINK_MTU;

    return _io->get_mtu(interface);
}

template <typename A>
bool
Ospf<A>::join_multicast_group(const string& interface, const string& vif,
			      A mcast)
{
    debug_msg("Interface %s Vif %s mcast %s\n", interface.c_str(),
	      vif.c_str(), cstring(mcast));

    return _io->join_multicast_group(interface, vif, mcast);
}

template <typename A>
bool
Ospf<A>::leave_multicast_group(const string& interface, const string& vif,
			      A mcast)
{
    debug_msg("Interface %s Vif %s mcast %s\n", interface.c_str(),
	      vif.c_str(), cstring(mcast));

    return _io->leave_multicast_group(interface, vif, mcast);
}

template <typename A>
bool
Ospf<A>::transmit(const string& interface, const string& vif,
		  A dst, A src,
		  uint8_t* data, uint32_t len)
{
    XLOG_TRACE(trace()._packets, "Interface %s Vif %s data %p len %u\n",
	      interface.c_str(), vif.c_str(), data, len);
    debug_msg("Interface %s Vif %s data %p len %u\n",
	      interface.c_str(), vif.c_str(), data, len);

    // If the transport is IPv6 then the checksum has to include the
    // pseudo header. In the IPv4 case this function is a noop.
    ipv6_checksum_apply<A>(src, dst, data, len, Packet::CHECKSUM_OFFSET,
			   _io->get_ip_protocol_number());

    if (trace()._packets) {
	try {
	    // Decode the packet in order to pretty print it.
	    Packet *packet = _packet_decoder.decode(data, len);
	    XLOG_TRACE(trace()._packets, "Transmit: %s\n", cstring(*packet));
	    delete packet;
	} catch(InvalidPacket& e) {
	    XLOG_TRACE(trace()._packets, "Unable to decode packet\n");
	}
    }

#ifdef	DEBUG_LOGGING
    try {
	// Decode the packet in order to pretty print it.
	Packet *packet = _packet_decoder.decode(data, len);
	debug_msg("Transmit: %s\n", cstring(*packet));
	delete packet;
    } catch(InvalidPacket& e) {
	debug_msg("Unable to decode packet\n");
    }
#endif

    return _io->send(interface, vif, dst, src, data, len);
}

template <typename A>
bool
Ospf<A>::set_hello_interval(const string& interface, const string& vif,
			    OspfTypes::AreaID area,
			    uint16_t hello_interval)
{
    try {
	_peer_manager.set_hello_interval(_peer_manager.
					 get_peerid(interface, vif),
					 area, hello_interval);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

#if	0
template <typename A>
bool 
Ospf<A>::set_options(const string& interface, const string& vif,
		     OspfTypes::AreaID area,
		     uint32_t options)
{
    try {
	_peer_manager.set_options(_peer_manager.get_peerid(interface, vif),
				  area, options);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}
#endif

template <typename A>
bool
Ospf<A>::create_virtual_link(OspfTypes::RouterID rid)
{
    try {
	_peer_manager.create_virtual_link(rid);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

template <typename A>
bool
Ospf<A>::delete_virtual_link(OspfTypes::RouterID rid)
{
    try {
	_peer_manager.delete_virtual_link(rid);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

template <typename A>
bool
Ospf<A>::transit_area_virtual_link(OspfTypes::RouterID rid,
				   OspfTypes::AreaID transit_area)
{
    try {
	_peer_manager.transit_area_virtual_link(rid, transit_area);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}
    
template <typename A>
bool
Ospf<A>::set_router_priority(const string& interface, const string& vif,
			     OspfTypes::AreaID area,
			     uint8_t priority)
{
    try {
	_peer_manager.set_router_priority(_peer_manager.
					  get_peerid(interface, vif),
					  area, priority);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

template <typename A>
bool
Ospf<A>::set_router_dead_interval(const string& interface, const string& vif,
			 OspfTypes::AreaID area,
			 uint32_t router_dead_interval)
{
    try {
	_peer_manager.set_router_dead_interval(_peer_manager.
					       get_peerid(interface,vif),
					       area, router_dead_interval);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

template <typename A>
bool
Ospf<A>::set_interface_cost(const string& interface, const string& vif,
			    OspfTypes::AreaID area,
			    uint16_t interface_cost)
{
    try {
	_peer_manager.set_interface_cost(_peer_manager.
					 get_peerid(interface,vif),
					 area, interface_cost);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

template <typename A>
bool
Ospf<A>::set_retransmit_interval(const string& interface, const string& vif,
				 OspfTypes::AreaID area,
				 uint16_t retransmit_interval)
{
    if (0 == retransmit_interval) {
	XLOG_ERROR("Zero is not a legal value for RxmtInterval");
	return false;
    }

    try {
	_peer_manager.set_retransmit_interval(_peer_manager.
					      get_peerid(interface,vif),
					      area, retransmit_interval);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

template <typename A>
bool
Ospf<A>::set_inftransdelay(const string& interface, const string& vif,
			   OspfTypes::AreaID area,
			   uint16_t inftransdelay)
{
    if (0 == inftransdelay) {
	XLOG_ERROR("Zero is not a legal value for inftransdelay");
	return false;
    }

    try {
	_peer_manager.set_inftransdelay(_peer_manager.
					get_peerid(interface,vif),
					area, inftransdelay);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

template <typename A>
bool
Ospf<A>::set_simple_authentication_key(const string&		interface,
				       const string&		vif,
				       OspfTypes::AreaID	area,
				       const string&		password,
				       string&			error_msg)
{
    OspfTypes::PeerID peerid;

    try {
	peerid = _peer_manager.get_peerid(interface, vif);
    } catch(BadPeer& e) {
	error_msg = e.str();
	XLOG_ERROR("%s", error_msg.c_str());
	return false;
    } 

    if (_peer_manager.set_simple_authentication_key(peerid, area,
						    password, error_msg)
	!= true) {
	XLOG_ERROR("%s", error_msg.c_str());
	return false;
    }

    return true;
}

template <typename A>
bool
Ospf<A>::delete_simple_authentication_key(const string&		interface,
					  const string&		vif,
					  OspfTypes::AreaID	area,
					  string&		error_msg)
{
    OspfTypes::PeerID peerid;

    try {
	peerid = _peer_manager.get_peerid(interface, vif);
    } catch(BadPeer& e) {
	error_msg = e.str();
	XLOG_ERROR("%s", error_msg.c_str());
	return false;
    } 

    if (_peer_manager.delete_simple_authentication_key(peerid, area, error_msg)
	!= true) {
	XLOG_ERROR("%s", error_msg.c_str());
	return false;
    }

    return true;
}

template <typename A>
bool
Ospf<A>::set_md5_authentication_key(const string&	interface,
				    const string&	vif,
				    OspfTypes::AreaID	area,
				    uint8_t		key_id,
				    const string&	password,
				    const TimeVal&	start_timeval,
				    const TimeVal&	end_timeval,
				    const TimeVal&	max_time_drift,
				    string&		error_msg)

{
    OspfTypes::PeerID peerid;

    try {
	peerid = _peer_manager.get_peerid(interface, vif);
    } catch(BadPeer& e) {
	error_msg = e.str();
	XLOG_ERROR("%s", error_msg.c_str());
	return false;
    } 

    if (_peer_manager.set_md5_authentication_key(peerid, area, key_id,
						 password, start_timeval,
						 end_timeval, max_time_drift,
						 error_msg)
	!= true) {
	XLOG_ERROR("%s", error_msg.c_str());
	return false;
    }

    return true;
}

template <typename A>
bool
Ospf<A>::delete_md5_authentication_key(const string&		interface,
				       const string&		vif,
				       OspfTypes::AreaID	area,
				       uint8_t			key_id,
				       string&			error_msg)
{
    OspfTypes::PeerID peerid;

    try {
	peerid = _peer_manager.get_peerid(interface, vif);
    } catch(BadPeer& e) {
	error_msg = e.str();
	XLOG_ERROR("%s", error_msg.c_str());
	return false;
    } 

    if (_peer_manager.delete_md5_authentication_key(peerid, area, key_id,
						    error_msg)
	!= true) {
	XLOG_ERROR("%s", error_msg.c_str());
	return false;
    }

    return true;
}

template <typename A>
bool
Ospf<A>::set_passive(const string& interface, const string& vif,
		     OspfTypes::AreaID area, bool passive, bool host)
{
    try {
	_peer_manager.set_passive(_peer_manager.
				  get_peerid(interface,vif),
				  area, passive, host);
    } catch(BadPeer& e) {
	XLOG_ERROR("%s", cstring(e));
	return false;
    }
    return true;
}

template <typename A>
bool
Ospf<A>::originate_default_route(OspfTypes::AreaID area, bool enable)
{
    debug_msg("Area %s enable %s\n", pr_id(area).c_str(), bool_c_str(enable));

    return _peer_manager.originate_default_route(area, enable);
}

template <typename A>
bool
Ospf<A>::stub_default_cost(OspfTypes::AreaID area, uint32_t cost)
{
    debug_msg("Area %s cost %u\n", pr_id(area).c_str(), cost);

    return _peer_manager.stub_default_cost(area, cost);
}

template <typename A>
bool
Ospf<A>::summaries(OspfTypes::AreaID area, bool enable)
{
    debug_msg("Area %s enable %s\n", pr_id(area).c_str(), bool_c_str(enable));

    return _peer_manager.summaries(area, enable);
}

template <typename A>
bool 
Ospf<A>::set_ip_router_alert(bool alert)
{
    return _io->set_ip_router_alert(alert);
}

template <typename A>
bool
Ospf<A>::area_range_add(OspfTypes::AreaID area, IPNet<A> net, bool advertise)
{
    debug_msg("Area %s Net %s advertise %s\n", pr_id(area).c_str(),
	      cstring(net), bool_c_str(advertise));

    return _peer_manager.area_range_add(area, net, advertise);
}

template <typename A>
bool
Ospf<A>::area_range_delete(OspfTypes::AreaID area, IPNet<A> net)
{
    debug_msg("Area %s Net %s\n", pr_id(area).c_str(), cstring(net));

    return _peer_manager.area_range_delete(area, net);
}

template <typename A>
bool
Ospf<A>::area_range_change_state(OspfTypes::AreaID area, IPNet<A> net,
				 bool advertise)
{
    debug_msg("Area %s Net %s advertise %s\n", pr_id(area).c_str(),
	      cstring(net), bool_c_str(advertise));

    return _peer_manager.area_range_change_state(area, net, advertise);
}

template <typename A>
bool
Ospf<A>::get_lsa(const OspfTypes::AreaID area, const uint32_t index,
		 bool& valid, bool& toohigh, bool& self, vector<uint8_t>& lsa)
{
    debug_msg("Area %s index %u\n", pr_id(area).c_str(), index);

    return _peer_manager.get_lsa(area, index, valid, toohigh, self, lsa);
}

template <typename A>
bool
Ospf<A>::get_area_list(list<OspfTypes::AreaID>& areas) const
{
    debug_msg("\n");

    return _peer_manager.get_area_list(areas);
}

template <typename A>
bool
Ospf<A>::get_neighbour_list(list<OspfTypes::NeighbourID>& neighbours) const
{
    debug_msg("\n");

    return _peer_manager.get_neighbour_list(neighbours);
}

template <typename A>
bool
Ospf<A>::get_neighbour_info(OspfTypes::NeighbourID nid,
			    NeighbourInfo& ninfo) const
{
    return _peer_manager.get_neighbour_info(nid, ninfo);
}

template <typename A>
bool
Ospf<A>::clear_database()
{
    return _peer_manager.clear_database();
}

template <typename A>
bool
Ospf<A>::add_route(IPNet<A> net, A nexthop, uint32_t nexthop_id,
		   uint32_t metric, bool equal, bool discard,
		   const PolicyTags& policytags)
{
    debug_msg("Net %s Nexthop %s metric %d equal %s discard %s policy %s\n",
	      cstring(net), cstring(nexthop), metric, bool_c_str(equal),
	      bool_c_str(discard), cstring(policytags));

    XLOG_TRACE(trace()._routes,
	       "Add route "
	       "Net %s Nexthop %s metric %d equal %s discard %s policy %s\n",
	       cstring(net), cstring(nexthop), metric, bool_c_str(equal),
	       bool_c_str(discard), cstring(policytags));

    return _io->add_route(net, nexthop, nexthop_id, metric, equal, discard,
			  policytags);
}

template <typename A>
bool
Ospf<A>::replace_route(IPNet<A> net, A nexthop, uint32_t nexthop_id, 
		       uint32_t metric, bool equal, bool discard,
		       const PolicyTags& policytags)
{
    debug_msg("Net %s Nexthop %s metric %d equal %s discard %s policy %s\n",
	      cstring(net), cstring(nexthop), metric, bool_c_str(equal),
	      bool_c_str(discard), cstring(policytags));

    XLOG_TRACE(trace()._routes,
	       "Replace route "
	       "Net %s Nexthop %s metric %d equal %s discard %s policy %s\n",
	       cstring(net), cstring(nexthop), metric, bool_c_str(equal),
	       bool_c_str(discard), cstring(policytags));

    return _io->replace_route(net, nexthop, nexthop_id, metric, equal, discard,
			      policytags);
}

template <typename A>
bool
Ospf<A>::delete_route(IPNet<A> net)
{
    debug_msg("Net %s\n", cstring(net));

    XLOG_TRACE(trace()._routes, "Delete route Net %s\n", cstring(net));

    return _io->delete_route(net);
}

template <typename A>
void
Ospf<A>::configure_filter(const uint32_t& filter, const string& conf)
{
    _policy_filters.configure(filter,conf);
}

template <typename A>
void
Ospf<A>::reset_filter(const uint32_t& filter)
{
    _policy_filters.reset(filter);
}

template <typename A>
void
Ospf<A>::push_routes()
{
    _peer_manager.external_push_routes();
    _routing_table.push_routes();
}

template <typename A>
bool 
Ospf<A>::originate_route(const IPNet<A>& net,
			 const A& nexthop,
			 const uint32_t& metric,
			 const PolicyTags& policytags)
{
    return _peer_manager.external_announce(net, nexthop, metric, policytags);
}

template <typename A>
bool 
Ospf<A>::withdraw_route(const IPNet<A>& net)
{
    return _peer_manager.external_withdraw(net);
}

template <typename A>
void
Ospf<A>::set_router_id(OspfTypes::RouterID id)
{
    _peer_manager.router_id_changing();
    _router_id = id;
}

template class Ospf<IPv4>;
template class Ospf<IPv6>;
