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

// $XORP: xorp/bgp/internal_message.hh,v 1.5 2004/05/07 09:23:56 mjh Exp $

#ifndef __BGP_INTERNAL_MESSAGES_HH__
#define __BGP_INTERNAL_MESSAGES_HH__

#include "libxorp/xorp.h"
#include "subnet_route.hh"
class PeerHandler;

#define GENID_UNKNOWN 0

/**
 * @short InternalMessage is used to pass route changes between BGP
 * route table classes.
 *
 * XORP BGP is implemented as a pipelined series of route_tables,
 * starting with a RibInTable for each peering, converging on a
 * DecisionTable to decide which competing route is prefered, and then
 * fanning out again to a RibOutTable for each peer.  Routing changes
 * such as add_route, delete_route, and replace_route propagate
 * through this pipeline.  The "payload" of these changes is an
 * InternalMessage, which contains the route itself, the peering from
 * which this route originated, and the generation ID of the RibIn at
 * that peering.
 */
template<class A>
class InternalMessage
{
public:
    InternalMessage(const SubnetRoute<A> *route,
		    const PeerHandler *origin_peer,
		    uint32_t genid);
    ~InternalMessage();
    const IPNet<A>& net() const;
    const SubnetRoute<A> *route() const { return _subnet_route; }
    const PeerHandler* origin_peer() const { return _origin_peer; }
    const A& nexthop() const { return _subnet_route->nexthop(); }

    bool changed() const { return _changed; }
    void set_changed() { _changed = true; }
    void clear_changed() const { _changed = false; }

    bool push() const { return _push; }
    void set_push() { _push = true; }
    void clear_push() { _push = false; }

    bool from_previous_peering() const { return _from_previous_peering; }
    void set_from_previous_peering() { _from_previous_peering = true; }

    uint32_t genid() const { return _genid; }

    // This is a hack to override const in DecisionTable.
    void force_clear_push() const { _push = false; }

    void inactivate() const {
	_subnet_route->unref();
	_subnet_route = NULL;
    }
    string str() const;
protected:
private:
    /**
     * the actual route data.
     */
    mutable const SubnetRoute<A> *_subnet_route;

    /**
     * we need origin_peer to make sure we don't send a route back to
     * the peer it came from, or send an IBGP route to an IBGP peer.
     */
    const PeerHandler *_origin_peer;

    /**
     * changed indicates that the route data has been modified since
     * the route was last stored (and so needs storing by a
     * CacheTable).
     */
    mutable bool _changed;

    /**
     * genid is the generation ID from the RibIn, if known, or zero if
     * it's not known.
     */
    uint32_t _genid;

    /**
     * push indicates that this is the last route in a batch, so the
     * push to peers is implicit.
     */
    mutable bool _push;

    /**
     * from_previous_peering is set on messages where the deleted route
     * originates from a previous peering that has now gone down.
     */
    bool _from_previous_peering;
};

#endif // __BGP_INTERNAL_MESSAGES_HH__
