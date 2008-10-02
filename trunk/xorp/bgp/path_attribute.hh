// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/bgp/path_attribute.hh,v 1.51 2008/07/23 05:09:34 pavlin Exp $

#ifndef __BGP_PATH_ATTRIBUTE_HH__
#define __BGP_PATH_ATTRIBUTE_HH__

#include "libxorp/xorp.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"

#include <list>
#include <string>
#include <set>

#include <openssl/md5.h>

#include "exceptions.hh"	// for CorruptMessage exception
#include "aspath.hh"
#include "parameter.hh"
class BGPPeerData;



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
    ORIGINATOR_ID = 9,
    CLUSTER_LIST = 10,
    MP_REACH_NLRI = 14,
    MP_UNREACH_NLRI = 15,
    AS4_PATH = 17,
    AS4_AGGREGATOR = 18,
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
				 size_t& actual_length, 
				 const BGPPeerData* peerdata) 
	throw(CorruptMessage);

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
    virtual ~PathAttribute()			{ }

    virtual bool encode(uint8_t* buf, size_t &length, const BGPPeerData* peerdata) const = 0;


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
    void set_partial() { _flags |= Partial; }

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
	size_t length = 4096;
	uint8_t buf[4096];
	// XXX should probably do something more efficient here
	if (!encode(buf, length, NULL)) {
	    XLOG_WARNING("Insufficient space to encode PA list for MD5 hash\n");
	}
	MD5_Update(context, buf, length);
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
    int sorttype() const			{
	return type() == NEXT_HOP ? -1 : type();
    }

    /**
     * helper constructor used when creating an object from a derived class.
     */
    PathAttribute(Flags f, PathAttType t)
	    : _flags(f & ValidFlags), _type(t)	{}

    /**
     * basic constructor from data, assumes that the block has at least the
     * required size.
     */
    PathAttribute(const uint8_t *d)
	    : _flags(d[0] & ValidFlags), _type(d[1])	{}

    /**
     * helper function to fill the header. Needs _flags and _type
     * properly initialized. Writes into data buffer, and returns
     * pointer to first byte of buffer after the header.
     */
    uint8_t *set_header(uint8_t *data, size_t payload_size, size_t &wire_size) 
	const;

    /**
     * fetch the length from the header. Assume the header is there.
     */
    static size_t length(const uint8_t* d)	{
	return (d[0] & Extended) ?
		( (d[2]<<8) + d[3] ) : d[2] ;
    }

    /**
     * Total length including the header. Used to send the whole TLV
     * back when an error has been detected.
     */
    static size_t total_tlv_length(const uint8_t* d) {
	return length(d) + ((d[0] & Extended) ? 4 : 3);
    }

    // helper function returning a pointer to the payload
    const uint8_t *payload(const uint8_t *d)	{
	return d + ((d[0] & Extended) ? 4 : 3);
    }

#if 0
    // storage for information in the attribute.

    size_t	_size;	// this is only the size of the payload.
    uint8_t *	_data;	// wire representation
#endif
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

    bool encode(uint8_t* buf, size_t &wire_size, const BGPPeerData* peerdata) const;


protected:
private:
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

    ASPathAttribute(const ASPath& p);
    ASPathAttribute(const uint8_t* d, bool use_4byte_asnums) 
	throw(CorruptMessage);
    PathAttribute *clone() const;

    string str() const				{
	return "AS Path Attribute " + as_path().str();
    }

    ASPath &as_path() const		{ return (ASPath &)*_as_path; }
    AS4Path &as4_path() const		{ return (AS4Path &)*_as_path;}

    bool encode(uint8_t* buf, size_t &wire_size, const BGPPeerData* peerdata) const;


protected:
private:
    ASPath *_as_path;
};

/**
 * AS4PathAttribute contain an AS4Path, whose structure is documented
 * in aspath.hh.  See comment there for usage.
 */
class AS4PathAttribute : public PathAttribute
{
public:
    ~AS4PathAttribute()				{ delete _as_path; }

    AS4PathAttribute(const AS4Path& p);
    AS4PathAttribute(const uint8_t* d) throw(CorruptMessage);
    PathAttribute *clone() const;

    string str() const				{
	return "AS4 Path Attribute " + as_path().str();
    }

    ASPath &as_path() const		{ return (ASPath &)*_as_path; }
    AS4Path &as4_path() const		{ return (AS4Path &)*_as_path;}

    bool encode(uint8_t* buf, size_t &wire_size, const BGPPeerData* peerdata) const;

protected:
private:
    AS4Path *_as_path;
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

    bool encode(uint8_t* buf, size_t &wire_size, const BGPPeerData* peerdata) const;

protected:
private:
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

    bool encode(uint8_t* buf, size_t &wire_size, const BGPPeerData* peerdata) const;

protected:
private:
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

    bool encode(uint8_t* buf, size_t &wire_size, const BGPPeerData* peerdata) const;

protected:
private:
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

    bool encode(uint8_t* buf, size_t &wire_size, const BGPPeerData* peerdata) const;

protected:
private:
};

class AggregatorAttribute : public PathAttribute
{
public:
    AggregatorAttribute(const IPv4& speaker, const AsNum& as);
    AggregatorAttribute(const uint8_t* d, bool use_4byte_asnums) throw(CorruptMessage);
    PathAttribute *clone() const;

    string str() const;

    const IPv4& route_aggregator() const	{ return _speaker; }
    const AsNum& aggregator_as() const		{ return _as; }

    bool encode(uint8_t* buf, size_t &wire_size, const BGPPeerData* peerdata) const;

protected:
private:
    IPv4 _speaker;
    AsNum _as;
};

class AS4AggregatorAttribute : public PathAttribute
{
public:
    AS4AggregatorAttribute(const IPv4& speaker, const AsNum& as);
    AS4AggregatorAttribute(const uint8_t* d) throw(CorruptMessage);
    PathAttribute *clone() const;

    string str() const;

    const IPv4& route_aggregator() const	{ return _speaker; }
    const AsNum& aggregator_as() const		{ return _as; }

    bool encode(uint8_t* buf, size_t &wire_size, const BGPPeerData* peerdata) const;

protected:
private:
    IPv4 _speaker;
    AsNum _as;
};

class CommunityAttribute : public PathAttribute
{
public:
    static const uint32_t NO_EXPORT = 0xFFFFFF01;  // RFC 1997
    static const uint32_t NO_ADVERTISE = 0xFFFFFF02;  // RFC 1997
    static const uint32_t NO_EXPORT_SUBCONFED = 0xFFFFFF03;  // RFC 1997

    typedef set <uint32_t>::const_iterator const_iterator;
    CommunityAttribute();
    CommunityAttribute(const uint8_t* d) throw(CorruptMessage);
    PathAttribute *clone() const;

    string str() const;

    const set <uint32_t>& community_set() const { return _communities; }
    void add_community(uint32_t community);
    bool contains(uint32_t community) const;

    bool encode(uint8_t* buf, size_t &wire_size, const BGPPeerData* peerdata) const;

protected:
private:
    set <uint32_t> _communities;
};

/**
 * OriginatorIDAttribute is an optional non-transitive uint32
 */
class OriginatorIDAttribute : public PathAttribute
{
public:
    OriginatorIDAttribute(const IPv4 originator_id);
    OriginatorIDAttribute(const uint8_t* d) throw(CorruptMessage);
    PathAttribute *clone() const;

    string str() const;

    IPv4 originator_id() const     { return _originator_id; }

    bool encode(uint8_t* buf, size_t &wire_size, const BGPPeerData* peerdata) const;

protected:
private:
    IPv4 _originator_id;	
};

class ClusterListAttribute : public PathAttribute
{
public:
    typedef list <IPv4>::const_iterator const_iterator;
    ClusterListAttribute();
    ClusterListAttribute(const uint8_t* d) throw(CorruptMessage);
    PathAttribute *clone() const;

    string str() const;

    const list <IPv4>& cluster_list() const { return _cluster_list; }
    void prepend_cluster_id(IPv4 cluster_id);
    bool contains(IPv4 cluster_id) const;

    bool encode(uint8_t* buf, size_t &wire_size, const BGPPeerData* peerdata) const;

protected:
private:
    list <IPv4> _cluster_list;
};


template <class A>
class MPReachNLRIAttribute : public PathAttribute
{
public:
    typedef typename list<IPNet<A> >::const_iterator const_iterator;

    /**
     * Specialise these constructors for each AFI.
     */
    MPReachNLRIAttribute(Safi safi);
    MPReachNLRIAttribute(const uint8_t* d) throw(CorruptMessage);
    PathAttribute *clone() const;

    string str() const;

    const A& nexthop() const		{ return _nexthop; }
    void set_nexthop(const A& nexthop)	{ _nexthop = nexthop; }

    void add_nlri(const IPNet<A>& nlri) {_nlri.push_back(nlri);}
    const list<IPNet<A> >& nlri_list() const { return _nlri;}

    // IPv6 specific
    const A& link_local_nexthop() const	{ return _link_local_next_hop; }
    void set_link_local_nexthop(const A& n) { _link_local_next_hop = n;}

    // SNPA - Don't deal. (ATM, FRAME RELAY, SMDS)

    Safi safi()				{ return _safi; }

    bool encode(uint8_t* buf, size_t &wire_size, 
		const BGPPeerData* peerdata) const;

protected:
private:

    Afi _afi;			// Address Family Identifier.
    Safi _safi;			// Subsequent Address Family Identifier.
    
    A _nexthop;			// Next Hop.
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
     * Specialise these constructors for each AFI.
     */
    MPUNReachNLRIAttribute(Safi safi);
    MPUNReachNLRIAttribute(const uint8_t* d) throw(CorruptMessage);
    PathAttribute *clone() const;

    string str() const;

    void add_withdrawn(const IPNet<A>& nlri) {_withdrawn.push_back(nlri);}
    const list<IPNet<A> >& wr_list() const { return _withdrawn;}

    Safi safi()				{ return _safi; }

    bool encode(uint8_t* buf, size_t &wire_size, 
		const BGPPeerData* peerdata) const;

protected:
private:

    Afi _afi;			// Address Family Identifier.
    Safi _safi;			// Subsequent Address Family Identifier.
    
    list<IPNet<A> > _withdrawn;	// Withdrawn routes.
};

class UnknownAttribute : public PathAttribute
{
public:
    UnknownAttribute(const uint8_t* d) throw(CorruptMessage);
    UnknownAttribute(uint8_t *data, size_t size, uint8_t flags);
    PathAttribute *clone() const;

    string str() const;

    bool encode(uint8_t* buf, size_t &wire_size, 
		const BGPPeerData* peerdata) const;

protected:
private:
    // storage for information in the attribute.

    size_t	_size;	// this is only the size of the payload.
    uint8_t *	_data;	// wire representation
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
    /**
     * Add this path attribute to the list after making a local copy.
     */
    void add_path_attribute(const PathAttribute &att);
    /**
     * Add this path attribute to the list don't make a local copy.
     */
    void add_path_attribute(PathAttribute *att);
    const A& nexthop() const		{ return _nexthop_att->nexthop(); }
    const ASPath& aspath() const	{ return _aspath_att->as_path(); }
    uint8_t origin() const		{ return _origin_att->origin(); }

    const MEDAttribute* med_att() const;
    const LocalPrefAttribute* local_pref_att() const;
    const AtomicAggAttribute* atomic_aggregate_att() const;
    const AggregatorAttribute* aggregator_att() const;
    const CommunityAttribute* community_att() const;
    const OriginatorIDAttribute* originator_id() const;
    const ClusterListAttribute* cluster_list() const;

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
    void replace_AS_path(const ASPath& as_path);
    void replace_origin(const OriginType& origin);
    void remove_attribute_by_type(PathAttType type);
    void remove_attribute_by_pointer(PathAttribute*);

    /**
     * For unknown attributes:
     *	1) If transitive set the partial bit.
     *  2) If not transitive remove.
     */
    void process_unknown_attributes();

    /**
     * Encode the PA List for transmission to the specified peer.
     * Note that as Some peers speak 4-byte AS numbers and some don't,
     * the encoding is peer-specific.
     *
     * @return true if the data was successfully encoded; false if
     * there wasn't enough space in the buffer for the data.
     *
     * @param buf is the buffer to encode into.
     *
     * @param wire_size is given the size of the buffer to encode
     * into, and returns the amount of data placed in the buffer.
     *
     * @param peer is the peer to encode this for.  Some peers want
     * 4-byte AS numbers and some don't.
     *
     */
    bool encode(uint8_t* buf, size_t &wire_size, 
		const BGPPeerData* peerdata) const;

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
