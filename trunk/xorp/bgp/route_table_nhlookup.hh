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

// $XORP: xorp/bgp/route_table_nhlookup.hh,v 1.3 2002/12/17 22:06:06 mjh Exp $

#ifndef __BGP_ROUTE_TABLE_NHLOOKUP_HH__
#define __BGP_ROUTE_TABLE_NHLOOKUP_HH__

#include "route_table_base.hh"
#include "libxorp/trie.hh"
#include "next_hop_resolver.hh"

template<class A>
class MessageQueueEntry {
public:
    MessageQueueEntry(const InternalMessage<A>* add_msg,
		      const InternalMessage<A>* delete_msg);
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
    const InternalMessage<A>* add_msg() const {return _add_msg;}
    const SubnetRoute<A>* added_route() const {return _cloned_added_route;}

    const InternalMessage<A>* delete_msg() const {return _delete_msg;}
    const SubnetRoute<A>* deleted_route() const {return _cloned_deleted_route;}
    const IPNet<A>& net() const {return _add_msg->net();}
    string str() const;
private:
    void copy_in(const InternalMessage<A>* add_msg,
		 const InternalMessage<A>* delete_msg);
    InternalMessage<A>* _add_msg;
    InternalMessage<A>* _delete_msg;
    const SubnetRoute<A>* _cloned_added_route;
    const SubnetRoute<A>* _cloned_deleted_route;
};

template<class A>
class NhLookupTable : public BGPRouteTable<A>  {
public:
    NhLookupTable(string tablename, 
		  NextHopResolver<A> *nexthop_resolver,
		  BGPRouteTable<A> *parent);
    int add_route(const InternalMessage<A> &rtmsg,
		  BGPRouteTable<A> *caller);
    int replace_route(const InternalMessage<A> &old_rtmsg,
		      const InternalMessage<A> &new_rtmsg,
		      BGPRouteTable<A> *caller);
    int delete_route(const InternalMessage<A> &rtmsg, 
		     BGPRouteTable<A> *caller);
    int push(BGPRouteTable<A> *caller);
    const SubnetRoute<A> *lookup_route(const IPNet<A> &net) const;
    void route_used(const SubnetRoute<A>* route, bool in_use);
    
    virtual void RIB_lookup_done(const A& nexthop, 
			 const set <IPNet<A> >& nets,
			 bool lookup_succeeded);

    RouteTableType type() const {return NHLOOKUP_TABLE;}
    string str() const;
private:
    //access the message queue by subnet or an address on the subnet
    Trie<A, const MessageQueueEntry<A> > _queue_by_net;
    //access the message queue by nexthop
    multimap <A, const MessageQueueEntry<A>*> _queue_by_nexthop;

    NextHopResolver<A>* _next_hop_resolver;
};

#endif // __BGP_ROUTE_TABLE_NHLOOKUP_HH__
