// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/peer_data.cc,v 1.7 2003/03/10 23:20:01 hodson Exp $"

// #define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "config.h"
#include "libxorp/debug.h"

#include "peer_data.hh"

BGPPeerData::BGPPeerData(const Iptuple& iptuple, AsNum as,
			 const IPv4& next_hop, const uint16_t holdtime)
    : _iptuple(iptuple), _as(as)
{
    _unicast_ipv4 = _unicast_ipv6 = _multicast_ipv4 = _multicast_ipv6 = false;

    set_v4_local_addr(next_hop);
    set_configured_hold_time(holdtime);

    set_retry_duration(2 * 60 * 1000);	// Connect retry time.

    // we support routing of IPv4 unicast
//     add_sent_parameter(
// 	    new BGPMultiProtocolCapability(AFI_IPV4, SAFI_NLRI_UNICAST));
    // we support route refresh
    // add_sent_parameter( new BGPRefreshCapability() );
    // add_sent_parameter( new BGPMultiRouteCapability() );
}

BGPPeerData::~BGPPeerData()
{
}


// Set whether a peer is internal or external
void
BGPPeerData::set_internal_peer(bool i)
{
    debug_msg("Internal peer set as %i\n", i);
    _internal = i;
}

bool
BGPPeerData::get_internal_peer() const
{
    debug_msg("Internal Peer retrieved as %i\n", _internal);
    return _internal;
}

uint32_t
BGPPeerData::get_hold_duration() const
{
    debug_msg("BGP hold duration retrieved as %d\n", _hold_duration);
    return _hold_duration;
}

void
BGPPeerData::set_hold_duration(uint32_t d)
{
    _hold_duration = d;
    debug_msg("BGP hold duration set as %d\n", _hold_duration);
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
BGPPeerData::add_parameter(const BGPParameter* p,
			   ParameterList& p_list)
{
    debug_msg("add_parameter %s\n", p->str().c_str());
    debug_msg("%p\n", p);
    p_list.push_back(p);
}

void
BGPPeerData::remove_parameter(const BGPParameter* p,
			      ParameterList& p_list)
{
    ParameterList::iterator iter;
    iter = p_list.begin();
    while (iter != p_list.end()) {
	if (*iter == p) {
	    // XXX I think we have to delete it here, because no-one
	    // else should be storing this, but there's no way to be
	    // sure.
	    debug_msg("deleting parameter %x\n", (uint)p);
	    delete p;
	    p_list.erase(iter);
	    break;
	}
	++iter;
    }
}

void
BGPPeerData::save_parameters(const ParameterList& parameter_list)
{
    ParameterList::const_iterator iter;
    iter = parameter_list.begin();
    while (iter != parameter_list.end()) {
	add_recv_parameter(iter->get());
	++iter;
    }
}

void
BGPPeerData::open_negotiation()
{
    XLOG_WARNING("Need to complete negotiation code");
}

void
BGPPeerData::dump_peer_data() const
{
    debug_msg("Iptuple: %s AS: %s\n", iptuple().str().c_str(),
	      as().str().c_str());
}
