// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/bgp/route_table_filter.hh,v 1.9 2004/05/15 15:12:17 mjh Exp $

#ifndef __BGP_ROUTE_TABLE_FILTER_HH__
#define __BGP_ROUTE_TABLE_FILTER_HH__

#include "route_table_base.hh"
#include "next_hop_resolver.hh"

template<class A>
class BGPRouteFilter {
public:
    BGPRouteFilter() {};

    /* "modified" is needed because we need to know whether we should
       free the rtmsg or not if we modify the route. If this
       filterbank has modified it, and then modifies it again, then we
       should free the rtmsg.  Otherwise it's the responsibility of
       whoever created the rtmsg*/
    virtual const InternalMessage<A>* 
       filter(const InternalMessage<A> *rtmsg, 
	      bool &modified) const = 0;
protected:
    void propagate_flags(const InternalMessage<A> *rtmsg,
			 InternalMessage<A> *new_rtmsg) const;
    virtual void drop_message(const InternalMessage<A> *rtmsg, 
			      bool &modified) const ;
private:
};

template<class A>
class SimpleASFilter : public BGPRouteFilter<A> {
public:
    SimpleASFilter(const AsNum &as_num);
    const InternalMessage<A>* 
       filter(const InternalMessage<A> *rtmsg, 
	      bool &modified) const ;
private:
    AsNum _as_num;
};

template<class A>
class ASPrependFilter : public BGPRouteFilter<A> {
public:
    ASPrependFilter(const AsNum &as_num);
    const InternalMessage<A>* 
       filter(const InternalMessage<A> *rtmsg, 
	      bool &modified) const;
private:
    AsNum _as_num;
};

template<class A>
class NexthopRewriteFilter : public BGPRouteFilter<A> {
public:
    NexthopRewriteFilter(const A &local_nexthop);
    const InternalMessage<A>* 
       filter(const InternalMessage<A> *rtmsg, 
	      bool &modified) const ;
private:
    A _local_nexthop;
};

template<class A>
class IBGPLoopFilter : public BGPRouteFilter<A> {
public:
    IBGPLoopFilter();
    const InternalMessage<A>* 
       filter(const InternalMessage<A> *rtmsg, 
	      bool &modified) const ;
private:
};

template<class A>
class LocalPrefInsertionFilter : public BGPRouteFilter<A> {
public:
    LocalPrefInsertionFilter(uint32_t default_local_pref);
    const InternalMessage<A>* filter(const InternalMessage<A> *rtmsg, 
				     bool &modified) const ;
private:
    uint32_t _default_local_pref;
};

template<class A>
class LocalPrefRemovalFilter : public BGPRouteFilter<A> {
public:
    LocalPrefRemovalFilter();
    const InternalMessage<A>* filter(const InternalMessage<A> *rtmsg, 
				     bool &modified) const ;
private:
};

template<class A>
class MEDInsertionFilter : public BGPRouteFilter<A> {
public:
    MEDInsertionFilter(NextHopResolver<A>& next_hop_resolver);
    const InternalMessage<A>* filter(const InternalMessage<A> *rtmsg, 
				     bool &modified) const ;
private:
    NextHopResolver<A>& _next_hop_resolver;
};

template<class A>
class MEDRemovalFilter : public BGPRouteFilter<A> {
public:
    MEDRemovalFilter();
    const InternalMessage<A>* filter(const InternalMessage<A> *rtmsg, 
				     bool &modified) const ;
private:
};

template<class A>
class UnknownFilter : public BGPRouteFilter<A> {
public:
    UnknownFilter();
    const InternalMessage<A>* filter(const InternalMessage<A> *rtmsg, 
				     bool &modified) const ;
private:
};

/**
 * Perform any filter operations that are required for routes that we
 * originate.
 * 
 * Currently this only involves prepending our AS number on IBGP
 * peers. We assume that on EBGP peers the AS number has already been
 * prepended.
 *
 */
template<class A>
class OriginateRouteFilter : public BGPRouteFilter<A> {
public:
    OriginateRouteFilter(const AsNum &as_num, const bool ibgp);
    const InternalMessage<A>* filter(const InternalMessage<A> *rtmsg, 
				     bool &modified) const ;
private:
    AsNum _as_num;
    bool _ibgp;
};

/*
 * FilterTable is a route table that can hold banks of route
 * filters.  Normally FilterTable propagates add_route,
 * delete_route and the response to lookup_route directly through from
 * the parent to the child.  A route filter can cause these to fail to
 * be propagated, or can modify the attributes of the route in the
 * message.
 */

template<class A>
class FilterTable : public BGPRouteTable<A>  {
public:
    FilterTable(string tablename,
		Safi safi,
		BGPRouteTable<A> *parent, 
		NextHopResolver<A>& next_hop_resolver);
    ~FilterTable();
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
    void route_used(const SubnetRoute<A>* route, bool in_use);

    RouteTableType type() const {return FILTER_TABLE;}
    string str() const;

    /* mechanisms to implement flow control in the output plumbing */
    void output_state(bool busy, BGPRouteTable<A> *next_table);
    bool get_next_message(BGPRouteTable<A> *next_table);

    int add_simple_AS_filter(const AsNum &asn);
    int add_AS_prepend_filter(const AsNum &asn);
    int add_nexthop_rewrite_filter(const A& nexthop);
    int add_ibgp_loop_filter();
    int add_localpref_insertion_filter(uint32_t default_local_pref);
    int add_localpref_removal_filter();
    int add_med_insertion_filter();
    int add_med_removal_filter();
    int add_unknown_filter();
    int add_originate_route_filter(const AsNum &asn, const bool);
private:
    const InternalMessage<A> *
        apply_filters(const InternalMessage<A> *rtmsg) const;
    list <BGPRouteFilter<A> *> _filters;
    NextHopResolver<A>& _next_hop_resolver;
};

#endif // __BGP_ROUTE_TABLE_FILTER_HH__
