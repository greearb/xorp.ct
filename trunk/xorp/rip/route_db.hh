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

// $XORP: xorp/rip/route_db.hh,v 1.2 2003/04/23 17:06:49 hodson Exp $

#ifndef __RIP_ROUTE_DB_HH__
#define __RIP_ROUTE_DB_HH__

#include "libxorp/ref_ptr.hh"
#include "libxorp/trie.hh"
#include "route_entry.hh"
#include "peer.hh"

class EventLoop;

template <typename A>
class UpdateQueue;

template <typename A>
class PacketRouteEntry;

template <typename A>
class RouteDB {
public:
    typedef A					Addr;
    typedef IPNet<A> 				Net;
    typedef RouteEntry<A>			Route;
    typedef RouteEntryOrigin<A>			RouteOrigin;
    typedef ref_ptr<RouteEntry<A> >		DBRouteEntry;
    typedef ref_ptr<const RouteEntry<A> >	ConstDBRouteEntry;
    typedef Trie<A, DBRouteEntry>		RouteTrie;
    typedef PacketRouteEntry<A>			PacketizedRoute;
    typedef Peer<A>				RipPeer;
public:
    RouteDB(EventLoop& e);
    ~RouteDB();

    /**
     * Update Route Entry in database for specified route.
     *
     * If the route does not exist or the values provided differ from the
     * existing route, then an update is placed in the update queue.
     *
     * @param net the network route being updated.
     * @param nexthop the corresponding nexthop address.
     * @param cost the corresponding metric value as received from the peer.
     * @param tag the corresponding route tag.
     * @param peer the peer proposing update.
     *
     * @return true if an update occurs, false otherwise.
     */
    bool update_route(const Net&   net,
		      const Addr&  nexthop,
		      uint32_t	   cost,
		      uint32_t	   tag,
		      RipPeer*	   peer);

    /**
     * Flatten routing table representation from Trie to Vector.
     *
     * @param routes vector where routes are to be appended.
     */
    void dump_routes(vector<ConstDBRouteEntry>& routes);

    /**
     * Resolve a route and take a reference to it.  While the reference
     * exists the route will not be deleted from memory, though may be
     * remove from table.
     *
     * @param net network to be resolved.
     * @param cdbe reference pointer to route entry.
     *
     * @return true if route resolves, false otherwise.
     */
    bool resolve_and_reference(const Net& net, ConstDBRouteEntry& cdbe);

    UpdateQueue<A>& update_queue();
    const UpdateQueue<A>& update_queue() const;

protected:
    RouteDB(const RouteDB&);			// not implemented
    RouteDB& operator=(const RouteDB&);		// not implemented

    void expire_route(Route* r);
    void set_expiry_timer(Route* r);

    void delete_route(Route* r);
    void set_deletion_timer(Route* r);

protected:
    EventLoop&		_eventloop;
    RouteTrie		_routes;
    UpdateQueue<A>*	_uq;
};

#endif // __RIP_ROUTE_DB_HH__
