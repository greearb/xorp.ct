// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/bgp/path_attribute.hh,v 1.54 2008/12/11 21:05:59 mjh Exp $

#ifndef __BGP_PATH_ATTRIBUTE_HH__
#define __BGP_PATH_ATTRIBUTE_HH__

#include "libxorp/xorp.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"
#include "libxorp/ref_ptr.hh"

#include <list>
#include <string>
#include <set>
#include <vector>

#include <boost/noncopyable.hpp>

#include <openssl/md5.h>

#include "exceptions.hh"	// for CorruptMessage exception
#include "aspath.hh"
#include "parameter.hh"
class BGPPeerData;
class BGPPeer;
class BGPMain;

template <class A>
class AttributeManager;



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
    UNKNOWN_TESTING1 = 19, // reserved for testing only - should be one
                           // more than the highest known attribute.
    UNKNOWN_TESTING2 = 20  // reserved for testing only - should be two
                           // more than the highest known attribute.
};

// MAX_ATTRIBUTE must contain the largest attribute number from the enum above
#define MAX_ATTRIBUTE 20

class PathAttribute :
    public boost::noncopyable
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

    /**
     * main routine to create a PathAttribute from incoming data.
     * Takes a chunk of memory of size l, returns an object of the
     * appropriate type and actual_length is the number of bytes used
     * from the packet.
     * Throws an exception on error.
     */
    static PathAttribute *create(const uint8_t* d, uint16_t max_len,
				 size_t& actual_length, 
				 const BGPPeerData* peerdata,
				 uint32_t ip_version) 
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
    PathAttribute();	// Not directly constructible.
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

    A& nexthop() 			{ return _next_hop; }
    // This method is for use in MPReachNLRIAttribute only.
    void set_nexthop(const A& n) 		{ _next_hop = n; }

    bool encode(uint8_t* buf, size_t &wire_size, const BGPPeerData* peerdata) const;

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

    Safi safi() const			{ return _safi; }

    bool encode(uint8_t* buf, size_t &wire_size, 
		const BGPPeerData* peerdata) const;

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

    Safi safi()	const			{ return _safi; }

    bool encode(uint8_t* buf, size_t &wire_size, 
		const BGPPeerData* peerdata) const;

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

private:
    // storage for information in the attribute.

    size_t	_size;	// this is only the size of the payload.
    uint8_t *	_data;	// wire representation
};

/* it ought to be possible to typedef this, but I don't know how */
#define FPAListRef ref_ptr<FastPathAttributeList<A> >
#define FPAList4Ref ref_ptr<FastPathAttributeList<IPv4> >
#define FPAList6Ref ref_ptr<FastPathAttributeList<IPv6> >

template<class A>
class FastPathAttributeList;

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
class PathAttributeList {
public:
    typedef list<PathAttribute*>::const_iterator const_iterator;
    typedef list<PathAttribute*>::iterator iterator;

    PathAttributeList();
    PathAttributeList(const PathAttributeList<A>& palist);
    PathAttributeList(FPAListRef& fpa_list);
    virtual ~PathAttributeList();


    // complete() is true when all the mandatory attributes are present
    // normally call this on a FastPathAttributeList
    //virtual bool complete() const;



    string str() const;

    /* operator< is used to store and search for PathAttributeLists in
       STL containers.  In principle, it doesn't matter what the order
       is, so long as there is a strict monotonicity to the ordering */
    /* In practice, the ordering is important - we want
       PathAttributesLists to be ordered first in order of NextHop, as
       this makes the RIB-In's task much easier when a nexthop changes */
    bool operator< (const PathAttributeList<A> &them) const;

    bool operator== (const PathAttributeList<A> &them) const;

    const uint8_t* canonical_data() const {return _canonical_data;}
    size_t canonical_length() const {return _canonical_length;}

    void incr_refcount(uint32_t change) const {
	XLOG_ASSERT(0xffffffff - change > _refcount);
	_refcount += change;
	//	printf("incr_refcount for %p: now %u\n", this, _refcount);
    }

    void decr_refcount(uint32_t change) const {
	XLOG_ASSERT(_refcount >= change);
	_refcount -= change;
	//	printf("decr_refcount for %p: now %u\n", this, _refcount);
	if (_refcount == 0 && _managed_refcount == 0) {
	    delete this;
	}
    }
    
    uint32_t references() const { return _refcount; }


    void incr_managed_refcount(uint32_t change) const {
	XLOG_ASSERT(0xffffffff - change > _managed_refcount);
	_managed_refcount += change;
	//	printf("incr_managed_refcount for %p: now %u\n", this, _managed_refcount);
    }

    void decr_managed_refcount(uint32_t change) const {
	XLOG_ASSERT(_refcount >= change);
	_managed_refcount -= change;
	//	printf("decr_managed_refcount for %p: now %u\n", this, _managed_refcount);
	if (_refcount == 0 && _managed_refcount == 0) {
	    delete this;
	}
    }
    
    uint32_t managed_references() const { return _managed_refcount; }

protected:
    // Canonical data is the path attribute list stored in BGP wire
    // format in the most canonical form we know.  For example, AS
    // numbers are always 4-byte.  Note that as we speak IPv6, there
    // should be no IPv6 NLRI path attributes in here.  They're in the
    // prefix instead.  When we send to a particular peer, we will
    // typically re-encode a path attribute for that peer, so this
    // should not be sent directly - it's only for internal storage.
    uint8_t* _canonical_data;
    uint16_t _canonical_length;

private:
    //    void assert_rehash() const;
    //    const PathAttribute* find_attribute_by_type(PathAttType type) const;

    // ref count is the number of FastPathAttributeList objects
    // currently referencing this PathAttributeList.  It's used to
    // avoid premature deletion, and to keep track if we've asked for
    // this to be deleted.
    mutable uint32_t _refcount;

    // managed refcount is the number of routes referencing this PA
    // list when this PA list is stored in the attribute manager.
    mutable uint32_t _managed_refcount;

    //    uint8_t			_hash[16];	// used for fast comparisons
};

template<class A>
class PAListRef {
public:
    PAListRef(const PathAttributeList<A>* _palist);
    PAListRef(const PAListRef& palistref);
    PAListRef() : _palist(0) {}
    ~PAListRef();
    PAListRef& operator=(const PAListRef& palistref);
    PAListRef& operator=(FPAListRef& fpalistref);
    bool operator==(const PAListRef& palistref) const;
    bool operator<(const PAListRef& palistref) const;
    const PathAttributeList<A>* operator->() const {
	return _palist;
    }
    const PathAttributeList<A>* operator*() const {
	return _palist;
    }

    void register_with_attmgr();
    void deregister_with_attmgr();

    inline bool is_empty() const {return _palist == 0;}
    void release();
    const PathAttributeList<A>* attributes() const {return _palist;}
    void create_attribute_manager() {
	_att_mgr = new AttributeManager<A>();
    };

   /**
     * DEBUGGING ONLY
     */
    int number_of_managed_atts() const {
	return _att_mgr->number_of_managed_atts();
    }

private:
    // this ought not be a pointer, but the the two classes would
    // mutually self-reference, and you can't do that.
    static AttributeManager<A> *_att_mgr;

    const PathAttributeList<A>* _palist;
};


/* FastPathAttributeList is an subclass of a PathAttributeList that is
   used to quickly access the path attributes.  It takes a slave
   PathAttributeList if it is constructed from one, rather than using
   its own storage, because it is intended to be transient and the
   slave is intended to be more permanent, so we typically want the
   slave to persist when this is deleted */

template<class A>
class FastPathAttributeList /*: public PathAttributeList<A>*/ {
public:
    FastPathAttributeList(PAListRef<A>& palist);
    FastPathAttributeList(FastPathAttributeList<A>& fpalist);
    FastPathAttributeList(const NextHopAttribute<A> &nexthop,
			  const ASPathAttribute &aspath,
			  const OriginAttribute &origin);
    FastPathAttributeList();
    virtual ~FastPathAttributeList();

    /**
     *  Load the raw path attribute data from an update message.  This
     * data will not yet be in canonical form.  Call canonicalize() to
     * put the data in canonical form.
     */
    void load_raw_data(const uint8_t *data, size_t size, 
		       const BGPPeerData* peer, bool have_nlri,
		       BGPMain *mainprocess,
		       bool do_checks);


    /* see commemt on _locked variable */
    void lock() const { 
	XLOG_ASSERT(_locked == false);
	_locked = true;
    }
    void unlock() const { 
	XLOG_ASSERT(_locked == true);
	_locked = false;
    }

    bool is_locked() const {return _locked;}

    // All known attributes need accessor methods here
    NextHopAttribute<A>* nexthop_att();
    ASPathAttribute* aspath_att();
    AS4PathAttribute* as4path_att();
    OriginAttribute* origin_att();
    MEDAttribute* med_att();
    LocalPrefAttribute* local_pref_att();
    AtomicAggAttribute* atomic_aggregate_att();
    AggregatorAttribute* aggregator_att();
    CommunityAttribute* community_att();
    OriginatorIDAttribute* originator_id();
    ClusterListAttribute* cluster_list();
    template <typename A2> MPReachNLRIAttribute<A2> *mpreach(Safi) ;
    template <typename A2> MPUNReachNLRIAttribute<A2> *mpunreach(Safi);



    // short cuts
    A& nexthop();
    ASPath& aspath();
    OriginType origin();

    // complete() is true when all the mandatory attributes are present
    virtual bool complete() const			{
	return ((_att_bytes[NEXT_HOP] || _att[NEXT_HOP]) &&
		(_att_bytes[AS_PATH] || _att[AS_PATH]) && 
		(_att_bytes[ORIGIN] || _att[ORIGIN]));
    }

    /**
     * Add this path attribute to the list after making a local copy.
     */
    void add_path_attribute(const PathAttribute &att);

    /**
     * Add this path attribute to the list don't make a local copy.
     */
    void add_path_attribute(PathAttribute *att);

    /**
     * return the relevant path attribute, given the PA type.
     */
    PathAttribute* find_attribute_by_type(PathAttType type);

    /**
     * return the highest attribute type.
     */
    int max_att() const {return _att.size();}

    /**
     * For unknown attributes:
     *	1) If transitive set the partial bit.
     *  2) If not transitive remove.
     */
    void process_unknown_attributes();

    void replace_nexthop(const A& nexthop);
    void replace_AS_path(const ASPath& as_path);
    void replace_origin(const OriginType& origin);
    void remove_attribute_by_type(PathAttType type);
    void remove_attribute_by_pointer(PathAttribute*);
  
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

    void canonicalize() const;
    const uint8_t* canonical_data() const {return _canonical_data;}
    size_t canonical_length() const {return _canonical_length;}
    bool canonicalized() const {return _canonicalized;}

    bool operator==(const FastPathAttributeList<A>& him) const;

    bool is_empty() const {
	return _attribute_count == 0;
    }

    int attribute_count() const {return _attribute_count;}

private:
    void quick_decode(const uint8_t *canonical_data, uint16_t canonical_length);
    void replace_attribute(PathAttribute *att);

    uint32_t att_order(uint32_t index) const {
	switch(index) {
	case 1:
	    return (uint32_t)NEXT_HOP;
	case 2:
	    return (uint32_t)ORIGIN;
	case 3:
	    return (uint32_t)AS_PATH;
	default:
	    return index;
	}
    }

    void count_attributes() {
	_attribute_count = 0;
	for (uint32_t i = 0; i < _att.size(); i++) {
	    if (_att[i]) {
		_attribute_count++;
		continue;
	    }
	    if (i <= MAX_ATTRIBUTE && _att_bytes[i])
		_attribute_count++;
	}
    }


    /**
     * We need to encode an attribute to send to a peer.  However we only
     * have the canonically encoded byte stream data for it.  Sometimes
     * that is fine, and we should just send that; sometimes we need to
     * decode and re-encode for this specific peer.
     */
    bool encode_and_decode_attribute(const uint8_t* att_data,
				     const size_t& att_len,
				     uint8_t *buf,
				     size_t& wire_size ,
				     const BGPPeerData* peerdata) const;

    const PAListRef<A> _slave_pa_list;

    // Break out of the path attribute list by type for quick access.
    // bytes and lengths include the header so we preserve the flags.
    // _att is only filled out on demand.  These can be arrays because
    // we don't fill them out unless they're attributes we know about,
    // so the maximum size is bounded.
    const uint8_t* _att_bytes[MAX_ATTRIBUTE+1];
    size_t _att_lengths[MAX_ATTRIBUTE+1];

    // This is mutable because the nominal content of the class
    // doesn't change, but we cache the result we calculate in const
    // methods.  
    // We use a vector here because we want to be able to
    // resize it if we see an unknown path attribute that is greater
    // than MAX_ATTRIBUTE.
    mutable vector<PathAttribute*> _att;

    /**
     * A count of the number of attributes in this attribute list.
     */
    int _attribute_count;

    /**
     * We pass around ref_ptrs to FastPathAttributeLists for
     * efficiency reasons, and this greatly simplifies memory
     * management.  However, it makes it easy to store one temporarily
     * in one place and the while it's stored, modify it via a
     * reference somewhere else.  This would be a bug, but it's hard to
     * catch.  We add the ability to lock the contents to detect such
     * a condition and aid debugging. 
     */
    mutable bool _locked;

    // Canonical data is the path attribute list stored in BGP wire
    // format in the most canonical form we know.  For example, AS
    // numbers are always 4-byte.  Note that as we speak IPv6, there
    // should be no IPv6 NLRI path attributes in here.  They're in the
    // prefix instead.  When we send to a particular peer, we will
    // typically re-encode a path attribute for that peer, so this
    // should not be sent directly - it's only for internal storage.
    mutable uint8_t* _canonical_data;
    mutable uint16_t _canonical_length;
    mutable bool _canonicalized; // is the canonical data up-to-date?
};

template<class A>
template<class A2>
MPReachNLRIAttribute<A2>*
FastPathAttributeList<A>::mpreach(Safi safi)
{
    debug_msg("%p\n", this);
    PathAttribute* att = find_attribute_by_type(MP_REACH_NLRI);
    MPReachNLRIAttribute<A2>* mp_att 
	= dynamic_cast<MPReachNLRIAttribute<A2>*>(att);
    if (mp_att && safi == mp_att->safi())
	return mp_att;
    return 0;
}

template<class A>
template<class A2>
MPUNReachNLRIAttribute<A2>*
FastPathAttributeList<A>::mpunreach(Safi safi)
{
    debug_msg("%p\n", this);
    PathAttribute* att = find_attribute_by_type(MP_UNREACH_NLRI);
    MPUNReachNLRIAttribute<A2>* mp_att 
	= dynamic_cast<MPUNReachNLRIAttribute<A2>*>(att);
    if (mp_att && safi == mp_att->safi())
	return mp_att;
    return 0;
}



#endif // __BGP_PATH_ATTRIBUTE_HH__
