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

// $XORP: xorp/bgp/route_table_base.hh,v 1.5 2004/02/24 03:16:55 atanu Exp $

#ifndef __BGP_ROUTE_TABLE_BASE_HH__
#define __BGP_ROUTE_TABLE_BASE_HH__

#include <iostream>
#include <string>

#include "libxorp/ipv4net.hh"
#include "libxorp/ipv4.hh"
#include "internal_message.hh"
#include "dump_iterators.hh"

enum RouteTableType {
    RIB_IN_TABLE = 1,
    RIB_OUT_TABLE = 2,
    CACHE_TABLE = 3,
    DECISION_TABLE = 4,
    FANOUT_TABLE = 5,
    FILTER_TABLE = 6,
    DELETION_TABLE = 7,
    DUMP_TABLE = 8,
    NHLOOKUP_TABLE = 9,
    DEBUG_TABLE = 10
};

#define MAX_TABLE_TYPE 10

#define ADD_USED 1
#define ADD_UNUSED 2
#define ADD_FAILURE 3 
#define ADD_FILTERED 4 

template<class A>
class BGPRouteTable {
public:
    BGPRouteTable(string tablename, Safi safi);
    virtual ~BGPRouteTable();
    virtual int add_route(const InternalMessage<A> &rtmsg, 
			  BGPRouteTable<A> *caller) = 0;
    virtual int replace_route(const InternalMessage<A> &old_rtmsg, 
			      const InternalMessage<A> &new_rtmsg, 
			      BGPRouteTable<A> *caller) = 0;
    virtual int delete_route(const InternalMessage<A> &rtmsg, 
			     BGPRouteTable<A> *caller) = 0;
    virtual int route_dump(const InternalMessage<A> &rtmsg, 
			   BGPRouteTable<A> *caller,
			   const PeerHandler *dump_peer);
    virtual int push(BGPRouteTable<A> *caller) = 0;
    virtual const 
    SubnetRoute<A> *lookup_route(const IPNet<A> &net) const = 0;

    virtual void route_used(const SubnetRoute<A>* /*route*/, 
			    bool /*in_use*/){
	abort();
    }

    void set_next_table(BGPRouteTable<A>* table) {
	_next_table = table;
    }
    BGPRouteTable<A> *next_table() { return _next_table;}

    /* parent is only supposed to be called on single-parent tables*/
    virtual BGPRouteTable<A> *parent() { return _parent; }
    virtual void set_parent(BGPRouteTable<A> *parent) { _parent = parent; }

    virtual RouteTableType type() const = 0;
    const string& tablename() const {return _tablename;}
    virtual string str() const = 0;

    /* mechanisms to implement flow control in the output plumbing */
    virtual void output_state(bool, BGPRouteTable *) {abort();}
    virtual bool get_next_message(BGPRouteTable *) {abort(); return false; }

    virtual bool dump_next_route(DumpIterator<A>& dump_iter);
    /**
     * Notification that the status of this next hop has changed.
     *
     * @param bgp_nexthop The next hop that has changed.
     */
    virtual void igp_nexthop_changed(const A& bgp_nexthop);

    virtual void peering_went_down(const PeerHandler *peer, uint32_t genid,
				   BGPRouteTable<A> *caller);
    virtual void peering_down_complete(const PeerHandler *peer, uint32_t genid,
				       BGPRouteTable<A> *caller);
    virtual void peering_came_up(const PeerHandler *peer, uint32_t genid,
				 BGPRouteTable<A> *caller);

    Safi safi() const {return _safi; }
protected:
    BGPRouteTable<A> *_next_table, *_parent;
    string _tablename;
    const Safi _safi;
};

#endif // __BGP_ROUTE_TABLE_BASE_HH__
