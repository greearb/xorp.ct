// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/bgp/route_queue.hh,v 1.9 2003/03/10 23:20:03 hodson Exp $

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
    }

    //for push only
    RouteQueueEntry(RouteQueueOp op, const PeerHandler *origin_peer) :
	_route_ref(NULL)
    {
	assert(op == RTQUEUE_OP_PUSH);
	_op = op;
	_origin_peer = origin_peer; // NULL is valid.
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

    string str() const;
private:
    RouteQueueOp _op;

    SubnetRouteConstRef<A> _route_ref;
    const PeerHandler *_origin_peer;
    uint32_t _genid;
};

#endif // __BGP_ROUTE_QUEUE_HH__
