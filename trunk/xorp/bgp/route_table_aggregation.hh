// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

// $XORP: xorp/bgp/route_table_aggregation.hh,v 1.14 2007/05/23 12:12:32 pavlin Exp $

#ifndef __BGP_ROUTE_TABLE_AGGREGATION_HH__
#define __BGP_ROUTE_TABLE_AGGREGATION_HH__

#include "peer_handler.hh"
#include "route_table_base.hh"
#include "libxorp/ref_trie.hh"


/**
 * @short classes used for BGP prefix aggregation.
 *
 * The main aggregation processing occurs and all the state is kept
 * in an AggregationTable stage that is plumbed between the decission
 * and fanout tables.  Additionally, static filters immediately following
 * the fanout table may filter out certain routes marked by the
 * AggregationTable, depending on whether the outbound branch is of IBGP
 * or EBGP type.
 *                                              +---------+
 *                                            ->| Filters |->IBGP...
 *  \                                        /  +---------+
 *   ->+-----------+  +--------+  +--------+/   +---------+
 *  -->| Decission |->| Aggreg.|->| Fanout |--->| Filters |->IBGP...
 *   ->+-----------+  +--------+  +--------+\   +---------+
 *  /                                        \  +---------+
 *                    Processing,             ->| Filters |->EBGP...
 *                     state,                   +---------+
 *                     marking                 Static filters
 * 
 * Routes can be marked as candidates for aggregation using the generic
 * policy framework before they enter the Aggregation stage.  Routes not
 * marked for aggregation, or those improperly marked, will be
 * propagated through both the aggregation and static filtering stages
 * without any processing whatsoever, regardless on the downstream
 * peering type (IBGP or EBGP). 
 *
 * Routes that can trigger instantiation of an aggregate route (those
 * that have been properly marked) will be copied and copies stored in
 * the AggregationTable for further processing.  Regardless of the
 * outcome of the aggregation processing, every original route will be
 * marked as IBGP_ONLY and propagated downstream.  Filtering stages
 * residing on IBGP paths will propagate such routes further downstream,
 * while on EBGP paths all routes marked as IBGP_ONLY will be discarded.
 *
 * In addition to original routes marked as described in the previous
 * paragraph, the AggregationStage can emit both the aggregate and
 * copies of the original routes.  Any such routes will be marked with
 * an appropriate EBGP_ONLY_* marker, instructing the filtering stages
 * on IBGP branches to unconditionally drop them.  On EBGP branches...
 *
 *
 * Open issues:
 *
 * - return values for add/delete/replace routes
 *
 * - aggregate route tagging / dumping when a new EBGP peering comes up
 *
 * - {de|re}aggragetion of large aggregates -> timer based vs. atomic
 */

class XrlStdRouter;

template<class A>
class DumpIterator;

template<class A>
class AggregationTable;

template<class A>
class ComponentRoute {
public:
    ComponentRoute(const SubnetRoute<A>* route,
		   const PeerHandler *origin,
		   uint32_t genid,
		   bool from_previous_peering) : _routeref(route) {
	_origin_peer = origin;
	_genid = genid;
	_from_previous_peering = from_previous_peering;
    }
    const SubnetRoute<A>* route() const { return _routeref.route(); }
    const PeerHandler* origin_peer() const { return _origin_peer; }
    uint32_t genid() const { return _genid; }
    bool from_previous_peering() const { return _from_previous_peering; }
private:
    SubnetRouteConstRef<A> _routeref;
    const PeerHandler *_origin_peer;
    uint32_t _genid;
    bool _from_previous_peering;
};


template<class A>
class AggregateRoute {
public:
    AggregateRoute(IPNet<A> net,
		   bool brief_mode,
		   IPv4 bgp_id,
		   AsNum asnum)
        : _net(net), _brief_mode(brief_mode),
	  _was_announced(0), _is_suppressed(0) {
	    NextHopAttribute<A> nhatt(A::ZERO());
	    ASPath aspath;
	    OriginAttribute igp_origin_att(IGP);
	_pa_list = new PathAttributeList<A>(nhatt, aspath, igp_origin_att);
	_pa_list->rehash();
	_aggregator_attribute = new AggregatorAttribute(bgp_id, asnum);
    }
    ~AggregateRoute() {
	if (_components_table.begin() != _components_table.end())
	    XLOG_WARNING("ComponentsTable trie was not empty on deletion\n");
	delete _pa_list;
	delete _aggregator_attribute;
    }
    const PathAttributeList<A> *pa_list() const { return _pa_list; }
    const IPNet<A> net() const		{ return _net; }
    bool was_announced() const		{ return _was_announced; }
    bool is_suppressed() const		{ return _is_suppressed; }
    bool brief_mode() const		{ return _brief_mode; }
    void was_announced(bool value)	{ _was_announced = value; }
    void is_suppressed(bool value)	{ _is_suppressed = value; }
    void brief_mode(bool value)		{ _brief_mode = value; }
    void reevaluate(AggregationTable<A> *parent);
    RefTrie<A, const ComponentRoute<A> > *components_table() {
	return &_components_table;
    }

private:
    const IPNet<A> _net;
    bool _brief_mode;
    AggregatorAttribute *_aggregator_attribute;

    RefTrie<A, const ComponentRoute<A> > _components_table;
    PathAttributeList<A> *_pa_list;
    bool _was_announced;
    bool _is_suppressed;
};


template<class A>
class AggregationTable : public BGPRouteTable<A>  {
public:
    AggregationTable(string tablename,
		     BGPPlumbing& master,
		     BGPRouteTable<A> *parent);
    ~AggregationTable();
    int add_route(const InternalMessage<A> &rtmsg,
                  BGPRouteTable<A> *caller);
    int replace_route(const InternalMessage<A> &old_rtmsg,
                      const InternalMessage<A> &new_rtmsg,
                      BGPRouteTable<A> *caller);
    int delete_route(const InternalMessage<A> &rtmsg,
                     BGPRouteTable<A> *caller);
    int push(BGPRouteTable<A> *caller);

    bool dump_next_route(DumpIterator<A>& dump_iter);
    int route_dump(const InternalMessage<A> &rtmsg,
		   BGPRouteTable<A> *caller,
		   const PeerHandler *dump_peer);

    const SubnetRoute<A> *lookup_route(const IPNet<A> &net,
                                       uint32_t& genid) const;
    void route_used(const SubnetRoute<A>* route, bool in_use);

    RouteTableType type() const {return AGGREGATION_TABLE;}
    string str() const;

    /* mechanisms to implement flow control in the output plumbing */
    bool get_next_message(BGPRouteTable<A> *next_table);

    int route_count() const {
        return _aggregates_table.route_count(); // XXX is this OK?
    }

    void peering_went_down(const PeerHandler *peer, uint32_t genid,
                           BGPRouteTable<A> *caller);
    void peering_down_complete(const PeerHandler *peer, uint32_t genid,
                               BGPRouteTable<A> *caller);
    void peering_came_up(const PeerHandler *peer, uint32_t genid,
                         BGPRouteTable<A> *caller);

private:
    friend class AggregateRoute<A>;

    RefTrie<A, const AggregateRoute<A> > _aggregates_table;
    BGPPlumbing& _master_plumbing;
};


class AggregationHandler : public PeerHandler {
public:
    AggregationHandler();

    virtual PeerType get_peer_type() const {
	return PEER_TYPE_INTERNAL;
    }

    uint32_t get_unique_id() const	{ return _fake_unique_id; }
    virtual bool originate_route_handler() const { return true; }
    const uint32_t _fake_unique_id;
};


#endif // __BGP_ROUTE_TABLE_AGGREGATION_HH__
