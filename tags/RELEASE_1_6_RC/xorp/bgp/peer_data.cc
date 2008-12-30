// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/bgp/peer_data.cc,v 1.36 2008/07/23 05:09:34 pavlin Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"

#include <functional>

#include "peer_data.hh"


BGPPeerData::BGPPeerData(const LocalData& local_data, const Iptuple& iptuple,
			 AsNum as,
			 const IPv4& next_hop, const uint16_t holdtime)
    : _local_data(local_data), _iptuple(iptuple), _as(as),
      _use_4byte_asnums(false),
      _route_reflector(false), _confederation(false),
      _prefix_limit(0, false),
      _delay_open_time(0),
      _hold_duration(0), _retry_duration(0), _keepalive_duration(0),
      _peer_type(PEER_TYPE_UNCONFIGURED)
{
    set_v4_local_addr(next_hop);
    set_configured_hold_time(holdtime);

    set_retry_duration(2 * 60);	// Connect retry time.

    // if we're configured to use 4byte AS numbers, make sure we send
    // the relevant capability.
    if (_local_data.use_4byte_asnums()) {
	add_sent_parameter( new BGP4ByteASCapability(as) );
    }

    // we support route refresh
    // add_sent_parameter( new BGPRefreshCapability() );
    // add_sent_parameter( new BGPMultiRouteCapability() );

    // Call this here to initialize all the state.
    open_negotiation();
}

BGPPeerData::~BGPPeerData()
{
}

const AsNum
BGPPeerData::my_AS_number() const
{
    AsNum confid = _local_data.get_confed_id();

    if (confid.as() == AsNum::AS_INVALID) {
	return _local_data.get_as();
    } else {
	if (confederation()) {
	    return _local_data.get_as();
	} else {
	    return confid;
	}
    }

    XLOG_UNREACHABLE();

    return AsNum(AsNum::AS_INVALID);
}

void
BGPPeerData::compute_peer_type()
{
    if (_local_data.get_as() == as()) {
	_peer_type = _local_data.get_route_reflector() && route_reflector() ?
	    PEER_TYPE_IBGP_CLIENT : PEER_TYPE_IBGP;
    } else {
	_peer_type = _local_data.get_confed_id().as() != AsNum::AS_INVALID &&
	    confederation() ?
	    PEER_TYPE_EBGP_CONFED : PEER_TYPE_EBGP;
    }
}

string
BGPPeerData::get_peer_type_str() const
{
    string s; 
    switch (get_peer_type()) {
    case PEER_TYPE_EBGP:
	s += "EBGP";
	break;

    case PEER_TYPE_IBGP:
	s += "IBGP";
	break;

    case PEER_TYPE_EBGP_CONFED:
	s += "Confederation EBGP";
	break;

    case PEER_TYPE_IBGP_CLIENT:
	s += "IBGP CLIENT";
	break;

    case PEER_TYPE_INTERNAL:
	XLOG_UNREACHABLE();  // this should never happen
	break;

    default:
	s += c_format("UNKNOWN(%d)", get_peer_type());
    }
    return s;
}

PeerType
BGPPeerData::get_peer_type() const
{
    debug_msg("Peer type retrieved as %s\n", get_peer_type_str().c_str());
    return _peer_type;
}

bool
BGPPeerData::ibgp() const
{
    XLOG_ASSERT(PEER_TYPE_UNCONFIGURED != _peer_type);
    return _peer_type == PEER_TYPE_IBGP || _peer_type == PEER_TYPE_IBGP_CLIENT;
}

bool
BGPPeerData::ibgp_vanilla() const
{
    XLOG_ASSERT(PEER_TYPE_UNCONFIGURED != _peer_type);
    return _peer_type == PEER_TYPE_IBGP;
}

bool
BGPPeerData::ibgp_client() const
{
    XLOG_ASSERT(PEER_TYPE_UNCONFIGURED != _peer_type);
    return _peer_type == PEER_TYPE_IBGP_CLIENT;
}

bool
BGPPeerData::ebgp_vanilla() const
{
    XLOG_ASSERT(PEER_TYPE_UNCONFIGURED != _peer_type);
    return _peer_type == PEER_TYPE_EBGP;
}

bool
BGPPeerData::ebgp_confed() const
{
    XLOG_ASSERT(PEER_TYPE_UNCONFIGURED != _peer_type);
    return _peer_type == PEER_TYPE_EBGP_CONFED;
}

uint32_t
BGPPeerData::get_hold_duration() const
{
    debug_msg("BGP hold duration retrieved as %u s\n",
	      XORP_UINT_CAST(_hold_duration));
    return _hold_duration;
}

void
BGPPeerData::set_hold_duration(uint32_t d)
{
    _hold_duration = d;
    debug_msg("BGP hold duration set as %u s\n",
	      XORP_UINT_CAST(_hold_duration));
}

uint32_t
BGPPeerData::get_retry_duration() const
{
    return _retry_duration;
}

void
BGPPeerData::set_retry_duration(uint32_t d)
{
    _retry_duration = d;
}

uint32_t
BGPPeerData::get_keepalive_duration() const
{
    return _keepalive_duration;
}

void
BGPPeerData::set_keepalive_duration(uint32_t d)
{
    _keepalive_duration = d;
}

void
BGPPeerData::add_parameter(ParameterList& p_list, const ParameterNode& p)
{
    debug_msg("add_parameter %s\n", p->str().c_str());

    // Its possible that a parameter is added more than once, so
    // remove any old instances.
    remove_parameter(p_list, p);

    p_list.push_back(p);
}

void
BGPPeerData::remove_parameter(ParameterList& p_list, const ParameterNode& p)
{
    debug_msg("remove_parameter %s\n", p->str().c_str());

    const BGPParameter *par = p.get();
    ParameterList::iterator iter;
    for(iter = p_list.begin(); iter != p_list.end(); iter++) {
	const ParameterNode& pnode = *iter;
	if (par->compare(*(pnode.get()))) {
	    debug_msg("removing %s\n", pnode.get()->str().c_str());
	    p_list.erase(iter);
	    return;
	}
    }
//     XLOG_WARNING("Could not find %s", p->str().c_str());
}

void
BGPPeerData::save_parameters(const ParameterList& plist)
{
    bool multiprotocol = false;
    ParameterList::const_iterator i;
    for (i = plist.begin(); i != plist.end(); i++) {
	add_recv_parameter(*i);
	if (dynamic_cast<const BGPMultiProtocolCapability *>((*i).get())) {
	    multiprotocol = true;
	}
    }
    // If there wasn't any MP capability parameters in the open packet, we must
    // make fallback and assume that peer supports IPv4 unicast only.    
    if (!multiprotocol)
	add_recv_parameter(new BGPMultiProtocolCapability(AFI_IPV4,
							  SAFI_UNICAST));
}

void
BGPPeerData::open_negotiation()
{
    // Set everything to false and use the parameters to fill in the values.
    _ipv4_unicast[SENT] = _ipv6_unicast[SENT]
	= _ipv4_multicast[SENT] = _ipv6_multicast[SENT] = false;

    _ipv4_unicast[RECEIVED] = _ipv6_unicast[RECEIVED]
	= _ipv4_multicast[RECEIVED] = _ipv6_multicast[RECEIVED] = false;

    _ipv4_unicast[NEGOTIATED] = _ipv6_unicast[NEGOTIATED]
	= _ipv4_multicast[NEGOTIATED] = _ipv6_multicast[NEGOTIATED] = false;

    /*
    ** Compare the parameters that we have sent against the ones we
    ** have received and place the common ones in negotiated.
    */
    _negotiated_parameters.clear();
    ParameterList::iterator iter_sent;
    ParameterList::iterator iter_recv;
    for(iter_sent = _sent_parameters.begin(); 
	iter_sent != _sent_parameters.end(); iter_sent++) {
	for(iter_recv = _recv_parameters.begin(); 
	    iter_recv != _recv_parameters.end(); iter_recv++) {
	    ParameterNode& pn_sent = *iter_sent;
	    ParameterNode& pn_recv = *iter_recv;
	    debug_msg("sent: %s recv: %s\n", pn_sent.get()->str().c_str(),
		      pn_recv.get()->str().c_str());
	    const BGPParameter *sent = pn_sent.get();
	    const BGPParameter *recv = pn_recv.get();
	    if(recv->compare(*sent)) {
		debug_msg("match\n");
		_negotiated_parameters.push_back(pn_sent);
	    }
	}
    }

    /*
    ** To save lookup time cache the multiprotocol parameters that we
    ** have sent, received and negotiated (the union of sent and
    ** received).
    */
    XLOG_ASSERT(SENT < ARRAY_SIZE);
    for(iter_sent = _sent_parameters.begin(); 
	iter_sent != _sent_parameters.end(); iter_sent++) {
	ParameterNode& pn = *iter_sent;
	if(const BGPMultiProtocolCapability *multi = 
	   dynamic_cast<const BGPMultiProtocolCapability *>(pn.get())) {
	    Afi afi = multi->get_address_family();
	    Safi safi = multi->get_subsequent_address_family_id();
	    debug_msg("sent AFI = %d SAFI = %d\n", afi, safi);
	    switch(afi) {
	    case AFI_IPV4:
		switch(safi) {
		case SAFI_UNICAST:
		    _ipv4_unicast[SENT] = true;
		    break;
		case SAFI_MULTICAST:
		    _ipv4_multicast[SENT] = true;
		    break;
		}
		break;
	    case AFI_IPV6:
		switch(safi) {
		case SAFI_UNICAST:
		    _ipv6_unicast[SENT] = true;
		    break;
		case SAFI_MULTICAST:
		    _ipv6_multicast[SENT] = true;
		    break;
		}
		break;
	    }
	}
    }

    XLOG_ASSERT(RECEIVED < ARRAY_SIZE);
    for(iter_recv = _recv_parameters.begin(); 
	iter_recv != _recv_parameters.end(); iter_recv++) {
	ParameterNode& pn = *iter_recv;
	if(const BGPMultiProtocolCapability *multi = 
	   dynamic_cast<const BGPMultiProtocolCapability *>(pn.get())) {
	    Afi afi = multi->get_address_family();
	    Safi safi = multi->get_subsequent_address_family_id();
	    debug_msg("recv AFI = %d SAFI = %d\n", afi, safi);
	    switch(afi) {
	    case AFI_IPV4:
		switch(safi) {
		case SAFI_UNICAST:
		    _ipv4_unicast[RECEIVED] = true;
		    break;
		case SAFI_MULTICAST:
		    _ipv4_multicast[RECEIVED] = true;
		    break;
		}
		break;
	    case AFI_IPV6:
		switch(safi) {
		case SAFI_UNICAST:
		    _ipv6_unicast[RECEIVED] = true;
		    break;
		case SAFI_MULTICAST:
		    _ipv6_multicast[RECEIVED] = true;
		    break;
		}
		break;
	    }
	}
    }

    XLOG_ASSERT(NEGOTIATED < ARRAY_SIZE);
    ParameterList::iterator iter_negotiated;
    for(iter_negotiated = _negotiated_parameters.begin(); 
	iter_negotiated != _negotiated_parameters.end(); iter_negotiated++) {
 	ParameterNode& pn = *iter_negotiated;
	if(const BGPMultiProtocolCapability *multi = 
	   dynamic_cast<const BGPMultiProtocolCapability *>(pn.get())) {
	    Afi afi = multi->get_address_family();
	    Safi safi = multi->get_subsequent_address_family_id();
	    debug_msg("negotiated AFI = %d SAFI = %d\n", afi, safi);
	    switch(afi) {
	    case AFI_IPV4:
		switch(safi) {
		case SAFI_UNICAST:
		    _ipv4_unicast[NEGOTIATED] = true;
		    break;
		case SAFI_MULTICAST:
		    _ipv4_multicast[NEGOTIATED] = true;
		    break;
		}
		break;
	    case AFI_IPV6:
		switch(safi) {
		case SAFI_UNICAST:
		    _ipv6_unicast[NEGOTIATED] = true;
		    break;
		case SAFI_MULTICAST:
		    _ipv6_multicast[NEGOTIATED] = true;
		    break;
		}
		break;
	    }
	}
    }

    // if we are configured for 4 byte AS numbers, and the other side
    // sent this capability too, then extract their AS number from it.
    if (_local_data.use_4byte_asnums()) {
	for(iter_recv = _recv_parameters.begin(); 
	    iter_recv != _recv_parameters.end(); iter_recv++) {
	    ParameterNode& pn = *iter_recv;
	    if(const BGP4ByteASCapability *cap4byte = 
	       dynamic_cast<const BGP4ByteASCapability *>(pn.get())) {
		_use_4byte_asnums = true;
		_as = AsNum(cap4byte->as());
	    }
	}
    }
}

void
BGPPeerData::dump_peer_data() const
{
    debug_msg("Iptuple: %s AS: %s\n", iptuple().str().c_str(),
	      as().str().c_str());
}
