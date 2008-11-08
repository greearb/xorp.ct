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

// $XORP: xorp/bgp/route_table_deletion.hh,v 1.24 2008/10/02 21:56:19 bms Exp $

#ifndef __BGP_ROUTE_TABLE_DELETION_HH__
#define __BGP_ROUTE_TABLE_DELETION_HH__

#include "route_table_base.hh"
#include "peer_handler.hh"
#include "bgp_trie.hh"
#include "crash_dump.hh"


class EventLoop;
/**
 * @short DeletionTable is a temporary BGPRouteTable used to delete
 * routes when a peer goes down
 * 
 * When a peer goes down, all the routes stored in a RibIn need to be
 * deleted.  However, this can take some time, so it cannot occur in
 * one atomic operation, so it must be done route-by-route as a
 * background task.  This is complicated by the fact that the peering
 * may come back up while this background deletion is occuring, and
 * new routes may appear.  To handle the background deletion while
 * keeping the RibIn simple, we simply create a new DeletionTable
 * route table, plumb it in directly after the RibIn, and pass the
 * RibIn's entire route trie to the DeletionTable.  RibIn can now
 * forget these routes ever existed, and DeletionTable can get on with
 * the background deletion task, unplumbing and deleting itself when
 * no routes remain.
 *
 * Care must be taken to ensure that the downstream routing tables see
 * consistent information.  For example, if there is a route for
 * subnet X in the DeletionTable that has not yet been deleted, and an
 * add_route for X comes downstream from rthe RibIn, then this would
 * need to be propagated downstream as a replace_route.
 *
 * Note that if a peering flaps multiple times, multiple
 * DeletionTables may be plumbed in, one after another, behind a
 * RibInTable.
 */
template<class A>
class DeletionTable : public BGPRouteTable<A>, CrashDumper  {
public:
    DeletionTable(string tablename,
		  Safi safi,
		  BgpTrie<A>* route_table,
		  const PeerHandler *peer,
		  uint32_t genid,
		  BGPRouteTable<A> *parent);
    ~DeletionTable();
    int add_route(InternalMessage<A> &rtmsg,
		  BGPRouteTable<A> *caller);
    int replace_route(InternalMessage<A> &old_rtmsg,
		      InternalMessage<A> &new_rtmsg,
		      BGPRouteTable<A> *caller);
    int delete_route(InternalMessage<A> &rtmsg,
		     BGPRouteTable<A> *caller);
    int route_dump(InternalMessage<A> &rtmsg,
		   BGPRouteTable<A> *caller,
		   const PeerHandler *dump_peer);
    int push(BGPRouteTable<A> *caller);
    const SubnetRoute<A> *lookup_route(const IPNet<A> &net,
				       uint32_t& genid,
				       FPAListRef& pa_list) const;
    void route_used(const SubnetRoute<A>* route, bool in_use);


    RouteTableType type() const { return DELETION_TABLE; }
    string str() const;

    /* mechanisms to implement flow control in the output plumbing */
    void output_state(bool /*busy*/, BGPRouteTable<A> */*next_table*/) {
	abort();
    }
    bool get_next_message(BGPRouteTable<A> */*next_table*/) {
	abort();
	return false;
    }

    void initiate_background_deletion();

    /**
     * @return the generation id.
     */
    uint32_t genid() const {return _genid;}

    string dump_state() const;

private:
    void unplumb_self();
    bool delete_next_chain();
    EventLoop& eventloop() const {
	return _peer->eventloop();
    }

    const PeerHandler *_peer;
    uint32_t _genid;
    BgpTrie<A>* _route_table;
    typename BgpTrie<A>::PathmapType::const_iterator _del_sweep;

    int _deleted, _chains;
    XorpTask _deletion_task;
};

#endif // __BGP_ROUTE_TABLE_DELETION_HH__
