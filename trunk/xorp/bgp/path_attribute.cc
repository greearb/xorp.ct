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

#ident "$XORP: xorp/bgp/path_attribute.cc,v 1.33 2003/09/05 01:02:24 atanu Exp $"

// #define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "config.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "path_attribute.hh"
#include "packet.hh"

#include <stdio.h>
#include <netinet/in.h>
#include <vector>

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

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
    encode();
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
    if (length(d) != 1)
	xorp_throw(CorruptMessage,
                c_format("OriginAttribute bad length %u", (uint32_t)length(d)),
                        UPDATEMSGERR, UNSPECIFIED);
    if (!well_known() || !transitive())
	xorp_throw(CorruptMessage,
		   "Bad Flags in Origin attribute",
		   UPDATEMSGERR, ATTRFLAGS);

    d = payload(d);	// skip header.

    switch (d[0]) {
    case IGP:
    case EGP:
    case INCOMPLETE:
	_origin = (OriginType)d[0];
	break;

    default:
	xorp_throw(CorruptMessage,
		   c_format("Unknown Origin Type %d", d[0]),
		   UPDATEMSGERR, INVALORGATTR);
    }
    encode();
}

void
OriginAttribute::encode()
{
    uint8_t *d = set_header(1);
    d[0] = origin();
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

ASPathAttribute::ASPathAttribute(const AsPath& p)
	: PathAttribute(Transitive, AS_PATH)
{
    _as_path = new AsPath(p);
    encode();
}

PathAttribute *
ASPathAttribute::clone() const
{
    return new ASPathAttribute(as_path());
}

ASPathAttribute::ASPathAttribute(const uint8_t* d)
	throw(CorruptMessage)
	: PathAttribute(d)
{
    if (!well_known() || !transitive())
	xorp_throw(CorruptMessage,
		   "Bad Flags in AS Path attribute",
		   UPDATEMSGERR, ATTRFLAGS);

    _as_path = new AsPath(payload(d), length(d));
    encode();
}

void
ASPathAttribute::encode()
{
    debug_msg("ASPathAttribute encode()\n");
    size_t l = as_path().wire_size();

    uint8_t *d = set_header(l);	// allocate buffer and skip header
    as_path().encode(l, d);	// encode the payload in the buffer
    assert(l == _size);		// a posteriori check
}

/**
 * NextHopAttribute
 */

template <class A>
NextHopAttribute<A>::NextHopAttribute<A>(const A& n)
	: PathAttribute(Transitive, NEXT_HOP), _next_hop(n)
{
    encode();
}

template <class A>
PathAttribute *
NextHopAttribute<A>::clone() const
{
    return new NextHopAttribute(nexthop());
}

template <class A>
NextHopAttribute<A>::NextHopAttribute<A>(const uint8_t* d)
	throw(CorruptMessage)
	: PathAttribute(d)
{
    if (!well_known() || !transitive())
	xorp_throw(CorruptMessage, "Bad Flags in NextHop attribute",
		   UPDATEMSGERR, ATTRFLAGS);
    if (length(d) != A::addr_size())
	xorp_throw(CorruptMessage, "Bad size in NextHop address",
		   UPDATEMSGERR, INVALNHATTR);

    _next_hop = A(payload(d));

    if (!_next_hop.is_unicast())
	xorp_throw(CorruptMessage,
		   c_format("NextHop %s is not a unicast address",
			    _next_hop.str().c_str()),
		   UPDATEMSGERR, INVALNHATTR);
    encode();
}

template<class A>
void
NextHopAttribute<A>::encode()
{
    uint8_t *d = set_header(_next_hop.addr_size());
    _next_hop.copy_out(d);
}

template class NextHopAttribute<IPv4>;
template class NextHopAttribute<IPv6>;


/**
 * MEDAttribute
 */

MEDAttribute::MEDAttribute(const uint32_t med)
	: PathAttribute(Optional, MED), _med(med)
{
    encode();
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
	xorp_throw(CorruptMessage, "Bad Flags in MEDAttribute",
		   UPDATEMSGERR, ATTRFLAGS);
    if (length(d) != 4)
	xorp_throw(CorruptMessage, "Bad size in MEDAttribute",
		   UPDATEMSGERR, INVALNHATTR);
    memcpy(&_med, payload(d), 4);
    _med = ntohl(_med);
    encode();
}

void
MEDAttribute::encode()
{
    uint8_t *d = set_header(4);
    uint32_t x = htonl(_med);
    memcpy(d, &x, 4);
}

string
MEDAttribute::str() const
{
    return c_format("Multiple Exit Descriminator Attribute: MED=%d",
		    med());
}

/**
 * LocalPrefAttribute
 */

LocalPrefAttribute::LocalPrefAttribute(const uint32_t localpref)
	: PathAttribute(Transitive, LOCAL_PREF), _localpref(localpref)
{
    encode();
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
	xorp_throw(CorruptMessage, "Bad Flags in LocalPrefAttribute",
		   UPDATEMSGERR, ATTRFLAGS);
    if (length(d) != 4)
	xorp_throw(CorruptMessage, "Bad size in LocalPrefAttribute",
		   UPDATEMSGERR, INVALNHATTR);
    memcpy(&_localpref, payload(d), 4);
    _localpref = ntohl(_localpref);
    encode();
}

void
LocalPrefAttribute::encode()
{
    uint8_t *d = set_header(4);
    uint32_t x = htonl(_localpref);
    memcpy(d, &x, 4);
}

string
LocalPrefAttribute::str() const
{
    return c_format("Local Preference Attribute - %d", _localpref);
}

/**
 * AtomicAggAttribute has length 0
 */

AtomicAggAttribute::AtomicAggAttribute()
	: PathAttribute(Transitive, ATOMIC_AGGREGATE)
{
    set_header(0);	// this is all encode() has to do
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
                c_format("AtomicAggregate bad length %u", (uint32_t)length(d)),
                        UPDATEMSGERR, UNSPECIFIED);
    if (!well_known() || !transitive())
	xorp_throw(CorruptMessage,
		   "Bad Flags in AtomicAggregate attribute",
		   UPDATEMSGERR, ATTRFLAGS);
    set_header(0);	// this is all encode() has to do
}

/**
 * AggregatorAttribute
 */

AggregatorAttribute::AggregatorAttribute(const IPv4& speaker,
			const AsNum& as)
	: PathAttribute((Flags)(Optional|Transitive), AGGREGATOR),
		_speaker(speaker), _as(as)            
{
    encode();
}

PathAttribute *
AggregatorAttribute::clone() const
{
    return new AggregatorAttribute(route_aggregator(), aggregator_as());
}

AggregatorAttribute::AggregatorAttribute(const uint8_t* d)
	throw(CorruptMessage)
	: PathAttribute(d), _as(AsNum::AS_INVALID)
{
    if (length(d) != 6)
	xorp_throw(CorruptMessage,
                c_format("Aggregator bad length %u", (uint32_t)length(d)),
                        UPDATEMSGERR, UNSPECIFIED);
    if (!optional() || !transitive())
	xorp_throw(CorruptMessage,
		   "Bad Flags in AtomicAggregate attribute",
		   UPDATEMSGERR, ATTRFLAGS);
    d = payload(d);
    _as = AsNum(d);
    _speaker = IPv4(d+2);
    encode();
}

void
AggregatorAttribute::encode()
{
    uint8_t *d = set_header(6);
    _as.copy_out(d);
    _speaker.copy_out(d + 2); // defined to be an IPv4 address in RFC 2858
}

string AggregatorAttribute::str() const
{
    return "Aggregator Attribute " + _as.str() + " " + _speaker.str();
}


/**
 * CommunityAttribute
 */

CommunityAttribute::CommunityAttribute()
	: PathAttribute((Flags)(Optional | Transitive), COMMUNITY)
{
    encode();

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
    d = payload(d);
    for (size_t l = _size; l >= 4;  d += 4, l -= 4) {
	uint32_t value;
	memcpy(&value, d, 4);
	_communities.insert(ntohl(value));
    }
    encode();
}

void
CommunityAttribute::encode()
{
    assert(_size == (4 * _communities.size()) );
    assert(_size < 256);

    delete[] _data;
    uint8_t *d = set_header(_size);
    const_iterator i = _communities.begin();
    for (; i != _communities.end(); d += 4, i++) {
	uint32_t value = htonl(*i);
	memcpy(d, &value, 4);
    }
}

string
CommunityAttribute::str() const
{
    string s = "Community Attribute ";
    const_iterator i = _communities.begin();
    for ( ; i != _communities.end();  ++i)
	s += c_format("%u ", *i);
    return s;
}

void
CommunityAttribute::add_community(uint32_t community)
{
    _communities.insert(community);
    _size += 4;
    encode();
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
void
MPReachNLRIAttribute<IPv6>::encode()
{
    delete[] _data;	// Zap any old allocation.

    XLOG_ASSERT(AFI_IPV6 == _afi && SAFI_NLRI_UNICAST == _safi);
    XLOG_ASSERT(16 == IPv6::addr_size());

    /*
    ** Figure out how many bytes we need to allocate.
    */
    size_t len;
    len = 2;	// AFI
    len += 1;	// SAFI
    len += 1;	// Length of Next Hop Address
    len += IPv6::addr_size();	// Next Hop
    if (!_link_local_next_hop.is_zero())
	len += IPv6::addr_size();
    len += 1;	// Number of SNPAs

    const_iterator i;
    for (i = _nlri.begin(); i != _nlri.end(); i++) {
	len += 1;
	len += (i->prefix_len() + 8) / 8;
    }

    uint8_t *d = set_header(len);
    
    /*
    ** Fill in the buffer.
    */
    *d++ = (_afi << 8) & 0xff;	// AFI
    *d++ = _afi & 0xff;		// AFI
    
    *d++ = _safi;		// SAFIs

    if (_link_local_next_hop.is_zero()) {
	*d++ = IPv6::addr_size();
	nexthop().copy_out(d);
	d += IPv6::addr_size();
    } else {
	*d++ = IPv6::addr_size() * 2;
	nexthop().copy_out(d);
	d += IPv6::addr_size();
	_link_local_next_hop.copy_out(d);
	d += IPv6::addr_size();
    }
    
    *d++ = 0;	// Number of SNPAs

    for (i = _nlri.begin(); i != _nlri.end(); i++) {
	int bytes = (i->prefix_len() + 8)/ 8;
	debug_msg("encode %s bytes = %d\n", i->str().c_str(), bytes);
	uint8_t buf[IPv6::addr_size()];
	i->masked_addr().copy_out(buf);

	*d++ = i->prefix_len();
	memcpy(d, buf, bytes);
	d += bytes;
    }
}

template <>
MPReachNLRIAttribute<IPv6>::MPReachNLRIAttribute()
    : PathAttribute((Flags)(Optional | Transitive), MP_REACH_NLRI)
{
    _afi = AFI_IPV6;
    _safi = SAFI_NLRI_UNICAST;

    encode();
}

template <class A>
PathAttribute *
MPReachNLRIAttribute<A>::clone() const
{
    // Cheat go through the wire format to make the copy.
    return new MPReachNLRIAttribute(data());
}

template <>
MPReachNLRIAttribute<IPv6>::MPReachNLRIAttribute(const uint8_t* d)
    throw(CorruptMessage)
    : PathAttribute(d)
{
    if (!optional() || !transitive())
	xorp_throw(CorruptMessage,
		   "Bad Flags in Multiprotocol Reachable NLRI attribute",
		   UPDATEMSGERR, ATTRFLAGS);

    const uint8_t *data = payload(d);
    const uint8_t *end = payload(d) + length(d);

    _afi = *data++;
    _afi <<= 8;
    _afi |= *data++;

    _safi = *data++;

    if (_afi != AFI_IPV6 || _safi != SAFI_NLRI_UNICAST)
	xorp_throw(CorruptMessage,
		   c_format("Expected AFI/SAFI to be %d/%d not %d/%d",
			    AFI_IPV6, SAFI_NLRI_UNICAST, _afi, _safi),
		   UPDATEMSGERR, OPTATTR);

    /*
    ** Next Hop
    */
    uint8_t len = *data++;

    XLOG_ASSERT(16 == IPv6::addr_size());
    IPv6 temp;

    switch(len) {
    case 16:
	temp.copy_in(data);
	set_nexthop(temp);
	data += IPv6::addr_size();
	break;
    case 32:
	temp.copy_in(data);
	set_nexthop(temp);
	data += IPv6::addr_size();
	_link_local_next_hop.copy_in(data);
	data += IPv6::addr_size();
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
	int bytes = (prefix_length + 8)/ 8;
	uint8_t buf[16];
	memset(buf, 0, sizeof(buf));
	memcpy(buf, data, bytes);
	IPv6 nlri;
	nlri.set_addr(buf);
	_nlri.push_back(IPNet<IPv6>(nlri, prefix_length));
	data += bytes;
	debug_msg("decode %s/%d bytes = %d\n", nlri.str().c_str(), 
		  prefix_length, bytes);
    }

    encode();
}

template <class A>
string
MPReachNLRIAttribute<A>::str() const
{
    string s = c_format("Multiprotocol Reachable NLRI AFI = %d SAFI = %d\n",
			_afi, _safi);
    s += c_format("   - Next Hop Attribute %s\n", nexthop().str().c_str());
    s += c_format("   - Link Local Next Hop Attribute %s\n",
		  link_local_nexthop().str().c_str());
    typename list<IPNet<A> >::const_iterator i = nlri_list().begin();
    for(; i != nlri_list().end(); i++)
	s += c_format("   - Nlri %s", i->str().c_str());
    return s;
}

template <>
void
MPUNReachNLRIAttribute<IPv6>::encode()
{
    delete[] _data;	// Zap any old allocation.

    XLOG_ASSERT(AFI_IPV6 == _afi && SAFI_NLRI_UNICAST == _safi);
    XLOG_ASSERT(16 == IPv6::addr_size());

    /*
    ** Figure out how many bytes we need to allocate.
    */
    size_t len;
    len = 2;	// AFI
    len += 1;	// SAFI

    const_iterator i;
    for (i = _withdrawn.begin(); i != _withdrawn.end(); i++) {
	len += 1;
	len += i->prefix_len() / 8;
    }

    uint8_t *d = set_header(len);
    
    /*
    ** Fill in the buffer.
    */
    *d++ = (_afi << 8) & 0xff;	// AFI
    *d++ = _afi & 0xff;		// AFI

    *d++ = _safi;		// SAFIs
    
    for (i = _withdrawn.begin(); i != _withdrawn.end(); i++) {
	int bytes = i->prefix_len() / 8;
	uint8_t buf[IPv6::addr_size()];
	i->masked_addr().copy_out(buf);

	*d++ = bytes;
	memcpy(d, buf, bytes);
	d += bytes;
    }
}

template <>
MPUNReachNLRIAttribute<IPv6>::MPUNReachNLRIAttribute()
	: PathAttribute((Flags)(Optional | Transitive), MP_UNREACH_NLRI)
{
    _afi = AFI_IPV6;
    _safi = SAFI_NLRI_UNICAST;

    encode();
}

template <class A>
PathAttribute *
MPUNReachNLRIAttribute<A>::clone() const
{
    // Cheat go through the wire format to make the copy.
    return new MPUNReachNLRIAttribute(data());
}

template <>
MPUNReachNLRIAttribute<IPv6>::MPUNReachNLRIAttribute(const uint8_t* d)
    throw(CorruptMessage)
    : PathAttribute(d)
{
    if (!optional() || !transitive())
	xorp_throw(CorruptMessage,
		   "Bad Flags in Multiprotocol UNReachable NLRI attribute",
		   UPDATEMSGERR, ATTRFLAGS);

    const uint8_t *data = payload(d);
    const uint8_t *end = payload(d) + length(d);

    _afi = *data++;
    _afi <<= 8;
    _afi |= *data++;

    _safi = *data++;

    if (_afi != AFI_IPV6 || _safi != SAFI_NLRI_UNICAST)
	xorp_throw(CorruptMessage,
		   c_format("Expected AFI/SAFI to be %d/%d not %d/%d",
			    AFI_IPV6, SAFI_NLRI_UNICAST, _afi, _safi),
		   UPDATEMSGERR, OPTATTR);

    /*
    ** NLRI
    */
    while(data < end) {
	uint8_t prefix_length = *data++;
	int bytes = prefix_length / 8;
	uint8_t buf[16];
	memcpy(buf, data, bytes);
	IPv6 nlri;
	nlri.set_addr(buf);
	_withdrawn.push_back(IPNet<IPv6>(nlri, prefix_length));
	data += bytes;
    }

    encode();
}

template <class A>
string
MPUNReachNLRIAttribute<A>::str() const
{
    string s = c_format("Multiprotocol UNReachable NLRI AFI = %d SAFI =%d\n",
			_afi, _safi);
    typename list<IPNet<A> >::const_iterator i = wr_list().begin();
    for(; i != wr_list().end(); i++)
	s += c_format("   - Nlri %s", i->str().c_str());
    return s;
}

/**
 * UnknownAttribute
 */
UnknownAttribute::UnknownAttribute(const uint8_t* d)
	throw(CorruptMessage)
	: PathAttribute(d)
{
    if (!optional() /*|| !transitive()*/)
	xorp_throw(CorruptMessage,
		   "Bad Flags in Unknown attribute",
		   UPDATEMSGERR, ATTRFLAGS);
    _size = length(d);
    _data = new uint8_t[wire_size()];
    memcpy(_data, d, wire_size());
}

PathAttribute *
UnknownAttribute::clone() const
{
    return new UnknownAttribute(data());
}

string
UnknownAttribute::str() const
{
    string s = "Unknown Attribute ";
    for (size_t i=0; i< wire_size(); i++)
	s += c_format("%x ", _data[i]);
    return s;
}


/**
 * PathAttribute
 */

PathAttribute *
PathAttribute::create(const uint8_t* d, uint16_t max_len,
		size_t& l /* actual length */)
	throw(CorruptMessage)
{
    PathAttribute *pa;

    if (max_len < 3)	// must be at least 3 bytes!
	xorp_throw(CorruptMessage,
                c_format("PathAttribute too short %d bytes", max_len),
                        UPDATEMSGERR, UNSPECIFIED);

    // compute length, which is 1 or 2 bytes depending on flags d[0]
    if ( (d[0] & Extended) && max_len < 4)
	xorp_throw(CorruptMessage,
                c_format("PathAttribute (extended) too short %d bytes",
			    max_len),
                        UPDATEMSGERR, UNSPECIFIED);
    l = length(d) + (d[0] & Extended ? 4 : 3);
    if (max_len < l)
	xorp_throw(CorruptMessage,
                c_format("PathAttribute too short %d bytes need %u",
			    max_len, (uint32_t)l),
                        UPDATEMSGERR, UNSPECIFIED);

    // now we are sure that the data block is large enough.
    debug_msg("++ create type %d max_len %d actual_len %u\n",
	d[1], max_len, (uint32_t)l);
    switch (d[1]) {	// depending on type, do the right thing.
    case ORIGIN:
	pa = new OriginAttribute(d);
	break; 

    case AS_PATH:
	pa = new ASPathAttribute(d);
	break;  
     
    case NEXT_HOP:
	pa = new IPv4NextHopAttribute(d);
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
	pa = new AggregatorAttribute(d);
	break;

    case COMMUNITY:
	pa = new CommunityAttribute(d);
	break;
         
    case MP_REACH_NLRI:
	pa = new MPReachNLRIAttribute<IPv6>(d);
	break;

    case MP_UNREACH_NLRI:
	pa = new MPUNReachNLRIAttribute<IPv6>(d);
	break;
	
    default:
	pa = new UnknownAttribute(d);
	break;
    }
    return pa;
}

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

    case COMMUNITY:
	s += "COMMUNITY";
	break;

    case MP_REACH_NLRI:
	s += "MP_REACH_NLRI";
	break;

    case MP_UNREACH_NLRI:
	s += "MP_UNREACH_NLRI";
	break;

    default:
	s += c_format("UNKNOWN(%d)", type());
    }
    return s;
}

bool
PathAttribute::operator<(const PathAttribute& him) const
{
    if (sorttype() < him.sorttype())
	return true;
    if (sorttype() > him.sorttype())
	return false;
    // equal sorttypes imply equal types
    if (wire_size() < him.wire_size())
	return true;
    if (wire_size() > him.wire_size())
	return false;
    // same size, must compare payload
    switch (type()) {
    default:
	return (memcmp(data(), him.data(), wire_size()) < 0);
	break;

    case AS_PATH:
	return ( ((ASPathAttribute &)*this).as_path() <
		((ASPathAttribute &)him).as_path() );

    case NEXT_HOP:
	return ( ((NextHopAttribute<IPv4> &)*this).nexthop() <
		((NextHopAttribute<IPv4> &)him).nexthop() );
    }
}

bool
PathAttribute::operator==(const PathAttribute& him) const
{
    return (type() == him.type() && wire_size() == him.wire_size() &&
    	memcmp(data(), him.data(), wire_size()) == 0);
}

uint8_t *
PathAttribute::set_header(size_t payload_size)
{
    _size = payload_size;
    if (payload_size > 255)
	_flags |= Extended;
    else
	_flags &= ~Extended;
	
    _data = new uint8_t[wire_size()];	// allocate buffer
    _data[0] = flags() & ValidFlags;
    _data[1] = type();
    if (extended()) {
	_data[2] = (_size>>8) & 0xff;
	_data[3] = _size & 0xff;
	return _data + 4;
    } else {
	_data[2] = _size & 0xff;
	return _data + 3;
    }
}

/*
 * PathAttributeList
 */

//#define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#define PARANOID

template<class A>
PathAttributeList<A>::PathAttributeList<A>()
{
    _nexthop_att = NULL;
    _aspath_att = NULL;
    _origin_att = NULL;
    memset(_hash, 0, 16);
}

template<class A>
PathAttributeList<A>::
  PathAttributeList<A>(const NextHopAttribute<A> &nexthop_att,
			  const ASPathAttribute &aspath_att,
			  const OriginAttribute &origin_att)
{
    // no need to clear the *_att pointers, will be done by these 3 adds.
    add_path_attribute(origin_att);
    add_path_attribute(nexthop_att);
    add_path_attribute(aspath_att);
    rehash();
}

template<class A>
PathAttributeList<A>::PathAttributeList<A>(const PathAttributeList<A>& palist)
	: list <PathAttribute *>()
{
    _nexthop_att = NULL;
    _aspath_att = NULL;
    _origin_att = NULL;
    for (const_iterator i = palist.begin(); i != palist.end() ; ++i)
	add_path_attribute(**i);
    rehash();
}

template<class A>
PathAttributeList<A>::~PathAttributeList<A>()
{
    debug_msg("++ ~PathAttributeList delete:\n%s\n", str().c_str());
    for (const_iterator i = begin(); i != end() ; ++i)
	delete (*i);
}

template<class A>
void
PathAttributeList<A>::add_path_attribute(const PathAttribute &att)
{
    PathAttribute *a = att.clone();
    // store a reference to the mandatory attributes, ignore others
    switch (att.type()) {
    default:
	break;

    case ORIGIN:
	_origin_att = dynamic_cast<OriginAttribute *>(a);
	break;

    case AS_PATH:
	_aspath_att = dynamic_cast<ASPathAttribute *>(a);
	break;

    case NEXT_HOP:
	_nexthop_att = dynamic_cast<NextHopAttribute<A> *>(a);
	/* If a nexthop of the wrong address family sneaks through nail it */
	XLOG_ASSERT(0 != _nexthop_att);
	break;
    }
    // Keep the list sorted
    debug_msg("++ add_path_attribute %s\n", att.str().c_str());
    if (!empty()) {
	iterator i;
	for (i = begin(); i != end(); i++)
	    if ( *(*i) > *a) {
		insert(i, a);
		memset(_hash, 0, 16);
		return;
	    }
    }
    // list empty, or tail insertion:
    push_back(a);
    memset(_hash, 0, 16);
}

template<class A>
string
PathAttributeList<A>::str() const
{
    string s;
    for (const_iterator i = begin(); i != end() ; ++i)
	s += (*i)->str() + "\n";
    return s;
}

template<class A>
void
PathAttributeList<A>::rehash()
{
    MD5_CTX context;
    MD5_Init(&context);
    for (const_iterator i = begin(); i != end(); i++)
	(*i)->add_hash(&context);
    MD5_Final(_hash, &context);
}

template<class A>
bool
PathAttributeList<A>::
operator< (const PathAttributeList<A> &him) const
{
    assert_rehash();
    him.assert_rehash();
    debug_msg("PathAttributeList operator< %p %p\n", this, &him);

    if ((*_nexthop_att) < (*(him._nexthop_att)))
        return true;
    if ((*(him._nexthop_att)) < (*_nexthop_att))
        return false;
    if (size() < him.size())
        return true;
    if (him.size() < size())
        return false;

    //    return (memcmp(_hash, him.hash(), 16) < 0);
    const_iterator my_i = begin();
    const_iterator his_i = him.begin();
    for (;;) {
	// are they equal.
	if ((my_i == end()) && (his_i == him.end())) {
	    return false;
	}
	if (my_i == end()) {
	    return true;
	}
	if (his_i == him.end()) {
	    return false;
	}
	if ((**my_i) < (**his_i)) {
	    return true;
	}
	if ((**his_i) < (**my_i)) {
	    return false;
	}
	my_i++;
	his_i++;
    }
    XLOG_UNREACHABLE();
}

template<class A>
bool
PathAttributeList<A>::
operator == (const PathAttributeList<A> &him) const
{
    debug_msg("PathAttributeList operator== %p %p\n", this, &him);
    assert_rehash();
    him.assert_rehash();
    return (memcmp(_hash, him.hash(), 16) == 0);
}

template<class A>
void
PathAttributeList<A>::replace_attribute(PathAttribute* new_att)
{
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
	    memset(_hash, 0, 16);
	    return;
	}
    XLOG_UNREACHABLE();
}

template<class A>
void
PathAttributeList<A>::replace_AS_path(const AsPath& new_as_path)
{
    replace_attribute(new ASPathAttribute(new_as_path));
}

template<class A>
void
PathAttributeList<A>::replace_nexthop(const A& new_nexthop)
{
    replace_attribute(new NextHopAttribute<A>(new_nexthop));
}

template<class A>
void
PathAttributeList<A>::remove_attribute_by_type(PathAttType type)
{
    // we only remove the first instance of an attribute with matching type
    iterator i;
    for (i = begin(); i != end(); i++) {
	if ((*i)->type() == type) {
	    delete *i;
	    erase(i);
	    memset(_hash, 0, 16);
	    return;
	}
    }
}

template<class A>
void
PathAttributeList<A>::process_unknown_attributes()
{
    iterator i;
    for (i = begin(); i != end();) {
	iterator tmp = i++;
	if (dynamic_cast<UnknownAttribute *>(*tmp)) {
	    if((*tmp)->transitive()) {
		(*tmp)->set_partial();
	    } else {
		delete *tmp;
		erase(tmp);
	    }
	    memset(_hash, 0, 16);
	}
    }
}

template<class A>
const PathAttribute*
PathAttributeList<A>::find_attribute_by_type(PathAttType type) const
{
    const_iterator i;
    for (i = begin(); i != end(); i++)
	if ((*i)->type() == type)
	    return *i;
    return NULL;
}

template<class A>
const MEDAttribute* 
PathAttributeList<A>::med_att() const 
{
    return dynamic_cast<const MEDAttribute*>(find_attribute_by_type(MED));
}

template<class A>
const LocalPrefAttribute*
PathAttributeList<A>::local_pref_att() const 
{
    return dynamic_cast<const LocalPrefAttribute*>(
		find_attribute_by_type(LOCAL_PREF));
}

template<class A>
const AtomicAggAttribute*
PathAttributeList<A>::atomic_aggregate_att() const 
{
    return dynamic_cast<const AtomicAggAttribute*>(
	find_attribute_by_type(ATOMIC_AGGREGATE));
}

template<class A>
const AggregatorAttribute*
PathAttributeList<A>::aggregator_att() const 
{
    return dynamic_cast<const AggregatorAttribute*>(
	find_attribute_by_type(AGGREGATOR));
}

/*
 * check whether the class has been properly rehashed.
 * A _hash of all 0's means with high probability that we forgot to
 * recompute it.
 */
template<class A>
void
PathAttributeList<A>::assert_rehash() const
{
#ifdef PARANOID
    for (int i = 0; i<16; i++)
	if (_hash[i]!=0)
	    return;
    XLOG_FATAL("Missing rehash - attempted to use modified PathAttributeList without first calling rehash()\n");
#endif
}

template class PathAttributeList<IPv4>;
template class PathAttributeList<IPv6>;

