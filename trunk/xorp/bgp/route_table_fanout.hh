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

// $XORP: xorp/bgp/route_table_fanout.hh,v 1.8 2004/02/24 03:16:56 atanu Exp $

#ifndef __BGP_ROUTE_TABLE_FANOUT_HH__
#define __BGP_ROUTE_TABLE_FANOUT_HH__

#include <map>
#include "route_table_base.hh"
#include "peer_route_pair.hh"
#include "route_queue.hh"

#define NEWMAP

template<class A> class DumpTable;

template<class A>
class NextTableMapIterator {
public:
    NextTableMapIterator() {};
#ifdef NEWMAP
    NextTableMapIterator(const typename multimap<uint32_t, PeerRoutePair<A>*>::iterator& iter) {
	_iter = iter;
    }
    BGPRouteTable<A>* first() {
	return _iter->second->route_table();
    }
    PeerRoutePair<A>& second() {
	return *(_iter->second);
    }
#else
    NextTableMapIterator(const typename map <BGPRouteTable<A>*, PeerRoutePair<A>*>::iterator& iter) {
	_iter = iter;
    }
    BGPRouteTable<A>* first() {
	return _iter->first;
    }
    PeerRoutePair<A>& second() {
	return *(_iter->second);
    }
#endif
    inline void operator ++(int) {
	_iter++;
    }
    inline bool operator==(const NextTableMapIterator& them) const {
	return _iter == them._iter;
    }
private:
#ifdef NEWMAP
    typename multimap <uint32_t, PeerRoutePair<A>*>::iterator _iter;
#else
    typename map <BGPRouteTable<A>*, PeerRoutePair<A>*>::iterator _iter;
#endif
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
		const PeerHandler *ph);
    void erase(iterator& iter);
    iterator find(BGPRouteTable<A> *next_table);
    iterator begin();
    iterator end();
private:
    map<BGPRouteTable<A> *, PeerRoutePair<A>* > _next_tables;
    multimap<uint32_t, PeerRoutePair<A>* > _next_table_order;
};

template<class A>
class FanoutTable : public BGPRouteTable<A>  {
public:
    FanoutTable(string tablename, Safi safi, BGPRouteTable<A> *parent);
    int add_next_table(BGPRouteTable<A> *next_table,
		       const PeerHandler *ph);
    int remove_next_table(BGPRouteTable<A> *next_table);
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
    const SubnetRoute<A> *lookup_route(const IPNet<A> &net) const;

    RouteTableType type() const {return FANOUT_TABLE;}
    string str() const;

    int dump_entire_table(BGPRouteTable<A> *child_to_dump_to, Safi safi,
			  string ribname);
#ifdef NOTDEF
    void peering_went_down(const PeerHandler* peer_handler);
#endif

    /* mechanisms to implement flow control in the output plumbing */
    void output_state(bool busy, BGPRouteTable<A> *next_table);
    bool get_next_message(BGPRouteTable<A> *next_table);

    void peering_went_down(const PeerHandler *peer, uint32_t genid,
			   BGPRouteTable<A> *caller);
    void peering_down_complete(const PeerHandler *peer, uint32_t genid,
			       BGPRouteTable<A> *caller);

    void print_queue();
private:
    void add_to_queue(RouteQueueOp operation,
		      const InternalMessage<A> &rtmsg,
		      const list<PeerRoutePair<A>*>& queued_peers);
    void add_replace_to_queue(const InternalMessage<A> &old_rtmsg,
			      const InternalMessage<A> &new_rtmsg,
			      const list<PeerRoutePair<A>*>& queued_peers);
    void add_push_to_queue(const list<PeerRoutePair<A>*>& queued_peers,
			   const PeerHandler *origin_peer);
    void set_queue_positions(const list<PeerRoutePair<A>*>& queued_peers);
    void skip_entire_queue(BGPRouteTable<A> *next_table);

    void add_dump_table(DumpTable<A> *dump_table); 
    void remove_dump_table(DumpTable<A> *dump_table);
    //    map<BGPRouteTable<A> *, PeerRoutePair<A> > _next_tables;
    NextTableMap<A> _next_tables;

    list <const RouteQueueEntry<A>*> _output_queue;
    set <DumpTable<A>*> _dump_tables;
};

#endif // __BGP_ROUTE_TABLE_FANOUT_HH__
