// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/bgp/route_table_fanout.hh,v 1.25 2008/07/23 05:09:36 pavlin Exp $

#ifndef __BGP_ROUTE_TABLE_FANOUT_HH__
#define __BGP_ROUTE_TABLE_FANOUT_HH__

#include <map>
#include "route_table_base.hh"
#include "peer_route_pair.hh"
#include "route_queue.hh"
#include "crash_dump.hh"


template<class A> class DumpTable;

template<class A>
class NextTableMapIterator {
public:
    NextTableMapIterator() {};
    NextTableMapIterator(const typename multimap<uint32_t, PeerTableInfo<A>*>::iterator& iter) {
	_iter = iter;
    }
    BGPRouteTable<A>* first() { return _iter->second->route_table(); }
    PeerTableInfo<A>& second() { return *(_iter->second); }
    void operator ++(int) { _iter++; }
    bool operator==(const NextTableMapIterator& them) const {
	return _iter == them._iter;
    }
private:
    typename multimap <uint32_t, PeerTableInfo<A>*>::iterator _iter;
};

/**
 * NextTableMap behaves more or less like a map between a
   BGPRouteTable<A>* and a const PeerHandler*, but it gives
   deterministic ordering semantics so our test suites aren't
   dependent on the memory allocator.  This class basically hides the
   complexity of maintaining two parallel data structures from the
   main FanoutTable code */

template<class A> 
class NextTableMap  {
public:
    typedef NextTableMapIterator<A> iterator;
    NextTableMap() {};
    ~NextTableMap();
    void insert(BGPRouteTable<A> *next_table,
		const PeerHandler *ph, uint32_t genid);
    void erase(iterator& iter);
    iterator find(BGPRouteTable<A> *next_table);
    iterator begin();
    iterator end();
private:
    map<BGPRouteTable<A> *, PeerTableInfo<A>* > _next_tables;
    multimap<uint32_t, PeerTableInfo<A>* > _next_table_order;
};

template<class A>
class FanoutTable : public BGPRouteTable<A>, CrashDumper  {
public:
    FanoutTable(string tablename,
		Safi safi,
		BGPRouteTable<A> *parent,
		PeerHandler *aggr_handler,
		BGPRouteTable<A> *aggr_table);
    ~FanoutTable();
    int add_next_table(BGPRouteTable<A> *next_table,
		       const PeerHandler *ph, uint32_t genid);
    int remove_next_table(BGPRouteTable<A> *next_table);
    int replace_next_table(BGPRouteTable<A> *old_next_table,
			   BGPRouteTable<A> *new_next_table);
    int add_route(const InternalMessage<A> &rtmsg,
		  BGPRouteTable<A> *caller);
    int replace_route(const InternalMessage<A> &old_rtmsg,
		      const InternalMessage<A> &new_rtmsg,
		      BGPRouteTable<A> *caller);
    int delete_route(const InternalMessage<A> &rtmsg, 
		     BGPRouteTable<A> *caller);
    int route_dump(const InternalMessage<A> &rtmsg,
		   BGPRouteTable<A> *caller,
		   const PeerHandler *dump_peer);
    int push(BGPRouteTable<A> *caller);
    const SubnetRoute<A> *lookup_route(const IPNet<A> &net,
				       uint32_t& genid) const;

    RouteTableType type() const {return FANOUT_TABLE;}
    string str() const;

    void peer_table_info(list<const PeerTableInfo<A>*>& peer_list);

    int dump_entire_table(BGPRouteTable<A> *child_to_dump_to, Safi safi,
			  string ribname);
    /* mechanisms to implement flow control in the output plumbing */
    bool get_next_message(BGPRouteTable<A> *next_table);

    void peering_went_down(const PeerHandler *peer, uint32_t genid,
			   BGPRouteTable<A> *caller);
    void peering_down_complete(const PeerHandler *peer, uint32_t genid,
			       BGPRouteTable<A> *caller);
    void peering_came_up(const PeerHandler *peer, uint32_t genid,
			 BGPRouteTable<A> *caller);

    void crash_dump() const {
	CrashDumper::crash_dump();
    }
    string dump_state() const;

    void print_queue();
private:
    void add_to_queue(RouteQueueOp operation,
		      const InternalMessage<A> &rtmsg,
		      const list<PeerTableInfo<A>*>& queued_peers);
    void add_replace_to_queue(const InternalMessage<A> &old_rtmsg,
			      const InternalMessage<A> &new_rtmsg,
			      const list<PeerTableInfo<A>*>& queued_peers);
    void add_push_to_queue(const list<PeerTableInfo<A>*>& queued_peers,
			   const PeerHandler *origin_peer);
    void set_queue_positions(const list<PeerTableInfo<A>*>& queued_peers);
    void skip_entire_queue(BGPRouteTable<A> *next_table);
    void wakeup_downstream(list <PeerTableInfo<A>*>& queued_peers);

    void add_dump_table(DumpTable<A> *dump_table); 
    void remove_dump_table(DumpTable<A> *dump_table);
    NextTableMap<A> _next_tables;

    list <const RouteQueueEntry<A>*> _output_queue;
    set <DumpTable<A>*> _dump_tables;

    PeerTableInfo<A> *_aggr_peerinfo;
};

#endif // __BGP_ROUTE_TABLE_FANOUT_HH__
