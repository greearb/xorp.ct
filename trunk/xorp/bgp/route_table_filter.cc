// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/bgp/route_table_filter.cc,v 1.55 2008/07/23 05:09:36 pavlin Exp $"

//#define DEBUG_LOGGING
//#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "route_table_filter.hh"
#include "peer_handler.hh"
#include "bgp.hh"

/*************************************************************************/

template<class A>
void
BGPRouteFilter<A>::drop_message(const InternalMessage<A> *rtmsg,
				bool &modified) const
{
    if (rtmsg->changed()) {
	//It's the responsibility of the final recipient of a
	//changed route to store it or free it.
	rtmsg->route()->unref();
    }
    if (modified) {
	//This filterbank created this message.  We need to delete
	//it because no-one else can.
	delete rtmsg;
    }
}

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

/*************************************************************************/

template<class A>
AggregationFilter<A>::AggregationFilter(bool is_ibgp) 
    : _is_ibgp(is_ibgp)
{
}

template<class A>
const InternalMessage<A>* 
AggregationFilter<A>::filter(const InternalMessage<A> *rtmsg, 
			  bool &modified) const 
{
    uint8_t aggr_tag = rtmsg->route()->aggr_prefix_len();

    if (aggr_tag == SR_AGGR_IGNORE) {
	// Route was not even marked for aggregation
	return rtmsg;
    }

    // Has our AggregationTable properly marked the route?
    XLOG_ASSERT(aggr_tag >= SR_AGGR_EBGP_AGGREGATE);

    if (_is_ibgp) {
	// Peering is IBGP
	if (aggr_tag == SR_AGGR_IBGP_ONLY) {
	    return rtmsg;
	} else {
	    drop_message(rtmsg, modified);
	    return NULL;
	}
    } else {
	// Peering is EBGP
	if (aggr_tag != SR_AGGR_IBGP_ONLY) {
	    // EBGP_AGGREGATE | EBGP_NOT_AGGREGATED | EBGP_WAS_AGGREGATED
	    return rtmsg;
	} else {
	    drop_message(rtmsg, modified);
	    return NULL;
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
const InternalMessage<A>* 
SimpleASFilter<A>::filter(const InternalMessage<A> *rtmsg, 
			  bool &modified) const 
{
    const PathAttributeList<A> *attributes = rtmsg->route()->attributes();
    const ASPath &as_path = attributes->aspath();
    debug_msg("Filter: AS_Path filter for >%s< checking >%s<\n",
	   _as_num.str().c_str(), as_path.str().c_str());
    if (as_path.contains(_as_num)) {
	drop_message(rtmsg, modified);
	return NULL;
    }
    return rtmsg;
}

/*************************************************************************/

template<class A>
RRInputFilter<A>::RRInputFilter(IPv4 bgp_id, IPv4 cluster_id)
    : _bgp_id(bgp_id), _cluster_id(cluster_id)
{
}

template<class A>
const InternalMessage<A>* 
RRInputFilter<A>::filter(const InternalMessage<A> *rtmsg, 
			 bool &modified) const 
{
    const PathAttributeList<A> *attributes = rtmsg->route()->attributes();
    const OriginatorIDAttribute *oid = attributes->originator_id();
    if (0 != oid && oid->originator_id() == _bgp_id) {
	drop_message(rtmsg, modified);
	return NULL;
    }
    const ClusterListAttribute *cl = attributes->cluster_list();
    if (0 != cl && cl->contains(_cluster_id)) {
	drop_message(rtmsg, modified);
	return NULL;
    }

    return rtmsg;
}

/*************************************************************************/

template<class A>
ASPrependFilter<A>::ASPrependFilter(const AsNum &as_num, 
				    bool is_confederation_peer) 
    : _as_num(as_num), _is_confederation_peer(is_confederation_peer)
{
}

template<class A>
const InternalMessage<A>* 
ASPrependFilter<A>::filter(const InternalMessage<A> *rtmsg,
			   bool &modified) const
{
    //Create a new AS path with our AS number prepended to it.
    ASPath new_as_path(rtmsg->route()->attributes()->aspath());

    if (_is_confederation_peer) { 
	new_as_path.prepend_confed_as(_as_num);
    } else {
	new_as_path.remove_confed_segments();
	new_as_path.prepend_as(_as_num);
    } 

    //Form a new path attribute list containing the new AS path
    PathAttributeList<A> palist(*(rtmsg->route()->attributes()));
    palist.replace_AS_path(new_as_path);
    palist.rehash();
    
    //Create a new route message with the new path attribute list
    SubnetRoute<A> *new_route 
	= new SubnetRoute<A>(rtmsg->net(), &palist, 
			     rtmsg->route()->original_route(), 
			     rtmsg->route()->igp_metric());

    // policy needs this
    propagate_flags(*(rtmsg->route()),*new_route);
    
    InternalMessage<A> *new_rtmsg = 
	new InternalMessage<A>(new_route, rtmsg->origin_peer(), 
			       rtmsg->genid());

    propagate_flags(rtmsg, new_rtmsg);
    
    //drop and free the old message
    drop_message(rtmsg, modified);

    //note that we changed the route
    modified = true;
    new_rtmsg->set_changed();

    return new_rtmsg;
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
const InternalMessage<A>* 
NexthopRewriteFilter<A>::filter(const InternalMessage<A> *rtmsg,
				bool &modified) const
{
#if	0
    // If we originated this route don't rewrite the nexthop
    // Locally originated aggregates are exception, they need NH rewrite
    if (rtmsg->origin_peer()->originate_route_handler() &&
	rtmsg->route()->aggr_prefix_len() != SR_AGGR_EBGP_AGGREGATE)
	return rtmsg;
#endif

    // If the peer and the router are directly connected and the
    // NEXT_HOP is in the same network don't rewrite the
    // NEXT_HOP. This is known as a third party NEXT_HOP.
    if (_directly_connected && _subnet.contains(rtmsg->route()->nexthop())) {
	return rtmsg;
    }

    //Form a new path attribute list containing the new nexthop
    PathAttributeList<A> palist(*(rtmsg->route()->attributes()));
    palist.replace_nexthop(_local_nexthop);
    palist.rehash();
    
    //Create a new route message with the new path attribute list
    SubnetRoute<A> *new_route 
	= new SubnetRoute<A>(rtmsg->net(), &palist,
			     rtmsg->route()->original_route(), 
			     rtmsg->route()->igp_metric());
    
    // policy needs this
    propagate_flags(*(rtmsg->route()),*new_route);

    debug_msg("NexthopRewriteFilter: new route: %p with attributes %p\n",
	      new_route, new_route->attributes());
    InternalMessage<A> *new_rtmsg = 
	new InternalMessage<A>(new_route, rtmsg->origin_peer(), 
			       rtmsg->genid());

    propagate_flags(rtmsg, new_rtmsg);

    //drop and free the old message
    drop_message(rtmsg, modified);

    //note that we changed the route
    modified = true;
    new_rtmsg->set_changed();

    return new_rtmsg;
}

/*************************************************************************/

template<class A>
NexthopPeerCheckFilter<A>::NexthopPeerCheckFilter(const A& local_nexthop,
						  const A& peer_address) 
    : _local_nexthop(local_nexthop), _peer_address(peer_address)
{
}

template<class A>
const InternalMessage<A>* 
NexthopPeerCheckFilter<A>::filter(const InternalMessage<A> *rtmsg,
				  bool &modified) const
{
    // Only consider rewritting if this is a self originated route.
    if (! rtmsg->origin_peer()->originate_route_handler()) {
	return rtmsg;
    }

    // If the nexthop does not match the peer's address all if fine.
    if (rtmsg->route()->nexthop() != _peer_address) {
	return rtmsg;
    }

    // The nexthop matches the peer's address so rewrite it.

    //Form a new path attribute list containing the new nexthop
    PathAttributeList<A> palist(*(rtmsg->route()->attributes()));
    palist.replace_nexthop(_local_nexthop);
    palist.rehash();
    
    //Create a new route message with the new path attribute list
    SubnetRoute<A> *new_route 
	= new SubnetRoute<A>(rtmsg->net(), &palist,
			     rtmsg->route()->original_route(), 
			     rtmsg->route()->igp_metric());
    
    // policy needs this
    propagate_flags(*(rtmsg->route()),*new_route);

    debug_msg("NexthopRewriteFilter: new route: %p with attributes %p\n",
	      new_route, new_route->attributes());
    InternalMessage<A> *new_rtmsg = 
	new InternalMessage<A>(new_route, rtmsg->origin_peer(), 
			       rtmsg->genid());

    propagate_flags(rtmsg, new_rtmsg);

    //drop and free the old message
    drop_message(rtmsg, modified);

    //note that we changed the route
    modified = true;
    new_rtmsg->set_changed();

    return new_rtmsg;
}

/*************************************************************************/

template<class A>
IBGPLoopFilter<A>::IBGPLoopFilter() 
{
}

template<class A>
const InternalMessage<A>* 
IBGPLoopFilter<A>::filter(const InternalMessage<A> *rtmsg,
			  bool &modified) const 
{

    //If the route originated from a vanilla IBGP, then this filter
    //will drop the route.  This filter should only be plumbed on the
    //output branch to a vanilla IBGP peer.

    if (rtmsg->origin_peer()->get_peer_type() == PEER_TYPE_IBGP) {
	drop_message(rtmsg, modified);
	return NULL;
    }
    return rtmsg;
}

/*************************************************************************/

template<class A>
RRIBGPLoopFilter<A>::RRIBGPLoopFilter(bool rr_client, IPv4 bgp_id,
				      IPv4 cluster_id) 
    : _rr_client(rr_client), _bgp_id(bgp_id), _cluster_id(cluster_id)
{
}

template<class A>
const InternalMessage<A>* 
RRIBGPLoopFilter<A>::filter(const InternalMessage<A> *rtmsg,
			  bool &modified) const 
{
    // Only if this is *not* a route reflector client should the
    // packet be filtered. Note PEER_TYPE_IBGP_CLIENT is just passed
    // through.
    if (rtmsg->origin_peer()->get_peer_type() == PEER_TYPE_IBGP && 
	!_rr_client) {
	drop_message(rtmsg, modified);
	return NULL;
    }

    // If as ORIGINATOR_ID is not present add one.
    //Form a new path attribute list containing the new AS path
    PathAttributeList<A> palist(*(rtmsg->route()->attributes()));
    if (0 == palist.originator_id()) {
	if (rtmsg->origin_peer()->get_peer_type() == PEER_TYPE_INTERNAL) {
	    OriginatorIDAttribute originator_id_att(_bgp_id);
	    palist.add_path_attribute(originator_id_att);
	} else {
	    OriginatorIDAttribute
		originator_id_att(rtmsg->origin_peer()->id());
	    palist.add_path_attribute(originator_id_att);
	}
    }

    // Prepend the CLUSTER_ID to the CLUSTER_LIST, if the CLUSTER_LIST
    // does not exist add one.
    const ClusterListAttribute *cla = palist.cluster_list();
    ClusterListAttribute *ncla = 0;
    if (0 == cla) {
	ncla = new ClusterListAttribute;
    } else {
	ncla = dynamic_cast<ClusterListAttribute *>(cla->clone());
	palist.remove_attribute_by_type(CLUSTER_LIST);
    }
    ncla->prepend_cluster_id(_cluster_id);
    palist.add_path_attribute(ncla);
    palist.rehash();
    
    //Create a new route message with the new path attribute list
    SubnetRoute<A> *new_route 
	= new SubnetRoute<A>(rtmsg->net(), &palist,
			     rtmsg->route()->original_route(), 
			     rtmsg->route()->igp_metric());

    // policy needs this
    propagate_flags(*(rtmsg->route()),*new_route);

    InternalMessage<A> *new_rtmsg = 
	new InternalMessage<A>(new_route, rtmsg->origin_peer(), 
			       rtmsg->genid());

    propagate_flags(rtmsg, new_rtmsg);
    
    //drop and free the old message
    drop_message(rtmsg, modified);

    //note that we changed the route
    modified = true;
    new_rtmsg->set_changed();

    XLOG_ASSERT(new_rtmsg->changed());

    debug_msg("Route with originator id and cluster list %s\n",
	      new_rtmsg->str().c_str());

    return new_rtmsg;
}

/*************************************************************************/

template<class A>
RRPurgeFilter<A>::RRPurgeFilter() 
{
}

template<class A>
const InternalMessage<A>* 
RRPurgeFilter<A>::filter(const InternalMessage<A> *rtmsg,
			 bool &modified) const 
{
    if (!rtmsg->route()->attributes()->originator_id() &&
	!rtmsg->route()->attributes()->cluster_list())
	return rtmsg;

    //Form a new path attribute list.
    PathAttributeList<A> palist(*(rtmsg->route()->attributes()));

    // If an ORIGINATOR_ID is present remove it.
    if (0 != palist.originator_id())
	palist.remove_attribute_by_type(ORIGINATOR_ID);

    // If a CLUSTER_LIST is present remove it.
    if (0 != palist.cluster_list())
	palist.remove_attribute_by_type(CLUSTER_LIST);

    palist.rehash();
    
    //Create a new route message with the new path attribute list
    SubnetRoute<A> *new_route 
	= new SubnetRoute<A>(rtmsg->net(), &palist,
			     rtmsg->route()->original_route(), 
			     rtmsg->route()->igp_metric());

    // policy needs this
    propagate_flags(*(rtmsg->route()),*new_route);

    InternalMessage<A> *new_rtmsg = 
	new InternalMessage<A>(new_route, rtmsg->origin_peer(), 
			       rtmsg->genid());

    propagate_flags(rtmsg, new_rtmsg);
    
    //drop and free the old message
    drop_message(rtmsg, modified);

    //note that we changed the route
    modified = true;
    new_rtmsg->set_changed();

    XLOG_ASSERT(new_rtmsg->changed());

    debug_msg("Route with originator id and cluster list %s\n",
	      new_rtmsg->str().c_str());

    return new_rtmsg;
}

/*************************************************************************/

template<class A>
LocalPrefInsertionFilter<A>::
LocalPrefInsertionFilter(uint32_t default_local_pref) 
    : _default_local_pref(default_local_pref)
{
}

template<class A>
const InternalMessage<A>* 
LocalPrefInsertionFilter<A>::filter(const InternalMessage<A> *rtmsg,
			   bool &modified) const
{
    debug_msg("local preference insertion filter\n");
    //Form a new path attribute list containing the new AS path
    PathAttributeList<A> palist(*(rtmsg->route()->attributes()));
    LocalPrefAttribute local_pref_att(_default_local_pref);

    //Either a bad peer or running this filter multiple times mean
    //that local preference may already be present so remove it.
    palist.remove_attribute_by_type(LOCAL_PREF);

    palist.add_path_attribute(local_pref_att);
    palist.rehash();
    
    //Create a new route message with the new path attribute list
    SubnetRoute<A> *new_route 
	= new SubnetRoute<A>(rtmsg->net(), &palist,
			     rtmsg->route()->original_route(), 
			     rtmsg->route()->igp_metric());

    // policy needs this
    propagate_flags(*(rtmsg->route()),*new_route);

    InternalMessage<A> *new_rtmsg = 
	new InternalMessage<A>(new_route, rtmsg->origin_peer(), 
			       rtmsg->genid());

    propagate_flags(rtmsg, new_rtmsg);
    
    //drop and free the old message
    drop_message(rtmsg, modified);

    //note that we changed the route
    modified = true;
    new_rtmsg->set_changed();

    XLOG_ASSERT(new_rtmsg->changed());

    debug_msg("Route with local preference %s\n", new_rtmsg->str().c_str());

    return new_rtmsg;
}

/*************************************************************************/

template<class A>
LocalPrefRemovalFilter<A>::LocalPrefRemovalFilter() 
{
}

template<class A>
const InternalMessage<A>* 
LocalPrefRemovalFilter<A>::filter(const InternalMessage<A> *rtmsg,
				 bool &modified) const
{
    debug_msg("local preference removal filter\n");

    //Form a new path attribute list containing the new AS path
    PathAttributeList<A> palist(*(rtmsg->route()->attributes()));
    palist.remove_attribute_by_type(LOCAL_PREF);
    palist.rehash();
    
    //Create a new route message with the new path attribute list
    SubnetRoute<A> *new_route 
	= new SubnetRoute<A>(rtmsg->net(), &palist,
			     rtmsg->route()->original_route(), 
			     rtmsg->route()->igp_metric());

    // policy needs this
    propagate_flags(*(rtmsg->route()),*new_route);

    InternalMessage<A> *new_rtmsg = 
	new InternalMessage<A>(new_route, rtmsg->origin_peer(), 
			       rtmsg->genid());

    propagate_flags(rtmsg, new_rtmsg);
    
    //drop and free the old message
    drop_message(rtmsg, modified);

    //note that we changed the route
    modified = true;
    new_rtmsg->set_changed();

    return new_rtmsg;
}

/*************************************************************************/

template<class A>
MEDInsertionFilter<A>::
MEDInsertionFilter(NextHopResolver<A>& next_hop_resolver) 
    : _next_hop_resolver(next_hop_resolver)
{
}

template<class A>
const InternalMessage<A>* 
MEDInsertionFilter<A>::filter(const InternalMessage<A> *rtmsg,
			      bool &modified) const
{
    debug_msg("MED insertion filter\n");
    debug_msg("Route: %s\n", rtmsg->route()->str().c_str());

    //XXX theoretically unsafe test for debugging purposes
    XLOG_ASSERT(rtmsg->route()->igp_metric() != 0xffffffff);

    //Form a new path attribute list containing the new MED attribute
    PathAttributeList<A> palist(*(rtmsg->route()->attributes()));
    MEDAttribute med_att(rtmsg->route()->igp_metric());
    palist.add_path_attribute(med_att);
    palist.rehash();
    
    //Create a new route message with the new path attribute list
    SubnetRoute<A> *new_route 
	= new SubnetRoute<A>(rtmsg->net(), &palist,
			     rtmsg->route()->original_route(), 
			     rtmsg->route()->igp_metric());
    
    // policy needs this
    propagate_flags(*(rtmsg->route()),*new_route);

    InternalMessage<A> *new_rtmsg = 
	new InternalMessage<A>(new_route, rtmsg->origin_peer(), 
			       rtmsg->genid());

    propagate_flags(rtmsg, new_rtmsg);
    
    //drop and free the old message
    drop_message(rtmsg, modified);

    //note that we changed the route
    modified = true;
    new_rtmsg->set_changed();

    XLOG_ASSERT(new_rtmsg->changed());

    debug_msg("Route with local preference %s\n", new_rtmsg->str().c_str());

    return new_rtmsg;
}

/*************************************************************************/

template<class A>
MEDRemovalFilter<A>::MEDRemovalFilter() 
{
}

template<class A>
const InternalMessage<A>* 
MEDRemovalFilter<A>::filter(const InternalMessage<A> *rtmsg,
				 bool &modified) const
{
    debug_msg("MED removal filter\n");

    //Form a new path attribute list containing the new AS path
    PathAttributeList<A> palist(*(rtmsg->route()->attributes()));
    palist.remove_attribute_by_type(MED);
    palist.rehash();
    
    //Create a new route message with the new path attribute list
    SubnetRoute<A> *new_route 
	= new SubnetRoute<A>(rtmsg->net(), &palist, 
			     rtmsg->route()->original_route(), 
			     rtmsg->route()->igp_metric());

    // policy needs this
    propagate_flags(*(rtmsg->route()),*new_route);
    
    InternalMessage<A> *new_rtmsg = 
	new InternalMessage<A>(new_route, rtmsg->origin_peer(), 
			       rtmsg->genid());

    propagate_flags(rtmsg, new_rtmsg);
    
    //drop and free the old message
    drop_message(rtmsg, modified);

    //note that we changed the route
    modified = true;
    new_rtmsg->set_changed();

    return new_rtmsg;
}

/*************************************************************************/

template<class A>
KnownCommunityFilter<A>::KnownCommunityFilter(PeerType peer_type) 
    : _peer_type(peer_type)
{
}

template<class A>
const InternalMessage<A>* 
KnownCommunityFilter<A>::filter(const InternalMessage<A> *rtmsg,
				bool &modified) const 
{
    const CommunityAttribute* ca = rtmsg->route()->attributes()
	->community_att();
    if (ca == NULL)
	return rtmsg;

    // Routes with NO_ADVERTISE don't get sent to anyone else 
    if (ca->contains(CommunityAttribute::NO_ADVERTISE)) {
	drop_message(rtmsg, modified);
	return NULL;
    }

    if (_peer_type == PEER_TYPE_EBGP) {
	// Routes with NO_EXPORT don't get sent to EBGP peers
	if (ca->contains(CommunityAttribute::NO_EXPORT)) {
	    drop_message(rtmsg, modified);
	    return NULL;
	}
    }
    
    if (_peer_type == PEER_TYPE_EBGP || _peer_type == PEER_TYPE_EBGP_CONFED) {
	// Routes with NO_EXPORT_SUBCONFED don't get sent to EBGP
	// peers or to other members ASes inside a confed 
	if (ca->contains(CommunityAttribute::NO_EXPORT_SUBCONFED)) {
	    drop_message(rtmsg, modified);
	    return NULL;
	}
    }
    return rtmsg;
}

/*************************************************************************/

template<class A>
UnknownFilter<A>::UnknownFilter() 
{
}

template<class A>
const InternalMessage<A>* 
UnknownFilter<A>::filter(const InternalMessage<A> *rtmsg,
				 bool &modified) const
{
    debug_msg("Unknown filter\n");

    //Form a new path attribute list containing the new AS path
    PathAttributeList<A> palist(*(rtmsg->route()->attributes()));
    palist.process_unknown_attributes();
    palist.rehash();
    
    //Create a new route message with the new path attribute list
    SubnetRoute<A> *new_route 
	= new SubnetRoute<A>(rtmsg->net(), &palist, 
			     rtmsg->route()->original_route(), 
			     rtmsg->route()->igp_metric());

    debug_msg("PA list from new route: %s\n", new_route->attributes()->str().c_str());

    // policy needs this
    propagate_flags(*(rtmsg->route()),*new_route);
    
    InternalMessage<A> *new_rtmsg = 
	new InternalMessage<A>(new_route, rtmsg->origin_peer(), 
			       rtmsg->genid());

    propagate_flags(rtmsg, new_rtmsg);
    
    //drop and free the old message
    drop_message(rtmsg, modified);

    //note that we changed the route
    modified = true;
    new_rtmsg->set_changed();

    return new_rtmsg;
}

/*************************************************************************/

template<class A>
OriginateRouteFilter<A>::OriginateRouteFilter(const AsNum &as_num,
					      PeerType peer_type)
    :  _as_num(as_num), _peer_type(peer_type)
{
}

template<class A>
const InternalMessage<A>* 
OriginateRouteFilter<A>::filter(const InternalMessage<A> *rtmsg,
				bool &/*modified*/) const
{
    debug_msg("Originate Route Filter\n");

    // If we didn't originate this route forget it.
    if (!rtmsg->origin_peer()->originate_route_handler())
	return rtmsg;

    return rtmsg;
#if	0
    // If this is an EBGP peering then assume the AS is already
    // present. Perhaps we should check.
    if (peer_type == PEER_TYPE_EBGP || peer_type == PEER_TYPE_EBGP_CONFED)
	return rtmsg;

    //Create a new AS path with our AS number prepended to it.
    ASPath new_as_path(rtmsg->route()->attributes()->aspath());
    new_as_path.prepend_as(_as_num);

    //Form a new path attribute list containing the new AS path
    PathAttributeList<A> palist(*(rtmsg->route()->attributes()));
    palist.replace_AS_path(new_as_path);
    palist.rehash();
    
    //Create a new route message with the new path attribute list
    SubnetRoute<A> *new_route 
	= new SubnetRoute<A>(rtmsg->net(), &palist, 
			     rtmsg->route()->original_route(), 
			     rtmsg->route()->igp_metric());
    
    // policy needs this
    propagate_flags(*(rtmsg->route()),*new_route);
    
    
    InternalMessage<A> *new_rtmsg = 
	new InternalMessage<A>(new_route, rtmsg->origin_peer(), 
			       rtmsg->genid());

    propagate_flags(rtmsg, new_rtmsg);
    
    //drop and free the old message
    drop_message(rtmsg, modified);

    //note that we changed the route
    modified = true;
    new_rtmsg->set_changed();

    return new_rtmsg;
#endif
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
const InternalMessage<A> *
FilterVersion<A>::apply_filters(const InternalMessage<A> *rtmsg,
				int ref_change)
{
    const InternalMessage<A> *filtered_msg = rtmsg;
    bool modified_by_us = false;
    _used = true;
    typename list <BGPRouteFilter<A> *>::const_iterator iter;
    iter = _filters.begin();
    while (iter != _filters.end()) {
	filtered_msg = (*iter)->filter(filtered_msg, modified_by_us);
	if (filtered_msg == NULL)
	    break;
	debug_msg("filtered %p %d rtmsg %p %d %d\n", filtered_msg,
		  filtered_msg->changed(),
		  rtmsg,
		  rtmsg->changed(),
		  filtered_msg == rtmsg);
	if (filtered_msg == rtmsg) {
	    debug_msg("same\n");
	    XLOG_ASSERT(!filtered_msg->changed());
	} else {
	    debug_msg("different\n");
	    XLOG_ASSERT(filtered_msg->changed());
	}
	//Check the filters correctly preserved the parent route
	XLOG_ASSERT(filtered_msg->route()->original_route() 
		    == rtmsg->route()->original_route());
	++iter;
    }
    _ref_count += ref_change;
    return filtered_msg;
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
	delete (*j);
    }
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
FilterTable<A>::add_route(const InternalMessage<A> &rtmsg, 
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

    XLOG_ASSERT(!rtmsg.changed());
    const InternalMessage<A> *filtered_msg = apply_filters(&rtmsg, 1);
    if (filtered_msg == NULL)
	return ADD_FILTERED;

    if (filtered_msg == &rtmsg) {
	XLOG_ASSERT(!filtered_msg->changed());	
    } else {
	XLOG_ASSERT(filtered_msg->changed());
    }

    //as the filter doesn't store a copy, it should return the return
    //value from downstream.  If we modify a route, a later cache
    //should return ADD_UNUSED, so there's no need for us to
    //explicitly force this.
    int result;
    result = this->_next_table->add_route(*filtered_msg, (BGPRouteTable<A>*)this);

    if (filtered_msg != &rtmsg) {
	//We created a modified message, so now we need to free it.
	//Don't delete the route, as it will be stored by the recipient
	delete filtered_msg;
    }
    return result;
}

template<class A>
int
FilterTable<A>::replace_route(const InternalMessage<A> &old_rtmsg, 
			      const InternalMessage<A> &new_rtmsg, 
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
    const InternalMessage<A> *filtered_new_msg = apply_filters(&new_rtmsg, 1);
    const InternalMessage<A> *filtered_old_msg = apply_filters(&old_rtmsg, -1);

    int result;
    if (filtered_old_msg == NULL && filtered_new_msg == NULL) {
	//neither old nor new routes passed the filter
	result = ADD_FILTERED;

    } else if (filtered_old_msg != NULL && filtered_new_msg == NULL) {
	//the new route failed to pass the filter
	//the replace becomes a delete
	this->_next_table->delete_route(*filtered_old_msg,
				  (BGPRouteTable<A>*)this); 
	result = ADD_FILTERED;

    } else if (filtered_old_msg == NULL && filtered_new_msg != NULL) {
	//the replace becomes an add
	result = this->_next_table->add_route(*filtered_new_msg,
					(BGPRouteTable<A>*)this);
    } else {
	result = this->_next_table->replace_route(*filtered_old_msg, 
					    *filtered_new_msg,
					    (BGPRouteTable<A>*)this);
    }

    if (filtered_old_msg != NULL && filtered_old_msg != &old_rtmsg) {
	//We created a modified message, so now we need to free it.
	delete filtered_old_msg;
    }

    if (filtered_new_msg != NULL && filtered_new_msg != &new_rtmsg) {
	//We created a modified message, so now we need to free it.
	delete filtered_new_msg;
    }

    return result;
}

template<class A>
int
FilterTable<A>::route_dump(const InternalMessage<A> &rtmsg, 
			   BGPRouteTable<A> *caller,
			   const PeerHandler *dump_peer)
{
    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(this->_next_table != NULL);

    const InternalMessage<A> *filtered_msg = apply_filters(&rtmsg, 0);
    if (filtered_msg == NULL)
	return ADD_FILTERED;

    int result;
    result = this->_next_table->route_dump(*filtered_msg, 
				     (BGPRouteTable<A>*)this, dump_peer);

    if (filtered_msg != &rtmsg) {
	//We created a modified message, so now we need to free it.
	delete filtered_msg;
    }
    return result;
}

template<class A>
int
FilterTable<A>::delete_route(const InternalMessage<A> &rtmsg, 
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

    const InternalMessage<A> *filtered_msg = apply_filters(&rtmsg, -1);
    if (filtered_msg == NULL)
	return 0;

    if (filtered_msg == &rtmsg) {
	XLOG_ASSERT(!filtered_msg->changed());	
    } else {
	XLOG_ASSERT(filtered_msg->changed());
    }

    int result;
    result = this->_next_table->delete_route(*filtered_msg, 
				       (BGPRouteTable<A>*)this);
    if (filtered_msg != &rtmsg) {
	//We created a modified message, so now we need to free it.
	delete filtered_msg;
    }
    return result;
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
			     uint32_t& genid) const
{
    //We should never get called with a route that gets modified by
    //our filters, because there's no persistent storage to return as
    //the result.  But we can get called with a route that gets
    //dropped by our filters.
    const SubnetRoute<A> *found_route;
    uint32_t found_genid;
    found_route = this->_parent->lookup_route(net, found_genid);

    if (found_route == NULL)
	return NULL;

    InternalMessage<A> msg(found_route, NULL, found_genid);
    const InternalMessage<A> *filtered_msg = apply_filters(&msg);
    
    if (filtered_msg == NULL)
	return NULL;

    // Not the case anymore---policy filters could have dropped the route and
    // route never made it to cache!
#if 0
    //the filters MUST NOT modify the route
    XLOG_ASSERT(!filtered_msg->changed());
    XLOG_ASSERT(filtered_msg == &msg);
#endif

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
const InternalMessage<A> *
FilterTable<A>::apply_filters(const InternalMessage<A> *rtmsg) const
{
    // in this case, this is actually const, so this hack is safe
    return const_cast<FilterTable<A>*>(this)->apply_filters(rtmsg, 0);
}

template<class A>
const InternalMessage<A> *
FilterTable<A>::apply_filters(const InternalMessage<A> *rtmsg,
			      int ref_change)
{
    const InternalMessage<A>* msg;
    FilterVersion<A> *filter;
    if (_do_versioning) {
	typename map<uint32_t, FilterVersion<A>* >::iterator i;
	uint32_t genid = rtmsg->genid();
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

	msg = filter->apply_filters(rtmsg, ref_change);

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
	msg = _current_filter->apply_filters(rtmsg, ref_change);
    }

    return msg;
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




