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

// $XORP: xorp/bgp/dump_iterators.hh,v 1.21 2008/10/02 21:56:15 bms Exp $

#ifndef __BGP_DUMP_ITERATORS_HH__
#define __BGP_DUMP_ITERATORS_HH__



#include "path_attribute.hh"
#include "bgp_trie.hh"
#include "route_queue.hh"
#include "peer_route_pair.hh"
#include "route_table_aggregation.hh"

class BGPPlumbing;
class PeerHandler;
template <class A> class InternalMessage;

typedef enum {
    STILL_TO_DUMP,
    CURRENTLY_DUMPING,
    DOWN_DURING_DUMP,
    DOWN_BEFORE_DUMP,
    COMPLETELY_DUMPED,
    NEW_PEER,
    FIRST_SEEN_DURING_DUMP
} PeerDumpStatus;
    
template <class A>
class PeerDumpState {
public:
    PeerDumpState(const PeerHandler* peer,
		  PeerDumpStatus status,
		  uint32_t genid);
    ~PeerDumpState();
    string str() const;
    const PeerHandler* peer_handler() const { return _peer; }

    const IPNet<A>& last_net() const { return _last_net_before_down; }
    
    void set_down_during_dump(IPNet<A>& last_net, uint32_t genid) {
	XLOG_ASSERT(genid == _genid);
	_status = DOWN_DURING_DUMP;
	_last_net_before_down = last_net;
	set_delete_occurring(genid);
    }

    void set_down(uint32_t genid) {
	XLOG_ASSERT(_status == STILL_TO_DUMP 
		    || _status == CURRENTLY_DUMPING);
	_status = DOWN_BEFORE_DUMP;
	set_delete_occurring(genid);
    }

    void start_dump() {
	XLOG_ASSERT(_status == STILL_TO_DUMP);
	_status = CURRENTLY_DUMPING;
    }

    void set_dump_complete() {
	XLOG_ASSERT(_status == CURRENTLY_DUMPING);
	_status = COMPLETELY_DUMPED;
    }

    uint32_t genid() const { return _genid; }
    void set_genid(uint32_t genid) { _genid = genid; }
    
    PeerDumpStatus status() const { return _status; }

    void set_delete_complete(uint32_t genid);
    void set_delete_occurring(uint32_t genid) { 
	_deleting_genids.insert(genid); 
    }
    bool delete_complete() const { return _deleting_genids.empty(); }
private:
    const PeerHandler* _peer;
    bool _routes_dumped;
    IPNet<A> _last_net_before_down;
    uint32_t _genid;
    set <uint32_t> _deleting_genids;
    PeerDumpStatus _status;
};

template <class A>
class DumpIterator {
public:
    DumpIterator(const PeerHandler* peer,
		 const list <const PeerTableInfo<A>*>& peers_to_dump);
    ~DumpIterator();
    string str() const;
    void route_dump(const InternalMessage<A> &rtmsg);
    const PeerHandler* current_peer() const { 
	return _current_peer->peer_handler(); 
    }
    const PeerHandler* peer_to_dump_to() const { return _peer; }
    bool is_valid() const;
    bool route_iterator_is_valid() const { return _route_iterator_is_valid; }
    bool next_peer();
    const typename BgpTrie<A>::iterator& route_iterator() const {
	return _route_iterator;
    }
    const typename RefTrie<A, const AggregateRoute<A> >::iterator& aggr_iterator() const {
	return _aggr_iterator;
    }
    //    const IPNet<A>& net() const { return _route_iterator_net; }
    void set_route_iterator(typename BgpTrie<A>::iterator& new_iter) {
	_route_iterator = new_iter;
	_route_iterator_is_valid = true;
    }
    void set_aggr_iterator(typename RefTrie<A, const AggregateRoute<A> >::iterator& new_iter) {
	_aggr_iterator = new_iter;
	_route_iterator_is_valid = true;
    }
    //void set_route_iterator_net(const IPNet<A>& net) {
    //    	_route_iterator_net = net;
    //    }

    //    uint32_t rib_version() const { return _rib_version; }
    //    void set_rib_version(uint32_t version) { _rib_version = version; }
    bool route_change_is_valid(const PeerHandler* origin_peer,
			       const IPNet<A>& net,
			       uint32_t genid, RouteQueueOp op);

    /**
     * A peer which is down but still deleting routes when this peer
     * is brought up.
     */
    void peering_is_down(const PeerHandler *peer, uint32_t genid);

    /**
     * A peer which does down while dumping is taking place.
     */
    void peering_went_down(const PeerHandler *peer, uint32_t genid);

    /**
     * A peer that is down and has now completed deleting all its routes.
     */
    void peering_down_complete(const PeerHandler *peer, uint32_t genid);

    /**
     * A peer that was down came up.
     */
    void peering_came_up(const PeerHandler *peer, uint32_t genid);

    /**
     * @return true while peers we deleting routes.
     */
    bool waiting_for_deletion_completion() const;

    /**
     * @return true if the iterator got moved since the last delete
     */
    bool iterator_got_moved(IPNet<A> new_net) const;
private:
    const PeerHandler *_peer;

    /**
     * The list of peers that remain to be dumped
     */
    list <PeerTableInfo<A> > _peers_to_dump;
    typename list <PeerTableInfo<A> >::iterator _current_peer;
    PeerTableInfo<A>* _current_peer_debug; //XXX just to aid debugging in gdb

    bool _route_iterator_is_valid;
    typename BgpTrie<A>::iterator _route_iterator;
    typename RefTrie<A, const AggregateRoute<A> >::iterator _aggr_iterator;

    bool _routes_dumped_on_current_peer;
    IPNet<A> _last_dumped_net;

    map <const PeerHandler*, PeerDumpState<A>* > _peers;

};

#endif // __BGP_DUMP_ITERATORS_HH__
