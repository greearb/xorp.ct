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

// $XORP: xorp/bgp/path_attribute_list.hh,v 1.24 2002/12/09 18:28:44 hodson Exp $

#ifndef __BGP_PATH_ATTRIBUTE_LIST_HH__
#define __BGP_PATH_ATTRIBUTE_LIST_HH__

#include "libxorp/xorp.h"
#include <list>
#include "path_attribute.hh"

template<class A>
class PathAttributeList
{
public:
    PathAttributeList();
    PathAttributeList(const NextHopAttribute<A> &nexthop,
			 const ASPathAttribute &aspath,
			 const OriginAttribute &origin);
    PathAttributeList(const PathAttributeList<A>& palist);
    ~PathAttributeList();
    void add_path_attribute(const PathAttribute &att);
    const A& nexthop() const { return _nexthop_att->nexthop(); }
    const AsPath& aspath() const { return _aspath_att->as_path(); }
    const uint8_t origin() const { return _origin_att->origintype(); }

    void rehash();
    const uint8_t* hash() const {
	assert_rehash();
	return _hash;
    }

    const list<const PathAttribute*>& att_list() const {
// 	printf("PathAttributeList:att_list(): size = %d\n", _att_list.size());
	return _att_list;
    }

    // complete() is true when all the mandatory attributes are present
    bool complete() const {
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
    void replace_attribute(const PathAttribute *att, PathAttType type);
    void assert_rehash() const;

    NextHopAttribute<A> *_nexthop_att;
    ASPathAttribute *_aspath_att;
    OriginAttribute *_origin_att;
    list <const PathAttribute*> _att_list;

    // hash is used for fast comparisons
    uint8_t _hash[16];
    // A _nexthop;
};

#endif // __BGP_PATH_ATTRIBUTE_LIST_HH__
