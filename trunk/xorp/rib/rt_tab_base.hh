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

// $XORP: xorp/rib/rt_tab_base.hh,v 1.2 2003/03/10 23:20:56 hodson Exp $

#ifndef __RIB_RT_TAB_BASE_HH__
#define __RIB_RT_TAB_BASE_HH__

#include <map>
#include "libxorp/xorp.h"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv4.hh"
#include "route.hh"
#include "protocol.hh"
#include "libxorp/trie.hh"

#define IGP 1
#define EGP 2

#define ROUTE_USED 4
#define ROUTE_UNUSED 8

#define ORIGIN_TABLE 1
#define MERGED_TABLE 2
#define EXTINT_TABLE 4
#define REDIST_TABLE 8
#define EXPORT_TABLE 16
#define REGISTER_TABLE 32
#define MAX_TABLE_TYPE 32

/**
 * @short Stores a Route and bounds on the validity of the route.
 *
 * The RouteRange class is used to hold an annotated routing entry.
 * It is used when the @ref RegisterTable is registering interest in
 * routing information associated with a specific address.  It holds
 * an IP address, the route that would be used to route that address,
 * and the top and bottom addresses of the route range that includes
 * that address for which this route applies without being overlayed
 * by a more specific route.  For example:
 *
 * Suppose an @ref OriginTable holds the routes 1.0.0.0/16 and
 * 1.0.1.0/24.  The address we're interested in is 1.0.0.10.  Then if
 * we ask this OriginTable for the RouteRange for 1.0.0.10, we get:
 *
 *   address: 1.0.0.10
 *   route: the route for 1.0.0.0/16
 *   top: 1.0.0.255
 *   bottom: 1.0.0.0
 *
 * Ie, the route for 1.0.0.10 is 1.0.0.0/16, and this answer is also
 * valid for addresses in the range 1.0.0.0 to 1.0.0.255 inclusive.  
 */
template<class A>
class RouteRange {
public:
    RouteRange(const A& req_addr, const IPRouteEntry<A>* route,
	       const A& top, const A& bottom) :
	_req_addr(req_addr), _route(route), _top(top), _bottom(bottom) {}

    const A& top() const			{ return _top;		}
    const A& bottom() const			{ return _bottom;	}
    const IPRouteEntry<A>* route() const	{ return _route;	}
    const IPNet<A>& net() const			{ return _route->net();	}

    /**
     * merge this entry with rr:
     * replace route with the entry from rr if it is better, (XXX why ?)
     * shrink the intervals if the other one is smaller.
     */
    void merge(const RouteRange *his_rr)	{
	    const IPRouteEntry<A> *rrr = his_rr->route();

	    if (_route == NULL)
		_route = rrr;
	    else if (rrr != NULL) {
		int my_prefix = net().prefix_len();
		int his_prefix = his_rr->net().prefix_len();
		if (his_prefix > my_prefix) // his route beats mine
		    _route = rrr;
		else if (his_prefix == my_prefix) {
		    // routes are equivalent, compare distance
		    if (_route->admin_distance() >
			    rrr->admin_distance()) // his is better
			_route = rrr;
		    // note: if routes have same admin distance, mine wins.
		}
	    }

	    if (_top > his_rr->top())	// he wins, shrink _top
		_top = his_rr->top();
	    if (_bottom < his_rr->bottom()) // he wins, shrink _bottom
		_bottom = his_rr->bottom();
    }

    // returns the largest subnet contained in the range.
    IPNet<A> minimal_subnet() const {
	    for (size_t bits = 0; bits <= A::addr_bitlen(); bits++) {
		IPNet<A> net(_req_addr, bits);
		if (net.masked_addr() >= _bottom && net.top_addr() <= _top)
		    return net; // we got it.
	    }
	    // can't get here
	    abort();
    }

private:
    A _req_addr;
    const IPRouteEntry<A>* _route;
    A _top;
    A _bottom;
};

//A = Address Type, eg IPv4

/**
 * @short Base class for a routing table.
 *
 * This is the base class for a routing table.  A RIB consists of a
 * tree of RouteTables that take routes from routing protocols and
 * merge them together so that routes that emerge from the last table
 * are the best routes available, and have nexthop information that is
 * for an immediate neighbor of this router.  See the RIB design
 * document for an overview of how RouteTable are plumbed together to
 * form a RIB.
 *
 * All RouteTables that form the RIB are derived from this base class.
 *
 * RouteTables take routing changes in from one or more parents, and
 * pass on those changes to the _next_table if the change is for the
 * best of the alternative routes.
 *
 * RouteTables take route lookup requests from their _next_table, and
 * pass on those requests to their parents.  If more than one parent
 * has a response, only the best is returned as the answer.  
 */
template<class A>
class RouteTable {
public:
    RouteTable(const string& name) : _tablename(name), _next_table(0) {}
    virtual ~RouteTable() {}

    virtual int add_route(const IPRouteEntry<A>& route,
			  RouteTable *caller) = 0;
    virtual int delete_route(const IPRouteEntry<A> *,
			     RouteTable *caller) = 0;
    virtual const
    IPRouteEntry<A> *lookup_route(const IPNet<A>& net) const = 0;

    virtual const
    IPRouteEntry<A> *lookup_route(const A& addr) const = 0;

    virtual RouteRange<A> *lookup_route_range(const A& addr) const = 0;

    void set_next_table(RouteTable *next_table) { _next_table = next_table; }
    RouteTable *next_table() { return _next_table; }

    // parent is only supposed to be called on single-parent tables
    virtual RouteTable *parent() { abort(); }

    virtual int type() const = 0;
    const string& tablename() const { return _tablename; }
    virtual void replumb(RouteTable *old_parent, RouteTable *new_parent) = 0;
    virtual string str() const = 0;
    virtual void flush() {}

protected:
    string _tablename;
    RouteTable* _next_table;
};

#endif // __RIB_RT_TAB_BASE_HH__
