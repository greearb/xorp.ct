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

// $XORP: xorp/bgp/dump_iterators.hh,v 1.9 2002/12/09 18:28:42 hodson Exp $

#ifndef __BGP_DUMP_ITERATORS_HH__
#define __BGP_DUMP_ITERATORS_HH__


#include <map>
#include "path_attribute_list.hh"
#include "bgp_trie.hh"
#include "route_queue.hh"

class BGPPlumbing;
class PeerHandler;
template <class A> class InternalMessage;

template <class A>
class DownedPeer {
public:
    DownedPeer(const PeerHandler* peer,
	       bool routes_dumped,
	       const IPNet<A>& last_net,
	       uint32_t genid);
    ~DownedPeer();
    const PeerHandler* peer_handler() const { return _peer; }
    bool routes_dumped() const { return _routes_dumped; }
    const IPNet<A>& last_net() const { return _last_net_before_down; }
    uint32_t genid() const { return _genid; }
    void set_genid(uint32_t genid) { _genid = genid; }
    void set_delete_complete() { _delete_complete = true; }
    bool delete_complete() const { return _delete_complete; }
private:
    const PeerHandler* _peer;
    bool _routes_dumped;
    IPNet<A> _last_net_before_down;
    uint32_t _genid;
    bool _delete_complete;
};

template <class A>
class DumpIterator {
public:
    DumpIterator(const PeerHandler* peer,
		 const list <const PeerHandler*>& peers_to_dump);
    void route_dump(const InternalMessage<A> &rtmsg);
    const PeerHandler* current_peer() const { return *_current_peer; }
    const PeerHandler* peer_to_dump_to() const { return _peer; }
    bool is_valid() const;
    bool route_iterator_is_valid() const { return _route_iterator_is_valid; }
    bool next_peer();
    const BgpTrie<A>::iterator& route_iterator() const {
	return _route_iterator;
    }
    const IPNet<A>& net() const { return _route_iterator_net; }
    void set_route_iterator(BgpTrie<A>::iterator& new_iter) {
	_route_iterator = new_iter;
	_route_iterator_is_valid = true;
    }
    void set_route_iterator_net(const IPNet<A>& net) {
    	_route_iterator_net = net;
    }

    uint32_t rib_version() const { return _rib_version; }
    void set_rib_version(uint32_t version) { _rib_version = version; }
    bool route_change_is_valid(const PeerHandler* origin_peer,
			       const IPNet<A>& net,
			       uint32_t genid, RouteQueueOp op);

    void peering_went_down(const PeerHandler *peer, uint32_t genid);
    void peering_down_complete(const PeerHandler *peer, uint32_t genid);
    bool waiting_for_deletion_completion() const;
private:
    BGPPlumbing *_plumbing;
    const PeerHandler *_peer;
    list <const PeerHandler*> _peers_to_dump;
    list <const PeerHandler*>::iterator _current_peer;
    bool _route_iterator_is_valid;
    BgpTrie<A>::iterator _route_iterator;
    IPNet<A> _route_iterator_net;
    uint32_t _rib_version;

    bool _routes_dumped_on_current_peer;
    IPNet<A> _last_dumped_net;
    list <DownedPeer<A> > _downed_peers;
};

#endif // __BGP_DUMP_ITERATORS_HH__
