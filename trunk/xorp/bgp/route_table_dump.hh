// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the Xorp LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the Xorp LICENSE file; the license in that file is
// legally binding.

// $XORP: xorp/bgp/route_table_dump.hh,v 1.2 2002/12/17 22:06:05 mjh Exp $

#ifndef __BGP_ROUTE_TABLE_DUMP_HH__
#define __BGP_ROUTE_TABLE_DUMP_HH__

#include "route_table_base.hh"
#include "peer_handler.hh"
#include "bgp_trie.hh"

class EventLoop;

template<class A>
class DumpTable : public BGPRouteTable<A>  {
public:
    DumpTable(string tablename,
	      const PeerHandler *peer,
	      const list <const PeerHandler*>& peer_list,
	      BGPRouteTable<A> *parent);
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
    void route_used(const SubnetRoute<A>* route, bool in_use);


    RouteTableType type() const {return DUMP_TABLE;}
    string str() const;

    /* mechanisms to implement flow control in the output plumbing */
    void output_state(bool busy, BGPRouteTable<A> *next_table);
    bool get_next_message(BGPRouteTable<A> *next_table);

    void initiate_background_dump();
    void suspend_dump();
    void peering_went_down(const PeerHandler *peer, uint32_t genid,
			   BGPRouteTable<A> *caller);
    void peering_down_complete(const PeerHandler *peer, uint32_t genid,
			       BGPRouteTable<A> *caller);
private:
    void unplumb_self();
    void do_next_route_dump();
    EventLoop *get_eventloop() const {return _peer->get_eventloop();}

    const PeerHandler *_peer;
    DumpIterator<A> _dump_iter;
    bool _output_busy;

    int _dumped;
    bool _dump_active;
    XorpTimer _dump_timer;

    //if we get to the end of the route dump, and some peers that went
    //down during our dump have not yet finished their deletion
    //process, we need to delay unplumbing ourself until they finish
    //to avoid propagating unnecessary deletes downstream.
    bool _waiting_for_deletion_completion;
};

#endif // __BGP_ROUTE_TABLE_DUMP_HH__
