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

// $XORP: xorp/bgp/path_attribute.hh,v 1.23 2003/09/04 18:03:25 atanu Exp $

#ifndef __BGP_PATH_ATTRIBUTE_HH__
#define __BGP_PATH_ATTRIBUTE_HH__

#include "config.h"
#include "libxorp/xorp.h"

#include <unistd.h>
#include <openssl/md5.h>

#include <list>
#include <string>
#include <set>

#include "libxorp/debug.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"
#include "exceptions.hh"	// for CorruptMessage exception

#include "aspath.hh"

/**
 * PathAttribute
 *
 * components of the path attribute. They have variable sizes
 * The actual layout on the wire is the following:
 *	[ flags ]       1 byte
 *	[ type  ]       1 byte
 *	[ len   ][....] 1 or 2 bytes
 *	[ data     ]    len bytes
 *
 * PathAttribute is the base class for a set of derived class which
 * represent the various attributes.
 * A PathAttribute object of a given type can be created explicitly,
 * using one of the constructors, and then adding components to it;
 * or it can be created by calling the create() method on a block
 * of data received from the wire.
 *
 * In addition to the parsed components (next hops, AS numbers and paths,
 * and various other attributes), the objects always contain the wire
 * representation of the object, a pointer to which is accessible with
 * the data() method, and whose size is size().
 * Whenever the object is altered, the wire representation needs to be
 * recomputed.
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
    MP_REACH_NLRI = 14,
    MP_UNREACH_NLRI = 15,
};

class PathAttribute {
public:
    enum Flags {
	Optional	= 0x80,
	Transitive	= 0x40,
	Partial		= 0x20,
	Extended	= 0x10,
	ValidFlags	= 0xf0,
	NoFlags		= 0
    };

    /**
     * main routine to create a PathAttribute from incoming data.
     * Takes a chunk of memory of size l, returns an object of the
     * appropriate type and actual_length is the number of bytes used
     * from the packet.
     * Throws an exception on error.
     */
    static PathAttribute *create(const uint8_t* d, uint16_t max_len,
                size_t& actual_length) throw(CorruptMessage);

    /**
     * Make a copy of the current attribute.
     * The derived class should use new to generate a copy of
     * itself. The wire format representation will not be used by the
     * caller.
     */
    virtual PathAttribute *clone() const = 0;

    /*
     * The destructor, invoked after the derived class' destructors,
     * frees the internal representation of the object.
     */
    virtual ~PathAttribute()			{ delete[] _data; }

    /*@
     * @return a pointer to the wire representation of the packet.
     */
    const uint8_t *data() const			{
	assert( _data != 0 );
	return _data;
    }

    /**
     * @return the size of the wire representation.
     */
    size_t wire_size() const			{
	return _size + header_size();
    }

    /**
     * @return the size of the header.
     */
    size_t header_size() const				{
	return extended() ? 4 : 3;
    }
    /**
     * @return the type of the attribute
     */
    PathAttType type() const			{ return (PathAttType)_type; }

    /**
     * @return the flags for the attribute
     */
    Flags flags() const				{ return (Flags)_flags; }

    /**
     * Set the partial flag
     */
    void set_partial() { _data[0] |= Partial;_flags |= Partial; }

    /**
     * comparison operators are used to sort attributes.
     * Right now the sort order is based on the type,
     * size() and payload representation.
     */
    bool operator<(const PathAttribute& him) const;
    bool operator==(const PathAttribute& him) const;

    /**
     * compute the hash for this object.
     */
    void add_hash(MD5_CTX *context) const	{
	MD5_Update(context, data(), wire_size());
    }

    virtual string str() const;

    void pretty_print();

    bool optional() const			{ return _flags & Optional; }
    bool transitive() const			{ return _flags & Transitive; }
    bool partial() const			{ return _flags & Partial; }
    bool extended() const			{ return _flags & Extended; }
    bool well_known() const			{ return !optional(); }

protected:
    /**
     * sorttype() is only used in sorting a path attribute list.
     * It is different from PathAttType because we want to sort the path
     * attribute list on NextHop for less expensive processing when the IGP
     * information for a nexthop changes.
     * So we give priority to NEXT_HOP and keep other values unchanged.
     */
    const int sorttype() const			{
	return type() == NEXT_HOP ? -1 : type();
    }

    /**
     * helper constructor used when creating an object from a derived class.
     * NOTE: it does not provide a usable object as _data is invalid.
     */
    PathAttribute(Flags f, PathAttType t)
	    : _size(0), _data(0),
		_flags(f & ValidFlags), _type(t)	{}

    /**
     * basic constructor from data, assumes that the block has at least the
     * required size.
     * NOTE: it does not provide a usable object as _data is invalid.
     */
    PathAttribute(const uint8_t *d)
	    : _size(length(d)), _data(0),
		_flags(d[0] & ValidFlags), _type(d[1])	{}

    /**
     * helper function to fill the header. Needs _flags and _type
     * properly initialized, overwrites _size and _data by allocating a
     * new block of appropriate size (payload_size + 3 or 4 header bytes).
     */
    uint8_t *set_header(size_t payload_size);

    /**
     * fetch the length from the header. Assume the header is there.
     */
    static size_t length(const uint8_t* d)	{
	return (d[0] & Extended) ?
		( (d[2]<<8) + d[3] ) : d[2] ;
    }

    // helper function returning a pointer to the payload
    const uint8_t *payload(const uint8_t *d)	{
	return d + ((d[0] & Extended) ? 4 : 3);
    }

    // storage for information in the attribute.

    size_t	_size;	// this is only the size of the payload.
    uint8_t *	_data;	// wire representation
    uint8_t	_flags;
    uint8_t	_type;

private:
    PathAttribute();	// forbidden
    PathAttribute(const PathAttribute &); // forbidden
    PathAttribute& operator=(const PathAttribute &); // forbidden
};


// Origin values
enum OriginType {
    IGP = 0,
    EGP = 1,
    INCOMPLETE = 2
};

/**
 * OriginAttribute has a payload of size 1 containing the origin type.
 */

class OriginAttribute : public PathAttribute
{
public:
    OriginAttribute(OriginType t);
    OriginAttribute(const uint8_t* d) throw(CorruptMessage);
    PathAttribute *clone() const;

    string str() const;

    OriginType origin() const			{ return _origin; }

protected:
private:
    void encode();
    OriginType	_origin;
};

/**
 * ASPathAttribute contain an ASPath, whose structure is documented
 * in aspath.hh
 */
class ASPathAttribute : public PathAttribute
{
public:
    ~ASPathAttribute()				{ delete _as_path; }

    ASPathAttribute(const AsPath& p);
    ASPathAttribute(const uint8_t* d) throw(CorruptMessage);
    PathAttribute *clone() const;

    string str() const				{
	return "AS Path Attribute " + as_path().str();
    }

    const AsPath &as_path() const		{ return (AsPath &)*_as_path; }

protected:
private:
    void encode();
    AsPath *_as_path;
};

/**
 * NextHopAttribute contains the IP address of the next hop.
 */
template <class A>
class NextHopAttribute : public PathAttribute
{
public:
    NextHopAttribute(const A& n);
    NextHopAttribute(const uint8_t* d) throw(CorruptMessage);
    PathAttribute *clone() const;

    string str() const				{
	return "Next Hop Attribute " + _next_hop.str();
    }

    const A& nexthop() const			{ return _next_hop; }
    // This method is for use in MPReachNLRIAttribute only.
    void set_nexthop(const A& n) 		{ _next_hop = n; }
protected:
private:
    void encode();
    A _next_hop;
};

typedef NextHopAttribute<IPv4> IPv4NextHopAttribute;
typedef NextHopAttribute<IPv6> IPv6NextHopAttribute;


/**
 * MEDAttribute is an optional non-transitive uint32
 */
class MEDAttribute : public PathAttribute
{
public:
    MEDAttribute(const uint32_t med);
    MEDAttribute(const uint8_t* d) throw(CorruptMessage);
    PathAttribute *clone() const;

    string str() const;

    uint32_t med() const			{ return _med; }
protected:
private:
    void encode();
    uint32_t _med;	// XXX stored in host format!
};


/**
 * LocalPrefAttribute is a well-known uint32
 */
class LocalPrefAttribute : public PathAttribute
{
public:
    LocalPrefAttribute(const uint32_t localpref);
    LocalPrefAttribute(const uint8_t* d) throw(CorruptMessage);
    PathAttribute *clone() const;

    string str() const;

    uint32_t localpref() const			{ return _localpref; }
    static uint32_t default_value() {
	// The default Local Preference value is 100 according to Halabi.
	// This should probably be a configuration option.
	return 100;
    }
protected:
private:
    void encode();
    uint32_t _localpref;

};

class AtomicAggAttribute : public PathAttribute
{
public:
    AtomicAggAttribute();
    AtomicAggAttribute(const uint8_t* d) throw(CorruptMessage);
    PathAttribute *clone() const;

    string str() const				{
	return "Atomic Aggregate Attribute";
    }
protected:
private:
};

class AggregatorAttribute : public PathAttribute
{
public:
    AggregatorAttribute(const IPv4& speaker, const AsNum& as);
    AggregatorAttribute(const uint8_t* d) throw(CorruptMessage);
    PathAttribute *clone() const;

    string str() const;

    const IPv4& route_aggregator() const	{ return _speaker; }
    const AsNum& aggregator_as() const		{ return _as; }
protected:
private:
    void encode();
    IPv4 _speaker;
    AsNum _as;
};

class CommunityAttribute : public PathAttribute
{
public:
    typedef set <uint32_t>::const_iterator const_iterator;
    CommunityAttribute();
    CommunityAttribute(const uint8_t* d) throw(CorruptMessage);
    PathAttribute *clone() const;

    string str() const;

    const set <uint32_t>& community_set() const { return _communities; }
    void add_community(uint32_t community);
protected:
private:
    void encode();
    set <uint32_t> _communities;
};

template <class A>
class MPReachNLRIAttribute : public PathAttribute
{
public:
    typedef typename list<IPNet<A> >::const_iterator const_iterator;

    /**
     * Specialise these constructors for each AFI/SAFI pairing.
     */
    MPReachNLRIAttribute();
    MPReachNLRIAttribute(const uint8_t* d) throw(CorruptMessage);
    PathAttribute *clone() const;

    string str() const;

    const A& nexthop() const		{ return _nexthop_att.nexthop(); }
    void set_nexthop(const A& nexthop)	{ _nexthop_att.set_nexthop(nexthop); }
    NextHopAttribute<A> *nexthop_att() { return &_nexthop_att;}

    void add_nlri(const IPNet<A>& nlri) {_nlri.push_back(nlri); }
    const list<IPNet<A> >& nlri_list() const { return _nlri;}

    // IPv6 specific
    const A& link_local_nexthop() const	{ return _link_local_next_hop; }
    void set_link_local_nexthop(const A& n) { _link_local_next_hop = n;}

    // SNPA - Don't deal. (ATM, FRAME RELAY, SMDS)

    void encode();
protected:
private:

    uint16_t _afi;		// Address Family Identifier.
    uint8_t _safi;		// Subsequent Address Family Identifier.
    
    NextHopAttribute<A> _nexthop_att;	// Place the nexthop value in
					// a NextHopAttribute so that
					// we can return a pointer to
					// a path attribute when required.
//     list<A> _snpa;		// Subnetwork point of attachment.
    list<IPNet<A> > _nlri;	// Network level reachability information.

    A _link_local_next_hop;	// Link local next hop IPv6 specific.
};

template <class A>
class MPUNReachNLRIAttribute : public PathAttribute
{
public:
    typedef typename list<IPNet<A> >::const_iterator const_iterator;

    /**
     * Specialise these constructors for each AFI/SAFI pairing.
     */
    MPUNReachNLRIAttribute();
    MPUNReachNLRIAttribute(const uint8_t* d) throw(CorruptMessage);
    PathAttribute *clone() const;

    string str() const;

    void add_withdrawn(const IPNet<A>& nlri) {_withdrawn.push_back(nlri); }
    const list<IPNet<A> >& wr_list() const { return _withdrawn;}

    void encode();
protected:
private:

    uint16_t _afi;		// Address Family Identifier.
    uint8_t _safi;		// Subsequent Address Family Identifier.
    
    list<IPNet<A> > _withdrawn;	// Withdrawn routes.
};

class UnknownAttribute : public PathAttribute
{
public:
    UnknownAttribute(const uint8_t* d) throw(CorruptMessage);
    PathAttribute *clone() const;

    string str() const;
protected:
private:
};

/**
 * PathAttributeList is used to handle efficiently path attribute lists.
 *
 * An object in the class is initialized from explicit PathAttribute
 * objects passed in by reference. The initialization creates a copy
 * of the attribute, links it into a list, and for mandatory attributes
 * it also stores a pointer to the newly created attribute into a
 * class member (e.g. _aspath_att ...) for ease of use.
 */
template<class A>
class PathAttributeList : public list <PathAttribute*> {
public:
    typedef list<PathAttribute*>::const_iterator const_iterator;
    typedef list<PathAttribute*>::iterator iterator;

    PathAttributeList();
    PathAttributeList(const NextHopAttribute<A> &nexthop,
			 const ASPathAttribute &aspath,
			 const OriginAttribute &origin);
    PathAttributeList(const PathAttributeList<A>& palist);
    ~PathAttributeList();
    void add_path_attribute(const PathAttribute &att);
    const A& nexthop() const		{ return _nexthop_att->nexthop(); }
    const AsPath& aspath() const	{ return _aspath_att->as_path(); }
    const uint8_t origin() const	{ return _origin_att->origin(); }

    const MEDAttribute* med_att() const;
    const LocalPrefAttribute* local_pref_att() const;
    const AtomicAggAttribute* atomic_aggregate_att() const;
    const AggregatorAttribute* aggregator_att() const;

    void rehash();
    const uint8_t* hash() const			{
	assert_rehash();
	return _hash;
    }

    // complete() is true when all the mandatory attributes are present
    bool complete() const			{
	return ((_nexthop_att != NULL) &&
		(_aspath_att != NULL) && (_origin_att != NULL));
    }

    void replace_nexthop(const A& nexthop);
    void replace_AS_path(const AsPath& as_path);
    void remove_attribute_by_type(PathAttType type);
    /**
     * For unknown attributes:
     *	1) If transitive set the partial bit.
     *  2) If not transitive remove.
     */
    void process_unknown_attributes();

    string str() const;

    /* operator< is used to store and search for PathAttributeLists in
       STL containers.  In principle, it doesn't matter what the order
       is, so long as there is a strict monotonicity to the ordering */
    /* In practice, the ordering is important - we want
       PathAttributesLists to be ordered first in order of NextHop, as
       this makes the RIB-In's task much easier when a nexthop changes */
    bool operator< (const PathAttributeList<A> &them) const;

    /* operator== is a direct comparison of MD5 hashes */
    bool operator== (const PathAttributeList<A> &them) const;
protected:
private:
    void replace_attribute(PathAttribute *att);
    void assert_rehash() const;
    const PathAttribute* find_attribute_by_type(PathAttType type) const;

    NextHopAttribute<A> *	_nexthop_att;
    ASPathAttribute *		_aspath_att;
    OriginAttribute *		_origin_att;

    uint8_t			_hash[16];	// used for fast comparisons
};

#endif // __BGP_PATH_ATTRIBUTE_HH__
