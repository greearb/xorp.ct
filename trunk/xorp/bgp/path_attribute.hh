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

// $XORP: xorp/bgp/path_attribute.hh,v 1.4 2003/01/17 05:51:07 mjh Exp $

#ifndef __BGP_PATH_ATTRIBUTE_HH__
#define __BGP_PATH_ATTRIBUTE_HH__

#include <unistd.h>
#include <string>
#include <set>
#include "libxorp/debug.h"

#include "config.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "aspath.hh"
#include "md5.h"

/**
 * PathAttribute
 *
 * components of the path attribute. They have variable sizes
 * The actual layout in the packet is the following:
 *	[ flags ]       1 byte
 *	[ type  ]       1 byte
 *	[ len   ][....] 1 or 2 bytes
 *	[ data     ]    len bytes
 */

enum PathAttType {
    ORIGIN = 1,
    AS_PATH = 2,
    NEXT_HOP = 3,
    MED = 4,
    LOCAL_PREF = 5,
    ATOMIC_AGGREGATE = 6,
    AGGREGATOR = 7,
    COMMUNITY = 8,
    UNKNOWN = 255
};

//PathAttSortType is only used in sorting a path attribute list - it's
// different from PathAttType because we want to sort the path
//attribute list on NextHop for less expensive processing when the IGP
// information for a nexthop changes.
typedef uint8_t  PathAttSortType;

#define SORT_NEXT_HOP  1
#define SORT_ORIGIN  2
#define SORT_AS_PATH  3
#define SORT_MED  4
#define SORT_LOCAL_PREF  5
#define SORT_ATOMIC_AGGREGATE 6
#define SORT_AGGREGATOR 7
#define SORT_COMMUNITY 8


//Origin values
enum OriginType {
    IGP = 0,
    EGP = 1,
    INCOMPLETE = 2
};

class PathAttribute
{
public:
    enum Flags {
	Optional	= 0x80,
	Transitive	= 0x40,
	Partial		= 0x20,
	Extended	= 0x10,
	ValidFlags	= 0xf0,
	NoFlags		= 0
    };

    PathAttribute(uint8_t f) : _data(0), _attribute_length(0), _length(0) {
	set_flags(f);
    }

    virtual ~PathAttribute()			{ delete[] _data; }
    virtual void encode() = 0;
    virtual void decode() = 0;
    virtual const PathAttType type() const = 0;
    virtual const PathAttSortType sorttype() const = 0;
    virtual void add_hash(MD5_CTX *context);

    const uint8_t * encode_and_get_data();
    const uint8_t * get_data() const {
	assert(_data != NULL);
	return _data;
    }
    uint16_t get_size() const			{ return _length; }

    void pretty_print();

    void dump();
    uint8_t flags() const			{ return _flags; }
    void set_flags(const uint8_t f)		{
	assert ( (f & ValidFlags) == f);
	_flags = f;
    }

    virtual string str() const;
    bool operator<(const PathAttribute& him) const;
    bool operator==(const PathAttribute& him) const;
    bool optional() const		{ return _flags & Optional; }
    bool transitive() const		{ return _flags & Transitive; }
    bool partial() const		{ return _flags & Partial; }
    bool extended() const		{ return _flags & Extended; }
    bool well_known() const		{ return !optional(); }

protected:
    const uint8_t* _data; // data includes the PathAttribute header

    uint16_t _attribute_length; // the length of the attribute, minus
                                // the PathAttribute header
    uint8_t _length; // the length of the _data buffer (ie the
                             // attribute data plus the PathAttribute header
    uint8_t	_flags;
private:
    PathAttribute();	// not allowed.
};

class OriginAttribute : public PathAttribute
{
public:
    OriginAttribute(OriginType t);
    OriginAttribute(const OriginAttribute& origin);
    OriginAttribute(const uint8_t* d, uint16_t l);
    void encode();
    void decode();
    void add_hash(MD5_CTX *context);
    const PathAttType type() const { return ORIGIN; }
    const PathAttSortType sorttype() const { return SORT_ORIGIN; }
    OriginType origintype() const { return _origin; }
    string str() const;
    bool operator<(const OriginAttribute& him) const {
	return (_origin < him.origintype());
    }
    bool operator==(const OriginAttribute& him) const {
	return (_origin == him.origintype());
    }
protected:
private:
    OriginType _origin;
};

class ASPathAttribute : public PathAttribute
{
public:
    ASPathAttribute(const AsPath& as_path);
    ASPathAttribute(const ASPathAttribute& as_path);
    ASPathAttribute(const uint8_t* d, uint16_t l);
    void encode();
    void decode();
    const PathAttType type() const { return AS_PATH; }
    const PathAttSortType sorttype() const { return SORT_AS_PATH; }
    const AsPath &as_path() const { return _as_path; }
    string str() const;
    bool operator<(const ASPathAttribute& him) const {
	return _as_path < him.as_path();
    }
    bool operator==(const ASPathAttribute& him) const {
	return _as_path == him.as_path();
    }
protected:
private:
    AsPath _as_path;
    uint8_t _as_path_len;
    bool _as_path_ordered;
};

template <class A>
class NextHopAttribute : public PathAttribute
{
public:
    NextHopAttribute(const A& n);
    NextHopAttribute(const NextHopAttribute& nexthop);
    NextHopAttribute(const uint8_t* d, uint16_t l);
    // void set_nexthop(uint32_t t);
    //    void set_nexthop(const A &n) { _next_hop = n; debug_msg("IPv4 addr %s set as next hop\n", cstring(n)); }
    const A& nexthop() const { return _next_hop; }
    void encode();
    void decode();
    void add_hash(MD5_CTX *context);
    const PathAttType type() const { return NEXT_HOP; }
    const PathAttSortType sorttype() const { return SORT_NEXT_HOP; }
    string str() const;
    bool operator<(const NextHopAttribute& him) const {
	return (_next_hop < him.nexthop());
    }
    bool operator==(const NextHopAttribute& him) const {
	return (_next_hop == him.nexthop());
    }
protected:
private:
    A _next_hop;
};

typedef NextHopAttribute<IPv4> IPv4NextHopAttribute;
typedef NextHopAttribute<IPv6> IPv6NextHopAttribute;

class MEDAttribute : public PathAttribute
{
public:
    MEDAttribute(uint32_t med);
    MEDAttribute(const MEDAttribute& med_att);
    MEDAttribute(const uint8_t* d, uint16_t l);
    void encode();
    void decode();
    const PathAttType type() const { return MED; }
    const PathAttSortType sorttype() const { return SORT_MED; }
    uint32_t med() const { return _multiexitdisc; }
    string str() const;
    bool operator<(const MEDAttribute& him) const {
	return (_multiexitdisc < him.med());
    }
     bool operator==(const MEDAttribute& him) const {
	return (_multiexitdisc == him.med());
     }
protected:
private:
    uint32_t _multiexitdisc;
};

class LocalPrefAttribute : public PathAttribute
{
public:
    LocalPrefAttribute(uint32_t localpref);
    LocalPrefAttribute(const LocalPrefAttribute& local_pref);
    LocalPrefAttribute(const uint8_t* d, uint16_t l);
    void encode();
    void decode();
    const PathAttType type() const { return LOCAL_PREF; }
    const PathAttSortType sorttype() const { return SORT_LOCAL_PREF; }
    uint32_t localpref() const { return _localpref; }
    string str() const;
    bool operator<(const LocalPrefAttribute& him) const {
	return (_localpref < him.localpref());
    }
    bool operator==(const LocalPrefAttribute& him) const {
	return (_localpref == him.localpref());
    }
    inline static uint32_t default_value() {
	// The default Local Preference value is 100 according to Halabi.
	// This should probably be a configuration option.
	return 100;
    }
protected:
private:
    uint32_t _localpref;

};

class AtomicAggAttribute : public PathAttribute
{
public:
    AtomicAggAttribute();
    AtomicAggAttribute(const AtomicAggAttribute& atomic);
    AtomicAggAttribute(const uint8_t* d, uint16_t l);
    void encode();
    void decode();
    const PathAttType type() const { return ATOMIC_AGGREGATE; }
    const PathAttSortType sorttype() const { return SORT_ATOMIC_AGGREGATE; }
    string str() const;
    bool operator<(const AtomicAggAttribute&) const {
	return false;
    }
    bool operator==(const AtomicAggAttribute&) const {
	return true;
    }
protected:
private:
};

class AggregatorAttribute : public PathAttribute
{
public:
    AggregatorAttribute(const IPv4& routeaggregator,
			   const AsNum& aggregatoras);
    AggregatorAttribute(const AggregatorAttribute& agg);
    AggregatorAttribute(const uint8_t* d, uint16_t l);
    void encode();
    void decode();
    const PathAttType type() const { return AGGREGATOR; }
    const PathAttSortType sorttype() const { return SORT_AGGREGATOR; }
    const IPv4& route_aggregator() const { return _routeaggregator; }
    const AsNum& aggregator_as() const { return _aggregatoras; }
    string str() const;
    bool operator<(const AggregatorAttribute& him) const {
	if (_routeaggregator == him.route_aggregator())
	    return (_aggregatoras < him.aggregator_as());
	return (_routeaggregator < him.route_aggregator());
    }
    bool operator==(const AggregatorAttribute& him) const {
	if ((_routeaggregator == him.route_aggregator())
	    && (_aggregatoras == him.aggregator_as()))
	    return true;

	return false;
    }
protected:
private:
    IPv4 _routeaggregator;
    AsNum _aggregatoras;
};

class CommunityAttribute : public PathAttribute
{
public:
    CommunityAttribute();
    CommunityAttribute(const CommunityAttribute& agg);
    CommunityAttribute(const uint8_t* d, uint16_t l);
    void add_community(uint32_t community);
    const set <uint32_t>& community_set() const { return _communities; }
    void encode();
    void decode();
    const PathAttType type() const { return COMMUNITY; }
    const PathAttSortType sorttype() const { return SORT_COMMUNITY; }
    string str() const;
    bool operator<(const CommunityAttribute& him) const;
    bool operator==(const CommunityAttribute& him) const;
protected:
private:
    set <uint32_t> _communities;
};

class UnknownAttribute : public PathAttribute
{
public:
    UnknownAttribute();
    UnknownAttribute(const UnknownAttribute& agg);
    UnknownAttribute(const uint8_t* d, uint16_t l);
    void encode();
    void decode();
    const PathAttType type() const { return UNKNOWN; }
    const PathAttSortType sorttype() const { return _type; }
    string str() const;
    bool operator<(const UnknownAttribute& him) const;
    bool operator==(const UnknownAttribute& him) const;
protected:
private:
    uint8_t _type;
};

#endif // __BGP_PATH_ATTRIBUTE_HH__
