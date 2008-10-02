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

// $XORP: xorp/bgp/route_queue.hh,v 1.16 2008/07/23 05:09:35 pavlin Exp $

#ifndef __BGP_ROUTE_QUEUE_HH__
#define __BGP_ROUTE_QUEUE_HH__

#include "subnet_route.hh"

class PeerHandler;

typedef enum ribout_queue_op {
    RTQUEUE_OP_ADD = 1,
    RTQUEUE_OP_DELETE = 2,
    RTQUEUE_OP_REPLACE_OLD = 3,
    RTQUEUE_OP_REPLACE_NEW = 4,
    RTQUEUE_OP_PUSH = 5
} RouteQueueOp;

template<class A>
class RouteQueueEntry {
public:
    RouteQueueEntry(const SubnetRoute<A>* rt, RouteQueueOp op) :
	_route_ref(rt)
    {
	_op = op;
	_origin_peer = 0;
	_push = false;
    }

    //for push only
    RouteQueueEntry(RouteQueueOp op, const PeerHandler *origin_peer) :
	_route_ref(NULL)
    {
	assert(op == RTQUEUE_OP_PUSH);
	_op = op;
	_origin_peer = origin_peer; // NULL is valid.
	_push = false;
    }

    ~RouteQueueEntry() {
    }

    const SubnetRoute<A>* route() const		
    { 
	return _route_ref.route();	
    }

    const IPNet<A>& net() const			
    { 
	return _route_ref.route()->net();	
    }

    const PathAttributeList<A> *attributes() const 
    {
	return _route_ref.route()->attributes();
    }
    RouteQueueOp op() const			{ return _op;		}

    void set_origin_peer(const PeerHandler *peer) {_origin_peer = peer;	}
    const PeerHandler *origin_peer() const	{ return _origin_peer;	}
    void set_genid(uint32_t genid)		{ _genid = genid; 	}
    uint32_t genid() const			{ return _genid;	}
    void set_push(bool push)		{ _push = push; }
    bool push() const		{ return _push;}

    string str() const;
private:
    RouteQueueOp _op;

    SubnetRouteConstRef<A> _route_ref;
    const PeerHandler *_origin_peer;
    uint32_t _genid;
    bool _push;
};

#endif // __BGP_ROUTE_QUEUE_HH__
