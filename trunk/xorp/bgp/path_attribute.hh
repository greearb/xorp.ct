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

// $XORP: xorp/bgp/path_attribute.hh,v 1.1.1.1 2002/12/11 23:55:49 hodson Exp $

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

enum PathAttType {
    ORIGIN = 1,
    AS_PATH = 2,
    NEXT_HOP = 3,
    MED = 4,
    LOCAL_PREF = 5,
    ATOMIC_AGGREGATE = 6,
    AGGREGATOR = 7,
    COMMUNITY = 8
};

//PathAttSortType is only used in sorting a path attribute list - it's
// different from PathAttType because we want to sort the path
//attribute list on NextHop for less expensive processing when the IGP
// information for a nexthop changes.
enum  PathAttSortType {
    SORT_NEXT_HOP = 1,
    SORT_ORIGIN = 2,
    SORT_AS_PATH = 3,
    SORT_MED = 4,
    SORT_LOCAL_PREF = 5,
    SORT_ATOMIC_AGGREGATE = 6,
    SORT_AGGREGATOR = 7,
    SORT_COMMUNITY = 8
};

//Origin values
enum OriginType {
    IGP = 0,
    EGP = 1,
    INCOMPLETE = 2
};

class PathAttribute
{
public:
    PathAttribute();
    PathAttribute(bool optional, bool transitive,
		     bool partial, bool extended);
    virtual ~PathAttribute();
    virtual void encode() const = 0;
    virtual void decode() = 0;
    virtual PathAttType type() const = 0;
    virtual PathAttSortType sorttype() const = 0;
    virtual void add_hash(MD5_CTX *context) const;

    const uint8_t * get_data() const;
    uint16_t get_size() const { return _length; }

    void pretty_print();

    void dump();
    uint8_t get_flags() const;
    void set_flags(const uint8_t f);
    virtual string str() const;
    bool operator<(const PathAttribute& him) const;
    bool operator==(const PathAttribute& him) const;
    bool optional() const { return _optional; }
    bool transitive() const { return _transitive; }
    bool partial() const { return _partial; }
    bool extended() const { return _extended; }
    bool well_known() const { return !_optional; }
protected:
    mutable const uint8_t* _data; // data includes the PathAttribute header

    mutable uint8_t _length; // the length of the _data buffer (ie the
                             // attribute data plus the PathAttribute header

    uint16_t _attribute_length; // the length of the attribute, minus
                                // the PathAttribute header
    bool _optional;
    bool _transitive;
    bool _partial;
    bool _extended;
private:
};

class OriginAttribute : public PathAttribute
{
public:
    OriginAttribute(OriginType t);
    OriginAttribute(const OriginAttribute& origin);
    OriginAttribute(const uint8_t* d, uint16_t l);
    void encode() const;
    void decode();
    void add_hash(MD5_CTX *context) const;
    PathAttType type() const { return ORIGIN; }
    PathAttSortType sorttype() const { return SORT_ORIGIN; }
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
    void encode() const;
    void decode();
    PathAttType type() const { return AS_PATH; }
    PathAttSortType sorttype() const { return SORT_AS_PATH; }
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
    void encode() const;
    void decode();
    void add_hash(MD5_CTX *context) const;
    PathAttType type() const { return NEXT_HOP; }
    PathAttSortType sorttype() const { return SORT_NEXT_HOP; }
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
    void encode() const;
    void decode();
    PathAttType type() const { return MED; }
    PathAttSortType sorttype() const { return SORT_MED; }
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
    void encode() const;
    void decode();
    PathAttType type() const { return LOCAL_PREF; }
    PathAttSortType sorttype() const { return SORT_LOCAL_PREF; }
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
    void encode() const;
    void decode();
    PathAttType type() const { return ATOMIC_AGGREGATE; }
    PathAttSortType sorttype() const { return SORT_ATOMIC_AGGREGATE; }
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
    void encode() const;
    void decode();
    PathAttType type() const { return AGGREGATOR; }
    PathAttSortType sorttype() const { return SORT_AGGREGATOR; }
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
    void encode() const;
    void decode();
    PathAttType type() const { return COMMUNITY; }
    PathAttSortType sorttype() const { return SORT_COMMUNITY; }
    string str() const;
    bool operator<(const CommunityAttribute& him) const;
    bool operator==(const CommunityAttribute& him) const;
protected:
private:
    set <uint32_t> _communities;
};

#endif // __BGP_PATH_ATTRIBUTE_HH__
