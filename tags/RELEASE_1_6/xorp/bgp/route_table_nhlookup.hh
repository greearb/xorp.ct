// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/bgp/route_table_nhlookup.hh,v 1.20 2008/11/08 06:14:39 mjh Exp $

#ifndef __BGP_ROUTE_TABLE_NHLOOKUP_HH__
#define __BGP_ROUTE_TABLE_NHLOOKUP_HH__

#include "route_table_base.hh"
#include "libxorp/ref_trie.hh"
#include "next_hop_resolver.hh"

template<class A>
class MessageQueueEntry {
public:
    MessageQueueEntry(InternalMessage<A>* add_msg,
		      InternalMessage<A>* delete_msg);
    MessageQueueEntry(const MessageQueueEntry& original);
    ~MessageQueueEntry();
    typedef enum op {
	ADD = 1,
	REPLACE = 2
    } Op;
    Op type() const {
	if (_add_msg!=NULL && _delete_msg==NULL) return ADD;
	if (_add_msg!=NULL && _delete_msg!=NULL) return REPLACE;
	abort();
    }
    InternalMessage<A>* add_msg() const {return _add_msg;}
    const SubnetRoute<A>* added_route() const {return _add_msg->route();}
    FPAListRef& added_attributes() const {return _add_msg->attributes();}

    InternalMessage<A>* delete_msg() const {return _delete_msg;}
    const SubnetRoute<A>* deleted_route() const {return _delete_msg->route();}
    FPAListRef& deleted_attributes() const {return _delete_msg->attributes();}
    const IPNet<A>& net() const {return _add_msg->route()->net();}
    string str() const;
private:
    void copy_in(InternalMessage<A>* add_msg,
		 InternalMessage<A>* delete_msg);

    InternalMessage<A>* _add_msg;
    InternalMessage<A>* _delete_msg;

    //These references are to ensure that the SubnetRoutes from the
    //add and delete messages don't get deleted before we're done with
    //them.
    SubnetRouteConstRef<A> _added_route_ref;
    SubnetRouteConstRef<A> _deleted_route_ref;
};

template<class A>
class NhLookupTable : public BGPRouteTable<A>  {
public:
    NhLookupTable(string tablename,
		  Safi safi,
		  NextHopResolver<A> *nexthop_resolver,
		  BGPRouteTable<A> *parent);
    int add_route(InternalMessage<A> &rtmsg,
		  BGPRouteTable<A> *caller);
    int replace_route(InternalMessage<A> &old_rtmsg,
		      InternalMessage<A> &new_rtmsg,
		      BGPRouteTable<A> *caller);
    int delete_route(InternalMessage<A> &rtmsg, 
		     BGPRouteTable<A> *caller);
    int push(BGPRouteTable<A> *caller);
    const SubnetRoute<A> *lookup_route(const IPNet<A> &net,
				       uint32_t& genid,
				       FPAListRef& pa_list) const;
    void route_used(const SubnetRoute<A>* route, bool in_use);
    
    virtual void RIB_lookup_done(const A& nexthop, 
			 const set <IPNet<A> >& nets,
			 bool lookup_succeeded);

    RouteTableType type() const {return NHLOOKUP_TABLE;}
    string str() const;
private:
    //access the message queue by subnet or an address on the subnet
    RefTrie<A, MessageQueueEntry<A> > _queue_by_net;
    //access the message queue by nexthop
    multimap <A, MessageQueueEntry<A>*> _queue_by_nexthop;

    NextHopResolver<A>* _next_hop_resolver;

    /**
     * Add the message queue entry to both queues.
     */
    void add_to_queue(const A& nexthop, const IPNet<A>& net,
		      InternalMessage<A>* new_msg,
		      InternalMessage<A>* old_msg);

    /**
     * Lookup subnet in the _queue_by_net.
     *
     * @param nexthop if non zero compare against the net that is found.
     * @param net to lookup.
     * @return a message queue entry if found else zero.
     */
    MessageQueueEntry<A> *
    lookup_in_queue(const A& nexthop, const IPNet<A>& net) const;

    /**
     * Find the message queue entry and remove from both queues.
     */
    void remove_from_queue(const A& nexthop, const IPNet<A>& net);
};

#endif // __BGP_ROUTE_TABLE_NHLOOKUP_HH__
