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

#ident "$XORP: xorp/bgp/path_attribute.cc,v 1.82 2002/12/09 18:28:44 hodson Exp $"

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

PathAttribute::PathAttribute(bool optional, bool transitive,
				   bool partial, bool extended)
    : _optional(optional), _transitive(transitive),
    _partial(partial), _extended(extended)
{
    _length = 0;
    _data = NULL;
    _attribute_length = 0;
}

PathAttribute::PathAttribute()
{
}

PathAttribute::~PathAttribute()
{
    if (_data != NULL)
	delete[] _data;
}

void PathAttribute::decode()
{
    debug_msg("PathAttribute::decode called\n");
    dump_bytes(_data, _length);
    const uint8_t *data = _data;
    set_flags((uint8_t &)*data);

    data += 2;

    debug_msg("attribute type %d %d %d\n", type(), _length, _extended);

    if (_extended)
	{
	    debug_msg("Extended\n");
	    _attribute_length = ntohs((uint16_t &)(*data));
	    data += 2;
	}
    else
	{
	    debug_msg("Not Extended\n");
	    _attribute_length = (uint8_t &)(*data);
	    data++;
	}
}

void
PathAttribute::add_hash(MD5_CTX *context) const
{
    encode();
    MD5Update(context, _data, _length);
}

const uint8_t *
PathAttribute::get_data() const
{
    debug_msg("PathAttribute get_data() called\n");
    encode();
    return (const uint8_t *)_data;
}

void
PathAttribute::dump()
{
    switch (type()) {
    case ORIGIN:
	debug_msg("Path attribute of type ORIGIN \n");
	break;
    case AS_PATH:
	debug_msg("Path attribute of type AS_PATH \n");
	break;
    case NEXT_HOP:
	debug_msg("Path attribute of type NEXT_HOP \n");
	break;
    case MED:
	debug_msg("Path attribute of type MED \n");
	break;
    case LOCAL_PREF:
	debug_msg("Path attribute of type LOCAL_PREF \n");
	break;
    case ATOMIC_AGGREGATE:
	debug_msg("Path attribute of type ATOMIC_AGGREGATOR \n");
	break;
    case AGGREGATOR:
	debug_msg("Path attribute of type AGGREGATOR \n");
	break;
    case COMMUNITY:
	debug_msg("Path attribute of type COMMUNITY \n");
	break;
#if 0
    default:
	debug_msg("Path attribute of UNKNOWN type (%d)\n", type());
#endif
    }
}

// Returns the flags byte of a Path Attribute based on the parmeters set in the object.
uint8_t
PathAttribute::get_flags() const
{
    uint8_t flags = 0;
    if (_optional)
	flags = flags | 0x80;
    if (_transitive)
	flags = flags | 0x40;
    if (_partial)
	flags = flags | 0x20;
    if (_extended)
	flags = flags | 0x10;
    return flags;
}

void
PathAttribute::set_flags(uint8_t f)
{
    debug_msg("Flags value : %d\n", f);
    if ( (f & 0x80) == 0x80) {
	    _optional = true;
	    debug_msg("Optional flag\n");
    } else
	_optional = false;
    if ( (f & 0x40) == 0x40) {
	    _transitive = true;
	    debug_msg("Transative flag\n");
    } else
	_transitive = false;
    if ( (f & 0x20) == 0x20) {
	_partial = true;
	debug_msg("Partial flag\n");
    } else
	_partial = false;
    if ( (f & 0x10) == 0x10) {
	_extended = true;
	debug_msg("Extended flag\n");
    } else
	_extended = false;
}

string
PathAttribute::str() const
{
    string s;
    switch (type()) {
    case ORIGIN:
	s = "Path attribute of type ORIGIN";
	break;
    case AS_PATH:
	s = "Path attribute of type AS_PATH";
	break;
    case NEXT_HOP:
	s = "Path attribute of type NEXT_HOP";
	break;
    case MED:
	s = "Path attribute of type MED";
	break;
    case LOCAL_PREF:
	s = "Path attribute of type LOCAL_PREF";
	break;
    case ATOMIC_AGGREGATE:
	s = "Path attribute of type ATOMIC_AGGREGATOR";
	break;
    case AGGREGATOR:
	s = "Path attribute of type AGGREGATOR";
	break;
    case COMMUNITY:
	s = "Path attribute of type COMMUNITY";
	break;
#if 0
    default:
	// XXX this should be enabled in production code
	s = "Path attribute of UNKNOWN type";
	assert("UNKNOWN Path attribute\n");
#endif
    }
    return s;
}

bool
PathAttribute::operator<(const PathAttribute& him) const
{
    debug_msg("Comparing %s %s\n", str().c_str(), him.str().c_str());

    if (sorttype() < him.sorttype())
	return true;
    if (sorttype() > him.sorttype())
	return false;
    switch (type()) {
    case ORIGIN:
	return ((OriginAttribute&)*this) < ((OriginAttribute&)him);
    case AS_PATH:
	return ((ASPathAttribute&)*this) < ((ASPathAttribute&)him);
    case NEXT_HOP:
	if (dynamic_cast<const NextHopAttribute<IPv4>*>(this) != NULL) {
	    return (((NextHopAttribute<IPv4>&)*this)
		    < ((NextHopAttribute<IPv4>&)him));
	} else {
	    return (((NextHopAttribute<IPv6>&)*this)
		    < ((NextHopAttribute<IPv6>&)him));
	}
    case MED:
	return ((MEDAttribute&)*this) < ((MEDAttribute&)him);
    case LOCAL_PREF:
	return ((LocalPrefAttribute&)*this) < ((LocalPrefAttribute&)him);
    case ATOMIC_AGGREGATE:
	return ((AtomicAggAttribute&)*this) < ((AtomicAggAttribute&)him);
    case AGGREGATOR:
	return ((AggregatorAttribute&)*this) < ((AggregatorAttribute&)him);
    case COMMUNITY:
	return ((CommunityAttribute&)*this) < ((CommunityAttribute&)him);
    }
    abort();
}

bool
PathAttribute::operator==(const PathAttribute& him) const
{
    if (type() != him.type())
	return false;

    switch (type()) {
    case ORIGIN:
	return ((OriginAttribute&)*this) == ((OriginAttribute&)him);
    case AS_PATH:
	return ((ASPathAttribute&)*this) == ((ASPathAttribute&)him);
    case NEXT_HOP:
	if (dynamic_cast<const NextHopAttribute<IPv4>*>(this) != NULL) {
	    return (((NextHopAttribute<IPv4>&)*this)
		    == ((NextHopAttribute<IPv4>&)him));
	} else {
	    return (((NextHopAttribute<IPv6>&)*this)
		    == ((NextHopAttribute<IPv6>&)him));
	}
    case MED:
	return ((MEDAttribute&)*this) == ((MEDAttribute&)him);
    case LOCAL_PREF:
	return ((LocalPrefAttribute&)*this) == ((LocalPrefAttribute&)him);
    case ATOMIC_AGGREGATE:
	return ((AtomicAggAttribute&)*this) == ((AtomicAggAttribute&)him);
    case AGGREGATOR:
	return ((AggregatorAttribute&)*this) == ((AggregatorAttribute&)him);
    case COMMUNITY:
	return ((CommunityAttribute&)*this) == ((CommunityAttribute&)him);
    }
    abort();
}

/* ************ OriginAttribute ************* */

OriginAttribute::OriginAttribute(OriginType t)
    : PathAttribute(false, /*transitive*/true, false, false),
      _origin(t)
{
    XLOG_ASSERT(t == IGP || t == EGP || t == INCOMPLETE);

    _length = 4;
    _data = new uint8_t[_length];
}

OriginAttribute::OriginAttribute(const OriginAttribute& att)
    : PathAttribute(att._optional, att._transitive,
		       att._partial, att._extended)
{
    _origin = att._origin;
    if (att._data != NULL) {
	_length = att._length;
	uint8_t *p = new uint8_t[_length];
	memcpy(p, att._data, _length);
	_data = p;
    } else {
	_length = 0;
	_data = NULL;
    }
    _attribute_length = att._attribute_length;
}

OriginAttribute::OriginAttribute(const uint8_t* d, uint16_t l)
    : PathAttribute()
{
    dump_bytes(d, l);

    uint8_t *data = new uint8_t[l];
    memcpy(data, d, l);
    _length = l;
    _data = data;
    debug_msg("Origin length %d\n", l);
    decode();
}

//encode is const because we don't change the semantic value of the
// class instance - we only re-encode it.

void
OriginAttribute::encode() const
{
    if (_data != NULL)
	delete[] _data;


    // this attribute has static size of 4 bytes.
    _length = 4;
    uint8_t *data = new uint8_t[_length];
    data[0] = get_flags();
    data[1] = type();
    data[2] = 1; // length of data
    data[3] = _origin;
    _data = data;
}

void
OriginAttribute::decode()
{
    PathAttribute::decode();

    const uint8_t *data = _data;
    if (_extended) {
	data += 4;
    } else {
	data += 3;
    }

    _origin = (OriginType)(*data);
    debug_msg("Origin Attribute: ");
    switch (_origin) {
    case IGP:
	debug_msg("IGP\n");
	break;
    case EGP:
	debug_msg("EGP\n");
	break;
    case INCOMPLETE:
	debug_msg("INCOMPLETE\n");
	break;
    default:
	debug_msg("UNKNOWN (%d)\n", _origin);
	dump_bytes(data, _length);
	xorp_throw(CorruptMessage,
		   c_format("Unknown Origin Type %d", _origin),
		   UPDATEMSGERR, INVALORGATTR);
    }
    if (!well_known() || !transitive()) {
	xorp_throw(CorruptMessage,
		   "Bad Flags in Origin attribute",
		   UPDATEMSGERR, ATTRFLAGS,
		   get_data(), get_size());
    }
    debug_msg("Origin Attribute decode() finished \n");
}

void
OriginAttribute::add_hash(MD5_CTX *context) const
{
    MD5Update(context, (const uint8_t*)&_origin, sizeof(_origin));
}

string OriginAttribute::str() const
{
    string s = "Origin Path Attribute - ";
    switch (_origin) {
    case IGP:
	s = s + "IGP";
	break;
    case EGP:
	s = s + "EGP";
	break;
    case INCOMPLETE:
	s = s + "INCOMPLETE";
	break;
    default:
	s = s + "UNKNOWN";
    }
    return s;
}

#if 0
void
OriginAttribute::set_origintype(OriginType t)
{
    if (t == IGP || t == EGP || t == INCOMPLETE) {
	_origin = t;
    } else {
	debug_msg("Invalid Origin type (%d)\n", t);
	abort();
    }
}
#endif

/* ************ ASPathAttribute ************* */

ASPathAttribute::ASPathAttribute(const AsPath& init_as_path)
    : PathAttribute(false, /*transitive*/true, false, false),
    _as_path(init_as_path)
{
    _length = (uint8_t)0;
    _data = NULL;
}

ASPathAttribute::ASPathAttribute(const ASPathAttribute& att)
    : PathAttribute(att._optional, att._transitive,
		       att._partial, att._extended),
    _as_path(att._as_path)
{
    _as_path_len = att._as_path_len;
    _as_path_ordered = att._as_path_ordered;

    if (att._data != NULL) {
	_length = att._length;
	uint8_t *p = new uint8_t[_length];
	memcpy(p, att._data, _length);
	_data = p;
    } else {
	_length = 0;
	_data = NULL;
    }
    _attribute_length = att._attribute_length;
}

ASPathAttribute::ASPathAttribute(const uint8_t* d, uint16_t l)
    : PathAttribute()
{
    debug_msg("ASPathAttribute(%x, %d) constructor called\n",
	      (uint)d, l);
    uint8_t* data = new uint8_t[l];
    memcpy(data, d, l);
    _data = data;
    _length = l;
    decode();
}

void
ASPathAttribute::encode() const
{
    if (_data != NULL)
	delete[] _data;

    debug_msg("ASPathAttribute encode()\n");

    uint16_t counter = 0;

    const char * temp_data = _as_path.get_data(); // This must be first since it calculates the length as part of the process.
    uint16_t length = _as_path.get_byte_size(); // Get the length of the AS bytes, don't include this header length yet.
    debug_msg("ASPath length is %d\n", length);
    uint8_t *data = new uint8_t[length + 4];

    if (length > 255)
	PathAttribute::_extended = true;

    debug_msg("Length %d\n", length);

    data[0] = get_flags();
    data[1] = AS_PATH;

    if (_extended) {
	uint16_t temp = htons(length);
	memcpy(data+2,&temp, 2);
	counter = 4;
    } else {
	data[2] = length;
	counter = 3;
    }

    memcpy(data+counter, temp_data, length);

    _length = length + counter; // Include header length
    debug_msg("ASPath encode: length is %d\n", _length);
    _data = data;
}

void
ASPathAttribute::decode()
{
    PathAttribute::decode();
    debug_msg("AS Path Attribute:: decode\n");
    int temp_len;
    int as_len;

    const uint8_t *data = _data;
    int length = _attribute_length;

    if (_extended) {
	debug_msg("Extended AS Path!\n");
	data += 4;
    } else {
	data += 3;
    }

    while (length > 0) {
	dump_bytes(data, length);
	// TODO this assumes a 16 bit AS number
	temp_len = (*(data+1))*2+2;
	as_len = *(data+1);
	assert(temp_len <= length);

	debug_msg("temp_len %d, length %d, as_len %d\n", temp_len, length, as_len);
	int t = static_cast<ASPathSegType>(*data);
	switch (t) {
	case AS_SET:
	    debug_msg("Decoding AS Set\n");
	    _as_path.add_segment(AsSet(data, as_len));
	    break;
	case AS_SEQUENCE:
	    debug_msg("Decoding AS Sequence\n");
	    _as_path.add_segment(AsSequence(data, as_len));
	    break;
	default:
	    debug_msg("Wrong sequence type %d\n", t);
	    xorp_throw(CorruptMessage,
		   c_format("Wrong sequence type %d", t),
		       UPDATEMSGERR, MALASPATH);
	}
	length = length - temp_len;
	data = data + temp_len; // 2 bytes of header
    }
    if (!well_known() || !transitive()) {
	xorp_throw(CorruptMessage,
		   "Bad Flags in AS Path attribute",
		   UPDATEMSGERR, ATTRFLAGS,
		   get_data(), get_size());
    }
    debug_msg("after: >%s<\n", _as_path.str().c_str());
}

string
ASPathAttribute::str() const
{
    string s;
    s = "AS Path Attribute " + _as_path.str();
    return s;
}

/* ************ NextHopAttribute ************* */

template <class A>
NextHopAttribute<A>::NextHopAttribute<A>(const A& nh)
    :  PathAttribute(/*not optional*/false, /*transitive*/true,
			/*not partial*/false, /*not extended*/false),
    _next_hop(nh)
{
    _length = 3 + A::addr_size();
    _data = new uint8_t[_length];
}

template <class A>
NextHopAttribute<A>::
NextHopAttribute<A>(const NextHopAttribute<A>& att)
    : PathAttribute(att._optional, att._transitive,
		       att._partial, att._extended)
{
    _next_hop = att._next_hop;
    if (att._data != NULL) {
	_length = att._length;
	uint8_t *p = new uint8_t[_length];
	memcpy(p, att._data, _length);
	_data = p;
    } else {
	_length = 0;
	_data = NULL;
    }
    _attribute_length = att._attribute_length;
}

template <class A>
NextHopAttribute<A>::NextHopAttribute<A>(const uint8_t* d, uint16_t l)
{
    uint8_t *data = new uint8_t[l];
    memcpy(data, d, l);
    _data = data;
    _length = l;
    decode();
}

void
NextHopAttribute<IPv4>::encode() const
{
    if (_data != NULL)
	delete[] _data;

    // this attribute has static size of 7 bytes
    _length = 7;
    uint8_t *data = new uint8_t[_length];
    data[0] = get_flags();
    data[1] = NEXT_HOP;
    data[2] = 4; // length of data
    uint32_t addr = _next_hop.addr();
    memcpy(&data[3], &addr, 4);
    _data = data;
}

void
NextHopAttribute<IPv6>::encode() const
{
    // XXXX A true nexthop attribute in BGP can only be IPv4.  Need to
    // figure out the IPv6 stuff later.
    abort();
}

template <class A>
void
NextHopAttribute<A>::decode()
{
    PathAttribute::decode();

    const uint8_t *data = _data;
    if (_extended) {
	data += 4;
    } else {
	data += 3;
    }
    if (!well_known() || !transitive()) {
	xorp_throw(CorruptMessage,
		   "Bad Flags in NextHop attribute",
		   UPDATEMSGERR, ATTRFLAGS,
		   get_data(), get_size());
    }
    _next_hop = A((const uint8_t*)data);

    // check that the nexthop address is syntactically correct
    if (!_next_hop.is_unicast()) {
	xorp_throw(CorruptMessage,
		   c_format("NextHop %s is not a unicast address",
			    _next_hop.str().c_str()),
		   UPDATEMSGERR,
		   INVALNHATTR,
		   get_data(), get_size())
    }
    debug_msg("Next Hop Attribute \n");
}

template <class A>
void
NextHopAttribute<A>::add_hash(MD5_CTX *context) const {
    MD5Update(context, (const uint8_t*)&_next_hop, sizeof(_next_hop));
}

template <class A>
string
NextHopAttribute<A>::str() const
{
    string s;
    s = "Next Hop Attribute " + _next_hop.str();
    return s;
}


template class NextHopAttribute<IPv4>;
template class NextHopAttribute<IPv6>;


/* ************ MEDAttribute ************* */

MEDAttribute::MEDAttribute(uint32_t med)
    : PathAttribute(/*optional*/true, /*non-transitive*/false,
		       false, false),
    _multiexitdisc(med)
{
    _length = 7;
    _data = NULL;
}

MEDAttribute::MEDAttribute(const MEDAttribute& att)
    : PathAttribute(att._optional, att._transitive,
		       att._partial, att._extended),
    _multiexitdisc(att._multiexitdisc)
{
    if (att._data != NULL) {
	_length = att._length;
	uint8_t *p = new uint8_t[_length];
	memcpy(p, att._data, _length);
	_data = p;
    } else {
	_length = 0;
	_data = NULL;
    }
    _attribute_length = att._attribute_length;
}

MEDAttribute::MEDAttribute(const uint8_t* d, uint16_t l)
{
    uint8_t *data = new uint8_t[l];
    memcpy(data, d, l);
    _length = l;
    _data = data;
    decode();
}

void
MEDAttribute::encode() const
{
    if (_data != NULL)
	delete[] _data;

    // this attribute has static size of 7 bytes
    _length = 7;
    uint8_t *data = new uint8_t[_length];
    data[0] = get_flags();
    data[1] = MED;
    data[2] = 4; // length of data
    uint32_t med = htonl(_multiexitdisc);
    memcpy(data+3, &med, 4);
    _data = data;
}

void
MEDAttribute::decode()
{
    PathAttribute::decode();

    const uint8_t *data = _data;
    if (_extended) {
	data += 4;
    } else {
	data += 3;
    }
    if (!optional() || transitive()) {
	xorp_throw(CorruptMessage,
		   "Bad Flags in MED attribute",
		   UPDATEMSGERR, ATTRFLAGS,
		   get_data(), get_size());
    }
    debug_msg("Multi_exit discriminator Attribute\n");
    uint32_t med;
    memcpy(&med, data, 4);
    _multiexitdisc = htonl(med);
    debug_msg("Multi_exit desc %d\n", _multiexitdisc);
}

string
MEDAttribute::str() const
{
    return c_format("Multiple Exit Descriminator Attribute: MED=%d",
		    _multiexitdisc);
}
/* ************ LocalPrefAttribute ************* */

LocalPrefAttribute::LocalPrefAttribute(uint32_t localpref)
    : PathAttribute(false, /*transitive*/true, false, false),
      _localpref(localpref)
{
    //    _type = LOCAL_PREF;
    _length = 7;
    _data = NULL;
}

LocalPrefAttribute::
LocalPrefAttribute(const LocalPrefAttribute& att)
    : PathAttribute(att._optional, att._transitive,
		       att._partial, att._extended),
    _localpref(att._localpref)
{
    if (att._data != NULL) {
	_length = att._length;
	uint8_t *p = new uint8_t[_length];
	memcpy(p, att._data, _length);
	_data = p;
    } else {
	_length = 0;
	_data = NULL;
    }
    _attribute_length = att._attribute_length;
}

LocalPrefAttribute::LocalPrefAttribute(const uint8_t* d, uint16_t l)
{
    uint8_t *data = new uint8_t[l];
    memcpy(data, d, l);
    _data = data;
    _length = l;
    decode();
}

void
LocalPrefAttribute::encode() const
{
    if (_data != NULL)
	delete[] _data;

    // this attribute has static size of 7 bytes
    _length = 7;
    uint8_t *data = new uint8_t[_length];
    data[0] = get_flags();
    data[1] = LOCAL_PREF;
    data[2] = 4; // length of data
    debug_msg("Encoding Local Pref of %u\n", _localpref);
    uint32_t localpref = htonl(_localpref);
    memcpy(data+3, &localpref, 4);

    _data = data;
}

void
LocalPrefAttribute::decode()
{
    PathAttribute::decode();

    const uint8_t *data = _data;
    if (_extended) {
	data += 4;
    } else {
	data += 3;
    }
    if (!well_known() || !transitive()) {
	xorp_throw(CorruptMessage,
		   "Bad Flags in LocalPref attribute",
		   UPDATEMSGERR, ATTRFLAGS,
		   get_data(), get_size());
    }
    debug_msg("Local Preference Attribute\n");
    uint32_t localpref;
    memcpy(&localpref, data, 4);
    _localpref = ntohl(localpref);

    debug_msg("Local Preference Attribute %d\n", _localpref);

}

string
LocalPrefAttribute::str() const
{
    return c_format("Local Preference Attribute - %d", _localpref);
}
/* ************ AtomicAggAttribute ************* */

AtomicAggAttribute::
AtomicAggAttribute()
    : PathAttribute(false, true, false, false) /*well-known, discretionary*/
{
    _data = NULL;
    _length = 3;
}


AtomicAggAttribute::
AtomicAggAttribute(const AtomicAggAttribute& att)
    : PathAttribute(att._optional, att._transitive,
		       att._partial, att._extended)
{
    if (att._data != NULL) {
	_length = att._length;
	uint8_t *p = new uint8_t[_length];
	memcpy(p, att._data, _length);
	_data = p;
    } else {
	_length = 0;
	_data = NULL;
    }
    _attribute_length = att._attribute_length;
}

AtomicAggAttribute::AtomicAggAttribute(const uint8_t* d, uint16_t l)
{
    uint8_t *data = new uint8_t[l];
    memcpy(data, d, l);
    _data = data;
    _length = l;
    decode();
}

void
AtomicAggAttribute::encode() const
{
    if (_data != NULL)
	delete[] _data;

    // this attribute has static size of 3 bytes
    _length = 3;
    uint8_t *data = new uint8_t[_length];
    data[0] = get_flags();
    data[1] = ATOMIC_AGGREGATE;
    data[2] = 0;
    _data = data;
}

void
AtomicAggAttribute::decode()
{
    PathAttribute::decode();
    const uint8_t *data = _data;
    if (_extended) {
	data += 4;
    } else {
	data += 3;
    }
    if (!well_known() || !transitive()) {
	xorp_throw(CorruptMessage,
		   "Bad Flags in AtomicAggregate attribute",
		   UPDATEMSGERR, ATTRFLAGS,
		   get_data(), get_size());
    }
    debug_msg("Atomic Aggregate Attribute\n");

}

string
AtomicAggAttribute::str() const
{
    string s;
    s = "Atomic Aggregator Attribute";
    return s;
}
/* ************ AggregatorAttribute ************* */

AggregatorAttribute::
AggregatorAttribute(const IPv4& routeaggregator,
		       const AsNum& aggregatoras)
    : PathAttribute(/*optional*/true, /*transitive*/true, false, false),
      _routeaggregator(routeaggregator), _aggregatoras(aggregatoras)
{
    _length = 9;
    _data = NULL;
}

AggregatorAttribute::
AggregatorAttribute(const AggregatorAttribute& att)
    : PathAttribute(att._optional, att._transitive,
		       att._partial, att._extended),
    _routeaggregator(att._routeaggregator),
    _aggregatoras(att._aggregatoras)
{
    if (att._data != NULL) {
	_length = att._length;
	uint8_t *p = new uint8_t[_length];
	memcpy(p, att._data, _length);
	_data = p;
    } else {
	_length = 0;
	_data = NULL;
    }
    _attribute_length = att._attribute_length;
}

AggregatorAttribute::AggregatorAttribute(const uint8_t* d, uint16_t l)
    : _aggregatoras((uint32_t)0)
{
    uint8_t *data = new uint8_t[l];
    memcpy(data, d, l);
    _data = data;
    _length = l;
    decode();
}

void AggregatorAttribute::encode() const
{
    if (_data != NULL)
	delete[] _data;

    // this attribute has static size of 9 bytes;
    _length = 9;
    uint8_t *data = new uint8_t[_length];
    data[0] = get_flags();
    data[1] = AGGREGATOR;
    data[2] = 6;
    uint16_t agg_as = htons(_aggregatoras.as());
    memcpy(data + 3, &agg_as, 2);
    // This defined to be an IPv4 address in RFC 2858 (line 46)
    // Note: addr() is already in network byte order
    uint32_t agg_addr = _routeaggregator.addr();
    memcpy(data + 5, &agg_addr, 4);
    _data = data;
}

void AggregatorAttribute::decode()
{
    PathAttribute::decode();

    debug_msg("Aggregator Attribute\n");

    const uint8_t *data = _data;
    if (_extended) {
	data += 4;
    } else {
	data += 3;
    }
    // The internet draft sets this AS Number to be 2 octets (section 4.3)
    uint16_t agg_as;
    memcpy(&agg_as, data, 2);
    _aggregatoras = AsNum(ntohs(agg_as));
    data += 2;
    uint32_t agg_addr;
    memcpy(&agg_addr, data, 4);
    _routeaggregator = IPv4(agg_addr);
    if (!optional() || !transitive()) {
	xorp_throw(CorruptMessage,
		   "Bad Flags in Aggregator attribute",
		   UPDATEMSGERR, ATTRFLAGS,
		   get_data(), get_size());
    }
}

string AggregatorAttribute::str() const
{
    string s;

    s = "Aggregator Attribute " + _aggregatoras.str() + " " + _routeaggregator.str();
    return s;
}


/* ************ CommunityAttribute ************* */

CommunityAttribute::
CommunityAttribute()
    : PathAttribute(true, true, false, false) /*optional transitive*/
{
    _data = NULL;
    _length = 0;
}


CommunityAttribute::
CommunityAttribute(const CommunityAttribute& att)
    : PathAttribute(att._optional, att._transitive,
		    att._partial, att._extended)
{
    if (att._data != NULL) {
	_length = att._length;
	uint8_t *p = new uint8_t[_length];
	memcpy(p, att._data, _length);
	_data = p;
    } else {
	_length = 0;
	_data = NULL;
    }
    _attribute_length = att._attribute_length;
    set <uint32_t>::const_iterator iter = att._communities.begin();
    while (iter != att._communities.end()) {
	_communities.insert(*iter);
	++iter;
    }
}

CommunityAttribute::CommunityAttribute(const uint8_t* d, uint16_t l)
{
    uint8_t *data = new uint8_t[l];
    memcpy(data, d, l);
    _data = data;
    _length = l;
    decode();
}

void
CommunityAttribute::add_community(uint32_t community)
{
    _communities.insert(community);
}

void
CommunityAttribute::encode() const
{
    if (_data != NULL)
	delete[] _data;

    _length = 3 + (4 * _communities.size());
    assert(_length < 256);

    uint8_t *data = new uint8_t[_length];
    data[0] = get_flags();
    data[1] = COMMUNITY;
    data[2] = 4*_communities.size();
    set <uint32_t>::const_iterator iter = _communities.begin();
    uint8_t *current = data + 3;
    while (iter != _communities.end()) {
	uint32_t value;
	value = htonl(*iter);
	memcpy(current, &value, 4);
	current += 4;
	++iter;
    }
    _data = data;
}

void
CommunityAttribute::decode()
{
    PathAttribute::decode();
    const uint8_t *data = _data;
    if (_extended) {
	uint16_t len;
	memcpy(&len, data+2, 2);
	_length = htons(len);
	data += 4;
    } else {
	_length = data[2];
	data += 3;
    }
    if (!optional() || !transitive()) {
	xorp_throw(CorruptMessage,
		   "Bad Flags in Community attribute",
		   UPDATEMSGERR, ATTRFLAGS,
		   get_data(), get_size());
    }
    int remaining = _length;
    while (remaining > 0) {
	uint32_t value;
	memcpy(&value, data, 4);
	_communities.insert(htonl(value));
	data += 4;
	remaining -= 4;
    }

    debug_msg("Community Attribute\n");
}

string
CommunityAttribute::str() const
{
    string s;
    s = "Community Attribute ";
    set <uint32_t>::const_iterator iter = _communities.begin();
    while (iter != _communities.end()) {
	s += c_format("%u ", *iter);
	++iter;
    }

    return s;
}

bool
CommunityAttribute::operator<(const CommunityAttribute& him) const {
    if (_communities.size() < him.community_set().size())
	return true;
    if (him.community_set().size() < _communities.size())
	return false;
    set <uint32_t>::const_iterator his_i, my_i;
    my_i = _communities.begin();
    his_i = him.community_set().begin();
    while (my_i != _communities.end()) {
	if (*my_i < *his_i)
	    return true;
	if (*his_i < *my_i)
	    return false;
	his_i++;
	my_i++;
    }
    return false;
}

bool
CommunityAttribute::operator==(const CommunityAttribute& him) const {
    if (_communities.size() != him.community_set().size())
	return false;
    if (_communities == him.community_set())
	return true;
    return false;
}
