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

// $XORP: xorp/bgp/route_table_deletion.hh,v 1.2 2002/12/14 05:31:55 mjh Exp $

#ifndef __BGP_ROUTE_TABLE_DELETION_HH__
#define __BGP_ROUTE_TABLE_DELETION_HH__

#include "route_table_base.hh"
#include "peer_handler.hh"
#include "bgp_trie.hh"

class EventLoop;

template<class A>
class DeletionTable : public BGPRouteTable<A>  {
public:
    DeletionTable(string tablename,
		  BgpTrie<A>* route_table,
		  const PeerHandler *peer,
		  uint32_t genid,
		  BGPRouteTable<A> *parent);
    ~DeletionTable();
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


    RouteTableType type() const { return DELETION_TABLE; }
    string str() const;

    /* mechanisms to implement flow control in the output plumbing */
    void output_state(bool /*busy*/, BGPRouteTable<A> */*next_table*/) {
	abort();
    }
    bool get_next_message(BGPRouteTable<A> */*next_table*/) {
	abort();
    }

    void initiate_background_deletion();
private:
    void unplumb_self();
    void delete_next_chain();
    EventLoop *get_eventloop() const {
	return _peer->get_eventloop();
    }

    const PeerHandler *_peer;
    uint32_t _genid;
    BgpTrie<A>* _route_table;
    map<const PathAttributeList<A> *,
	const ChainedSubnetRoute<A>*>::const_iterator _del_sweep;
    int _deleted, _chains;
    XorpTimer _deletion_timer;

};

#endif // __BGP_ROUTE_TABLE_DELETION_HH__
