// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

// $XORP: xorp/bgp/route_table_cache.hh,v 1.3 2003/02/08 01:32:58 mjh Exp $

#ifndef __BGP_ROUTE_TABLE_CACHE_HH__
#define __BGP_ROUTE_TABLE_CACHE_HH__

#include "route_table_base.hh"
#include "libxorp/trie.hh"

/**
 * Specialize Trie so that the SubnetRoute payload is deleted using
 * the SubnetRoute's unref method, which permits delayed deletion.
 */
template<>
void
TrieNode<IPv4, const SubnetRoute<IPv4> >
::delete_payload(const SubnetRoute<IPv4>* p) 
{
    p->unref();
}

template<>
void
TrieNode<IPv6, const SubnetRoute<IPv6> >
::delete_payload(const SubnetRoute<IPv6>* p) 
{
    p->unref();
}

template<class A>
class CacheTable : public BGPRouteTable<A>  {
public:
    CacheTable(string tablename, BGPRouteTable<A> *parent);
    int add_route(const InternalMessage<A> &rtmsg,
		  BGPRouteTable<A> *caller);
    int replace_route(const InternalMessage<A> &old_rtmsg,
		      const InternalMessage<A> &new_rtmsg,
		      BGPRouteTable<A> *caller);
    int delete_route(const InternalMessage<A> &rtmsg, 
		     BGPRouteTable<A> *caller);
    int push(BGPRouteTable<A> *caller);
    int route_dump(const InternalMessage<A> &rtmsg,
		   BGPRouteTable<A> *caller,
		   const PeerHandler *dump_peer);

    void flush_cache();
    const SubnetRoute<A> *lookup_route(const IPNet<A> &net) const;
    void route_used(const SubnetRoute<A>* route, bool in_use);

    RouteTableType type() const {return CACHE_TABLE;}
    string str() const;

    /* mechanisms to implement flow control in the output plumbing */
    void output_state(bool busy, BGPRouteTable<A> *next_table);
    bool get_next_message(BGPRouteTable<A> *next_table);

    int route_count() const {
	return _route_table.route_count();
    }
private:
    Trie<A, const SubnetRoute<A> > _route_table;
};

#endif // __BGP_ROUTE_TABLE_CACHE_HH__
