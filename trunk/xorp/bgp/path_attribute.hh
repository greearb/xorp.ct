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

// $XORP$

#ifndef __BGP_PATH_ATTRIBUTE_HH__
#define __BGP_PATH_ATTRIBUTE_HH__

#include <unistd.h>
#include <string>
#include <set>
#include "libxorp/debug.h"

#include "config.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "exceptions.hh"	// for CorruptMessage exception

#include "aspath.hh"
#include "md5.h"

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
    UNKNOWN = 255
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
    size_t size() const				{
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
	MD5Update(context, data(), size());
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

    string str() const				{
	return "Next Hop Attribute " + _next_hop.str();
    }

    const A& nexthop() const			{ return _next_hop; }
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
    string str() const;

    const set <uint32_t>& community_set() const { return _communities; }
    void add_community(uint32_t community);
protected:
private:
    void encode();
    set <uint32_t> _communities;
};

class UnknownAttribute : public PathAttribute
{
public:
    UnknownAttribute(const uint8_t* d) throw(CorruptMessage);
    string str() const;
protected:
private:
};

#endif // __BGP_PATH_ATTRIBUTE_HH__
