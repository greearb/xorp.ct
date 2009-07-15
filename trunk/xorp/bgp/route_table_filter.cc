// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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
#include "route_table_filter.hh"
#include "peer_handler.hh"
#include "bgp.hh"

/*************************************************************************/

#if 0
template<class A>
void
BGPRouteFilter<A>::propagate_flags(const InternalMessage<A> *rtmsg,
				   InternalMessage<A> *new_rtmsg) const
{
    if (rtmsg->push())
	new_rtmsg->set_push();
    if (rtmsg->from_previous_peering())
	new_rtmsg->set_from_previous_peering();
}


template<class A>
void
BGPRouteFilter<A>::propagate_flags(const SubnetRoute<A>& route,
				   SubnetRoute<A>& new_route) const {
    new_route.set_filtered(route.is_filtered());
    new_route.set_policytags(route.policytags());
    new_route.set_aggr_prefix_len(route.aggr_prefix_len());
    for (int i = 0; i < 3; i++)
	new_route.set_policyfilter(i, route.policyfilter(i));
    if(route.is_winner())
	new_route.set_is_winner(route.igp_metric());
    new_route.set_nexthop_resolved(route.nexthop_resolved());
}	
#endif			   

/*************************************************************************/

template<class A>
AggregationFilter<A>::AggregationFilter(bool is_ibgp) 
    : _is_ibgp(is_ibgp)
{
}

template<class A>
bool AggregationFilter<A>::filter(InternalMessage<A>& rtmsg) const 
{
    uint8_t aggr_tag = rtmsg.route()->aggr_prefix_len();

    if (aggr_tag == SR_AGGR_IGNORE) {
	// Route was not even marked for aggregation
	return true;
    }

    // Has our AggregationTable properly marked the route?
    XLOG_ASSERT(aggr_tag >= SR_AGGR_EBGP_AGGREGATE);

    if (_is_ibgp) {
	// Peering is IBGP
	if (aggr_tag == SR_AGGR_IBGP_ONLY) {
	    return true;
	} else {
	    return false;
	}
    } else {
	// Peering is EBGP
	if (aggr_tag != SR_AGGR_IBGP_ONLY) {
	    // EBGP_AGGREGATE | EBGP_NOT_AGGREGATED | EBGP_WAS_AGGREGATED
	    return true;
	} else {
	    return false;
	}
    }
}

/*************************************************************************/

template<class A>
SimpleASFilter<A>::SimpleASFilter(const AsNum &as_num) 
    : _as_num(as_num)
{
}

template<class A>
bool
SimpleASFilter<A>::filter(InternalMessage<A>& rtmsg) const 
{
    FPAListRef& attributes = rtmsg.attributes();
    const ASPath& as_path = attributes->aspath();
    debug_msg("Filter: AS_Path filter for >%s< checking >%s<\n",
	   _as_num.str().c_str(), as_path.str().c_str());
    if (as_path.contains(_as_num)) {
	return false;
    }
    return true;
}

/*************************************************************************/

template<class A>
RRInputFilter<A>::RRInputFilter(IPv4 bgp_id, IPv4 cluster_id)
    : _bgp_id(bgp_id), _cluster_id(cluster_id)
{
}

template<class A>
bool
RRInputFilter<A>::filter(InternalMessage<A>& rtmsg) const 
{
    FPAListRef attributes = rtmsg.attributes();
    const OriginatorIDAttribute *oid = attributes->originator_id();
    if (0 != oid && oid->originator_id() == _bgp_id) {
	return false;
    }
    const ClusterListAttribute *cl = attributes->cluster_list();
    if (0 != cl && cl->contains(_cluster_id)) {
	return false;
    }

    return true;
}

/*************************************************************************/

template<class A>
ASPrependFilter<A>::ASPrependFilter(const AsNum &as_num, 
				    bool is_confederation_peer) 
    : _as_num(as_num), _is_confederation_peer(is_confederation_peer)
{
}

template<class A>
bool
ASPrependFilter<A>::filter(InternalMessage<A>& rtmsg) const
{
    //Create a new AS path with our AS number prepended to it.
    ASPath new_as_path(rtmsg.attributes()->aspath());

    if (_is_confederation_peer) { 
	new_as_path.prepend_confed_as(_as_num);
    } else {
	new_as_path.remove_confed_segments();
	new_as_path.prepend_as(_as_num);
    } 

    //Form a new path attribute list containing the new AS path
    FPAListRef& palist = rtmsg.attributes();
    palist->replace_AS_path(new_as_path);
    
    rtmsg.set_changed();

    return true;
}

/*************************************************************************/

template<class A>
NexthopRewriteFilter<A>::NexthopRewriteFilter(const A& local_nexthop,
					      bool directly_connected,
					      const IPNet<A>& subnet) 
    : _local_nexthop(local_nexthop),
      _directly_connected(directly_connected),
      _subnet(subnet)
{
}

template<class A>
bool
NexthopRewriteFilter<A>::filter(InternalMessage<A>& rtmsg) const
{

    // If the peer and the router are directly connected and the
    // NEXT_HOP is in the same network don't rewrite the
    // NEXT_HOP. This is known as a third party NEXT_HOP.
    if (_directly_connected && _subnet.contains(rtmsg.attributes()->nexthop())) {
	return true;
    }

    //Form a new path attribute list containing the new nexthop
    FPAListRef& palist = rtmsg.attributes();
    palist->replace_nexthop(_local_nexthop);
    

    //note that we changed the route
    rtmsg.set_changed();

    return true;
}

/*************************************************************************/

template<class A>
NexthopPeerCheckFilter<A>::NexthopPeerCheckFilter(const A& local_nexthop,
						  const A& peer_address) 
    : _local_nexthop(local_nexthop), _peer_address(peer_address)
{
}

template<class A>
bool
NexthopPeerCheckFilter<A>::filter(InternalMessage<A>& rtmsg) const
{
    // Only consider rewritting if this is a self originated route.
    if (! rtmsg.origin_peer()->originate_route_handler()) {
	return true;
    }

    // If the nexthop does not match the peer's address all if fine.
    if (rtmsg.attributes()->nexthop() != _peer_address) {
	return true;
    }

    // The nexthop matches the peer's address so rewrite it.
    FPAListRef& palist = rtmsg.attributes();
    palist->replace_nexthop(_local_nexthop);
    
    //note that we changed the route
    rtmsg.set_changed();

    return true;
}

/*************************************************************************/

template<class A>
IBGPLoopFilter<A>::IBGPLoopFilter() 
{
}

template<class A>
bool
IBGPLoopFilter<A>::filter(InternalMessage<A>& rtmsg) const 
{

    //If the route originated from a vanilla IBGP, then this filter
    //will drop the route.  This filter should only be plumbed on the
    //output branch to a vanilla IBGP peer.

    if (rtmsg.origin_peer()->get_peer_type() == PEER_TYPE_IBGP) {
	return false;
    }
    return true;
}

/*************************************************************************/

template<class A>
RRIBGPLoopFilter<A>::RRIBGPLoopFilter(bool rr_client, IPv4 bgp_id,
				      IPv4 cluster_id) 
    : _rr_client(rr_client), _bgp_id(bgp_id), _cluster_id(cluster_id)
{
}

template<class A>
bool
RRIBGPLoopFilter<A>::filter(InternalMessage<A>& rtmsg) const 
{
    // Only if this is *not* a route reflector client should the
    // packet be filtered. Note PEER_TYPE_IBGP_CLIENT is just passed
    // through.
    if (rtmsg.origin_peer()->get_peer_type() == PEER_TYPE_IBGP && 
	!_rr_client) {
	return false;
    }

    // If as ORIGINATOR_ID is not present add one.
    //Form a new path attribute list containing the new AS path
    FPAListRef& palist = rtmsg.attributes();
    if (0 == palist->originator_id()) {
	if (rtmsg.origin_peer()->get_peer_type() == PEER_TYPE_INTERNAL) {
	    OriginatorIDAttribute originator_id_att(_bgp_id);
	    palist->add_path_attribute(originator_id_att);
	} else {
	    OriginatorIDAttribute
		originator_id_att(rtmsg.origin_peer()->id());
	    palist->add_path_attribute(originator_id_att);
	}
    }

    // Prepend the CLUSTER_ID to the CLUSTER_LIST, if the CLUSTER_LIST
    // does not exist add one.
    const ClusterListAttribute *cla = palist->cluster_list();
    ClusterListAttribute *ncla = 0;
    if (0 == cla) {
	ncla = new ClusterListAttribute;
    } else {
	ncla = dynamic_cast<ClusterListAttribute *>(cla->clone());
	palist->remove_attribute_by_type(CLUSTER_LIST);
    }
    ncla->prepend_cluster_id(_cluster_id);
    palist->add_path_attribute(ncla);
    
    rtmsg.set_changed();
    debug_msg("Route with originator id and cluster list %s\n",
	      rtmsg.str().c_str());

    return true;
}

/*************************************************************************/

template<class A>
RRPurgeFilter<A>::RRPurgeFilter() 
{
}

template<class A>
bool
RRPurgeFilter<A>::filter(InternalMessage<A>& rtmsg) const 
{
    if (!rtmsg.attributes()->originator_id() &&
	!rtmsg.attributes()->cluster_list())
	return true;

    FPAListRef& palist = rtmsg.attributes();

    // If an ORIGINATOR_ID is present remove it.
    if (0 != palist->originator_id())
	palist->remove_attribute_by_type(ORIGINATOR_ID);

    // If a CLUSTER_LIST is present remove it.
    if (0 != palist->cluster_list())
	palist->remove_attribute_by_type(CLUSTER_LIST);

    
    rtmsg.set_changed();

    debug_msg("Route with originator id and cluster list %s\n",
	      rtmsg.str().c_str());

    return true;
}

/*************************************************************************/

template<class A>
LocalPrefInsertionFilter<A>::
LocalPrefInsertionFilter(uint32_t default_local_pref) 
    : _default_local_pref(default_local_pref)
{
}

template<class A>
bool
LocalPrefInsertionFilter<A>::filter(InternalMessage<A>& rtmsg) const
{
    debug_msg("local preference insertion filter\n");
    //Form a new path attribute list containing the new AS path
    FPAListRef& palist = rtmsg.attributes();
    LocalPrefAttribute local_pref_att(_default_local_pref);

    //Either a bad peer or running this filter multiple times mean
    //that local preference may already be present so remove it.
    palist->remove_attribute_by_type(LOCAL_PREF);

    palist->add_path_attribute(local_pref_att);
    
    rtmsg.set_changed();

    debug_msg("Route with local preference %s\n", rtmsg.str().c_str());

    return true;
}

/*************************************************************************/

template<class A>
LocalPrefRemovalFilter<A>::LocalPrefRemovalFilter() 
{
}

template<class A>
bool
LocalPrefRemovalFilter<A>::filter(InternalMessage<A>& rtmsg) const
{
    debug_msg("local preference removal filter\n");

    FPAListRef& palist = rtmsg.attributes();
    palist->remove_attribute_by_type(LOCAL_PREF);
    
    rtmsg.set_changed();

    return true;
}

/*************************************************************************/

template<class A>
MEDInsertionFilter<A>::
MEDInsertionFilter(NextHopResolver<A>& next_hop_resolver) 
    : _next_hop_resolver(next_hop_resolver)
{
}

template<class A>
bool
MEDInsertionFilter<A>::filter(InternalMessage<A>& rtmsg) const
{
    debug_msg("MED insertion filter\n");
    debug_msg("Route: %s\n", rtmsg.route()->str().c_str());

    //XXX theoretically unsafe test for debugging purposes
    XLOG_ASSERT(rtmsg.route()->igp_metric() != 0xffffffff);

    FPAListRef& palist = rtmsg.attributes();
    MEDAttribute med_att(rtmsg.route()->igp_metric());
    palist->add_path_attribute(med_att);
    
    //note that we changed the route
    rtmsg.set_changed();

    debug_msg("Route with local preference %s\n", rtmsg.str().c_str());

    return true;
}

/*************************************************************************/

template<class A>
MEDRemovalFilter<A>::MEDRemovalFilter() 
{
}

template<class A>
bool
MEDRemovalFilter<A>::filter(InternalMessage<A>& rtmsg) const
{
    debug_msg("MED removal filter\n");

    //Form a new path attribute list containing the new AS path
    FPAListRef& palist = rtmsg.attributes();
    palist->remove_attribute_by_type(MED);
    
    //note that we changed the route
    rtmsg.set_changed();

    return true;
}

/*************************************************************************/

template<class A>
KnownCommunityFilter<A>::KnownCommunityFilter(PeerType peer_type) 
    : _peer_type(peer_type)
{
}

template<class A>
bool
KnownCommunityFilter<A>::filter(InternalMessage<A>& rtmsg) const 
{
    const CommunityAttribute* ca = rtmsg.attributes()->community_att();
    if (ca == NULL)
	return true;

    // Routes with NO_ADVERTISE don't get sent to anyone else 
    if (ca->contains(CommunityAttribute::NO_ADVERTISE)) {
	return false;
    }

    if (_peer_type == PEER_TYPE_EBGP) {
	// Routes with NO_EXPORT don't get sent to EBGP peers
	if (ca->contains(CommunityAttribute::NO_EXPORT)) {
	    return false;
	}
    }
    
    if (_peer_type == PEER_TYPE_EBGP || _peer_type == PEER_TYPE_EBGP_CONFED) {
	// Routes with NO_EXPORT_SUBCONFED don't get sent to EBGP
	// peers or to other members ASes inside a confed 
	if (ca->contains(CommunityAttribute::NO_EXPORT_SUBCONFED)) {
	    return false;
	}
    }
    return true;
}

/*************************************************************************/

template<class A>
UnknownFilter<A>::UnknownFilter() 
{
}

template<class A>
bool
UnknownFilter<A>::filter(InternalMessage<A>& rtmsg) const
{
    debug_msg("Unknown filter\n");

    FPAListRef palist = rtmsg.attributes();
    palist->process_unknown_attributes();
    
    rtmsg.set_changed();

    return true;
}

/*************************************************************************/

template<class A>
OriginateRouteFilter<A>::OriginateRouteFilter(const AsNum &as_num,
					      PeerType peer_type)
    :  _as_num(as_num), _peer_type(peer_type)
{
}

template<class A>
bool
OriginateRouteFilter<A>::filter(InternalMessage<A>& rtmsg) const
{
    debug_msg("Originate Route Filter\n");

    // If we didn't originate this route forget it.
    if (!rtmsg.origin_peer()->originate_route_handler()) {
	return true;
    }

    return true;
}

/*************************************************************************/

template<class A>
FilterVersion<A>::FilterVersion(NextHopResolver<A>& next_hop_resolver)
    : _genid(0), _used(false), _ref_count(0), 
      _next_hop_resolver(next_hop_resolver)
{
}

template<class A>
FilterVersion<A>::~FilterVersion() {
    typename list <BGPRouteFilter<A> *>::iterator iter;
    iter = _filters.begin();
    while (iter != _filters.end()) {
	delete (*iter);
	++iter;
    }    
}

template<class A>
int
FilterVersion<A>::add_aggregation_filter(bool is_ibgp)
{
    AggregationFilter<A>* aggregation_filter;
    aggregation_filter = new AggregationFilter<A>(is_ibgp);
    _filters.push_back(aggregation_filter);
    return 0;
}

template<class A>
int
FilterVersion<A>::add_simple_AS_filter(const AsNum& as_num)
{
    SimpleASFilter<A>* AS_filter;
    AS_filter = new SimpleASFilter<A>(as_num);
    _filters.push_back(AS_filter);
    return 0;
}

template<class A>
int
FilterVersion<A>::add_route_reflector_input_filter(IPv4 bgp_id,
						   IPv4 cluster_id)
{
    RRInputFilter<A>* RR_input_filter;
    RR_input_filter = new RRInputFilter<A>(bgp_id, cluster_id);
    _filters.push_back(RR_input_filter);
    return 0;
}

template<class A>
int
FilterVersion<A>::add_AS_prepend_filter(const AsNum& as_num,
					bool is_confederation_peer)
{
    ASPrependFilter<A>* AS_prepender;
    AS_prepender = new ASPrependFilter<A>(as_num, is_confederation_peer);
    _filters.push_back(AS_prepender);
    return 0;
}

template<class A>
int
FilterVersion<A>::add_nexthop_rewrite_filter(const A& nexthop,
					     bool directly_connected,
					     const IPNet<A> &subnet)
{
    NexthopRewriteFilter<A>* nh_rewriter;
    nh_rewriter = new NexthopRewriteFilter<A>(nexthop, directly_connected,
					      subnet);
    _filters.push_back(nh_rewriter);
    return 0;
}

template<class A>
int
FilterVersion<A>::add_nexthop_peer_check_filter(const A& nexthop,
						const A& peer_address)
{
    NexthopPeerCheckFilter<A>* nh_peer_check;
    nh_peer_check = new NexthopPeerCheckFilter<A>(nexthop, peer_address);

    _filters.push_back(nh_peer_check);
    return 0;
}

template<class A>
int
FilterVersion<A>::add_ibgp_loop_filter()
{
    IBGPLoopFilter<A>* ibgp_filter;
    ibgp_filter = new IBGPLoopFilter<A>;
    _filters.push_back(ibgp_filter);
    return 0;
}

template<class A>
int
FilterVersion<A>::add_route_reflector_ibgp_loop_filter(bool client,
						       IPv4 bgp_id,
						       IPv4 cluster_id)
{
    RRIBGPLoopFilter<A>* rr_ibgp_filter;
    rr_ibgp_filter = new RRIBGPLoopFilter<A>(client, bgp_id, cluster_id);
    _filters.push_back(rr_ibgp_filter);
    return 0;
}

template<class A>
int
FilterVersion<A>::add_route_reflector_purge_filter()
{
    RRPurgeFilter<A>* rr_purge_filter;
    rr_purge_filter = new RRPurgeFilter<A>;
    _filters.push_back(rr_purge_filter);
    return 0;
}

template<class A>
int
FilterVersion<A>::add_localpref_insertion_filter(uint32_t default_local_pref)
{
    LocalPrefInsertionFilter<A> *localpref_filter;
    localpref_filter = new LocalPrefInsertionFilter<A>(default_local_pref);
    _filters.push_back(localpref_filter);
    return 0;
}

template<class A>
int
FilterVersion<A>::add_localpref_removal_filter()
{
    LocalPrefRemovalFilter<A> *localpref_filter;
    localpref_filter = new LocalPrefRemovalFilter<A>();
    _filters.push_back(localpref_filter);
    return 0;
}

template<class A>
int
FilterVersion<A>::add_med_insertion_filter()
{
    MEDInsertionFilter<A> *med_filter;
    med_filter = new MEDInsertionFilter<A>(_next_hop_resolver);
    _filters.push_back(med_filter);
    return 0;
}

template<class A>
int
FilterVersion<A>::add_med_removal_filter()
{
    MEDRemovalFilter<A> *med_filter;
    med_filter = new MEDRemovalFilter<A>();
    _filters.push_back(med_filter);
    return 0;
}

template<class A>
int
FilterVersion<A>::add_known_community_filter(PeerType peer_type)
{
    KnownCommunityFilter<A> *wkc_filter;
    wkc_filter = new KnownCommunityFilter<A>(peer_type);
    _filters.push_back(wkc_filter);
    return 0;
}

template<class A>
int
FilterVersion<A>::add_unknown_filter()
{
    UnknownFilter<A> *unknown_filter;
    unknown_filter = new UnknownFilter<A>();
    _filters.push_back(unknown_filter);
    return 0;
}

template<class A>
int
FilterVersion<A>::add_originate_route_filter(const AsNum &asn, 
					     PeerType peer_type)
{
    OriginateRouteFilter<A> *originate_route_filter;
    originate_route_filter = new OriginateRouteFilter<A>(asn, peer_type);
    _filters.push_back(originate_route_filter);
    return 0;
}


template<class A>
bool
FilterVersion<A>::apply_filters(InternalMessage<A>& rtmsg,
				int ref_change)
{
    bool filter_passed = true;
    _used = true;
    typename list <BGPRouteFilter<A> *>::const_iterator iter;
    iter = _filters.begin();
    while (iter != _filters.end()) {
	filter_passed = (*iter)->filter(rtmsg);
	if (filter_passed == false)
	    break;
	++iter;
    }
    _ref_count += ref_change;
    return filter_passed;
}

/*************************************************************************/

template<class A>
FilterTable<A>::FilterTable(string table_name,  
			    Safi safi,
			    BGPRouteTable<A> *parent_table,
			    NextHopResolver<A>& next_hop_resolver)
    : BGPRouteTable<A>("FilterTable-" + table_name, safi),
      _next_hop_resolver(next_hop_resolver), _do_versioning(false)
{
    this->_parent = parent_table;
    _current_filter = new FilterVersion<A>(_next_hop_resolver);
}

template<class A>
FilterTable<A>::~FilterTable() {
    set <FilterVersion<A>*> filters;
    typename map <uint32_t, FilterVersion<A>* >::iterator i;
    for (i = _filter_versions.begin(); i != _filter_versions.end(); i++) {
	filters.insert(i->second);
    }
    typename set <FilterVersion<A>* >::iterator j;
    for (j = filters.begin(); j != filters.end(); j++) {
	if ((*j) == _current_filter)
	    _current_filter = 0;
	delete (*j);
    }
    if (_current_filter)
	delete _current_filter;
}

template<class A>
void
FilterTable<A>::reconfigure_filter()
{
    // if the current filter has never been used, we can delete it now
    if (_current_filter->ref_count() == 0) {
	if (_current_filter->used()) {
	    _deleted_filters.insert(_current_filter->genid());
	    _filter_versions.erase(_current_filter->genid());
	}
	delete _current_filter;
    }

    _current_filter = new FilterVersion<A>(_next_hop_resolver);
}

template<class A>
int
FilterTable<A>::add_route(InternalMessage<A> &rtmsg, 
			  BGPRouteTable<A> *caller)
{
    debug_msg("\n         %s\n caller: %s\n rtmsg: %p genid: %d route: %p\n%s\n",
	      this->tablename().c_str(),
	      caller->tablename().c_str(),
	      &rtmsg,
	      rtmsg.genid(),
	      rtmsg.route(),
	      rtmsg.str().c_str());

    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(this->_next_table != NULL);

    XLOG_ASSERT(!rtmsg.copied());
    bool filtered_passed = apply_filters(rtmsg, 1);
    if (filtered_passed == false) {
	return ADD_FILTERED;
    }

    return this->_next_table->add_route(rtmsg, (BGPRouteTable<A>*)this);
}

template<class A>
int
FilterTable<A>::replace_route(InternalMessage<A> &old_rtmsg, 
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
    XLOG_ASSERT(this->_next_table != NULL);

    /* increment ref_count before decrementing it */
    bool new_passed_filter = apply_filters(new_rtmsg, 1);
    bool old_passed_filter = apply_filters(old_rtmsg, -1);

    int result;
    if (old_passed_filter == false && new_passed_filter == false) {
	//neither old nor new routes passed the filter
	result = ADD_FILTERED;

    } else if (old_passed_filter == true && new_passed_filter == false) {
	//the new route failed to pass the filter
	//the replace becomes a delete
	this->_next_table->delete_route(old_rtmsg,
					(BGPRouteTable<A>*)this); 
	result = ADD_FILTERED;

    } else if (old_passed_filter == false && new_passed_filter == true) {
	//the replace becomes an add
	result = this->_next_table->add_route(new_rtmsg,
					      (BGPRouteTable<A>*)this);
    } else {
	result = this->_next_table->replace_route(old_rtmsg, 
						  new_rtmsg,
						  (BGPRouteTable<A>*)this);
    }

    return result;
}

template<class A>
int
FilterTable<A>::route_dump(InternalMessage<A> &rtmsg, 
			   BGPRouteTable<A> *caller,
			   const PeerHandler *dump_peer)
{
    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(this->_next_table != NULL);

    bool filter_passed = apply_filters(rtmsg, 0);
    if (filter_passed == false)
	return ADD_FILTERED;

    int result;
    result = this->_next_table->route_dump(rtmsg, 
				     (BGPRouteTable<A>*)this, dump_peer);

    return result;
}

template<class A>
int
FilterTable<A>::delete_route(InternalMessage<A> &rtmsg, 
			     BGPRouteTable<A> *caller)
{
    debug_msg("\n         %s\n caller: %s\n rtmsg: %p route: %p\n%s\n",
	      this->tablename().c_str(),
	      caller->tablename().c_str(),
	      &rtmsg,
	      rtmsg.route(),
	      rtmsg.str().c_str());

    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(this->_next_table != NULL);

    bool filter_passed = apply_filters(rtmsg, -1);
    if (filter_passed == false)
	return 0;

    return this->_next_table->delete_route(rtmsg, 
				       (BGPRouteTable<A>*)this);
}

template<class A>
int
FilterTable<A>::push(BGPRouteTable<A> *caller)
{
    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(this->_next_table != NULL);
    return this->_next_table->push((BGPRouteTable<A>*)this);
}

template<class A>
const SubnetRoute<A>*
FilterTable<A>::lookup_route(const IPNet<A> &net,
			     uint32_t& genid,
			     FPAListRef& pa_list) const
{
    //We should never get called with a route that gets modified by
    //our filters, because there's no persistent storage to return as
    //the result.  But we can get called with a route that gets
    //dropped by our filters.
    const SubnetRoute<A> *found_route;
    uint32_t found_genid;
    found_route = this->_parent->lookup_route(net, found_genid, pa_list);

    if (found_route == NULL)
	return NULL;

    InternalMessage<A> msg(found_route, pa_list, NULL, found_genid);
    bool filter_passed = apply_filters(msg);
    
    if (filter_passed == false)
	return NULL;


    genid = found_genid;
    return found_route;
}

template<class A>
void
FilterTable<A>::route_used(const SubnetRoute<A>* rt, bool in_use)
{
    this->_parent->route_used(rt, in_use);
}

template<class A>
string
FilterTable<A>::str() const
{
    string s = "FilterTable<A>" + this->tablename();
    return s;
}


template<class A>
int
FilterTable<A>::add_aggregation_filter(bool is_ibgp)
{
    _current_filter->add_aggregation_filter(is_ibgp);
    return 0;
}

template<class A>
int
FilterTable<A>::add_simple_AS_filter(const AsNum& as_num)
{
    _current_filter->add_simple_AS_filter(as_num);
    return 0;
}

template<class A>
int
FilterTable<A>::add_route_reflector_input_filter(IPv4 bgp_id, IPv4 cluster_id)
{
    _current_filter->add_route_reflector_input_filter(bgp_id, cluster_id);
    return 0;
}

template<class A>
int
FilterTable<A>::add_AS_prepend_filter(const AsNum& as_num, 
				      bool is_confederation_peer)
{
    _current_filter->add_AS_prepend_filter(as_num, is_confederation_peer);
    return 0;
}

template<class A>
int
FilterTable<A>::add_nexthop_rewrite_filter(const A& nexthop,
					   bool directly_connected,
					   const IPNet<A> &subnet)
{
    _current_filter->add_nexthop_rewrite_filter(nexthop, directly_connected,
						subnet);
    return 0;
}

template<class A>
int
FilterTable<A>::add_nexthop_peer_check_filter(const A& nexthop,
					      const A& peer_address)
{
    _current_filter->add_nexthop_peer_check_filter(nexthop, peer_address);

    return 0;
}

template<class A>
int
FilterTable<A>::add_ibgp_loop_filter()
{
    _current_filter->add_ibgp_loop_filter();
    return 0;
}

template<class A>
int
FilterTable<A>::add_route_reflector_ibgp_loop_filter(bool client,
						     IPv4 bgp_id,
						     IPv4 cluster_id)
{
    _current_filter->add_route_reflector_ibgp_loop_filter(client,
							  bgp_id,
							  cluster_id);
    return 0;
}

template<class A>
int
FilterTable<A>::add_route_reflector_purge_filter()
{
    _current_filter->add_route_reflector_purge_filter();
    return 0;
}

template<class A>
int
FilterTable<A>::add_localpref_insertion_filter(uint32_t default_local_pref)
{
    _current_filter->add_localpref_insertion_filter(default_local_pref);
    return 0;
}

template<class A>
int
FilterTable<A>::add_localpref_removal_filter()
{
    _current_filter->add_localpref_removal_filter();
    return 0;
}

template<class A>
int
FilterTable<A>::add_med_insertion_filter()
{
    _current_filter->add_med_insertion_filter();
    return 0;
}

template<class A>
int
FilterTable<A>::add_known_community_filter(PeerType peer_type)
{
    _current_filter->add_known_community_filter(peer_type);
    return 0;
}

template<class A>
int
FilterTable<A>::add_med_removal_filter()
{
    _current_filter->add_med_removal_filter();
    return 0;
}

template<class A>
int
FilterTable<A>::add_unknown_filter()
{
    _current_filter->add_unknown_filter();
    return 0;
}

template<class A>
int
FilterTable<A>::add_originate_route_filter(const AsNum &asn, 
					   PeerType peer_type)
{
    _current_filter->add_originate_route_filter(asn, peer_type);
    return 0;
}

template<class A>
bool
FilterTable<A>::apply_filters(InternalMessage<A>& rtmsg) const
{
    // in this case, this is actually const, so this hack is safe
    return const_cast<FilterTable<A>*>(this)->apply_filters(rtmsg, 0);
}

template<class A>
bool
FilterTable<A>::apply_filters(InternalMessage<A>& rtmsg,
			      int ref_change)
{
    bool filter_passed = true;
    FilterVersion<A> *filter;
    if (_do_versioning) {
	typename map<uint32_t, FilterVersion<A>* >::iterator i;
	uint32_t genid = rtmsg.genid();
	i = _filter_versions.find(genid);
	if (i == _filter_versions.end()) {
	    // check we're not trying to use a GenID that has been retired.
	    XLOG_ASSERT(_deleted_filters.find(genid) == _deleted_filters.end());

	    _filter_versions[genid] = _current_filter;
	    _current_filter->set_genid(genid);
	    filter = _current_filter;
	} else {
	    filter = i->second;
	    XLOG_ASSERT(filter->genid() == genid);
	}

	filter_passed = filter->apply_filters(rtmsg, ref_change);

	// if there are no more routes that used an old filter, delete it now
	if (filter->ref_count() == 0) {
	    if (filter != _current_filter) {
		if (filter->used())
		    _deleted_filters.insert(filter->genid());
		delete filter;
		_filter_versions.erase(i);
	    }
	}
    } else {
	filter_passed = _current_filter->apply_filters(rtmsg, ref_change);
    }

    if (filter_passed == false)
	drop_message(&rtmsg);
    
    return filter_passed;
}

template<class A>
void
FilterTable<A>::drop_message(const InternalMessage<A> *rtmsg) const
{
    if (rtmsg->copied()) {
	//It's the responsibility of the final recipient of a
	//copied route to store it or free it.
	rtmsg->route()->unref();
    }
#if 0
    if (modified) {
	//This filterbank created this message.  We need to delete
	//it because no-one else can.
	delete rtmsg;
    }
#endif
}


template<class A>
bool 
FilterTable<A>::get_next_message(BGPRouteTable<A> *next_table)
{
    BGPRouteTable<A>* parent = this->_parent;

    XLOG_ASSERT(parent);
    XLOG_ASSERT(this->_next_table == next_table);

    return parent->get_next_message(this);
}

template class FilterTable<IPv4>;
template class FilterTable<IPv6>;




