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

#ident "$XORP: xorp/bgp/path_attribute.cc,v 1.15 2003/02/01 04:41:54 mjh Exp $"

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
         
    default:
	//this will throw an error if the attribute isn't
	// optional transitive
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

/******************** DERIVED CLASSES ***************************/

/**
 * OriginAttribute
 */

OriginAttribute::OriginAttribute(OriginType t)
	: PathAttribute(Transitive, ORIGIN), _origin(t)
{
    encode();
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
 * UnknownAttribute
 */
UnknownAttribute::UnknownAttribute(const uint8_t* d)
	throw(CorruptMessage)
	: PathAttribute(d)
{
    if (!optional() || !transitive())
	xorp_throw(CorruptMessage,
		   "Bad Flags in Unknown attribute",
		   UPDATEMSGERR, ATTRFLAGS);
    _size = length(d);
    _data = new uint8_t[wire_size()];
    memcpy(_data, d, wire_size());
}

string
UnknownAttribute::str() const
{
    string s = "Unknown Attribute ";
    for (size_t i=0; i< wire_size(); i++)
	s += c_format("%x ", _data[i]);
    return s;
}
