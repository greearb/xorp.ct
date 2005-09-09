// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

// $XORP: xorp/ospf/routing_table.hh,v 1.3 2005/08/16 21:52:45 atanu Exp $

#ifndef __OSPF_ROUTING_TABLE_HH__
#define __OSPF_ROUTING_TABLE_HH__

#include <libxorp/trie.hh>

template <typename A>
class RouteEntry {
 public:
    enum Path_type {
	intra_area,
	inter_area,
	type1,
	type2
    };
    
    OspfTypes::VertexType _destination_type;
    uint32_t _address;			// If dest type is Network
    OspfTypes:: RouterID _id;		// If dest type is Router

//     uint32_t _prefix_length;		// Not convinced this is required.
    OspfTypes::AreaID _area;		// Associated area.
    Path_type _path_type;
    uint32_t _cost;
    uint32_t _type_2_cost;

    A _nexthop;
    uint32_t	_advertising_router;
};

template <typename A>
class RoutingTable {
 public:
    RoutingTable(Ospf<A> &ospf)
	: _ospf(ospf), _current(0), _previous(0)
    {}

    void begin();

    bool add_entry(IPNet<A> net, RouteEntry<A>& rt);

    bool delete_entry(IPNet<A> net);

    void end();
    
 private:
    Ospf<A>& _ospf;			// Reference to the controlling class.

    Trie<A, RouteEntry<A> > *_current;
    Trie<A, RouteEntry<A> > *_previous;
};

#if	0
template <typename A>
class RouteEntry {
 public:
    list<A> _nexthops;		// Possible nexthops for this route

    bool _discard;		// True if this is a discard route.
    uint32_t _users;		// Only valid if this is a discard
				// route. The number of routes below
				// the discard route.
    
    OspfTypes::AreaID _area;	// Area the LSA came from.
    list<Lsa::LsaRef> lsars;	// LSAs the routes came from.
    bool _modified;		// True if this route has been
				// modified in the last sweep.
};

template <typename A>
class RoutingTable {
 public:
    RoutingTable(Ospf<A> &ospf)
	: _ospf(ospf)
    {}

    void begin();

    /**
     * Add route
     *
     * @param net network
     * @param nexthop
     * @param metric to network
     * @param equal true if this in another route to the same destination.
     */
    bool add_route(IPNet<A> net,
		   A nexthop,
		   uint32_t metric,
		   bool equal);

    /**
     * Replace route
     *
     * @param net network
     * @param nexthop
     * @param metric to network
     * @param equal true if this in another route to the same destination.
     */
    bool replace_route(IPNet<A> net,
		       A nexthop,
		       uint32_t metric,
		       bool equal);

    /**
     * Delete route
     */
    bool delete_route(IPNet<A> net);

    void end();
    
 private:
    Ospf<A>& _ospf;			// Reference to the controlling class.

    Trie<A, RouteEntry<A> > _trie;
};
#endif

#endif // __OSPF_ROUTING_TABLE_HH__
