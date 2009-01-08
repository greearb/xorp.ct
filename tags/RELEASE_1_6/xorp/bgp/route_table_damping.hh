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

// $XORP: xorp/bgp/route_table_damping.hh,v 1.8 2008/11/08 06:14:38 mjh Exp $

#ifndef __BGP_ROUTE_TABLE_DAMPING_HH__
#define __BGP_ROUTE_TABLE_DAMPING_HH__

#include "libxorp/trie.hh"
#include "route_table_base.hh"
#include "damping.hh"
#include "peer_handler.hh"

/**
 * BGP Route Flap Damping RFC 2439
 */

/**
 * An entry of this form exists for all networks.
 */
class Damp {
 public:
    Damp(uint32_t time, uint32_t merit) : _time(time),
					  _merit(merit),
					  _damped(false)
    {}

    uint32_t _time;	// Last time the route was announced or withdrawn.
    uint32_t _merit;	// Figure of merit.
    bool _damped;	// True if this route is currently damped.
};

/**
 * An entry of this form exists only if the route has been damped.
 */
template<class A>
class DampRoute {
public:
    DampRoute(const SubnetRoute<A>* route, uint32_t genid) 
	: _routeref(route), _genid(genid) {}
    const SubnetRoute<A>* route() const { return _routeref.route(); }
    uint32_t genid() const { return _genid; }
    XorpTimer& timer() { return _timer; }
private:
    SubnetRouteConstRef<A> _routeref;
    uint32_t _genid;
    XorpTimer _timer;   // If this route is damped time when it should
			// be released. 
};

/**
 * Manage the damping of routes.
 *
 * NOTE: If damping was enabled and is then disabled it is possible
 * that some routes may be damped. While damped routes exist the
 * damping code is entered, no more routes are damped but routes are
 * only released as the timers fire.
 */
template<class A>
class DampingTable : public BGPRouteTable<A>  {
public:
    DampingTable(string tablename, Safi safi, BGPRouteTable<A>* parent,
		 const PeerHandler *peer,
		 Damping& damping);
    ~DampingTable();
    int add_route(InternalMessage<A> &rtmsg,
		  BGPRouteTable<A> *caller);
    int replace_route(InternalMessage<A> &old_rtmsg,
		      InternalMessage<A> &new_rtmsg,
		      BGPRouteTable<A> *caller);
    int delete_route(InternalMessage<A> &rtmsg, 
		     BGPRouteTable<A> *caller);
    int push(BGPRouteTable<A> *caller);
    int route_dump(InternalMessage<A> &rtmsg,
		   BGPRouteTable<A> *caller,
		   const PeerHandler *dump_peer);

    const SubnetRoute<A> *lookup_route(const IPNet<A> &net,
				       uint32_t& genid,
				       FPAListRef& pa_list) const;
    void route_used(const SubnetRoute<A>* route, bool in_use);

    RouteTableType type() const {return DAMPING_TABLE;}


    string str() const;

 private:

    bool damping() const {
	if (_peer->ibgp())
	    return false;

	if (0 != _damp_count)
	    return true;

	return _damping.get_damping();
    }

    bool damping_global() const { return _damping.get_damping(); }

    /**
     * Update the figure of merit for this route.
     *
     * @return true if the route should be damped.
     */
    bool update_figure_of_merit(Damp&damp, const InternalMessage<A> &rtmsg);

    /**
     * Is this route being damped.
     */
    bool is_this_route_damped(const IPNet<A> &net) const;

    /**
     * Callback method called to release damped route.
     */
    void undamp(IPNet<A> net);

    EventLoop& eventloop() const;

 private:
    const PeerHandler *_peer;
    Damping& _damping;
    
    Trie<A, Damp> _damp;
    RefTrie<A, DampRoute<A> > _damped;
    uint32_t _damp_count;	// Number of damped routes.
};

#endif // __BGP_ROUTE_TABLE_DAMPING_HH__
