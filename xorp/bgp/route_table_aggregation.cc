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



//#define DEBUG_LOGGING
//#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "path_attribute.hh"
#include "bgp.hh"
#include "dump_iterators.hh"
#include "route_table_aggregation.hh"


template<class A>
AggregationTable<A>::AggregationTable(string table_name,
				      BGPPlumbing& master,
				      BGPRouteTable<A> *parent_table)
    : BGPRouteTable<A>("AggregationTable-" + table_name, master.safi()),
      _master_plumbing(master)
{
    this->_parent = parent_table;
}


template<class A>
AggregationTable<A>::~AggregationTable()
{
    if (_aggregates_table.begin() != _aggregates_table.end()) {
        XLOG_WARNING("AggregatesTable trie was not empty on deletion\n");
    }
}


template<class A>
int
AggregationTable<A>::add_route(InternalMessage<A> &rtmsg,
			       BGPRouteTable<A> *caller)
{
    debug_msg("\n         %s\n caller: %s\n rtmsg: %p route: %p\n%s\n",
	      this->tablename().c_str(),
	      caller ? caller->tablename().c_str() : "NULL",
	      &rtmsg,
	      rtmsg.route(),
	      rtmsg.str().c_str());

    const SubnetRoute<A> *orig_route = rtmsg.route();
    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(this->_next_table != NULL);
    XLOG_ASSERT(orig_route->nexthop_resolved());
    XLOG_ASSERT(!rtmsg.attributes()->is_locked());
    bool must_push = false;

    /*
     * If not marked as aggregation candidate, pass the request
     * unmodified downstream.  DONE.
     */
    uint32_t aggr_prefix_len = rtmsg.route()->aggr_prefix_len();
    debug_msg("aggr_prefix_len=%d\n", aggr_prefix_len);
    if (aggr_prefix_len == SR_AGGR_IGNORE)
	return this->_next_table->add_route(rtmsg, (BGPRouteTable<A>*)this);

    /*
     * If the route has less a specific prefix length then the requested
     * aggregate, pass the request downstream without considering
     * to create an aggregate.  Since we have to modify the
     * aggr_prefix_len field of the route, we must operate on a copy
     * of the original route.
     */
    const IPNet<A> orig_net = rtmsg.net();
    const IPNet<A> aggr_net = IPNet<A>(orig_net.masked_addr(),
				       aggr_prefix_len);
    SubnetRoute<A> *ibgp_r = new SubnetRoute<A>(*orig_route);
    InternalMessage<A> ibgp_msg(ibgp_r, 
				rtmsg.attributes(),
				rtmsg.origin_peer(),
				rtmsg.genid());

    // propagate internal message flags
    if (rtmsg.push())
        must_push = true;
    if (rtmsg.from_previous_peering())
        ibgp_msg.set_from_previous_peering();

    if (orig_net.prefix_len() < aggr_prefix_len) {
	// Send the "original" version downstream, and we are DONE.
        debug_msg("Bogus marking\n");
	if (must_push)
	    ibgp_msg.set_push();
	ibgp_r->set_aggr_prefix_len(SR_AGGR_IGNORE);
	int res = this->_next_table->
	    add_route(ibgp_msg, (BGPRouteTable<A>*)this);
	ibgp_r->unref();
	return res;
    }

    // Find existing or create a new aggregate route
    typename RefTrie<A, const AggregateRoute<A> >::iterator ai;
    ai = _aggregates_table.lookup_node(aggr_net);
    if (ai == _aggregates_table.end()) {
	LocalData *local_data = _master_plumbing.main().get_local_data();
        const AggregateRoute<A> *new_aggr_route =
	    new AggregateRoute<A>(aggr_net,
				  orig_route->aggr_brief_mode(),
				  local_data->get_id(),
				  local_data->get_as());
	ai = _aggregates_table.insert(aggr_net, *new_aggr_route);
    }
    AggregateRoute<A> *aggr_route =
	const_cast<AggregateRoute<A> *> (&ai.payload());

    // Check we don't already have the original route stored
    XLOG_ASSERT(aggr_route->components_table()->lookup_node(orig_net) == aggr_route->components_table()->end());

    // We make the assumption here that nothing has modified the path
    // attribute list since the last time it was canonicalized, or
    // this will fail to store the chances.
    aggr_route->components_table()->insert(orig_net, ComponentRoute<A>(
						rtmsg.route(),
						rtmsg.origin_peer(),
						rtmsg.genid(),
						rtmsg.from_previous_peering()));

    /*
     * If our component route holds a more specific prefix than the
     * aggregate, announce it to EBGP peering branches.
     */
    if (aggr_route->net() != orig_net || aggr_route->is_suppressed()) {
	SubnetRoute<A> *ebgp_r = new SubnetRoute<A>(*orig_route);
	InternalMessage<A> ebgp_msg(ebgp_r, rtmsg.attributes(),
				    rtmsg.origin_peer(), rtmsg.genid());

	// propagate internal message flags
	if (rtmsg.from_previous_peering())
	    ebgp_msg.set_from_previous_peering();

	if (aggr_route->is_suppressed())
	    ebgp_r->set_aggr_prefix_len(SR_AGGR_EBGP_NOT_AGGREGATED);
	else
	    ebgp_r->set_aggr_prefix_len(SR_AGGR_EBGP_WAS_AGGREGATED);
	this->_next_table->add_route(ebgp_msg, (BGPRouteTable<A>*)this);
	ebgp_r->unref();
    }

    /*
     * Recompute the aggregate.  If pa_list different from the old one,
     * check whether we have to withdraw the old and / or announce the
     * new aggregate and / or all the component routes.
     */
    aggr_route->reevaluate(this);

    /*
     * Send the "original" version downstream.  SR_AGGR_IBGP_ONLY marker
     * will instruct the post-fanout static filters to propagate this
     * route only to IBGP peerings and localRIB.
     */
    ibgp_r->set_aggr_prefix_len(SR_AGGR_IBGP_ONLY);
    int res = this->_next_table->add_route(ibgp_msg, (BGPRouteTable<A>*)this);
    ibgp_r->unref();
    if (must_push)
	this->_next_table->push(this);
    return res;
}


template<class A>
int
AggregationTable<A>::delete_route(InternalMessage<A> &rtmsg,
				  BGPRouteTable<A> *caller)
{
    debug_msg("\n         %s\n caller: %s\n rtmsg: %p route: %p\n%s\n",
	      this->tablename().c_str(),
	      caller ? caller->tablename().c_str() : "NULL",
	      &rtmsg,
	      rtmsg.route(),
	      rtmsg.str().c_str());

    const SubnetRoute<A> *orig_route = rtmsg.route();
    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(this->_next_table != NULL);
    XLOG_ASSERT(orig_route->nexthop_resolved());
    bool must_push = false;

    /*
     * If not marked as aggregation candidate, pass the request
     * unmodified downstream.  DONE.
     */
    uint32_t aggr_prefix_len = orig_route->aggr_prefix_len();
    debug_msg("aggr_prefix_len=%d\n", aggr_prefix_len);
    if (aggr_prefix_len == SR_AGGR_IGNORE)
	return this->_next_table->delete_route(rtmsg, (BGPRouteTable<A>*)this);

    /*
     * If the route has less a specific prefix length then the requested
     * aggregate, pass the request downstream without considering
     * to create an aggregate.  Since we have to modify the
     * aggr_prefix_len field of the route, we must operate on a copy
     * of the original route.
     */
    const IPNet<A> orig_net = rtmsg.net();
    const IPNet<A> aggr_net = IPNet<A>(orig_net.masked_addr(),
				       aggr_prefix_len);
    SubnetRoute<A> *ibgp_r = new SubnetRoute<A>(*orig_route);
    InternalMessage<A> ibgp_msg(ibgp_r, rtmsg.origin_peer(), rtmsg.genid());

    // propagate internal message flags
    if (rtmsg.push())
        must_push = true;
    if (rtmsg.from_previous_peering())
        ibgp_msg.set_from_previous_peering();

    if (orig_net.prefix_len() < aggr_prefix_len) {
	// Send the "original" version downstream, and we are DONE.
        debug_msg("Bogus marking\n");
	if (must_push)
	    ibgp_msg.set_push();
	ibgp_r->set_aggr_prefix_len(SR_AGGR_IGNORE);
	int res = this->_next_table->
	    delete_route(ibgp_msg, (BGPRouteTable<A>*)this);
	ibgp_r->unref();
	return res;
    }

    // Find the appropriate aggregate route
    typename RefTrie<A, const AggregateRoute<A> >::iterator ai;
    ai = _aggregates_table.lookup_node(aggr_net);
    // Check that the aggregate exists, otherwise something's gone wrong.
    XLOG_ASSERT(ai != _aggregates_table.end());
    AggregateRoute<A> *aggr_route =
	const_cast<AggregateRoute<A> *> (&ai.payload());

    /*
     * If our component route holds a more specific prefix than the
     * aggregate, send a delete rquest for it to EBGP branches.
     */
    if (aggr_route->net() != orig_net || aggr_route->is_suppressed()) {
	SubnetRoute<A> *ebgp_r = new SubnetRoute<A>(*orig_route);
	InternalMessage<A> ebgp_msg(ebgp_r, rtmsg.origin_peer(), rtmsg.genid());

	// propagate internal message flags
	if (rtmsg.from_previous_peering())
	    ebgp_msg.set_from_previous_peering();

	if (aggr_route->is_suppressed())
	    ebgp_r->set_aggr_prefix_len(SR_AGGR_EBGP_NOT_AGGREGATED);
	else
	    ebgp_r->set_aggr_prefix_len(SR_AGGR_EBGP_WAS_AGGREGATED);
	this->_next_table->delete_route(ebgp_msg, (BGPRouteTable<A>*)this);
	ebgp_r->unref();
    }

    aggr_route->components_table()->erase(orig_net);

    /*
     * Recompute the aggregate.  If pa_list different from the old one,
     * check whether we have to withdraw the old and / or announce the
     * new aggregate.
     */
    aggr_route->reevaluate(this);

    if (aggr_route->components_table()->route_count() == 0)
	_aggregates_table.erase(aggr_net);

    /*
     * Send the "original" version downstream.  SR_AGGR_IBGP_ONLY marker
     * will instruct the post-fanout static filters to propagate this
     * route only to IBGP peerings and localRIB.
     */
    ibgp_r->set_aggr_prefix_len(SR_AGGR_IBGP_ONLY);
    int res = this->_next_table->
	delete_route(ibgp_msg, (BGPRouteTable<A>*)this);
    ibgp_r->unref();
    if (must_push)
	this->_next_table->push(this);
    return res;
}


template<class A>
void
AggregateRoute<A>::reevaluate(AggregationTable<A> *parent)
{
    typename RefTrie<A, const ComponentRoute<A> >::PostOrderIterator comp_iter;
    uint32_t med = 0;
    bool must_set_atomic_aggr = false;
    bool old_was_suppressed = _is_suppressed;
    bool old_was_announced = _was_announced;
    _is_suppressed = false;

    PAListRef<A> old_pa_list = _pa_list;
    // create an FPAList so we can access the elements, and perhaps
    // pass it downstream for deletion
    FPAListRef old_fpa_list = new FastPathAttributeList<A>(old_pa_list);

    NextHopAttribute<A> nhatt(A::ZERO());
    ASPath aspath;
    OriginAttribute igp_origin_att(IGP);
    FPAListRef fpa_list 
	= new FastPathAttributeList<A>(nhatt, aspath, igp_origin_att);

    /*
     * PHASE 1:
     *
     * Iterate through all component routes in order to compute the new
     * path attributes and determine whether we are allowed to announce
     * the aggregate or not.
     */
    debug_msg("PHASE 1\n");
    for (comp_iter = _components_table.begin();
         comp_iter != _components_table.end(); comp_iter++) {
	const SubnetRoute<A>* comp_route = comp_iter.payload().route();
	PAListRef<A> comp_pa_list = comp_route->attributes();
	FastPathAttributeList<A> comp_fpa_list(comp_pa_list);
	debug_msg("comp_route: %s\n    %s\n",
		  comp_route->net().str().c_str(),
		  comp_fpa_list.aspath().str().c_str());

	if (comp_iter == _components_table.begin()) {
	    if (comp_fpa_list.med_att()) {
		med = comp_fpa_list.med_att()->med();
	    }
	    fpa_list->replace_AS_path(comp_fpa_list.aspath());
	    fpa_list->replace_origin((OriginType)comp_fpa_list.origin());
	} else {
	    if (comp_fpa_list.med_att() &&
		med != comp_fpa_list.med_att()->med()) {
		_is_suppressed = true;
		break;
	    }

	    // Origin attr: INCOMPLETE overrides EGP which overrides IGP
	    if (comp_fpa_list.origin() > old_fpa_list->origin())
		fpa_list->replace_origin((OriginType)comp_fpa_list.origin());

	    // Update the aggregate AS path using this component route.
	    if (this->brief_mode()) {
		/*
		 * The agggregate will have an empty AS path, in which
		 * case we also must set the ATOMIC AGGREGATE attribute.
		 */
		if (fpa_list->aspath() != comp_fpa_list.aspath()) {
		    fpa_list->replace_AS_path(ASPath());
		    must_set_atomic_aggr = true;
		}
	    } else {
		/*
		 * Merge the current AS path with the component route's one
		 * by creating an AS SET for non-matching ASNs.
		 */
		fpa_list->replace_AS_path(ASPath(old_fpa_list->aspath(),
						comp_fpa_list.aspath()));
	    }
	}

	// Propagate the ATOMIC AGGREGATE attribute
	if (comp_fpa_list.atomic_aggregate_att())
	    must_set_atomic_aggr = true;
    }

    // Add a MED attr if needed and allowed to
    if (med &&
	!(fpa_list->aspath().num_segments() &&
	  fpa_list->aspath().segment(0).type() == AS_SET)) {
	MEDAttribute med_attr(med);
	fpa_list->add_path_attribute(med_attr);
    }

    if (must_set_atomic_aggr) {
	AtomicAggAttribute aa_attr;
	fpa_list->add_path_attribute(aa_attr);
    }

    fpa_list->add_path_attribute(*_aggregator_attribute);

    fpa_list->canonicalize();
    _pa_list = new PathAttributeList<A>(fpa_list);

    debug_msg("OLD ATTRIBUTES:%s\n", old_fpa_list->str().c_str());
    debug_msg("NEW ATTRIBUTES:%s\n", fpa_list->str().c_str());

    

    /*
     * Phase 2:
     *
     * If the aggregate was announced, and either it should be suppressed
     * from now on, or its path attributes have changed, delete the old
     * version.
     */
    debug_msg("PHASE 2\n");
    if (_was_announced && (_is_suppressed || !(*old_fpa_list == *fpa_list))) {
	/*
	 * We need to send a delete request for the old aggregate
	 * before proceeding any further.  Construct a temporary
	 * subnet route / message and send downstream.
	 */
	SubnetRoute<A>* tmp_route = new SubnetRoute<A>(_net,
						       old_pa_list,
						       NULL,
						       0);
	tmp_route->set_nexthop_resolved(true);	// Cheating
	tmp_route->set_aggr_prefix_len(SR_AGGR_EBGP_AGGREGATE);
	InternalMessage<A> tmp_rtmsg(tmp_route,
				     old_fpa_list,
				     parent->_master_plumbing.rib_handler(),
				     GENID_UNKNOWN);
	parent->_next_table->delete_route(tmp_rtmsg, parent);
	tmp_route->unref();
	_was_announced = false;
    }

    /*
     * Phase 3:
     *
     * If the aggregate must be suppressed now, but was active before, so
     * we must reannounce all the component routes with proper marking, i.e.
     * SR_AGGR_EBGP_NOT_AGGREGATED instead of SR_AGGR_EBGP_WAS_AGGREGATED.
     * Similarly, if previously the aggregate was suppressed, but now it
     * should not be any more, we must reannounce all the component routes
     * and mark them as SR_AGGR_EBGP_WAS_AGGREGATED.
     *
     * NOTE: If the aggregate contains a large number of component routes,
     * this naive approach of atomic replacing of all the component routes
     * until completion can lead to severe lockouts.  We might be much
     * better off with a timer-based replacement routine, but this would
     * significantly complicate the implementation.
     */
    debug_msg("PHASE 3\n");
    if (old_was_suppressed != _is_suppressed) {
	for (comp_iter = _components_table.begin();
	     comp_iter != _components_table.end(); comp_iter++) {
	    const SubnetRoute<A> *comp_route = comp_iter.payload().route();
	    debug_msg("%s\n", comp_route->str().c_str());
	    SubnetRoute<A> *old_r = new SubnetRoute<A>(*comp_route);
	    SubnetRoute<A> *new_r = new SubnetRoute<A>(*comp_route);
	    InternalMessage<A> old_msg(old_r,
				       comp_iter.payload().origin_peer(),
				       comp_iter.payload().genid());
	    InternalMessage<A> new_msg(new_r,
				       comp_iter.payload().origin_peer(),
				       comp_iter.payload().genid());

	    if (old_was_suppressed) {
		old_r->set_aggr_prefix_len(SR_AGGR_EBGP_NOT_AGGREGATED);
		new_r->set_aggr_prefix_len(SR_AGGR_EBGP_WAS_AGGREGATED);
	    } else {
		old_r->set_aggr_prefix_len(SR_AGGR_EBGP_WAS_AGGREGATED);
		new_r->set_aggr_prefix_len(SR_AGGR_EBGP_NOT_AGGREGATED);
	    }
	    /*
	     * Do not add / remove component routes if they would
	     * conflict with the aggregate.
	     */
	    if (!(old_was_announced && comp_route->net() == _net)) {
		debug_msg("deleting\n");
		parent->_next_table->delete_route(old_msg, parent);
	    }
	    if (!(!_is_suppressed && comp_route->net() == _net)) {
		debug_msg("announcing\n");
		parent->_next_table->add_route(new_msg, parent);
	    }
	    old_r->unref();
	    new_r->unref();
	}
    }

    /*
     * Phase 4:
     *
     * If the new path attributes are different from the old ones, delete
     * the old version of the aggregate route and announce a fresh one,
     * unless _is_suppressed flag was set.
     */
    debug_msg("PHASE 4\n");
    if (!_was_announced &&
	!_is_suppressed &&
	_components_table.route_count() != 0 &&
	(old_was_suppressed || !(*old_fpa_list == *fpa_list))) {
	SubnetRoute<A>* tmp_route = new SubnetRoute<A>(_net,
						       _pa_list,
						       NULL,
						       0);
	tmp_route->set_nexthop_resolved(true);	// Cheating
	tmp_route->set_aggr_prefix_len(SR_AGGR_EBGP_AGGREGATE);
	InternalMessage<A> tmp_rtmsg(tmp_route,
				     fpa_list,
				     parent->_master_plumbing.rib_handler(),
				     GENID_UNKNOWN);
	parent->_next_table->add_route(tmp_rtmsg, parent);
	tmp_route->unref();
	_was_announced = true;
    }
}


template<class A>
int
AggregationTable<A>::replace_route(InternalMessage<A> &old_rtmsg,
				   InternalMessage<A> &new_rtmsg,
				   BGPRouteTable<A> *caller)
{
    debug_msg("\n         %s\n"
	      "caller: %s\n"
	      "old rtmsg: %p new rtmsg: %p "
	      "old route: %p"
	      "new route: %p"
	      "old: %s\n new: %s\n",
	      this->tablename().c_str(),
	      caller->tablename().c_str(),
	      &old_rtmsg,
	      &new_rtmsg,
	      old_rtmsg.route(),
	      new_rtmsg.route(),
	      old_rtmsg.str().c_str(),
	      new_rtmsg.str().c_str());

    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(old_rtmsg.route()->nexthop_resolved());
    XLOG_ASSERT(new_rtmsg.route()->nexthop_resolved());

    if (old_rtmsg.route()->aggr_prefix_len() == SR_AGGR_IGNORE &&
        new_rtmsg.route()->aggr_prefix_len() == SR_AGGR_IGNORE) {
	return this->_next_table->replace_route(old_rtmsg,
						new_rtmsg,
						(BGPRouteTable<A>*)this);
    }

    this->delete_route(old_rtmsg, (BGPRouteTable<A>*)caller);
    return this->add_route(new_rtmsg, (BGPRouteTable<A>*)caller);
}


template<class A>
int
AggregationTable<A>::push(BGPRouteTable<A> *caller)
{
    debug_msg("Push\n");
    XLOG_ASSERT(caller == this->_parent);
    return this->_next_table->push((BGPRouteTable<A>*)this);
}


template<class A>
bool
AggregationTable<A>::dump_next_route(DumpIterator<A>& dump_iter) {
    const PeerHandler* peer = dump_iter.current_peer();

    // Propagate the request upstream if not for us.
    if (peer->get_unique_id() != AGGR_HANDLER_UNIQUE_ID)
	return this->_parent->dump_next_route(dump_iter);

    typename RefTrie<A, const AggregateRoute<A> >::iterator route_iterator;
    debug_msg("dump iter: %s\n", dump_iter.str().c_str());
   
    if (dump_iter.route_iterator_is_valid()) {
	debug_msg("route_iterator is valid\n");
 	route_iterator = dump_iter.aggr_iterator();
	// Make sure the iterator is valid. If it is pointing at a
	// deleted node this comparison will move it forward.
	if (route_iterator == _aggregates_table.end()) {
	    return false;
	}
	
	//we need to move on to the next node, except if the iterator
	//was pointing at a deleted node, because then it will have
	//just been moved to the next node to dump, so we need to dump
	//the node that the iterator is currently pointing at.
	if (dump_iter.iterator_got_moved(route_iterator.key()) == false)
	    route_iterator++;
    } else {
	debug_msg("route_iterator is not valid\n");
	route_iterator = _aggregates_table.begin();
    }

    if (route_iterator == _aggregates_table.end()) {
	return false;
    }

    const AggregateRoute<A>* aggr_rt;
    for ( ; route_iterator != _aggregates_table.end(); route_iterator++) {
	aggr_rt = &(route_iterator.payload());
	debug_msg("aggr_rt: %s\n", aggr_rt->net().str().c_str());

	// propagate downstream

	if (dump_iter.peer_to_dump_to() != NULL &&
	    aggr_rt->was_announced()) {
	    SubnetRoute<A> *tmp_route = new SubnetRoute<A>(aggr_rt->net(),
						           aggr_rt->pa_list(),
						           NULL,
						           0);
	    tmp_route->set_nexthop_resolved(true);	// Cheating
	    tmp_route->set_aggr_prefix_len(SR_AGGR_EBGP_AGGREGATE);
	    PAListRef<A> pa_list = aggr_rt->pa_list();
	    FPAListRef fpa_list = 
		new FastPathAttributeList<A>(pa_list);
	    InternalMessage<A> rt_msg(tmp_route, fpa_list, peer, GENID_UNKNOWN);
	   
	    this->_next_table->route_dump(rt_msg,
					  (BGPRouteTable<A>*)this,
					  dump_iter.peer_to_dump_to());
	    break;
	}
    }

    if (route_iterator == _aggregates_table.end())
	return false;

    // Store the new iterator value as its valid.
    dump_iter.set_aggr_iterator(route_iterator);

    return true;
}


template<class A>
int
AggregationTable<A>::route_dump(InternalMessage<A> &rtmsg,
				BGPRouteTable<A> *caller,
				const PeerHandler *dump_peer)
{
    debug_msg("\n         %s\n caller: %s\n rtmsg: %p route: %p\n%s\n",
	      this->tablename().c_str(),
	      caller ? caller->tablename().c_str() : "NULL",
	      &rtmsg,
	      rtmsg.route(),
	      rtmsg.str().c_str());

    const SubnetRoute<A> *orig_route = rtmsg.route();
    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(this->_next_table != NULL);
    XLOG_ASSERT(orig_route->nexthop_resolved());

    /*
     * If not marked as aggregation candidate, pass the request
     * unmodified downstream.  DONE.
     */
    uint32_t aggr_prefix_len = rtmsg.route()->aggr_prefix_len();
    debug_msg("aggr_prefix_len=%d\n", aggr_prefix_len);
    if (aggr_prefix_len == SR_AGGR_IGNORE)
	return this->_next_table->route_dump(rtmsg, (BGPRouteTable<A>*)this,
					     dump_peer);

    /*
     * If the route has less a specific prefix length then the requested
     * aggregate, pass the request downstream without considering
     * to create an aggregate.  Since we have to modify the
     * aggr_prefix_len field of the route, we must operate on a copy
     * of the original route.
     */
    const IPNet<A> orig_net = rtmsg.net();
    const IPNet<A> aggr_net = IPNet<A>(orig_net.masked_addr(),
				       aggr_prefix_len);
    SubnetRoute<A> *ibgp_r = new SubnetRoute<A>(*orig_route);
    InternalMessage<A> ibgp_msg(ibgp_r, rtmsg.origin_peer(), rtmsg.genid());

    // propagate internal message flags
    if (rtmsg.from_previous_peering())
        ibgp_msg.set_from_previous_peering();

    if (orig_net.prefix_len() < aggr_prefix_len || dump_peer->ibgp()) {
	// Send the "original" version downstream, and we are DONE.
	ibgp_r->set_aggr_prefix_len(SR_AGGR_IGNORE);
	int res =  this->_next_table->route_dump(ibgp_msg,
						 (BGPRouteTable<A>*)this,
						 dump_peer);
	ibgp_r->unref();
	return res;
    }

    // Find the appropriate aggregate route
    typename RefTrie<A, const AggregateRoute<A> >::iterator ai;
    ai = _aggregates_table.lookup_node(aggr_net);
    // Check that the aggregate exists, otherwise something's gone wrong.
    XLOG_ASSERT(ai != _aggregates_table.end());
    AggregateRoute<A> *aggr_route =
	const_cast<AggregateRoute<A> *> (&ai.payload());

    /*
     * If our component route holds a more specific prefix than the
     * aggregate, send it downstream.
     */
    if (aggr_route->net() != orig_net || aggr_route->is_suppressed()) {
	SubnetRoute<A> *ebgp_r = new SubnetRoute<A>(*orig_route);
	InternalMessage<A> ebgp_msg(ebgp_r, rtmsg.origin_peer(), rtmsg.genid());

	// propagate internal message flags
	if (rtmsg.from_previous_peering())
	    ebgp_msg.set_from_previous_peering();

	if (aggr_route->is_suppressed())
	    ebgp_r->set_aggr_prefix_len(SR_AGGR_EBGP_NOT_AGGREGATED);
	else
	    ebgp_r->set_aggr_prefix_len(SR_AGGR_EBGP_WAS_AGGREGATED);
	int res = this->_next_table->route_dump(ebgp_msg,
						(BGPRouteTable<A>*)this,
						dump_peer);
	ebgp_r->unref();
	return res;
    }

    debug_msg("XXX Shouldn't get here unless EBGP & aggregate == component\n");
    return 0; // XXX REVISIT!
}


template<class A>
const SubnetRoute<A>*
AggregationTable<A>::lookup_route(const IPNet<A> &net,
				  uint32_t& genid, 
				  FPAListRef& pa_list) const
{
    debug_msg("Lookup_route\n");
    // XXX Can this ever be called?  REVISIT!!!
    return this->_parent->lookup_route(net, genid, pa_list);
}


template<class A>
void
AggregationTable<A>::route_used(const SubnetRoute<A>* rt, bool in_use)
{
    debug_msg("route_used\n");
    // XXX Can this ever be called?  REVISIT!!!
    this->_parent->route_used(rt, in_use);
}


template<class A>
string
AggregationTable<A>::str() const
{
    string s = "AggregationTable<A>" + this->tablename();
    return s;
}


template<class A>
bool
AggregationTable<A>::get_next_message(BGPRouteTable<A> *next_table)
{
    debug_msg("next table: %s\n", next_table->tablename().c_str());
    // XXX Can this ever be called?  REVISIT!!!
    return 0; // XXX
}


template<class A>
void
AggregationTable<A>::peering_went_down(const PeerHandler *peer,
				       uint32_t genid,
				       BGPRouteTable<A> *caller)
{
    debug_msg("%s %d %s\n", peer->peername().c_str(), genid, caller->str().c_str());
    XLOG_ASSERT(this->_parent == caller);
    XLOG_ASSERT(this->_next_table != NULL);
    this->_next_table->peering_went_down(peer, genid, this);
}


template<class A>
void
AggregationTable<A>::peering_down_complete(const PeerHandler *peer,
					   uint32_t genid,
					   BGPRouteTable<A> *caller)
{
    debug_msg("%s %d %s\n", peer->peername().c_str(), genid, caller->str().c_str());
    XLOG_ASSERT(this->_parent == caller);
    XLOG_ASSERT(this->_next_table != NULL);
    this->_next_table->peering_down_complete(peer, genid, this);
}


template<class A>
void
AggregationTable<A>::peering_came_up(const PeerHandler *peer,
				     uint32_t genid,
				     BGPRouteTable<A> *caller)
{
    debug_msg("%s %d %s\n", peer->peername().c_str(), genid, caller->str().c_str());
    XLOG_ASSERT(this->_parent == caller);
    XLOG_ASSERT(this->_next_table != NULL);
    this->_next_table->peering_came_up(peer, genid, this);
}


AggregationHandler::AggregationHandler()
    : PeerHandler("AggregationHandler", NULL, NULL, NULL),
      _fake_unique_id(AGGR_HANDLER_UNIQUE_ID)
{
}


template class AggregationTable<IPv4>;
template class AggregationTable<IPv6>;

