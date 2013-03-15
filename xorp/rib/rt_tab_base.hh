// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/rib/rt_tab_base.hh,v 1.25 2008/10/02 21:58:12 bms Exp $

#ifndef __RIB_RT_TAB_BASE_HH__
#define __RIB_RT_TAB_BASE_HH__

#include "libxorp/xorp.h"

#include "route.hh"
#include "protocol.hh"
#include "libxorp/trie.hh"

enum TableType {
    ORIGIN_TABLE		= 1 << 0,
    MERGED_TABLE		= 1 << 1,
    EXTINT_TABLE		= 1 << 2,
    REDIST_TABLE		= 1 << 3,
    REGISTER_TABLE		= 1 << 4,
    DELETION_TABLE		= 1 << 5,
    EXPECT_TABLE		= 1 << 6,
    LOG_TABLE			= 1 << 7,
    POLICY_REDIST_TABLE		= 1 << 8,
    POLICY_CONNECTED_TABLE	= 1 << 9,
    MAX_TABLE_TYPE		= 1 << 9
};

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
 * I.e., the route for 1.0.0.10 is 1.0.0.0/16, and this answer is also
 * valid for addresses in the range 1.0.0.0 to 1.0.0.255 inclusive.
 */
template<class A>
class RouteRange {
public:
    RouteRange(const A& req_addr, const IPRouteEntry<A>* route,
	       const A& top, const A& bottom)
	: _req_addr(req_addr), _route(route), _top(top), _bottom(bottom) {}

    const A& top() const			{ return _top;		}
    const A& bottom() const			{ return _bottom;	}
    const IPRouteEntry<A>* route() const	{ return _route;	}
    const IPNet<A>& net() const			{ return _route->net();	}

    string str() const	{
	ostringstream oss;
	oss << "RouteRange: " << endl;
	oss << "Top - " << _top.str() << endl;
	oss << "Bottom - " << _bottom.str() << endl;
	return oss.str();
    }

    /**
     * Merge this entry with another entry.
     *
     * Replace route with the entry from rr if it is better, (XXX why ?)
     * shrink the intervals if the other one is smaller.
     *
     * @param his_rr the entry to merge with.
     */
    void merge(const RouteRange* his_rr)	{
	    const IPRouteEntry<A>* rrr = his_rr->route();

	    if (_route == NULL)
		_route = rrr;
	    else if (rrr != NULL) {
		int my_prefix_len = net().prefix_len();
		int his_prefix_len = his_rr->net().prefix_len();
		if (his_prefix_len > my_prefix_len) // his route beats mine
		    _route = rrr;
		else if (his_prefix_len == my_prefix_len) {
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

    // Return the largest subnet contained in the range.
    IPNet<A> minimal_subnet() const {
	    for (size_t bits = 0; bits <= A::addr_bitlen(); bits++) {
		IPNet<A> net(_req_addr, bits);
		if (net.masked_addr() >= _bottom && net.top_addr() <= _top)
		    return net; // we got it.
	    }
	    XLOG_UNREACHABLE();
    }

private:
    A	_req_addr;
    const IPRouteEntry<A>* _route;
    A	_top;
    A	_bottom;
};

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
 *
 * Note: A = Address Type, e.g., IPv4 or IPv6.
 */
template<class A>
class RouteTable {
public:
    RouteTable(const string& name) : _tablename(name), _next_table(NULL) {}
    virtual ~RouteTable();

    virtual int add_igp_route(const IPRouteEntry<A>& route) = 0;
    virtual int add_egp_route(const IPRouteEntry<A>& route) = 0;

    virtual int delete_igp_route(const IPRouteEntry<A>* route, bool b = false) = 0;
    virtual int delete_egp_route(const IPRouteEntry<A>* route, bool b = false) = 0;

    virtual void set_next_table(RouteTable* next_table);

    virtual TableType type() const = 0;
    virtual string str() const = 0;
    virtual void flush() {}

    const string& tablename() const		{ return _tablename; }
    RouteTable* next_table()			{ return _next_table; }
    const RouteTable* next_table() const	{ return _next_table; }

    // this call should be received and dealt with by the PolicyRedistTable.
    virtual void replace_policytags(const IPRouteEntry<A>& route,
				    const PolicyTags& prevtags);

protected:
    void set_tablename(const string& s) { _tablename = s; }

private:
    string	_tablename;
    RouteTable*	_next_table;
};

#endif // __RIB_RT_TAB_BASE_HH__
