// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/peer_data.cc,v 1.4 2003/01/24 22:14:45 rizzo Exp $"

#include "bgp_module.h"
#include "config.h"
#include "libxorp/debug.h"

#include "peer_data.hh"

BGPPeerData::BGPPeerData()
    // XXX assigning a default value here is bad
    : _as_num(AsNum::AS_INVALID)
{
    _unsupported_parameters = false;
    //    add_sent_parameter( new BGPMultiProtocolCapability( AFI_IPV4 , SAFI_NLRI_UNICAST ) );
}

BGPPeerData::BGPPeerData(const Iptuple& iptuple, AsNum as,
			 const IPv4& next_hop, const uint16_t holdtime)
    : _iptuple(iptuple), _as_num(as)
{
    set_v4_local_addr(next_hop);
    set_configured_hold_time(holdtime);

    set_retry_duration(2 * 60 * 1000);	// Connect retry time.

    assert(_negotiated_parameters.begin() == _negotiated_parameters.end());

    // we support routing of IPv4 unicast
    // add_sent_parameter( new BGPMultiProtocolCapability( AFI_IPV4 , SAFI_NLRI_UNICAST ) );
    // we support route refresh
    // add_sent_parameter( new BGPRefreshCapability() );
    // add_sent_parameter( new BGPMultiRouteCapability() );
}

BGPPeerData::~BGPPeerData()
{
    list <const BGPParameter*>::iterator iter;
    iter = _negotiated_parameters.begin();
    while (iter != _negotiated_parameters.end()) {
	delete *iter;
	++iter;
    }
    iter = _sent_parameters.begin();
    while (iter != _sent_parameters.end()) {
	delete *iter;
	++iter;
    }
    iter = _recv_parameters.begin();
    while (iter != _recv_parameters.end()) {
	delete *iter;
	++iter;
    }
}

void
BGPPeerData::set_id(const IPv4& id)
{
    debug_msg("\n");
    _id = id;
}

const
IPv4&
BGPPeerData::get_id() const
{
    debug_msg("\n");
    return _id;
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

const AsNum&
BGPPeerData::get_as_num() const
{
    debug_msg("AS num retrieved as %s\n", _as_num.str().c_str());
    return _as_num;
}

void
BGPPeerData::set_as_num(const AsNum& a)
{
    debug_msg("AS num set as %d\n", a.as());
    _as_num = a;
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
			   list<const BGPParameter*>& p_list)
{
    debug_msg("add_parameter %s\n", p->str().c_str());
    debug_msg("%x\n", (uint)p);
    p_list.push_back(p);
    // p->dump_contents();
    _num_parameters++;
    // TODO add bounds checking
    // XXX not sure here.
    _param_length += p->paramlength();
}

void
BGPPeerData::remove_parameter(const BGPParameter* p,
			      list<const BGPParameter*>& p_list)
{
    list <const BGPParameter*>::iterator iter;
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

bool
BGPPeerData::unsupported_parameters() const
{
    /* Currently we accept all parameters and capabilities sent */
    /* later we should put the negogiation code in here */
    debug_msg("BGPPeerData unsupported parameters called\n");

    return false;
}

void
BGPPeerData::clone_parameters(const list< BGPParameter*>&
			      parameter_list)
{
    list < BGPParameter*>::const_iterator iter;
    iter = parameter_list.begin();
    while (iter != parameter_list.end()) {
	BGPParameter *clone;
	clone = (*iter)->clone();
	if (clone != NULL)
	    add_recv_parameter(clone);
	++iter;
    }
}

void
BGPPeerData::dump_peer_data() const
{
    debug_msg("Iptuple: %s AS: %s\n", iptuple().str().c_str(),
	      get_as_num().str().c_str());
}
