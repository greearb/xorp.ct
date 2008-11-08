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

#ident "$XORP: xorp/bgp/path_attribute.cc,v 1.93 2008/07/23 05:09:34 pavlin Exp $"

//#define DEBUG_LOGGING
//#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#include <vector>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "peer_data.hh"
#include "path_attribute.hh"
#include "packet.hh"
#include "peer.hh"
#include "bgp.hh"

template<class A> AttributeManager<A>* PAListRef<A>::_att_mgr = 0;

#ifdef DEBUG_LOGGING
inline static void
dump_bytes(const uint8_t* d, uint8_t l)
{
    for (uint16_t i = 0; i < l; i++) {
	debug_msg("%3u 0x%02x\n", i, d[i]);
    }
}
#else
inline static void dump_bytes(const uint8_t*, uint8_t) {}
#endif /* DEBUG_LOGGING */

/*
 * Flags values crib:
 *
 * Well-known mandatory: optional = false, transitive = true
 * Well-known discretionary: optional = false, transitive = true
 * Optional transitive: optional = true, transitive = true,
 * Optional nontransitive: optional = true, transitive = false
 *
 * Yes, you really can't tell the difference between well-known
 * mandatory and well-known discretionary from the flags.  They're
 * well-known, so you should know :-)
 */

/******************** DERIVED CLASSES ***************************/

/**
 * OriginAttribute
 */

OriginAttribute::OriginAttribute(OriginType t)
	: PathAttribute(Transitive, ORIGIN), _origin(t)
{
}

PathAttribute *
OriginAttribute::clone() const
{
    return new OriginAttribute(origin());
}

OriginAttribute::OriginAttribute(const uint8_t* d)
	throw(CorruptMessage)
	: PathAttribute(d)
{
    if (length(d) != 1) {
	xorp_throw(CorruptMessage,
		   c_format("OriginAttribute bad length %u",
			    XORP_UINT_CAST(length(d))),
		   UPDATEMSGERR, ATTRLEN);
    }
    if (!well_known() || !transitive()) {
	xorp_throw(CorruptMessage,
		   c_format("Bad Flags in Origin attribute %#x",flags()),
		   UPDATEMSGERR, ATTRFLAGS, d, total_tlv_length(d));
    }

    const uint8_t* data = payload(d);	// skip header.

    switch (data[0]) {
    case IGP:
    case EGP:
    case INCOMPLETE:
	_origin = (OriginType)data[0];
	break;

    default:
	xorp_throw(CorruptMessage,
		   c_format("Unknown Origin Type %d", data[0]),
		   UPDATEMSGERR, INVALORGATTR, d, total_tlv_length(d));
    }
}

bool
OriginAttribute::encode(uint8_t *buf, size_t &wire_size, const BGPPeerData* peerdata)
const
{
    UNUSED(peerdata);
    debug_msg("OriginAttribute::encode\n");
    // ensure there's enough space
    if (wire_size < 4) 
	return false; 
    uint8_t *d = set_header(buf, 1, wire_size);
    d[0] = origin();
    return true;
}

string
OriginAttribute::str() const
{
    string s = "Origin Path Attribute - ";
    switch (origin()) {
    case IGP:
	s += "IGP";
	break;
    case EGP:
	s += "EGP";
	break;
    case INCOMPLETE:
	s += "INCOMPLETE";
	break;
    default:
	s += "UNKNOWN";
    }
    return s;
}

/**
 * ASPathAttribute
 */

ASPathAttribute::ASPathAttribute(const ASPath& p)
	: PathAttribute(Transitive, AS_PATH)
{
    _as_path = new ASPath(p);
}

PathAttribute *
ASPathAttribute::clone() const
{
    return new ASPathAttribute(as_path());
}

ASPathAttribute::ASPathAttribute(const uint8_t* d, bool use_4byte_asnums)
	throw(CorruptMessage)
	: PathAttribute(d)
{
    if (!well_known() || !transitive())
	xorp_throw(CorruptMessage,
		   c_format("Bad Flags in AS Path attribute %#x", flags()),
		   UPDATEMSGERR, ATTRFLAGS, d, total_tlv_length(d));

    if (use_4byte_asnums)
	_as_path = new AS4Path(payload(d), length(d));
    else
	_as_path = new ASPath(payload(d), length(d));
}


/**
 * see note in aspath.hh on using 4-byte AS numbers
 */
bool
ASPathAttribute::encode(uint8_t *buf, size_t &wire_size, const BGPPeerData* peerdata)
const
{
    debug_msg("ASPathAttribute encode(peerdata=%p)\n", peerdata);
    bool enc_4byte_asnums = false;
    if (peerdata == NULL) {
	/* when we're hashing the attributes, we don't know the peer.
	   Use the most general form, which is 4-byte ASnums */
	enc_4byte_asnums = true;
    } else if (peerdata->use_4byte_asnums() && peerdata->we_use_4byte_asnums()) {
	// both us and the peer use 4-byte AS numbers, so encode using
	// 4-byte ASnums.
	enc_4byte_asnums = true;
    }
    

    if (enc_4byte_asnums) {
	size_t l = as4_path().wire_size();
	if (l + 4 >= wire_size) {
	    // There's not enough space to encode this.
	    return false;
	}

	uint8_t *d = set_header(buf, l, wire_size);	// set and skip header
	as4_path().encode(l, d);	// encode the payload in the buffer
    } else {
	// either we're not using 4-byte AS numbers, or the peer isn't
	// so encode as two-byte AS nums.  If we've got any 4-byte AS
	// numbers in there, they'll be mapped to AS_TRAN.
	size_t l = as_path().wire_size();
	if (l + 4 >= wire_size) {
	    // There's not enough space to encode this.
	    return false;
	}

	uint8_t *d = set_header(buf, l, wire_size);	// set and skip header
	as_path().encode(l, d);	// encode the payload in the buffer
    }
    return true;
}



/*
 * AS4 Path Attribute - see note in aspath.hh for usage details
 */

AS4PathAttribute::AS4PathAttribute(const AS4Path& p)
	: PathAttribute((Flags)(Optional|Transitive), AS4_PATH)
{
    _as_path = new AS4Path(p);
}

AS4PathAttribute::AS4PathAttribute(const uint8_t* d)
	throw(CorruptMessage)
	: PathAttribute(d)
{
    if (!optional() || !transitive())
	xorp_throw(CorruptMessage,
		   c_format("Bad Flags in AS4 Path attribute %#x", flags()),
		   UPDATEMSGERR, ATTRFLAGS, d, total_tlv_length(d));

    _as_path = new AS4Path(payload(d), length(d));
}

PathAttribute *
AS4PathAttribute::clone() const
{
    return new AS4PathAttribute(as4_path());
}

bool
AS4PathAttribute::encode(uint8_t *buf, size_t &wire_size, const BGPPeerData* peerdata)
const
{
    // If we're encoding this, it's because we're not configured to do
    // 4-byte AS numbers, but we received a 4-byte AS Path from a peer
    // and now we're sending it on to another peer unchanged.  So this
    // should alwasy be encoded as a 4-byte AS Path, no matter who
    // we're sending it to.
    
    UNUSED(peerdata);
    debug_msg("AS4PathAttribute encode()\n");
    size_t l = as4_path().wire_size();
    if (l + 4 >= wire_size) {
	// There's not enough space to encode this.
	return false;
    }

    uint8_t *d = set_header(buf, l, wire_size);	// set and skip header
    as4_path().encode(l, d);	// encode the payload in the buffer
    return true;
}

/**
 * NextHopAttribute
 */

template <class A>
NextHopAttribute<A>::NextHopAttribute(const A& n)
	: PathAttribute(Transitive, NEXT_HOP), _next_hop(n)
{
}

template <class A>
PathAttribute *
NextHopAttribute<A>::clone() const
{
    return new NextHopAttribute(_next_hop);
}

template <class A>
NextHopAttribute<A>::NextHopAttribute(const uint8_t* d)
	throw(CorruptMessage)
	: PathAttribute(d)
{
    if (!well_known() || !transitive())
	xorp_throw(CorruptMessage,
		   c_format("Bad Flags in NextHop attribute %#x", flags()),
		   UPDATEMSGERR, ATTRFLAGS, d, total_tlv_length(d));
    if (length(d) != A::addr_bytelen()) {
	xorp_throw(CorruptMessage, 
		   c_format("Bad size in NextHop address, was %u, should be %u",
			    (uint32_t)length(d), (uint32_t)A::addr_bytelen()),
		   UPDATEMSGERR, ATTRLEN);
    }

    _next_hop = A(payload(d));

    if (!_next_hop.is_unicast())
	xorp_throw(CorruptMessage,
		   c_format("NextHop %s is not a unicast address",
			    _next_hop.str().c_str()),
		   UPDATEMSGERR, INVALNHATTR, d, total_tlv_length(d));
}

template<class A>
bool
NextHopAttribute<A>::encode(uint8_t *buf, size_t &wire_size, 
			    const BGPPeerData* peerdata) const
{
    UNUSED(peerdata);
    debug_msg("NextHopAttribute<A>::encode\n");
    if (wire_size <= 3 + A::addr_bytelen()) {
	return false;
    }
    uint8_t *d = set_header(buf, A::addr_bytelen(), wire_size);
    _next_hop.copy_out(d);
    return true;
}

template class NextHopAttribute<IPv4>;
template class NextHopAttribute<IPv6>;


/**
 * MEDAttribute
 */

MEDAttribute::MEDAttribute(const uint32_t med)
	: PathAttribute(Optional, MED), _med(med)
{
}

PathAttribute *
MEDAttribute::clone() const
{
    return new MEDAttribute(med());
}

MEDAttribute::MEDAttribute(const uint8_t* d) throw(CorruptMessage)
    : PathAttribute(d)
{
    if (!optional() || transitive())
	xorp_throw(CorruptMessage,
		   c_format("Bad Flags in MEDAttribute %#x", flags()),
		   UPDATEMSGERR, ATTRFLAGS, d, total_tlv_length(d));
    if (length(d) != 4)
	xorp_throw(CorruptMessage, "Bad size in MEDAttribute",
		   UPDATEMSGERR, ATTRLEN);
    memcpy(&_med, payload(d), 4);
    _med = ntohl(_med);
}

bool
MEDAttribute::encode(uint8_t *buf, size_t &wire_size, const BGPPeerData* peerdata)
const
{
    debug_msg("MEDAttribute::encode\n");
    UNUSED(peerdata);
    if (wire_size <= 7) {
	return false;
    }
    uint8_t *d = set_header(buf, 4, wire_size);
    uint32_t x = htonl(_med);
    memcpy(d, &x, 4);
    return true;
}

string
MEDAttribute::str() const
{
    return c_format("Multiple Exit Descriminator Attribute: MED=%u",
		    XORP_UINT_CAST(med()));
}

/**
 * LocalPrefAttribute
 */

LocalPrefAttribute::LocalPrefAttribute(const uint32_t localpref)
	: PathAttribute(Transitive, LOCAL_PREF), _localpref(localpref)
{
}

PathAttribute *
LocalPrefAttribute::clone() const
{
    return new LocalPrefAttribute(localpref());
}

LocalPrefAttribute::LocalPrefAttribute(const uint8_t* d)
	throw(CorruptMessage)
	: PathAttribute(d)
{
    if (!well_known() || !transitive())
	xorp_throw(CorruptMessage,
		   c_format("Bad Flags in LocalPrefAttribute %#x", flags()),
		   UPDATEMSGERR, ATTRFLAGS, d, total_tlv_length(d));
    if (length(d) != 4)
	xorp_throw(CorruptMessage, "Bad size in LocalPrefAttribute",
		   UPDATEMSGERR, ATTRLEN);
    memcpy(&_localpref, payload(d), 4);
    _localpref = ntohl(_localpref);
}

bool
LocalPrefAttribute::encode(uint8_t *buf, size_t &wire_size, 
			   const BGPPeerData* peerdata) const
{
    debug_msg("LocalPrefAttribute::encode\n");
    UNUSED(peerdata);
    if (wire_size < 7) {
	return false;
    }
    uint8_t *d = set_header(buf, 4, wire_size);
    uint32_t x = htonl(_localpref);
    memcpy(d, &x, 4);
    return true;
}

string
LocalPrefAttribute::str() const
{
    return c_format("Local Preference Attribute - %u",
		    XORP_UINT_CAST(_localpref));
}

/**
 * AtomicAggAttribute has length 0
 */

AtomicAggAttribute::AtomicAggAttribute()
	: PathAttribute(Transitive, ATOMIC_AGGREGATE)
{
}

bool
AtomicAggAttribute::encode(uint8_t *buf, size_t &wire_size, 
			   const BGPPeerData* peerdata) const
{
    debug_msg("AAggAttribute::encode\n");
    UNUSED(peerdata);
    if (wire_size < 3) {
	return false;
    }
    set_header(buf, 0, wire_size);
    return true;
}

PathAttribute *
AtomicAggAttribute::clone() const
{
    return new AtomicAggAttribute();
}

AtomicAggAttribute::AtomicAggAttribute(const uint8_t* d)
	throw(CorruptMessage)
	: PathAttribute(d)
{
    if (length(d) != 0)
	xorp_throw(CorruptMessage,
		   c_format("AtomicAggregate bad length %u",
			    XORP_UINT_CAST(length(d))),
		   UPDATEMSGERR, ATTRLEN);
    if (!well_known() || !transitive())
	xorp_throw(CorruptMessage,
		   c_format("Bad Flags in AtomicAggregate attribute %#x",
			    flags()),
		   UPDATEMSGERR, ATTRFLAGS, d, total_tlv_length(d));
}

/**
 * AggregatorAttribute
 */

AggregatorAttribute::AggregatorAttribute(const IPv4& speaker,
			const AsNum& as)
	: PathAttribute((Flags)(Optional|Transitive), AGGREGATOR),
		_speaker(speaker), _as(as)            
{
}

PathAttribute *
AggregatorAttribute::clone() const
{
    return new AggregatorAttribute(route_aggregator(), aggregator_as());
}

AggregatorAttribute::AggregatorAttribute(const uint8_t* d, bool use_4byte_asnums)
	throw(CorruptMessage)
	: PathAttribute(d), _as(AsNum::AS_INVALID)
{
    if (!use_4byte_asnums && length(d) != 6)
	xorp_throw(CorruptMessage,
		   c_format("Aggregator bad length %u",
			    XORP_UINT_CAST(length(d))),
		   UPDATEMSGERR, ATTRLEN);
    if (use_4byte_asnums && length(d) != 8)
	xorp_throw(CorruptMessage,
		   c_format("Aggregator bad length %u",
			    XORP_UINT_CAST(length(d))),
		   UPDATEMSGERR, ATTRLEN);
    if (!optional() || !transitive())
	xorp_throw(CorruptMessage,
		   c_format("Bad Flags in AtomicAggregate attribute %#x",
			    flags()),
		   UPDATEMSGERR, ATTRFLAGS, d, total_tlv_length(d));
    d = payload(d);
    _as = AsNum(d, use_4byte_asnums);
    if (use_4byte_asnums)
	_speaker = IPv4(d+4);
    else {
	_speaker = IPv4(d+2);
    }
}

bool
AggregatorAttribute::encode(uint8_t *buf, size_t &wire_size, 
			    const BGPPeerData* peerdata) const
{
    debug_msg("AggAttribute::encode\n");
    bool enc_4byte_asnums = false;
    if (peerdata == NULL) {
	/* when we're hashing the attributes, we don't know the peer.
	   Use the most general form, which is 4-byte ASnums */
	enc_4byte_asnums = true;
    } else if (peerdata->use_4byte_asnums() && peerdata->we_use_4byte_asnums()) {
	// both us and the peer use 4-byte AS numbers, so encode using
	// 4-byte ASnums.
	enc_4byte_asnums = true;
    }

    if (enc_4byte_asnums) {
	if (wire_size < 11) 
	    return false;
	uint8_t *d = set_header(buf, 8, wire_size);
	_as.copy_out4(d);
	_speaker.copy_out(d + 4); // defined to be an IPv4 address in RFC 2858
    } else {
	if (wire_size < 9) 
	    return false;
	uint8_t *d = set_header(buf, 6, wire_size);
	_as.copy_out(d);
	_speaker.copy_out(d + 2); // defined to be an IPv4 address in RFC 2858
    }
    return true;
}

string AggregatorAttribute::str() const
{
    return "Aggregator Attribute " + _as.str() + " " + _speaker.str();
}


/**
 * AS4AggregatorAttribute
 */

AS4AggregatorAttribute::AS4AggregatorAttribute(const IPv4& speaker,
			const AsNum& as)
	: PathAttribute((Flags)(Optional|Transitive), AS4_AGGREGATOR),
		_speaker(speaker), _as(as)            
{
}

PathAttribute *
AS4AggregatorAttribute::clone() const
{
    return new AS4AggregatorAttribute(route_aggregator(), aggregator_as());
}

AS4AggregatorAttribute::AS4AggregatorAttribute(const uint8_t* d)
	throw(CorruptMessage)
	: PathAttribute(d), _as(AsNum::AS_INVALID)
{
    if (length(d) != 8)
	xorp_throw(CorruptMessage,
		   c_format("AS4Aggregator bad length %u",
			    XORP_UINT_CAST(length(d))),
		   UPDATEMSGERR, ATTRLEN);
    if (!optional() || !transitive())
	xorp_throw(CorruptMessage,
		   c_format("Bad Flags in AtomicAggregate attribute %#x",
			    flags()),
		   UPDATEMSGERR, ATTRFLAGS, d, total_tlv_length(d));
    d = payload(d);
    _as = AsNum(d, true); //force interpretation as a 4-byte quantity
    _speaker = IPv4(d+4);
}

bool
AS4AggregatorAttribute::encode(uint8_t *buf, size_t &wire_size, 
			    const BGPPeerData* peerdata) const
{
    debug_msg("AS4AggAttribute::encode\n");
    if (wire_size < 11) 
	return false;

    if (peerdata && 
	peerdata->use_4byte_asnums() && peerdata->we_use_4byte_asnums()) {
	// we're configured to do 4-byte AS numbers and our peer is
	// also configured to do them.  We should not be encoding 4
	// byte AS numbers in an AS4AggregatorAttribute, but rather
	// doing native 4 byte AS numbers in a regular ASaggregator.
	XLOG_UNREACHABLE();  
    }

    uint8_t *d = set_header(buf, 8, wire_size);
    _as.copy_out4(d);
    _speaker.copy_out(d + 4); // defined to be an IPv4 address in RFC 2858
    return true;
}

string AS4AggregatorAttribute::str() const
{
    return "AS4Aggregator Attribute " + _as.str() + " " + _speaker.str();
}


/**
 * CommunityAttribute
 */

CommunityAttribute::CommunityAttribute()
	: PathAttribute((Flags)(Optional | Transitive), COMMUNITY)
{
}

PathAttribute *
CommunityAttribute::clone() const
{
    CommunityAttribute *ca = new CommunityAttribute();
    for(const_iterator i = community_set().begin(); 
	i != community_set().end(); i++)
	ca->add_community(*i);

    return ca;
}

CommunityAttribute::CommunityAttribute(const uint8_t* d)
	throw(CorruptMessage)
	: PathAttribute(d)
{
    if (!optional() || !transitive())
	xorp_throw(CorruptMessage,
		   "Bad Flags in Community attribute",
		   UPDATEMSGERR, ATTRFLAGS);
    size_t len = length(d);
    d = payload(d);
    for (size_t l = len; l >= 4;  d += 4, l -= 4) {
	uint32_t value;
	memcpy(&value, d, 4);
	_communities.insert(ntohl(value));
    }
}

bool
CommunityAttribute::encode(uint8_t *buf, size_t &wire_size, 
			   const BGPPeerData* peerdata) const
{
    debug_msg("CommAttribute::encode\n");
    UNUSED(peerdata);
    size_t size = 4 * _communities.size();
    if (wire_size < size + 4) {
	return false;
    }

    uint8_t *d = set_header(buf, size, wire_size);
    const_iterator i = _communities.begin();
    for (; i != _communities.end(); d += 4, i++) {
	uint32_t value = htonl(*i);
	memcpy(d, &value, 4);
    }
    return true;
}

string
CommunityAttribute::str() const
{
    string s = "Community Attribute ";
    const_iterator i = _communities.begin();
    for ( ; i != _communities.end();  ++i) {
	switch (*i) {
	case NO_EXPORT:
	    s += "NO_EXPORT ";
	    break;
	case NO_ADVERTISE:
	    s += "NO_ADVERTISE ";
	    break;
	case NO_EXPORT_SUBCONFED:
	    s += "NO_EXPORT_SUBCONFED ";
	    break;
	}
	s += c_format("%d:%d %#x ", XORP_UINT_CAST(((*i >> 16) & 0xffff)),
		      XORP_UINT_CAST((*i & 0xffff)),
		      XORP_UINT_CAST(*i));
    }
    return s;
}

void
CommunityAttribute::add_community(uint32_t community)
{
    _communities.insert(community);
}

bool
CommunityAttribute::contains(uint32_t community) const
{
    const_iterator i = _communities.find(community);
    if (i == _communities.end())
	return false;
    return true;
}


/**
 * OriginatorIDAttribute
 */

OriginatorIDAttribute::OriginatorIDAttribute(const IPv4 originator_id)
	: PathAttribute(Optional, ORIGINATOR_ID), _originator_id(originator_id)
{
}

PathAttribute *
OriginatorIDAttribute::clone() const
{
    return new OriginatorIDAttribute(originator_id());
}

OriginatorIDAttribute::OriginatorIDAttribute(const uint8_t* d)
    throw(CorruptMessage)
    : PathAttribute(d)
{
    if (!optional() || transitive())
	xorp_throw(CorruptMessage, "Bad Flags in OriginatorIDAttribute",
		   UPDATEMSGERR, ATTRFLAGS);
    if (length(d) != 4)
	xorp_throw(CorruptMessage, "Bad size in OriginatorIDAttribute",
		   UPDATEMSGERR, INVALNHATTR);

    _originator_id.copy_in(payload(d));
}

bool
OriginatorIDAttribute::encode(uint8_t *buf, size_t &wire_size, 
			      const BGPPeerData* peerdata) const
{
    debug_msg("OriginatorIDAttribute::encode\n");
    UNUSED(peerdata);
    if (wire_size < 7) {
	return false;
    }
    uint8_t *d = set_header(buf, 4, wire_size);
    _originator_id.copy_out(d);
    return true;
}

string
OriginatorIDAttribute::str() const
{
    return c_format("ORIGINATOR ID Attribute: %s", cstring(originator_id()));
}

/**
 * ClusterListAttribute
 */

ClusterListAttribute::ClusterListAttribute()
	: PathAttribute(Optional, CLUSTER_LIST)
{
}

ClusterListAttribute::ClusterListAttribute(const uint8_t* d)
	throw(CorruptMessage)
	: PathAttribute(d)
{
    if (!optional() || transitive())
	xorp_throw(CorruptMessage,
		   "Bad Flags in CLUSTER_LIST attribute",
		   UPDATEMSGERR, ATTRFLAGS);
    size_t size = length(d);
    d = payload(d);
    for (size_t l = size; l >= 4;  d += 4, l -= 4) {
	IPv4 i;
	i.copy_in(d);
	_cluster_list.push_back(i);
    }
}

PathAttribute *
ClusterListAttribute::clone() const
{
    ClusterListAttribute *ca = new ClusterListAttribute();
    list<IPv4>::const_reverse_iterator i = cluster_list().rbegin();
    for(; i != cluster_list().rend(); i++)
	ca->prepend_cluster_id(*i);
    return ca;
}

bool
ClusterListAttribute::encode(uint8_t *buf, size_t &wire_size, 
			     const BGPPeerData* peerdata) const
{
    debug_msg("CLAttribute::encode\n");
    UNUSED(peerdata);
    size_t size = 4 * _cluster_list.size();
    XLOG_ASSERT(size < 256);
    if (wire_size < size + 4) {
	return false;
    }

    uint8_t *d = set_header(buf, size, wire_size);
    const_iterator i = _cluster_list.begin();
    for (; i != _cluster_list.end(); d += 4, i++) {
	i->copy_out(d);
    }
    return true;
}

string
ClusterListAttribute::str() const
{
    string s = "Cluster List Attribute ";
    const_iterator i = _cluster_list.begin();
    for ( ; i != _cluster_list.end();  ++i)
	s += c_format("%s ", cstring(*i));
    return s;
}

void
ClusterListAttribute::prepend_cluster_id(IPv4 cluster_id)
{
    _cluster_list.push_front(cluster_id);
}

bool
ClusterListAttribute::contains(IPv4 cluster_id) const
{
    const_iterator i = find(_cluster_list.begin(), _cluster_list.end(),
			    cluster_id);
    if (i == _cluster_list.end())
	return false;
    return true;
}

/**
 * Multiprotocol Reachable NLRI - MP_REACH_NLRI (Type Code 14):
 *
 *  This is an optional non-transitive attribute that can be used for the
 *  following purposes:
 *
 *    (a) to advertise a feasible route to a peer
 *
 *    (b) to permit a router to advertise the Network Layer address of
 *    the router that should be used as the next hop to the destinations
 *    listed in the Network Layer Reachability Information field of the
 *    MP_NLRI attribute.
 *
 *    (c) to allow a given router to report some or all of the
 *    Subnetwork Points of Attachment (SNPAs) that exist within the
 *    local system
 */

template <>
bool
MPReachNLRIAttribute<IPv6>::encode(uint8_t *buf, size_t &wire_size, 
				   const BGPPeerData* peerdata) const
{
    debug_msg("MPReachAttribute::encode\n");
    UNUSED(peerdata);

    XLOG_ASSERT(AFI_IPV6 == _afi);
    XLOG_ASSERT((SAFI_UNICAST  == _safi) || (SAFI_MULTICAST == _safi));
    XLOG_ASSERT(16 == IPv6::addr_bytelen());

    /*
    ** Figure out how many bytes we need to allocate.
    */
    size_t len;
    len = 2;	// AFI
    len += 1;	// SAFI
    len += 1;	// Length of Next Hop Address
    len += IPv6::addr_bytelen();	// Next Hop
    if (!_link_local_next_hop.is_zero())
	len += IPv6::addr_bytelen();
    len += 1;	// Number of SNPAs

    const_iterator i;
    for (i = _nlri.begin(); i != _nlri.end(); i++) {
	len += 1;
	len += (i->prefix_len() + 7) / 8;

	// check it will fit
	if (wire_size < len + 4) {
	    // not enough space to encode!
	    return false;
	}
    }

    uint8_t *d = set_header(buf, len, wire_size);
    
    /*
    ** Fill in the buffer.
    */
    *d++ = (_afi << 8) & 0xff;	// AFI
    *d++ = _afi & 0xff;		// AFI
    
    *d++ = _safi;		// SAFIs

    if (_link_local_next_hop.is_zero()) {
	*d++ = IPv6::addr_bytelen();
	nexthop().copy_out(d);
	d += IPv6::addr_bytelen();
    } else {
	*d++ = IPv6::addr_bytelen() * 2;
	nexthop().copy_out(d);
	d += IPv6::addr_bytelen();
	_link_local_next_hop.copy_out(d);
	d += IPv6::addr_bytelen();
    }
    
    *d++ = 0;	// Number of SNPAs

    for (i = _nlri.begin(); i != _nlri.end(); i++) {
	int bytes = (i->prefix_len() + 7)/ 8;

	// if there's no space, truncate
	len -= 1 + bytes;
	if (len <= 0)
	    break;

	debug_msg("encode %s bytes = %d\n", i->str().c_str(), bytes);
	uint8_t buf[IPv6::addr_bytelen()];
	i->masked_addr().copy_out(buf);
	
	*d++ = i->prefix_len();
	memcpy(d, buf, bytes);
	d += bytes;
    }
    return true;
}

template <>
bool
MPReachNLRIAttribute<IPv4>::encode(uint8_t *buf, size_t &wire_size, 
				   const BGPPeerData* peerdata) const
{
    debug_msg("MPReachAttribute::encode\n");
    UNUSED(peerdata);

    XLOG_ASSERT(AFI_IPV4 == _afi && SAFI_MULTICAST == _safi);
    XLOG_ASSERT(4 == IPv4::addr_bytelen());

    /*
    ** Figure out how many bytes we need to allocate.
    */
    size_t len;
    len = 2;	// AFI
    len += 1;	// SAFI
    len += 1;	// Length of Next Hop Address
    len += IPv4::addr_bytelen();	// Next Hop
    len += 1;	// Number of SNPAs

    const_iterator i;
    for (i = _nlri.begin(); i != _nlri.end(); i++) {
	len += 1;
	len += (i->prefix_len() + 7) / 8;

	// check it will fit
	if (wire_size < len + 4) {
	    // not enough space to encode!
	    return false;
	}
    }

    uint8_t *d = set_header(buf, len, wire_size);
    
    /*
    ** Fill in the buffer.
    */
    *d++ = (_afi << 8) & 0xff;	// AFI
    *d++ = _afi & 0xff;		// AFI
    
    *d++ = _safi;		// SAFIs

    *d++ = IPv4::addr_bytelen();
    nexthop().copy_out(d);
    d += IPv4::addr_bytelen();
    
    *d++ = 0;	// Number of SNPAs

    for (i = _nlri.begin(); i != _nlri.end(); i++) {
	int bytes = (i->prefix_len() + 7) / 8;

	// if there's no space, truncate
	len -= 1 + bytes;
	if (len <= 0)
	    break;

	debug_msg("encode %s bytes = %d\n", i->str().c_str(), bytes);
	uint8_t buf[IPv4::addr_bytelen()];
	i->masked_addr().copy_out(buf);

	*d++ = i->prefix_len();
	memcpy(d, buf, bytes);
	d += bytes;
    }
    return true;
}

template <>
MPReachNLRIAttribute<IPv6>::MPReachNLRIAttribute(Safi safi)
    : PathAttribute((Flags)(Optional), MP_REACH_NLRI),
      _afi(AFI_IPV6),
      _safi(safi)
{
}

template <>
MPReachNLRIAttribute<IPv4>::MPReachNLRIAttribute(Safi safi)
    : PathAttribute((Flags)(Optional), MP_REACH_NLRI),
      _afi(AFI_IPV4),
      _safi(safi)
{
}

template <class A>
PathAttribute *
MPReachNLRIAttribute<A>::clone() const
{
    MPReachNLRIAttribute<A> *mp = new MPReachNLRIAttribute(_safi);

    mp->_afi = _afi;
    mp->_nexthop = _nexthop;
    for(const_iterator i = _nlri.begin(); i != _nlri.end(); i++)
	mp->_nlri.push_back(*i);

    return mp;
}

template <>
MPReachNLRIAttribute<IPv6>::MPReachNLRIAttribute(const uint8_t* d)
    throw(CorruptMessage)
    : PathAttribute(d)
{
    if (!optional() || transitive())
	xorp_throw(CorruptMessage,
		   "Bad Flags in Multiprotocol Reachable NLRI attribute",
		   UPDATEMSGERR, ATTRFLAGS);

    const uint8_t *data = payload(d);
    const uint8_t *end = payload(d) + length(d);

    uint16_t afi;
    afi = *data++;
    afi <<= 8;
    afi |= *data++;

    /*
    ** This is method is specialized for dealing with IPv6.
    */
    if (AFI_IPV6_VAL != afi)
	xorp_throw(CorruptMessage,
		   c_format("Expected AFI to be %d not %d",
			    AFI_IPV6, afi),
		   UPDATEMSGERR, OPTATTR);
    _afi = AFI_IPV6;

    uint8_t safi = *data++;
    switch(safi) {
    case SAFI_UNICAST_VAL:
	_safi = SAFI_UNICAST;
	break;
    case SAFI_MULTICAST_VAL:
	_safi = SAFI_MULTICAST;
	break;
    default:
	xorp_throw(CorruptMessage,
		   c_format("Expected SAFI to %d or %d not %d",
			    SAFI_UNICAST, SAFI_MULTICAST, _safi),
		   UPDATEMSGERR, OPTATTR);
    }

    /*
    ** Next Hop
    */
    uint8_t len = *data++;

    XLOG_ASSERT(16 == IPv6::addr_bytelen());
    IPv6 temp;

    switch(len) {
    case 16:
	temp.copy_in(data);
	set_nexthop(temp);
	data += IPv6::addr_bytelen();
	break;
    case 32:
	temp.copy_in(data);
	set_nexthop(temp);
	data += IPv6::addr_bytelen();
	_link_local_next_hop.copy_in(data);
	data += IPv6::addr_bytelen();
	break;
    default:
	xorp_throw(CorruptMessage,
		   c_format("BAD Next Hop size in "
			    "IPv6 Multiprotocol Reachable NLRI attribute "
			    "16 and 32 allowed not %u", len),
		   UPDATEMSGERR, OPTATTR);
    }
    
    if (data > end)
	xorp_throw(CorruptMessage,
		   "Premature end of Multiprotocol Reachable NLRI attribute",
		   UPDATEMSGERR, ATTRLEN);

    /*
    ** SNPA - I have no idea how these are supposed to be used for IPv6
    ** so lets just step over them.
    */
    uint8_t snpa_cnt = *data++;
    for (;snpa_cnt > 0; snpa_cnt--) {
	uint8_t snpa_length = *data++;
	data += snpa_length;
    }

    if (data > end) {
	xorp_throw(CorruptMessage,
		   "Premature end of Multiprotocol Reachable NLRI attribute",
		   UPDATEMSGERR, ATTRLEN);
    }

    /*
    ** NLRI
    */
    while(data < end) {
	uint8_t prefix_length = *data++;
	size_t bytes = (prefix_length + 7)/ 8;
	if (bytes > IPv6::addr_bytelen())
	    xorp_throw(CorruptMessage,
		       c_format("prefix length too long %d", prefix_length),
		       UPDATEMSGERR, OPTATTR);
	uint8_t buf[IPv6::addr_bytelen()];
	memset(buf, 0, sizeof(buf));
	memcpy(buf, data, bytes);
	IPv6 nlri;
	nlri.set_addr(buf);
	_nlri.push_back(IPNet<IPv6>(nlri, prefix_length));
	data += bytes;
	debug_msg("decode %s/%d bytes = %u\n", nlri.str().c_str(),
		  prefix_length, XORP_UINT_CAST(bytes));
    }

}

template <>
MPReachNLRIAttribute<IPv4>::MPReachNLRIAttribute(const uint8_t* d)
    throw(CorruptMessage)
    : PathAttribute(d)
{
    if (!optional() || transitive())
	xorp_throw(CorruptMessage,
		   "Bad Flags in Multiprotocol Reachable NLRI attribute",
		   UPDATEMSGERR, ATTRFLAGS);

    const uint8_t *data = payload(d);
    const uint8_t *end = payload(d) + length(d);

    uint16_t afi;
    afi = *data++;
    afi <<= 8;
    afi |= *data++;

    /*
    ** This is method is specialized for dealing with IPv4.
    */
    if (AFI_IPV4_VAL != afi)
	xorp_throw(CorruptMessage,
		   c_format("Expected AFI to be %d not %d",
			    AFI_IPV4, afi),
		   UPDATEMSGERR, OPTATTR);
    _afi = AFI_IPV4;

    uint8_t safi = *data++;
    switch(safi) {
    case SAFI_UNICAST_VAL:
	_safi = SAFI_UNICAST;
	break;
    case SAFI_MULTICAST_VAL:
	_safi = SAFI_MULTICAST;
	break;
    default:
	xorp_throw(CorruptMessage,
		   c_format("Expected SAFI to %d or %d not %d",
			    SAFI_UNICAST, SAFI_MULTICAST, _safi),
		   UPDATEMSGERR, OPTATTR);
    }

    // XXX - Temporary hack as SAFI_UNICAST causes problems.
    if (SAFI_UNICAST == _safi)
	xorp_throw(CorruptMessage,
		   c_format("Can't handle AFI_IPv4 and SAFI_UNICAST"),
		   UPDATEMSGERR, OPTATTR);

    /*
    ** Next Hop
    */
    uint8_t len = *data++;

    XLOG_ASSERT(4 == IPv4::addr_bytelen());
    IPv4 temp;

    switch(len) {
    case 4:
	temp.copy_in(data);
	set_nexthop(temp);
	data += IPv4::addr_bytelen();
	break;
    default:
	xorp_throw(CorruptMessage,
		   c_format("BAD Next Hop size in "
			    "IPv4 Multiprotocol Reachable NLRI attribute "
			    "4 allowed not %u", len),
		   UPDATEMSGERR, OPTATTR);
    }
    
    if (data > end)
	xorp_throw(CorruptMessage,
		   "Premature end of Multiprotocol Reachable NLRI attribute",
		   UPDATEMSGERR, ATTRLEN);

    /*
    ** SNPA - I have no idea how these are supposed to be used for IPv4
    ** so lets just step over them.
    */
    uint8_t snpa_cnt = *data++;
    for (;snpa_cnt > 0; snpa_cnt--) {
	uint8_t snpa_length = *data++;
	data += snpa_length;
    }

    if (data > end) {
	xorp_throw(CorruptMessage,
		   "Premature end of Multiprotocol Reachable NLRI attribute",
		   UPDATEMSGERR, ATTRLEN);
    }

    /*
    ** NLRI
    */
    while(data < end) {
	uint8_t prefix_length = *data++;
	size_t bytes = (prefix_length + 7) / 8;
	if (bytes > IPv4::addr_bytelen())
	    xorp_throw(CorruptMessage,
		       c_format("prefix length too long %d", prefix_length),
		       UPDATEMSGERR, OPTATTR);
	uint8_t buf[IPv4::addr_bytelen()];
	memset(buf, 0, sizeof(buf));
	memcpy(buf, data, bytes);
	IPv4 nlri;
	nlri.copy_in(buf);
	_nlri.push_back(IPNet<IPv4>(nlri, prefix_length));
	data += bytes;
	debug_msg("decode %s/%d bytes = %u\n", nlri.str().c_str(),
		  prefix_length, XORP_UINT_CAST(bytes));
    }

}

template <class A>
string
MPReachNLRIAttribute<A>::str() const
{
    string s = c_format("Multiprotocol Reachable NLRI AFI = %d SAFI = %d\n",
			_afi, _safi);
    s += c_format("   - Next Hop Attribute %s\n", nexthop().str().c_str());
    s += c_format("   - Link Local Next Hop Attribute %s",
		  link_local_nexthop().str().c_str());
    typename list<IPNet<A> >::const_iterator i = nlri_list().begin();
    for(; i != nlri_list().end(); i++)
	s += c_format("\n   - Nlri %s", i->str().c_str());
    return s;
}

template <>
bool
MPUNReachNLRIAttribute<IPv6>::encode(uint8_t *buf, size_t &wire_size, 
				     const BGPPeerData* peerdata) const
{
    debug_msg("MPUnreachAttribute::encode\n");
    UNUSED(peerdata);

    XLOG_ASSERT(AFI_IPV6 == _afi);
    XLOG_ASSERT((SAFI_UNICAST  == _safi) || (SAFI_MULTICAST == _safi));
    XLOG_ASSERT(16 == IPv6::addr_bytelen());

    /*
    ** Figure out how many bytes we need to allocate.
    */
    size_t len;
    len = 2;	// AFI
    len += 1;	// SAFI

    const_iterator i;
    for (i = _withdrawn.begin(); i != _withdrawn.end(); i++) {
	len += 1;
	len += (i->prefix_len() + 7)/ 8;

	// check it will fit
	if (wire_size < len + 4) {
	    // not enough space to encode!
	    return false;
	}
    }

    uint8_t *d = set_header(buf, len, wire_size);
    
    /*
    ** Fill in the buffer.
    */
    *d++ = (_afi << 8) & 0xff;	// AFI
    *d++ = _afi & 0xff;		// AFI

    *d++ = _safi;		// SAFIs
    
    for (i = _withdrawn.begin(); i != _withdrawn.end(); i++) {
	int bytes = (i->prefix_len() + 7)/ 8;

	// if there's no space, truncate
	len -= 1 + bytes;
	if (len <= 0)
	    break;

	debug_msg("encode %s bytes = %d\n", i->str().c_str(), bytes);
	uint8_t buf[IPv6::addr_bytelen()];
	i->masked_addr().copy_out(buf);

	*d++ = i->prefix_len();
	memcpy(d, buf, bytes);
	d += bytes;
    }
    return true;
}

template <>
bool
MPUNReachNLRIAttribute<IPv4>::encode(uint8_t *buf, size_t &wire_size, 
				     const BGPPeerData* peerdata) const
{
    debug_msg("MPUnreachNLRIAttribute::encode\n");
    UNUSED(peerdata);

    XLOG_ASSERT(AFI_IPV4 == _afi && SAFI_MULTICAST == _safi);
    XLOG_ASSERT(4 == IPv4::addr_bytelen());

    /*
    ** Figure out how many bytes we need to allocate.
    */
    size_t len;
    len = 2;	// AFI
    len += 1;	// SAFI

    const_iterator i;
    for (i = _withdrawn.begin(); i != _withdrawn.end(); i++) {
	len += 1;
	len += (i->prefix_len() + 7) / 8;
	// check it will fit
	if (wire_size < len + 4) {
	    // not enough space to encode!
	    return false;
	}
    }

    uint8_t *d = set_header(buf, len, wire_size);
    
    /*
    ** Fill in the buffer.
    */
    *d++ = (_afi << 8) & 0xff;	// AFI
    *d++ = _afi & 0xff;		// AFI

    *d++ = _safi;		// SAFIs
    
    for (i = _withdrawn.begin(); i != _withdrawn.end(); i++) {
	int bytes = (i->prefix_len() + 7) / 8;

	// if there's no space, truncate
	len -= 1 + bytes;
	if (len <= 0)
	    break;

	debug_msg("encode %s bytes = %d\n", i->str().c_str(), bytes);
	uint8_t buf[IPv4::addr_bytelen()];
	i->masked_addr().copy_out(buf);

	*d++ = i->prefix_len();
	memcpy(d, buf, bytes);
	d += bytes;
    }
    return true;
}

template <>
MPUNReachNLRIAttribute<IPv6>::MPUNReachNLRIAttribute(Safi safi)
    : PathAttribute((Flags)(Optional), MP_UNREACH_NLRI),
      _afi(AFI_IPV6),
      _safi(safi)
{
}

template <>
MPUNReachNLRIAttribute<IPv4>::MPUNReachNLRIAttribute(Safi safi)
    : PathAttribute((Flags)(Optional), MP_UNREACH_NLRI),
      _afi(AFI_IPV4),
      _safi(safi)
{
}

template <class A>
PathAttribute *
MPUNReachNLRIAttribute<A>::clone() const
{
    MPUNReachNLRIAttribute<A> *mp = new MPUNReachNLRIAttribute(_safi);

    mp->_afi = _afi;
    for(const_iterator i = _withdrawn.begin(); i != _withdrawn.end(); i++)
	mp->_withdrawn.push_back(*i);

    return mp;
}

template <>
MPUNReachNLRIAttribute<IPv6>::MPUNReachNLRIAttribute(const uint8_t* d)
    throw(CorruptMessage)
    : PathAttribute(d)
{
    if (!optional() || transitive())
	xorp_throw(CorruptMessage,
		   "Bad Flags in Multiprotocol UNReachable NLRI attribute",
		   UPDATEMSGERR, ATTRFLAGS);

    const uint8_t *data = payload(d);
    const uint8_t *end = payload(d) + length(d);

    uint16_t afi;
    afi = *data++;
    afi <<= 8;
    afi |= *data++;

    /*
    ** This is method is specialized for dealing with IPv6.
    */
    if (AFI_IPV6_VAL != afi)
	xorp_throw(CorruptMessage,
		   c_format("Expected AFI to be %d not %d",
			    AFI_IPV6, afi),
		   UPDATEMSGERR, OPTATTR);
    _afi = AFI_IPV6;

    uint8_t safi = *data++;
    switch(safi) {
    case SAFI_UNICAST_VAL:
	_safi = SAFI_UNICAST;
	break;
    case SAFI_MULTICAST_VAL:
	_safi = SAFI_MULTICAST;
	break;
    default:
	xorp_throw(CorruptMessage,
		   c_format("Expected SAFI to %d or %d not %d",
			    SAFI_UNICAST, SAFI_MULTICAST, _safi),
		   UPDATEMSGERR, OPTATTR);
    }

    /*
    ** NLRI
    */
    while(data < end) {
	uint8_t prefix_length = *data++;
	debug_msg("decode prefix length = %d\n", prefix_length);
	size_t bytes = (prefix_length + 7)/ 8;
	if (bytes > IPv6::addr_bytelen())
	    xorp_throw(CorruptMessage,
		       c_format("prefix length too long %d", prefix_length),
		       UPDATEMSGERR, OPTATTR);
	debug_msg("decode bytes = %u\n", XORP_UINT_CAST(bytes));
	uint8_t buf[IPv6::addr_bytelen()];
	memset(buf, 0, sizeof(buf));
	memcpy(buf, data, bytes);
	IPv6 nlri;
	nlri.set_addr(buf);
	_withdrawn.push_back(IPNet<IPv6>(nlri, prefix_length));
	data += bytes;
    }
}

template <>
MPUNReachNLRIAttribute<IPv4>::MPUNReachNLRIAttribute(const uint8_t* d)
    throw(CorruptMessage)
    : PathAttribute(d)
{
    if (!optional() || transitive())
	xorp_throw(CorruptMessage,
		   "Bad Flags in Multiprotocol UNReachable NLRI attribute",
		   UPDATEMSGERR, ATTRFLAGS);

    const uint8_t *data = payload(d);
    const uint8_t *end = payload(d) + length(d);

    uint16_t afi;
    afi = *data++;
    afi <<= 8;
    afi |= *data++;

    /*
    ** This is method is specialized for dealing with IPv4.
    */
    if (AFI_IPV4_VAL != afi)
	xorp_throw(CorruptMessage,
		   c_format("Expected AFI to be %d not %d",
			    AFI_IPV4, afi),
		   UPDATEMSGERR, OPTATTR);
    _afi = AFI_IPV4;

    uint8_t safi = *data++;
    switch(safi) {
    case SAFI_UNICAST_VAL:
	_safi = SAFI_UNICAST;
	break;
    case SAFI_MULTICAST_VAL:
	_safi = SAFI_MULTICAST;
	break;
    default:
	xorp_throw(CorruptMessage,
		   c_format("Expected SAFI to %d or %d not %d",
			    SAFI_UNICAST, SAFI_MULTICAST, _safi),
		   UPDATEMSGERR, OPTATTR);
    }

    // XXX - Temporary hack as SAFI_UNICAST causes problems.
    if (SAFI_UNICAST == _safi)
	xorp_throw(CorruptMessage,
		   c_format("Can't handle AFI_IPv4 and SAFI_UNICAST"),
		   UPDATEMSGERR, OPTATTR);

    /*
    ** NLRI
    */
    while(data < end) {
	uint8_t prefix_length = *data++;
	debug_msg("decode prefix length = %d\n", prefix_length);
	size_t bytes = (prefix_length + 7)/ 8;
	if (bytes > IPv4::addr_bytelen())
	    xorp_throw(CorruptMessage,
		       c_format("prefix length too long %d", prefix_length),
		       UPDATEMSGERR, OPTATTR);
	debug_msg("decode bytes = %u\n", XORP_UINT_CAST(bytes));
	uint8_t buf[IPv4::addr_bytelen()];
	memset(buf, 0, sizeof(buf));
	memcpy(buf, data, bytes);
	IPv4 nlri;
	nlri.copy_in(buf);
	_withdrawn.push_back(IPNet<IPv4>(nlri, prefix_length));
	data += bytes;
    }
}

template <class A>
string
MPUNReachNLRIAttribute<A>::str() const
{
    string s = c_format("Multiprotocol UNReachable NLRI AFI = %d SAFI = %d",
			_afi, _safi);
    typename list<IPNet<A> >::const_iterator i = wr_list().begin();
    for(; i != wr_list().end(); i++)
	s += c_format("\n   - Withdrawn %s", i->str().c_str());
    return s;
}

/**************************************************************************
 **** 
 **** PathAttribute
 **** 
 **************************************************************************/ 

UnknownAttribute::UnknownAttribute(const uint8_t* d)
	throw(CorruptMessage)
	: PathAttribute(d)
{
    // It shouldn't be possible to receive an unknown attribute that
    // is well known.
    if (well_known())
	xorp_throw(CorruptMessage,
		   "Bad Flags in Unknown attribute",
		   UPDATEMSGERR, UNRECOGWATTR, d, total_tlv_length(d));
	
    _size = total_tlv_length(d);
    _data = new uint8_t[_size];
    memcpy(_data, d, _size);
}

UnknownAttribute::UnknownAttribute(uint8_t *data, size_t size, uint8_t flags) 
    : PathAttribute(data)
{
    /* override the flags from the data because they may have been modified */
    _flags = flags;
    _size = size;
    _data = new uint8_t[_size];
    memcpy(_data, data, _size);
}

PathAttribute *
UnknownAttribute::clone() const
{
     return new UnknownAttribute(_data, _size, _flags);
}

string
UnknownAttribute::str() const
{
    string s = "Unknown Attribute ";
    for (size_t i=0; i < _size; i++)
	s += c_format("%x ", _data[i]);
    s += c_format("  flags: %x", _flags);
    return s;
}

bool
UnknownAttribute::encode(uint8_t *buf, size_t &wire_size, 
			 const BGPPeerData* peerdata) const
{
    debug_msg("UnknownAttribute::encode\n");
    UNUSED(peerdata);
    // check it will fit in the buffer
    if (wire_size < _size) 
	return false;
    memcpy(buf, _data, _size);

    /* reassert the flags because we may have changed them (typically
       to set Partial before propagating to a peer) */
    buf[0]=_flags;

    wire_size = _size;
    return true;
}

/**************************************************************************
 **** 
 **** PathAttribute
 **** 
 **************************************************************************/ 

PathAttribute *
PathAttribute::create(const uint8_t* d, uint16_t max_len,
		      size_t& l /* actual length */, const BGPPeerData* peerdata,
		      uint32_t ip_version)
	throw(CorruptMessage)
{
    PathAttribute *pa;
    if (max_len < 3) {
	// must be at least 3 bytes! 
	xorp_throw(CorruptMessage,
		   c_format("PathAttribute too short %d bytes", max_len),
		   UPDATEMSGERR, ATTRLEN, d, max_len);
    }

    // compute length, which is 1 or 2 bytes depending on flags d[0]
    if ( (d[0] & Extended) && max_len < 4) {
	xorp_throw(CorruptMessage,
		   c_format("PathAttribute (extended) too short %d bytes",
			    max_len),
		   UPDATEMSGERR, ATTRLEN, d, max_len);
    }
    l = length(d) + (d[0] & Extended ? 4 : 3);
    if (max_len < l) {
	xorp_throw(CorruptMessage,
		   c_format("PathAttribute too short %d bytes need %u",
			    max_len, XORP_UINT_CAST(l)),
		   UPDATEMSGERR, ATTRLEN, d, max_len);
    }

    // now we are sure that the data block is large enough.
    debug_msg("++ create type %d max_len %d actual_len %u\n",
	      d[1], max_len, XORP_UINT_CAST(l));

    bool use_4byte_asnums = true;
    if (peerdata) 
	use_4byte_asnums = peerdata->use_4byte_asnums();

    switch (d[1]) {	// depending on type, do the right thing.
    case ORIGIN:
	pa = new OriginAttribute(d);
	break; 

    case AS_PATH:
	pa = new ASPathAttribute(d, use_4byte_asnums);
	break;  
     
    case AS4_PATH:
	pa = new AS4PathAttribute(d);
	break;  
     
    case NEXT_HOP:
	switch (ip_version) {
	    case 4:
		pa = new IPv4NextHopAttribute(d);
		break;
	    case 6:
		pa = new IPv6NextHopAttribute(d);
		break;
	    default:
		XLOG_UNREACHABLE();
        }
	break;
    case MED:
	pa = new MEDAttribute(d);
	break;
    
    case LOCAL_PREF:
	pa = new LocalPrefAttribute(d);
	break;

    case ATOMIC_AGGREGATE:
	pa = new AtomicAggAttribute(d);
	break;

    case AGGREGATOR:
	pa = new AggregatorAttribute(d, use_4byte_asnums);
	break;

    case AS4_AGGREGATOR:
	pa = new AS4AggregatorAttribute(d);
	break;

    case COMMUNITY:
	pa = new CommunityAttribute(d);
	break;

    case ORIGINATOR_ID:
	pa = new OriginatorIDAttribute(d);
	break;

    case CLUSTER_LIST:
	pa = new ClusterListAttribute(d);
	break;

    case MP_REACH_NLRI:
	try {
	    pa = new MPReachNLRIAttribute<IPv6>(d);
	} catch(...) {
	    pa = new MPReachNLRIAttribute<IPv4>(d);
	}
	break;

    case MP_UNREACH_NLRI:
	try {
	    pa = new MPUNReachNLRIAttribute<IPv6>(d);
	} catch(...) {
	    pa = new MPUNReachNLRIAttribute<IPv4>(d);
	}
	break;
	
    default:
	pa = new UnknownAttribute(d);
	break;
    }
    return pa;
}


/**
 * We need to encode an attribute to send to a peer.  However we only
 * have the canonically encoded byte stream data for it.  Sometimes
 * that is fine, and we should just send that; sometimes we need to
 * decode and re-encode for this specific peer.
 */

string
PathAttribute::str() const
{
    string s = "Path attribute of type ";
    switch (type()) {
    case ORIGIN:
	s += "ORIGIN";
	break;

    case AS_PATH:
	s += "AS_PATH";
	break;

    case AS4_PATH:
	s += "AS4_PATH";
	break;

    case NEXT_HOP:
	s += "NEXT_HOP";
	break;

    case MED:
	s += "MED";
	break;

    case LOCAL_PREF:
	s += "LOCAL_PREF";
	break;

    case ATOMIC_AGGREGATE:
	s += "ATOMIC_AGGREGATOR";
	break;

    case AGGREGATOR:
	s += "AGGREGATOR";
	break;

    case AS4_AGGREGATOR:
	s += "AS4_AGGREGATOR";
	break;

    case COMMUNITY:
	s += "COMMUNITY";
	break;

    case ORIGINATOR_ID:
	s += "ORIGINATOR_ID";
	break;

    case CLUSTER_LIST:
	s += "CLUSTER_LIST";
	break;

    case MP_REACH_NLRI:
	s += "MP_REACH_NLRI";
	break;

    case MP_UNREACH_NLRI:
	s += "MP_UNREACH_NLRI";
	break;

    default:
	s += c_format("UNKNOWN(type: %d flags: %x): ", type(), _flags);
    }
    return s;
}


/**
 * encode a path attribute.  In a sane world, we'd use a virtual
 * function for this.  But we store so many path attributes that we
 * can't afford the overhead of a virtual function table for them, so
 * we have to do this the hard way 
 *
 * XXX THE ABOVE IS NO LONGER THE CASE.  WE SHOULD REWRITE THIS TO USE
 * VIRTUAL FUNCTIONS
 */

bool
PathAttribute::encode(uint8_t* buf, size_t &wire_size, 
		      const BGPPeerData* peerdata) const
{
    string s = "Path attribute of type ";
    switch (type()) {
    case ORIGIN:
	return ((const OriginAttribute*)this)->encode(buf, wire_size, peerdata);

    case AS_PATH:
	return ((const ASPathAttribute*)this)->encode(buf, wire_size, peerdata);

    case AS4_PATH:
	return ((const ASPathAttribute*)this)->encode(buf, wire_size, peerdata);

    case NEXT_HOP:
	if (dynamic_cast<const NextHopAttribute<IPv4>*>(this)) {
	    return ((const NextHopAttribute<IPv4>*)this)->encode(buf, wire_size, peerdata);
	} else {
	    return ((const NextHopAttribute<IPv6>*)this)->encode(buf, wire_size, peerdata);
	}

    case MED:
	return ((const MEDAttribute*)this)->encode(buf, wire_size, peerdata);

    case LOCAL_PREF:
	return ((const LocalPrefAttribute*)this)->encode(buf, wire_size, peerdata);

    case ATOMIC_AGGREGATE:
	return ((const AtomicAggAttribute*)this)->encode(buf, wire_size, peerdata);

    case AGGREGATOR:
	return ((const AggregatorAttribute*)this)->encode(buf, wire_size, peerdata);

    case AS4_AGGREGATOR:
	return ((const AS4AggregatorAttribute*)this)->encode(buf, wire_size, peerdata);

    case COMMUNITY:
	return ((const CommunityAttribute*)this)->encode(buf, wire_size, peerdata);

    case ORIGINATOR_ID:
	return ((const OriginatorIDAttribute*)this)->encode(buf, wire_size, peerdata);

    case CLUSTER_LIST:
	return ((const ClusterListAttribute*)this)->encode(buf, wire_size, peerdata);

    case MP_REACH_NLRI:
	if (dynamic_cast<const MPReachNLRIAttribute<IPv4>*>(this)) {
	    return ((const MPReachNLRIAttribute<IPv4>*)this)
		->encode(buf, wire_size, peerdata);
	} else {
	    return ((const MPReachNLRIAttribute<IPv6>*)this)
		->encode(buf, wire_size, peerdata);
	}

    case MP_UNREACH_NLRI:
	if (dynamic_cast<const MPUNReachNLRIAttribute<IPv4>*>(this)) {
	    return ((const MPUNReachNLRIAttribute<IPv4>*)this)
		->encode(buf, wire_size, peerdata);
	} else {
	    return ((const MPUNReachNLRIAttribute<IPv6>*)this)
		->encode(buf, wire_size, peerdata);
	}

    case UNKNOWN_TESTING1:
    case UNKNOWN_TESTING2:
	// This is only for the test harness
	return true;
    }
    return true;
}

bool
PathAttribute::operator<(const PathAttribute& him) const
{
    uint8_t mybuf[BGPPacket::MAXPACKETSIZE], hisbuf[BGPPacket::MAXPACKETSIZE];
    size_t mybuflen, hisbuflen;
    if (sorttype() < him.sorttype())
	return true;
    if (sorttype() > him.sorttype())
	return false;
    // equal sorttypes imply equal types
    switch (type()) {
    case ORIGIN:
	return ( ((OriginAttribute &)*this).origin() <
		((OriginAttribute &)him).origin() );

    case AS_PATH:
	return ( ((ASPathAttribute &)*this).as_path() <
		((ASPathAttribute &)him).as_path() );

    case AS4_PATH:
	return ( ((AS4PathAttribute &)*this).as_path() <
		((AS4PathAttribute &)him).as_path() );

    case NEXT_HOP:
	return ( ((NextHopAttribute<IPv4> &)*this).nexthop() <
		((NextHopAttribute<IPv4> &)him).nexthop() );

    case MED:
	return ( ((MEDAttribute &)*this).med() <
		((MEDAttribute &)him).med() );

    case LOCAL_PREF:
	return ( ((LocalPrefAttribute &)*this).localpref() <
		((LocalPrefAttribute &)him).localpref() );

    case ATOMIC_AGGREGATE:
	return false;

    case AGGREGATOR:
	if ( ((AggregatorAttribute &)*this).aggregator_as() 
	     == ((AggregatorAttribute &)him).aggregator_as() ) {
	    return ( ((AggregatorAttribute &)*this).route_aggregator() 
		     < ((AggregatorAttribute &)him).route_aggregator() );
	} else {
	    return ( ((AggregatorAttribute &)*this).aggregator_as() 
		     < ((AggregatorAttribute &)him).aggregator_as() );
	}

    case AS4_AGGREGATOR:
	if ( ((AS4AggregatorAttribute &)*this).aggregator_as() 
	     == ((AS4AggregatorAttribute &)him).aggregator_as() ) {
	    return ( ((AS4AggregatorAttribute &)*this).route_aggregator() 
		     < ((AS4AggregatorAttribute &)him).route_aggregator() );
	} else {
	    return ( ((AS4AggregatorAttribute &)*this).aggregator_as() 
		     < ((AS4AggregatorAttribute &)him).aggregator_as() );
	}

    case COMMUNITY:
	// XXX using encode for this is a little inefficient
	mybuflen = hisbuflen = BGPPacket::MAXPACKETSIZE;
	((const CommunityAttribute*)this)->encode(mybuf, mybuflen, NULL);
	((const CommunityAttribute&)him).encode(hisbuf, hisbuflen, NULL);
	if (mybuflen < hisbuflen)
	    return true;
	if (mybuflen > hisbuflen)
	    return false;
	return (memcmp(mybuf, hisbuf, mybuflen) < 0);

    case ORIGINATOR_ID:
	return ( ((const OriginatorIDAttribute &)*this).originator_id()
		 < ((const OriginatorIDAttribute &)him).originator_id() );

    case CLUSTER_LIST:
	// XXX using encode for this is a little inefficient
	mybuflen = hisbuflen = BGPPacket::MAXPACKETSIZE;
	((const ClusterListAttribute*)this)->encode(mybuf, mybuflen, NULL);
	((const ClusterListAttribute&)him).encode(hisbuf, hisbuflen, NULL);
	if (mybuflen < hisbuflen)
	    return true;
	if (mybuflen > hisbuflen)
	    return false;
	return (memcmp(mybuf, hisbuf, mybuflen) < 0);

    case MP_REACH_NLRI:
    case MP_UNREACH_NLRI:
	// at places in the code where we'll use this operator, these
	// shouldn't be present in the PathAttributes.
	XLOG_UNREACHABLE();
    default:
	/* this must be an unknown attribute */
#if 0
	XLOG_ASSERT(dynamic_cast<const UnknownAttribute*>(this) != 0);
	mybuflen = hisbuflen = BGPPacket::MAXPACKETSIZE;
	((const UnknownAttribute*)this)->encode(mybuf, mybuflen, NULL);
	((const UnknownAttribute&)him).encode(hisbuf, hisbuflen, NULL);
	if (mybuflen < hisbuflen)
	    return true;
	if (mybuflen > hisbuflen)
	    return false;
	return (memcmp(mybuf, hisbuf, mybuflen) < 0);
#endif
	mybuflen = hisbuflen = BGPPacket::MAXPACKETSIZE;
	this->encode(mybuf, mybuflen, NULL);
	him.encode(hisbuf, hisbuflen, NULL);
	if (mybuflen < hisbuflen)
	    return true;
	if (mybuflen > hisbuflen)
	    return false;
	return (memcmp(mybuf, hisbuf, mybuflen) < 0);
	
    }
    XLOG_UNREACHABLE();
}

bool
PathAttribute::operator==(const PathAttribute& him) const
{
    uint8_t mybuf[BGPPacket::MAXPACKETSIZE], hisbuf[BGPPacket::MAXPACKETSIZE];
    size_t mybuflen, hisbuflen;
    if (sorttype() != him.sorttype())
	return false;
    switch (type()) {
    case ORIGIN:
	return ( ((OriginAttribute &)*this).origin() ==
		((OriginAttribute &)him).origin() );

    case AS_PATH:
	return ( ((ASPathAttribute &)*this).as_path() ==
		((ASPathAttribute &)him).as_path() );

    case AS4_PATH:
	return ( ((AS4PathAttribute &)*this).as_path() ==
		((AS4PathAttribute &)him).as_path() );

    case NEXT_HOP:
	return ( ((NextHopAttribute<IPv4> &)*this).nexthop() ==
		((NextHopAttribute<IPv4> &)him).nexthop() );

    case MED:
	return ( ((MEDAttribute &)*this).med() ==
		((MEDAttribute &)him).med() );

    case LOCAL_PREF:
	return ( ((LocalPrefAttribute &)*this).localpref() ==
		((LocalPrefAttribute &)him).localpref() );

    case ATOMIC_AGGREGATE:
	return true;

    case AGGREGATOR:
	return ( ((AggregatorAttribute &)*this).aggregator_as() 
		 == ((AggregatorAttribute &)him).aggregator_as()
		 &&  ((AggregatorAttribute &)*this).route_aggregator() 
		 == ((AggregatorAttribute &)him).route_aggregator() );

    case AS4_AGGREGATOR:
	return ( ((AS4AggregatorAttribute &)*this).aggregator_as() 
		 == ((AS4AggregatorAttribute &)him).aggregator_as()
		 &&  ((AS4AggregatorAttribute &)*this).route_aggregator() 
		 == ((AS4AggregatorAttribute &)him).route_aggregator() );

    case COMMUNITY:
	// XXX using encode for this is a little inefficient
	mybuflen = hisbuflen = 4096;
	((const CommunityAttribute*)this)->encode(mybuf, mybuflen, NULL);
	((const CommunityAttribute&)him).encode(hisbuf, hisbuflen, NULL);
	if (mybuflen != hisbuflen)
	    return false;
	return memcmp(mybuf, hisbuf, mybuflen) == 0;

    case ORIGINATOR_ID:
	return ( ((OriginatorIDAttribute &)*this).originator_id()
		 == ((OriginatorIDAttribute &)him).originator_id() );

    case CLUSTER_LIST:
	// XXX using encode for this is a little inefficient
	mybuflen = hisbuflen = 4096;
	((const ClusterListAttribute*)this)->encode(mybuf, mybuflen, NULL);
	((const ClusterListAttribute&)him).encode(hisbuf, hisbuflen, NULL);
	if (mybuflen != hisbuflen) 
	    return false;
	return memcmp(mybuf, hisbuf, mybuflen) == 0;

    case MP_REACH_NLRI:
	// XXX using encode for this is a little inefficient
	mybuflen = hisbuflen = 4096;
	if (dynamic_cast<const MPReachNLRIAttribute<IPv4>*>(this)) {
	    ((const MPReachNLRIAttribute<IPv4>*)this)
		->encode(mybuf, mybuflen, NULL);
	    ((const MPReachNLRIAttribute<IPv4>&)him)
		.encode(hisbuf, hisbuflen, NULL);
	} else {
	    ((const MPReachNLRIAttribute<IPv6>*)this)
		->encode(mybuf, mybuflen, NULL);
	    ((const MPReachNLRIAttribute<IPv6>&)him)
		.encode(hisbuf, hisbuflen, NULL);
	}
	if (mybuflen != hisbuflen)
	    return false;
	return memcmp(mybuf, hisbuf, mybuflen) == 0;

    case MP_UNREACH_NLRI:
	// XXX using encode for this is a little inefficient
	mybuflen = hisbuflen = 4096;
	if (dynamic_cast<const MPUNReachNLRIAttribute<IPv4>*>(this)) {
	    ((const MPUNReachNLRIAttribute<IPv4>*)this)
		->encode(mybuf, mybuflen, NULL);
	    ((const MPUNReachNLRIAttribute<IPv4>&)him)
		.encode(hisbuf, hisbuflen, NULL);
	} else {
	    ((const MPUNReachNLRIAttribute<IPv6>*)this)
		->encode(mybuf, mybuflen, NULL);
	    ((const MPUNReachNLRIAttribute<IPv6>&)him)
		.encode(hisbuf, hisbuflen, NULL);
	}
	if (mybuflen != hisbuflen)
	    return false;
	return memcmp(mybuf, hisbuf, mybuflen) == 0;

    default:
	/* this must be an unknown attribute */
	XLOG_ASSERT(dynamic_cast<const UnknownAttribute*>(this) != 0);
	mybuflen = hisbuflen = BGPPacket::MAXPACKETSIZE;
	((const UnknownAttribute*)this)->encode(mybuf, mybuflen, NULL);
	((const UnknownAttribute&)him).encode(hisbuf, hisbuflen, NULL);
	if (mybuflen != hisbuflen) {
	    return false;
	}
	return memcmp(mybuf, hisbuf, mybuflen) == 0;
    }
    XLOG_UNREACHABLE();
}

uint8_t *
PathAttribute::set_header(uint8_t *data, size_t payload_size, size_t &wire_size)
    const
{
    uint8_t final_flags = _flags;
    if (payload_size > 255)
	final_flags |= Extended;
    else
	final_flags &= ~Extended;

    data[0] = final_flags & ValidFlags;
    data[1] = type();
    if (final_flags & Extended) {
	data[2] = (payload_size>>8) & 0xff;
	data[3] = payload_size & 0xff;
	wire_size = payload_size + 4;
	return data + 4;
    } else {
	data[2] = payload_size & 0xff;
	wire_size = payload_size + 3;
	return data + 3;
    }
}

/**************************************************************************
 **** 
 **** PathAttributeList
 **** 
 **************************************************************************/ 

//#define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#define PARANOID

template<class A>
PathAttributeList<A>::PathAttributeList() 
    : _refcount(0)
{
    debug_msg("%p\n", this);
    _canonical_data = 0;
    _canonical_length = 0;
}

template<class A>
PathAttributeList<A>::PathAttributeList(const PathAttributeList<A>& palist)
    : _refcount(0)
{
    debug_msg("%p\n", this);

    _canonical_length = palist._canonical_length;
    _canonical_data = new uint8_t[_canonical_length];
    memcpy(_canonical_data, palist._canonical_data, _canonical_length);
}

template<class A>
PathAttributeList<A>::PathAttributeList(FPAListRef& fpa_list)
    : _refcount(0)
{
    fpa_list->canonicalize();
    _canonical_length = fpa_list->canonical_length();
    _canonical_data = new uint8_t[_canonical_length];
    memcpy(_canonical_data, fpa_list->canonical_data(), _canonical_length);
}
    
template<class A>
PathAttributeList<A>::~PathAttributeList()
{
    //debug_msg("%p %s\n", this, str().c_str());
    XLOG_ASSERT(_refcount == 0);
    if (_canonical_data)
	delete[] _canonical_data;
}

template<class A>
string
PathAttributeList<A>::str() const
{
    // this isn't very efficient - better not to call this unless you really need it.
    PAListRef<A> pal(this);
    FastPathAttributeList<A> fast_pa_list(pal);
    return fast_pa_list.str();
}


template<>
bool
PathAttributeList<IPv4>::
operator< (const PathAttributeList<IPv4> &him) const
{
    // first we compare on nexthop
    int result;
    result = memcmp(_canonical_data, him.canonical_data(), 7);
    if (result < 0)
	return true;
    else if (result > 0)
	return false;

    if (_canonical_length < him.canonical_length())
	return true;
    if (_canonical_length > him.canonical_length())
	return false;
    return (memcmp(_canonical_data+7, him.canonical_data()+7, _canonical_length-7) < 0);
}

template<>
bool
PathAttributeList<IPv6>::
operator< (const PathAttributeList<IPv6> &him) const
{
    // first we compare on nexthop
    int result;
    result = memcmp(_canonical_data, him.canonical_data(), 19);
    if (result < 0)
	return true;
    else if (result > 0)
	return false;


    if (_canonical_length < him.canonical_length())
	return true;
    if (_canonical_length > him.canonical_length())
	return false;
    XLOG_ASSERT(_canonical_length >= 19);
    return (memcmp(_canonical_data+19, him.canonical_data()+19, _canonical_length-19) < 0);
}


template<class A>
bool
PathAttributeList<A>::
operator== (const PathAttributeList<A> &him) const
{
    if (_canonical_length != him.canonical_length())
	return false;
    return (memcmp(_canonical_data, him.canonical_data(), _canonical_length) == 0);
}

#if 0
template<class A>
void
PathAttributeList<A>::replace_attribute(PathAttribute* new_att)
{
    debug_msg("%p\n", this);

    PathAttType type = new_att->type();

    switch (new_att->type()) {
    default:
	break;

    case ORIGIN:
	_origin_att = (OriginAttribute *)new_att;
	break;

    case AS_PATH:
	_aspath_att = (ASPathAttribute *)new_att;
	break;

    case NEXT_HOP:
	_nexthop_att = (NextHopAttribute<A> *)new_att;
	break;
    }

    debug_msg("Replace attribute\n");
    debug_msg("Before: \n%s\n", str().c_str());
    debug_msg("New Att: %s\n", new_att->str().c_str());
    for (iterator i = begin(); i != end() ; ++i)
	if ((*i)->type() == type) {
	    insert(i, new_att);
	    delete (*i);
	    erase(i);
	    debug_msg("After: \n%s\n", str().c_str());
	    memset(_hash, 0, sizeof(_hash));
	    return;
	}
    XLOG_UNREACHABLE();
}
#endif


#if 0
/*
 * Are all the mandatory attributes there?
 */
template<class A>
bool
PathAttributeList<A>::complete() const {
    FastPathAttributeList<A> fast_pa_list(const_cast<PathAttributeList<A>>(*this));
    return fast_pa_list.complete();
}
#endif


/**************************************************************************
 **** 
 **** PAListRef
 **** 
 **************************************************************************/ 

template<class A>
PAListRef<A>::PAListRef(const PathAttributeList<A>* palist)
{
    _palist = palist;
    if (palist) {
	palist->incr_refcount(1);
    }
}

template<class A>
PAListRef<A>::PAListRef(const PAListRef& palistref)
{
    _palist = palistref.attributes();
    if (_palist) {
	_palist->incr_refcount(1);
    }
}

template<class A>
PAListRef<A>::~PAListRef() 
{
    if (_palist) {
	_palist->decr_refcount(1);
	_palist = 0;
    }
}

template<class A>
PAListRef<A>& 
PAListRef<A>::operator=(const PAListRef& palistref) 
{
    if (_palist) {
	// we're not empty
	if (_palist != palistref.attributes()) {
	    // it's not a no-op
	    _palist->decr_refcount(1);
	    _palist = palistref.attributes();
	    _palist->incr_refcount(1);
	}
    } else {
	// we were previously empty
	_palist = palistref.attributes();
	if (_palist) {
	    _palist->incr_refcount(1);
	}
    }
    return *this;
}

template<class A>
bool 
PAListRef<A>::operator==(const PAListRef& palistref) const
{
    if (_palist == palistref.attributes()) {
	// identical pointers
	return true;
    }
    if (_palist && palistref.attributes()) {
	// compare the content.
	return *_palist == *(palistref.attributes());
    }
    // if we got here, one or the other pointer is null, but not both.
    return false;
}


template<class A>
bool 
PAListRef<A>::operator<(const PAListRef& palistref) const 
{
    debug_msg("PAListRef::operator< %p vs %p\n",
	      _palist, palistref.attributes());
    if (_palist == palistref.attributes()) {
	// identical pointers
	debug_msg("identical ptrs -> false\n");
	return false;
    }
    if (_palist && palistref.attributes()) {
	// compare the content.
	debug_msg("comparing content\n");
	bool result = (*_palist) < (*(palistref.attributes()));
#if 0
	printf("first: %s\n", _palist->str().c_str());
	printf("second: %s\n", palistref.attributes()->str().c_str());
	if (result)
	    printf("true\n");
	else
	    printf("false\n");
#endif
	return result;
    }

    // if we got here, one or the other pointer is null, but not both.
    // This is an odd case - the order is arbitrary
    debug_msg("one is null\n");
    if (_palist) {
	return true;
    } else {
	return false;
    }
}

template<class A>
void 
PAListRef<A>::register_with_attmgr()
{
    XLOG_ASSERT(_palist);

    PAListRef<A> found_palist = _att_mgr->add_attribute_list(*this);
    if (_palist == found_palist.attributes()) {
	// we already held a copy of the stored version
	return;
    } else {
	// there's already a copy stored and it's not our one
	_palist->decr_refcount(1);
	_palist = found_palist.attributes();
	_palist->incr_refcount(1);
    }
}

template<class A>
void 
PAListRef<A>::deregister_with_attmgr()
{
    XLOG_ASSERT(_palist);
    _att_mgr->delete_attribute_list(*this);
}

template<class A>
void 
PAListRef<A>::release()
{
    if (_palist) {
	_palist->decr_refcount(1);
	_palist = 0;
    }
}

/**************************************************************************
 **** 
 **** FastPathAttributeList
 **** 
 **************************************************************************/ 

template<class A>
FastPathAttributeList<A>::FastPathAttributeList()
    :  	 _slave_pa_list(0),
	 _attribute_count(0),
	 _locked(false),
	 _canonical_data(0),
	 _canonical_length(0),
	 _canonicalized(false)
{
    _att.resize(MAX_ATTRIBUTE+1);
    for (int i = 0; i <= MAX_ATTRIBUTE; i++) {
	_att_bytes[i] = 0;
	_att_lengths[i] = 0;
	_att[i] = 0;
    }
}

template<class A>
FastPathAttributeList<A>::FastPathAttributeList(PAListRef<A>& palistref)
    :   _slave_pa_list(palistref), 
	_attribute_count(0),
	_locked(false),
	_canonical_data(0),
	_canonical_length(0),
	_canonicalized(false)
{
    _att.resize(MAX_ATTRIBUTE+1);
    for (int i = 0; i <= MAX_ATTRIBUTE; i++) {
	_att_bytes[i] = 0;
	_att_lengths[i] = 0;
	_att[i] = 0;
    }
    quick_decode(_slave_pa_list->canonical_data(), _slave_pa_list->canonical_length());
    count_attributes();
}

template<class A>
FastPathAttributeList<A>::FastPathAttributeList(FastPathAttributeList<A>& him)
    :   _slave_pa_list(him._slave_pa_list), 
	_locked(false),
	_canonical_data(0),
	_canonical_length(0),
	_canonicalized(false)
{
    _att.resize(MAX_ATTRIBUTE+1);
    for (int i = 0; i <= MAX_ATTRIBUTE; i++) {
	_att_bytes[i] = him._att_bytes[i];
	_att_lengths[i] = him._att_lengths[i];
	if (him._att[i]) {
	    _att[i] = him._att[i]->clone();
	}
    }
    count_attributes();
}

template<class A>
FastPathAttributeList<A>::
  FastPathAttributeList(const NextHopAttribute<A> &nexthop_att,
			const ASPathAttribute &aspath_att,
			const OriginAttribute &origin_att)
      : _slave_pa_list(),
	_attribute_count(0),
	_locked(false),
	_canonical_data(0),
	_canonical_length(0),
	_canonicalized(false)
{
    debug_msg("%p\n", this);
    _att.resize(MAX_ATTRIBUTE+1);
    for (int i = 0; i <= MAX_ATTRIBUTE; i++) {
	_att_bytes[i] = 0;
	_att_lengths[i] = 0;
	_att[i] = 0;
    }

    add_path_attribute(origin_att);
    add_path_attribute(nexthop_att);
    add_path_attribute(aspath_att);
}

template<class A>
FastPathAttributeList<A>::~FastPathAttributeList()
{
    XLOG_ASSERT(!_locked);
    if (_canonical_data)
	delete[] _canonical_data;
    for (int i = 0; i <= MAX_ATTRIBUTE; i++) {
	if (_att[i])
	    delete _att[i];
    }
}

template<class A>
bool
FastPathAttributeList<A>::operator==(const FastPathAttributeList<A>& him) const
{
    canonicalize();
    him.canonicalize();
    if (_canonical_length != him.canonical_length())
	return false;
    if (memcmp(_canonical_data, him.canonical_data(), _canonical_length)==0)
	return true;
    return false;
}

template<class A>
void
FastPathAttributeList<A>::quick_decode(const uint8_t* data, uint16_t length)
{
    XLOG_ASSERT(!_locked);
    _canonicalized = false;
    size_t remaining_length = length;
    size_t att_length;
    size_t tlv_att_length;
    while (remaining_length > 0) {
	size_t hdr_len;
	// compute length, which is 1 or 2 bytes depending on flags d[0]
	if ( (data[0] & PathAttribute::Extended) && remaining_length < 4) {
        xorp_throw(CorruptMessage,
                   c_format("PathAttribute (extended) too short %u bytes",
                            XORP_UINT_CAST(remaining_length)),
                   UPDATEMSGERR, ATTRLEN, data, remaining_length);
	}

	if (data[0] & PathAttribute::Extended) {
	    att_length = (data[2]<<8) + data[3];
	    hdr_len = 4;
	} else {
	    att_length = data[2];
	    hdr_len = 3;
        }
	tlv_att_length = att_length + hdr_len;

	if (remaining_length < tlv_att_length) {
	    xorp_throw(CorruptMessage,
		       c_format("PathAttribute too short %u bytes need %u",
				XORP_UINT_CAST(remaining_length), 
				XORP_UINT_CAST(tlv_att_length)),
		       UPDATEMSGERR, ATTRLEN, data, remaining_length);
	}

	uint8_t att_type = data[1];
	if (att_type <= MAX_ATTRIBUTE) {
	    _att_bytes[att_type] = data;
	    _att_lengths[att_type] = tlv_att_length;
	}

        data += tlv_att_length;
        remaining_length -= tlv_att_length;
    }
}

template<class A>
NextHopAttribute<A>*
FastPathAttributeList<A>::nexthop_att() {
    return (NextHopAttribute<A>*)find_attribute_by_type(NEXT_HOP);
}

template<class A>
A&
FastPathAttributeList<A>::nexthop() {
    NextHopAttribute<A>* nha = nexthop_att();
    return nha->nexthop();
}

template<class A>
OriginAttribute*
FastPathAttributeList<A>::origin_att() {
    return (OriginAttribute*)find_attribute_by_type(ORIGIN);
}

template<class A>
OriginType
FastPathAttributeList<A>::origin() {
    const OriginAttribute* oa = origin_att();
    return oa->origin();
}

template<class A>
ASPathAttribute*
FastPathAttributeList<A>::aspath_att() {
    return (ASPathAttribute*)find_attribute_by_type(AS_PATH);
}

template<class A>
AS4PathAttribute*
FastPathAttributeList<A>::as4path_att() {
    return (AS4PathAttribute*)find_attribute_by_type(AS4_PATH);
}

template<class A>
ASPath&
FastPathAttributeList<A>::aspath() {
    ASPathAttribute* aspa = aspath_att();
    return aspa->as_path();
}



template<class A>
void
FastPathAttributeList<A>::load_raw_data(const uint8_t *data, 
					size_t size, 
					const BGPPeerData* peerdata,
					bool have_nlri,
					BGPMain *mainprocess,
					bool do_checks)
{
    debug_msg("FastPathAttributeList::load_raw_data\n");
    XLOG_ASSERT(!_locked);
    _canonicalized = false;
    bool have_ipv4_nlri = have_nlri;

    // We need to decode the data into a set of path attributes, then
    // store the canonical form.

    int pa_count = 0;
    size_t pa_len = size;
    while (pa_len > 0) {
        size_t used = 0;
        PathAttribute *pa = PathAttribute::create(data, pa_len, used, peerdata, 
						  A::ip_version());
        debug_msg("attribute size %u\n", XORP_UINT_CAST(used));
        if (used == 0) {
	    xorp_throw(CorruptMessage,"Attribute Size", UPDATEMSGERR, ATTRLEN);
        }
	debug_msg("Decoded Attribute: %s\n", pa->str().c_str());
	
	pa_count++;
	uint32_t type = pa->type();
	XLOG_ASSERT(type <= 255);
	if (type > _att.size()) {
	    // hopefully this shouldn't happen often, and only when
	    // new Path Attributes are standardized
	    _att.resize(type+1, 0);
	}
	
	// check it's not a duplicate
	if (_att[type] != 0) {
	    // we've got a duplicate!
	    debug_msg("duplicate PA list entry, type: %d\n", (int)type);
	    xorp_throw(CorruptMessage,"Duplicate PA list entry", UPDATEMSGERR, MALATTRLIST);
	}

	// seeing as we've decoded to a PathAttribute, store it for now.
	_att[type] = pa;

	// Check it's not an unknown well-known attribute
	if ((dynamic_cast<UnknownAttribute*>(_att[type]) != 0) &&
	    _att[type]->well_known()) {
	    uint8_t buf[8192];
	    size_t wire_size = 8192;
	    pa->encode(buf, wire_size, peerdata);
	    xorp_throw(CorruptMessage,"Unknown well-known attribute", 
		       UPDATEMSGERR, UNRECOGWATTR,
		       buf, wire_size);
	}

	if (type <= MAX_ATTRIBUTE) {
	    // we don't yet have a canonical form stored, so these
	    // accessor shortcuts are not yet usable
	    _att_bytes[type] = 0;
	    _att_lengths[type] = 0;
	}

        data += used;
        pa_len -= used;
    }

    /*
    ** Check for multiprotocol parameters that haven't been "negotiated".
    **
    ** There are two questions that seem to ambiguous in the BGP specs.
    **
    ** 1) How do we deal with a multiprotocol attribute that hasn't
    **    been "negotiated".
    **    a) Strip out the offending attribute.
    **    b) Send a notify and drop the peering.
    ** 2) In order to accept a multiprotol attribute. I.E to consider
    **    it to have been "negotiated", what is the criteria.
    **    a) Is it sufficient for this speaker to have announced the
    **    capability.
    **    b) Do both speakers in a session need to announce a capability.
    **
    ** For the time being we are going with 1) (a) and 2 (a).
    */

    //     BGPPeerData::Direction dir = BGPPeerData::NEGOTIATED;
    BGPPeerData::Direction dir = BGPPeerData::SENT;  
    // Check the multiprotocol NLRI info.
    if (_att[MP_REACH_NLRI]) {
	// XXXX this duplicates the behaviour of older code, but I'm
	// not convinced it's correct.  It assumes there's only one
	// AFI/SAFI in the attribute.
	MPReachNLRIAttribute<IPv4>* mp4_reach_att =
	    dynamic_cast<MPReachNLRIAttribute<IPv4>*>(_att[MP_REACH_NLRI]);
	if (mp4_reach_att) {
	    Safi safi = mp4_reach_att->safi();
	    // if we didn't negotiate the SAFI, remove the attribute
	    if (!peerdata->multiprotocol<IPv4>(safi, dir)) {
		delete _att[MP_REACH_NLRI];
		_att[MP_REACH_NLRI] = 0;
	    } else {
		have_nlri = true;
		// if there's an NLRI, there must be a non-zero nexthop
		if (do_checks && mp4_reach_att->nexthop() == IPv4::ZERO()) {
		    uint8_t data = NEXT_HOP;
		    xorp_throw(CorruptMessage,"Illegal nexthop", UPDATEMSGERR, 
			       MISSWATTR, &data, 1);
		}
		if (mainprocess && 
		    mainprocess->interface_address4(mp4_reach_att->nexthop())) {
		    XLOG_ERROR("Nexthop in update belongs to this router:\n %s",
			       cstring(*this));
		    xorp_throw(UnusableMessage, "Nexthop belongs to this router");
		}
	    }
	}
	MPReachNLRIAttribute<IPv6>* mp6_reach_att =
	    dynamic_cast<MPReachNLRIAttribute<IPv6>*>(_att[MP_REACH_NLRI]);
	if (mp6_reach_att) {
	    Safi safi = mp6_reach_att->safi();
	    // if we didn't negotiate the SAFI, remove the attribute
	    if (!peerdata->multiprotocol<IPv6>(safi, dir)) {
		delete _att[MP_REACH_NLRI];
		_att[MP_REACH_NLRI] = 0;
	    } else {
		have_nlri = true;
		if (do_checks && mp6_reach_att->nexthop() == IPv6::ZERO()) {
		    uint8_t data = NEXT_HOP;
		    // if there's an NLRI, there must be a non-zero nexthop
		    xorp_throw(CorruptMessage,"Illegal nexthop", UPDATEMSGERR, 
			       MISSWATTR, &data, 1);
		}
		if (do_checks && mainprocess &&
		    mainprocess->interface_address6(mp6_reach_att->nexthop())) {
		    XLOG_ERROR("Nexthop in update belongs to this router:\n %s",
			       cstring(*this));
		    xorp_throw(UnusableMessage, "Nexthop6 belongs to this router");
		}
	    }
	}
	MPUNReachNLRIAttribute<IPv4>* mp4_unreach_att =
	    dynamic_cast<MPUNReachNLRIAttribute<IPv4>*>(_att[MP_UNREACH_NLRI]);
	if (mp4_unreach_att) {
	    Safi safi = mp4_unreach_att->safi();
	    // if we didn't negotiate the SAFI, remove the attribute
	    if (!peerdata->multiprotocol<IPv4>(safi, dir)) {
		delete _att[MP_UNREACH_NLRI];
		_att[MP_UNREACH_NLRI] = 0;
	    }
	}
	MPUNReachNLRIAttribute<IPv6>* mp6_unreach_att =
	    dynamic_cast<MPUNReachNLRIAttribute<IPv6>*>(_att[MP_UNREACH_NLRI]);
	if (mp6_unreach_att) {
	    Safi safi = mp6_unreach_att->safi();
	    // if we didn't negotiate the SAFI, remove the attribute
	    if (!peerdata->multiprotocol<IPv6>(safi, dir)) {
		delete _att[MP_UNREACH_NLRI];
		_att[MP_UNREACH_NLRI] = 0;
	    }
	}
    }

    // If we have NLRI entries, the PA list must be non-empty.  If we
    // don't have NLRI entries, we don't need PA list entries (in
    // which case we won't keep this instance) unless we have
    // Multiprotocol Attribute entries which serve the role of NLRI
    // entries.
    if (pa_count == 0 && have_nlri) {
	debug_msg("Empty path attribute list and "
		  "non-empty NLRI list\n");
	xorp_throw(CorruptMessage,"Illegal nexthop", UPDATEMSGERR, MALATTRLIST);
    }


    /*
    ** If a NLRI attribute is present check for the following
    ** mandatory fields:
    ** ORIGIN
    ** AS_PATH
    ** NEXT_HOP
    */ 
    if (do_checks && have_nlri) {
	if (_att[ORIGIN] == NULL) {
	    debug_msg("Missing ORIGIN\n");
	    uint8_t data = ORIGIN;
	    xorp_throw(CorruptMessage,"Missing Origin",
		       UPDATEMSGERR, MISSWATTR, &data, 1);
	}

	// The AS Path attribute is mandatory
	if (_att[AS_PATH] == NULL) {
	    debug_msg("Missing AS_PATH\n");
	    uint8_t data = AS_PATH;
	    xorp_throw(CorruptMessage,"Missing AS Path",
		       UPDATEMSGERR, MISSWATTR, &data, 1);
	}

	// The NEXT_HOP attribute is mandatory for IPv4 unicast.  For
	// multiprotocol NLRI its in the path attribute - we checked
	// this above.
	if (have_ipv4_nlri && _att[NEXT_HOP] == NULL) {                
	    debug_msg("Missing NEXT_HOP\n");
	    uint8_t data = NEXT_HOP;
	    xorp_throw(CorruptMessage,"Missing Next Hop",
		       UPDATEMSGERR, MISSWATTR, &data, 1);
	}
    }

    // If we got this far and as_path_attr is not set this is a
    // multiprotocol withdraw.
    if (_att[AS_PATH] != NULL) {
	if (!peerdata->ibgp()) {
	    // If this is an EBGP peering, the AS Path MUST NOT be empty
	    if (((ASPathAttribute*)_att[AS_PATH])->as_path().path_length() == 0)
		xorp_throw(CorruptMessage,"Empty AS Path",
			   UPDATEMSGERR, MALASPATH);

	    // If this is an EBGP peering, the AS Path MUST start
	    // with the AS number of the peer.
	    AsNum my_asnum(peerdata->as());
	    if (((ASPathAttribute*)_att[AS_PATH])->as_path().first_asnum() != my_asnum)
		xorp_throw(CorruptMessage,"AS path must list peer",
			   UPDATEMSGERR, MALASPATH);

	    // If this is an EBGP peering and a route reflector
	    // attribute has been received then generate an error.
	    if (_att[CLUSTER_LIST] || _att[ORIGINATOR_ID])
		xorp_throw(CorruptMessage,"RR on EBGP peering",
			   UPDATEMSGERR, MALATTRLIST);
	}
	// Receiving confederation path segments when the router
	// is not configured for confederations is an error. 
	if (!peerdata->confederation() &&
	    ((ASPathAttribute*)_att[AS_PATH])->as_path().contains_confed_segments())
	    xorp_throw(CorruptMessage,"Unexpected confederation",
		       UPDATEMSGERR, MALASPATH);
    }

    // If an update message is received that contains a nexthop
    // that belongs to this router then discard the update, don't
    // send a notification.

    if (_att[NEXT_HOP] != NULL) {
	if (mainprocess && 
	    mainprocess->interface_address4(((NextHopAttribute<IPv4>*)_att[NEXT_HOP])->nexthop())) {
	    XLOG_ERROR("Nexthop in update belongs to this router:\n %s",
		       cstring(((NextHopAttribute<IPv4>*)_att[NEXT_HOP])->nexthop()));
	    xorp_throw(UnusableMessage, "Nexthop belongs to this router");
	}
    }

    count_attributes();
}

template<class A>
void
FastPathAttributeList<A>::canonicalize() const { 
    if (_canonicalized)
	return;

    // allocate some extra space because canonical form can be a little longer than wire form.
    size_t remaining_space = 2 * BGPPacket::MAXPACKETSIZE;
    size_t size_so_far = 0;
    uint8_t buf[remaining_space];
    uint8_t *p = buf;
    for (uint32_t i = 0; i < _att.size(); i++) {

	// reorder so nexthop is first, so we can find nexthops when
	// IGP distance changes
	uint32_t type = att_order(i);

	// make sure we really have decoded this before canonicalizing it
	if (_att_bytes[type] && !_att[type]) {
	    // we're got data for an attribute, but not decoded it yet
	    // this must have come from a canonicalize PA list in the
	    // first place, because if we received it from a peer,
	    // we'd have decoded everything
	    size_t length = _att_lengths[type];
	    memcpy(p, _att_bytes[type], length);
	    p += length;
	    XLOG_ASSERT(remaining_space >= length);
	    remaining_space -= length;
	    size_so_far += length;
	} else {

	    // encode each attribute
	    if (_att[type]) {
		size_t length = remaining_space;
		// encode the most general peer-independent version
		if (!(_att[type]->encode(p, length, NULL))) {
		    // we ran out of space to encode; this can't happen 
		    XLOG_UNREACHABLE();
		}
		p += length;
		XLOG_ASSERT(remaining_space >= length);
		remaining_space -= length;
		size_so_far += length;
	    }
	}
    }
    
    bool alloc_space = true;
    if (_canonical_data) {
	if (_canonical_length >= size_so_far) {
	// we've got enough space already
	    alloc_space = false;
	} else {
	    delete[] _canonical_data;
	}
    } 

    if (alloc_space)
	this->_canonical_data = new uint8_t[size_so_far];

    memcpy(this->_canonical_data, buf, size_so_far);
    this->_canonical_length = size_so_far;
    _canonicalized = true;
}




template<class A>
void
FastPathAttributeList<A>::add_path_attribute(const PathAttribute &att)
{
    debug_msg("%p %s\n", this, att.str().c_str());

    PathAttribute *a = att.clone();
    add_path_attribute(a);
}

template<class A>
void
FastPathAttributeList<A>::add_path_attribute(PathAttribute *a)
{
    uint8_t type = a->type();
    _canonicalized = false;
    debug_msg("%p %s\n", this, a->str().c_str());
    XLOG_ASSERT(!_locked);

#if 0
    // we shouldn't need this code
    if (_att.size() <= type) {
	size_t old_size = _att.size();
	_att.resize(type + 1);
	printf("adding att of type %u, old size was %u, new size is %u\n",
	       (uint32_t)type, (uint32_t)old_size, (uint32_t)_att.size());
	for (size_t i = old_size; i < _att.size(); i++) {
	    _att[i] = 0;
	}
    }
#endif

    XLOG_ASSERT(_att[type] == 0);
    _att[type] = a;
    _attribute_count++;
}

template<class A>
PathAttribute*
FastPathAttributeList<A>::find_attribute_by_type(PathAttType type)
{
    if (_att[type]) {
	return _att[type];
    }
    if (_att_bytes[type] == 0)
	return 0;

    // we're got data for an attribute, but not decoded it yet
    size_t used = _att_lengths[type];
    PathAttribute *pa = PathAttribute::create(_att_bytes[type], 
					      _att_lengths[type], used, NULL,
					      A::ip_version());
    _att[type] = pa;
    return pa;
}

template<class A>
void
FastPathAttributeList<A>::replace_attribute(PathAttribute* new_att)
{
    debug_msg("%p\n", this);
    XLOG_ASSERT(!_locked);
    XLOG_ASSERT(new_att);
    _canonicalized = false;

    XLOG_ASSERT(_att[new_att->type()] != 0 || _att_bytes[new_att->type()] != 0);
    if (_att[new_att->type()]) {
	// remove the old decoded version
	delete _att[new_att->type()];
    } else {
	// we hadn't yet decoded it; now we never will
	_att_bytes[new_att->type()] = 0;
	_att_lengths[new_att->type()] = 0;
    }
    _att[new_att->type()] = new_att;

}

template<class A>
void
FastPathAttributeList<A>::replace_AS_path(const ASPath& new_as_path)
{
    debug_msg("%p\n", this);

    replace_attribute(new ASPathAttribute(new_as_path));
}

template<class A>
void
FastPathAttributeList<A>::replace_nexthop(const A& new_nexthop)
{
    debug_msg("%p\n", this);

    replace_attribute(new NextHopAttribute<A>(new_nexthop));
}

template<class A>
void
FastPathAttributeList<A>::replace_origin(const OriginType& new_origin)
{
    debug_msg("%p\n", this);

    replace_attribute(new OriginAttribute(new_origin));
}

template<class A>
void
FastPathAttributeList<A>::remove_attribute_by_type(PathAttType type)
{
    debug_msg("%p\n", this);
    XLOG_ASSERT(!_locked);
    _canonicalized = false;
    bool old_existed = false;
    if (_att[type]) {
	delete _att[type];
	_att[type] = 0;
	old_existed = true;
    }
    if (_att_bytes[type]) {
	_att_bytes[type] = 0;
	_att_lengths[type] = 0;
	old_existed = true;
    }
    if (old_existed)
	_attribute_count--;
}

template<class A>
void
FastPathAttributeList<A>::remove_attribute_by_pointer(PathAttribute *p)
{
    debug_msg("%p\n", this);
    XLOG_ASSERT(!_locked);

    remove_attribute_by_type(p->type());
}

template<class A>
MEDAttribute* 
FastPathAttributeList<A>::med_att()
{
    debug_msg("%p\n", this);

    return (MEDAttribute*)find_attribute_by_type(MED);
}

template<class A>
LocalPrefAttribute*
FastPathAttributeList<A>::local_pref_att()
{
    debug_msg("%p\n", this);

    return (LocalPrefAttribute*)find_attribute_by_type(LOCAL_PREF);
}

template<class A>
AtomicAggAttribute*
FastPathAttributeList<A>::atomic_aggregate_att()
{
    debug_msg("%p\n", this);

    return (AtomicAggAttribute*)find_attribute_by_type(ATOMIC_AGGREGATE);
}

template<class A>
AggregatorAttribute*
FastPathAttributeList<A>::aggregator_att()
{
    debug_msg("%p\n", this);

    return (AggregatorAttribute*)find_attribute_by_type(AGGREGATOR);
}

template<class A>
CommunityAttribute*
FastPathAttributeList<A>::community_att()
{
    debug_msg("%p\n", this);

    return (CommunityAttribute*)find_attribute_by_type(COMMUNITY);
}

template<class A>
OriginatorIDAttribute*
FastPathAttributeList<A>::originator_id()
{
    debug_msg("%p\n", this);

    return (OriginatorIDAttribute*)find_attribute_by_type(ORIGINATOR_ID);
}

template<class A>
ClusterListAttribute*
FastPathAttributeList<A>::cluster_list()
{
    debug_msg("%p\n", this);

    return (ClusterListAttribute*)find_attribute_by_type(CLUSTER_LIST);
}


template<class A>
void
FastPathAttributeList<A>::process_unknown_attributes()
{
    debug_msg("%p\n", this);
    debug_msg("process_unknown_attributes\n");
    for (uint32_t i = 0; i < _att.size(); i++) {
	if (_att[i] && dynamic_cast<UnknownAttribute *>(_att[i])) {
	    if (_att[i]->transitive()) {
		_att[i]->set_partial();
	    } else {
		delete _att[i];
		_att[i] = 0;
	    }
	}
    }
}

/*
 * encode the PA list for transmission to this specific peer.
 */
template<class A>
bool
FastPathAttributeList<A>::encode(uint8_t* buf, size_t &wire_size,
				 const BGPPeerData* peerdata) const
{
    size_t len_so_far = 0;
    size_t attr_len;

    // fill path attribute list
    for (uint32_t i = 0; i < _att.size(); i++) {
	uint32_t type = i;
	attr_len = wire_size - len_so_far;
	if (_att[type]) {
	    if (_att[type]->encode(buf + len_so_far, attr_len, peerdata) ) {
		len_so_far += attr_len;
		XLOG_ASSERT(len_so_far <= wire_size);
	    } else {
		// not enough space to encode the PA list.
		return false;
	    }
	} else if (_att_bytes[type]) {
	    // We've got an attribute that hasn't yet been decoded,
	    // which means it can't have changed.  Often we can
	    // shortcut the encoding process and just use the
	    // previously encoded version.

	    if (encode_and_decode_attribute(_att_bytes[type], _att_lengths[type],
					    buf + len_so_far, attr_len, 
					    peerdata)) {
		len_so_far += attr_len;
		XLOG_ASSERT(len_so_far <= wire_size);
	    } else {
		// not enough space to encode the PA list.
		return false;
	    }
	}
    }
    if (peerdata->we_use_4byte_asnums() && !peerdata->use_4byte_asnums()) {

	// we're using 4byte AS nums, but our peer isn't so we need to
	// add an AS4Path attribute
	XLOG_ASSERT(_att[AS_PATH]);  // surely we've decoded this by now?
	if (!((ASPathAttribute*)_att[AS_PATH])->as_path().two_byte_compatible()) {
	    // only add the AS4Path if we can't code the ASPath without losing information

	    attr_len = wire_size - len_so_far;
	    AS4PathAttribute as4path_att(((ASPathAttribute*)_att[AS_PATH])->as4_path());
	    if (as4path_att.encode(buf + len_so_far, attr_len, peerdata) ) {
		len_so_far += attr_len;
		XLOG_ASSERT(len_so_far <= wire_size);
	    } else {
		// not enough space to encode the PA list.
		return false;
	    }
	}

	// XXXXX we should do something here for AS4Aggregator if
	// we're aggregating
    }
    wire_size = len_so_far;
    
    return true;
}

template<class A>
bool
FastPathAttributeList<A>::encode_and_decode_attribute(const uint8_t* att_data,
						      const size_t& att_len, 
						      uint8_t *buf,
						      size_t& wire_size ,
						      const BGPPeerData* peerdata) const
{
    PathAttribute *pa;
    bool use_4byte_asnums = peerdata->use_4byte_asnums();
    switch (att_data[1]) {	// depending on type, do the right thing.

    case AS_PATH: 
    case AGGREGATOR:
	// AS Path and Aggregator can be encoded differently for each
	// peer, so we need to decode and re-encode if we are not
	// using 4-byte AS nums.
	if (use_4byte_asnums) {
	    if (wire_size < att_len) 
		return false;
	    memcpy(buf, att_data, att_len);
	    wire_size = att_len;
	    return true;
	} else {
	    if (att_data[1] == AS_PATH) {
		ASPathAttribute as_path_att(att_data, use_4byte_asnums);
		return as_path_att.encode(buf, wire_size, peerdata);
	    } else {
		AggregatorAttribute agg_att(att_data, use_4byte_asnums);
		return agg_att.encode(buf, wire_size, peerdata);
	    }
	}
     
    case AS4_PATH:
    case AS4_AGGREGATOR:
    case MP_REACH_NLRI:
    case MP_UNREACH_NLRI:
	// these can't exist in the canonical version, because we use
	// 4-byte AS nums internally and speak MP
	XLOG_UNREACHABLE();
     
    default:
	// all the remainder encode identically for all peers
	if (wire_size < att_len) 
	    return false;
	memcpy(buf, att_data, att_len);
	wire_size = att_len;
	return true;
    }
    return pa;
}

template<class A>
string
FastPathAttributeList<A>::str() const
{
    // Don't change this formatting or the isolation tests will fail.
    string s;
    for (uint32_t i = 0; i < _att.size(); i++) {
	uint32_t type = att_order(i);
	if (_att[type]) {
	    s += "\n\t" + _att[type]->str();
	} else if(_att_lengths[type]>0) {
	    // we're got data for an attribute, but not decoded it yet
	    size_t used = _att_lengths[type];
	    PathAttribute *pa = PathAttribute::create(_att_bytes[type], 
						      _att_lengths[type], 
						      used, NULL, A::ip_version());
	    _att[type] = pa;
	    s += "\n\t" + _att[type]->str();
	}
    }
    return s;
}


template class FastPathAttributeList<IPv4>;
template class FastPathAttributeList<IPv6>;
template class PathAttributeList<IPv4>;
template class PathAttributeList<IPv6>;
template class PAListRef<IPv4>;
template class PAListRef<IPv6>;
