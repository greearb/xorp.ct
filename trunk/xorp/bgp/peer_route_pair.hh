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

// $XORP: xorp/bgp/peer_route_pair.hh,v 1.1.1.1 2002/12/11 23:55:49 hodson Exp $

#ifndef __BGP_PEER_ROUTE_PAIR_HH__
#define __BGP_PEER_ROUTE_PAIR_HH__

#include <list>

template<class A>
class RouteQueueEntry;

template<class A>
class PeerRoutePair {
public:
    PeerRoutePair(const BGPRouteTable<A> *init_route_table, 
		  const PeerHandler *ph)
    {
	_route_table = init_route_table;
	_peer_handler = ph;
	_busy = false;
	_has_queued_data = false;
    }
    const BGPRouteTable<A> *route_table() const {
	return _route_table;
    }
    const PeerHandler* peer_handler() const {
	return _peer_handler;
    }

    bool busy() const {return _busy;}
    void set_busy(bool busy) {_busy = busy;}
    
    bool has_queued_data() const {return _has_queued_data;}
    void set_has_queued_data(bool has_data) {_has_queued_data = has_data;}
    
    void set_queue_position(typename list<const RouteQueueEntry<A>*>::iterator posn) {
	_posn = posn;
    }
    typename list<const RouteQueueEntry<A>*>::iterator queue_position() const {
	return _posn;
    }
private:
    const BGPRouteTable<A> *_route_table; //the next table after
                                          //the fanout table

    const PeerHandler *_peer_handler;

    bool _busy; //the RibOut has determined it's queue is too large,
                //so no more updates should be sent

    bool _has_queued_data; //there is data queued for this peer in the
                           //fanout table

    typename list<const RouteQueueEntry<A>*>::iterator _posn; 
    /*the next item of data to send to this peer in the fanout table
      queue.  This only has meaning if _has_queued_data is true */
};

#endif // __BGP_PEER_ROUTE_PAIR_HH__
