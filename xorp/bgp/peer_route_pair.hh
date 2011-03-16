// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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

// $XORP: xorp/bgp/peer_route_pair.hh,v 1.21 2008/10/02 21:56:17 bms Exp $

#ifndef __BGP_PEER_ROUTE_PAIR_HH__
#define __BGP_PEER_ROUTE_PAIR_HH__



#include "libxorp/xlog.h"
#include "libxorp/timer.hh"

#include "libxorp/timeval.hh"
#include "libxorp/timer.hh"

template<class A>
class RouteQueueEntry;

template<class A>
class BGPRouteTable;

template<class A>
class PeerTableInfo {
public:
    PeerTableInfo(BGPRouteTable<A> *init_route_table, 
		  const PeerHandler *ph,
		  uint32_t genid)
    {
	_route_table = init_route_table;
	_peer_handler = ph;
	_genid = genid;
	_is_ready = true;
	_has_queued_data = false;
	_waiting_for_get = false;
	TimerList::system_gettimeofday(&_wakeup_sent);
    }
    PeerTableInfo(const PeerTableInfo& other) {
	_route_table = other._route_table;
	_peer_handler = other._peer_handler;
	_genid = other._genid;
	_is_ready = other._is_ready;
	_has_queued_data = other._has_queued_data;
	_peer_number = other._peer_number;
	if (_has_queued_data) {
	    _posn = other._posn;
	}
	_waiting_for_get = other._waiting_for_get;
	_wakeup_sent = other._wakeup_sent;
    }
    ~PeerTableInfo() {
	_wakeup_sent = TimeVal::ZERO();
    }

    BGPRouteTable<A> *route_table() const {
	return _route_table;
    }
    const PeerHandler* peer_handler() const {
	return _peer_handler;
    }

    void set_genid(uint32_t genid) {
	_genid = genid;
    }

    uint32_t genid() const {
	return _genid;
    }

    bool is_ready() const {return _is_ready;}
    void set_is_ready() {_is_ready = true;}
    void set_is_not_ready() {_is_ready = false;}

    bool has_queued_data() const {return _has_queued_data;}
    void set_has_queued_data(bool has_data) {_has_queued_data = has_data;}
    
    void set_queue_position(typename list<const RouteQueueEntry<A>*>::iterator posn) {
	_posn = posn;
    }
    typename list<const RouteQueueEntry<A>*>::iterator queue_position() const {
	return _posn;
    }

    // The following two functions are here to do a sanity check.  If
    // we sent a wakeup to a peer more than 20 minutes ago, and still
    // haven't received a get_next_message, then something has failed
    // badly, and we need to dump a core and diagnose what it is that
    // happened.
    void received_get() {
	if (_waiting_for_get)
	    _waiting_for_get = false;
    }
    void wakeup_sent() {
	TimeVal now;
	TimerList::system_gettimeofday(&now);
	if (_waiting_for_get) {
	    if ((now.sec() - _wakeup_sent.sec()) > 1200) {
		/* we sent a wakeup 20 minutes ago, and the peer still
		   hasn't requested the data - something must have
		   gone badly wrong */
		string s = "Peer seems to have permanently locked up\n";
		s += "Time now: " + now.str() + 
		    ", time then: " + _wakeup_sent.str() + "\n";
		XLOG_FATAL("%s", s.c_str());
	    }
	} else {
	    XLOG_ASSERT(_wakeup_sent != TimeVal::ZERO());
	    _wakeup_sent = now;
	    _waiting_for_get = true;
	}
    }
    
    void peer_reset() { 
	_waiting_for_get = false;
   }

private:
    BGPRouteTable<A> *_route_table; //the next table after
                                          //the fanout table

    const PeerHandler *_peer_handler;

    bool _has_queued_data; //there is data queued for this peer in the
                           //fanout table

    int _peer_number; //used to ensure consistency of ordering 

    uint32_t _genid;

    // this is set when downstream has requested a message be sent,
    // and we didn't have anything to send.  It indicates that a
    // message may be immediately sent downstream.
    bool _is_ready;

    typename list<const RouteQueueEntry<A>*>::iterator _posn; 
    /*the next item of data to send to this peer in the fanout table
      queue.  This only has meaning if _has_queued_data is true */

    bool _waiting_for_get;
    TimeVal _wakeup_sent;
};

#endif // __BGP_PEER_ROUTE_PAIR_HH__
