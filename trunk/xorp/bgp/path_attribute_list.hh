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

#ifndef __BGP_PATH_ATTRIBUTE_LIST_HH__
#define __BGP_PATH_ATTRIBUTE_LIST_HH__

#include "libxorp/xorp.h"
#include <list>
#include "path_attribute.hh"

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
class PathAttributeList
{
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

    const list<PathAttribute*>& att_list() const {
 	debug_msg("PathAttributeList:att_list(): size = %u\n",
		(uint32_t)_att_list.size());
	return _att_list;
    }

    // complete() is true when all the mandatory attributes are present
    bool complete() const			{
	return ((_nexthop_att != NULL) &&
		(_aspath_att != NULL) && (_origin_att != NULL));
    }

    void replace_nexthop(const A& nexthop);
    void replace_AS_path(const AsPath& as_path);
    void remove_attribute_by_type(PathAttType type);

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

    list <PathAttribute*>	_att_list;

    uint8_t			_hash[16];	// used for fast comparisons
};

#endif // __BGP_PATH_ATTRIBUTE_LIST_HH__
