// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/bgp/peer_data.cc,v 1.24 2005/09/29 00:10:22 atanu Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include <functional>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bgp_module.h"

#include "libxorp/debug.h"
#include "peer_data.hh"

BGPPeerData::BGPPeerData(const Iptuple& iptuple, AsNum as,
			 const IPv4& next_hop, const uint16_t holdtime,
                         const PeerType peer_type)
    : _iptuple(iptuple), _as(as),
      _hold_duration(0), _retry_duration(0), _keepalive_duration(0),
      _peer_type(peer_type)
{
    set_v4_local_addr(next_hop);
    set_configured_hold_time(holdtime);

    set_retry_duration(2 * 60);	// Connect retry time.

    // we support routing of IPv4 unicast by default.
    add_sent_parameter(new BGPMultiProtocolCapability(AFI_IPV4, SAFI_UNICAST));

    // The peer has no way of telling us that it doesn't want IPv4
    // unicast. So we have to configure the received parameter ourselves.
    add_recv_parameter(new BGPMultiProtocolCapability(AFI_IPV4, SAFI_UNICAST));

    // we support route refresh
    // add_sent_parameter( new BGPRefreshCapability() );
    // add_sent_parameter( new BGPMultiRouteCapability() );

    // Call this here to initialize all the state.
    open_negotiation();
}

BGPPeerData::~BGPPeerData()
{
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


// Set type of peering session
void
BGPPeerData::set_peer_type(PeerType t)
{
    _peer_type = t;
    debug_msg("Peer type set as %s\n", get_peer_type_str().c_str());
}



bool
BGPPeerData::ibgp() const
{
    return _peer_type == PEER_TYPE_IBGP || _peer_type == PEER_TYPE_IBGP_CLIENT;
}

bool
BGPPeerData::ibgp_vanilla() const
{
    return _peer_type == PEER_TYPE_IBGP;
}

bool
BGPPeerData::ibgp_client() const
{
    return _peer_type == PEER_TYPE_IBGP_CLIENT;
}

bool
BGPPeerData::ebgp_vanilla() const
{
    return _peer_type == PEER_TYPE_EBGP;
}

bool
BGPPeerData::ebgp_confed() const
{
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
#if	0
    copy(plist.begin(), plist.end(),
	 inserter(_recv_parameters, _recv_parameters.begin()));
#else
    for(ParameterList::const_iterator i = plist.begin(); i != plist.end(); i++)
	add_recv_parameter(*i);
#endif
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
}

void
BGPPeerData::dump_peer_data() const
{
    debug_msg("Iptuple: %s AS: %s\n", iptuple().str().c_str(),
	      as().str().c_str());
}
