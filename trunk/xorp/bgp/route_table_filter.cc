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

#ident "$XORP: xorp/bgp/route_table_filter.cc,v 1.21 2004/05/15 15:12:16 mjh Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

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

/*************************************************************************/

template<class A>
SimpleASFilter<A>::SimpleASFilter<A>(const AsNum &as_num) 
    : _as_num(as_num)
{
}

template<class A>
const InternalMessage<A>* 
SimpleASFilter<A>::filter(const InternalMessage<A> *rtmsg, 
			  bool &modified) const 
{
    const PathAttributeList<A> *attributes = rtmsg->route()->attributes();
    const AsPath &as_path = attributes->aspath();
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
ASPrependFilter<A>::ASPrependFilter<A>(const AsNum &as_num) 
    : _as_num(as_num)
{
}

template<class A>
const InternalMessage<A>* 
ASPrependFilter<A>::filter(const InternalMessage<A> *rtmsg,
			   bool &modified) const
{
    //Create a new AS path with our AS number prepended to it.
    AsPath new_as_path(rtmsg->route()->attributes()->aspath());
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
NexthopRewriteFilter<A>::NexthopRewriteFilter<A>(const A& local_nexthop) 
    : _local_nexthop(local_nexthop)
{
}

template<class A>
const InternalMessage<A>* 
NexthopRewriteFilter<A>::filter(const InternalMessage<A> *rtmsg,
				bool &modified) const
{
    //Form a new path attribute list containing the new nexthop
    PathAttributeList<A> palist(*(rtmsg->route()->attributes()));
    palist.replace_nexthop(_local_nexthop);
    palist.rehash();
    
    //Create a new route message with the new path attribute list
    SubnetRoute<A> *new_route 
	= new SubnetRoute<A>(rtmsg->net(), &palist,
			     rtmsg->route()->original_route(), 
			     rtmsg->route()->igp_metric());

    debug_msg("NexthopRewriteFilter: new route: %x with attributes %x\n",
	   (uint)new_route, (uint)(new_route->attributes()));
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
IBGPLoopFilter<A>::IBGPLoopFilter<A>() 
{
}

template<class A>
const InternalMessage<A>* 
IBGPLoopFilter<A>::filter(const InternalMessage<A> *rtmsg,
			  bool &modified) const 
{

    //If the route originated from an IBGP peer, this filter will drop
    //the route.

    if (rtmsg->origin_peer()->ibgp() == true) {
	drop_message(rtmsg, modified);
	return NULL;
    }
    return rtmsg;
}

/*************************************************************************/

template<class A>
LocalPrefInsertionFilter<A>::
LocalPrefInsertionFilter<A>(uint32_t default_local_pref) 
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
LocalPrefRemovalFilter<A>::LocalPrefRemovalFilter<A>() 
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
MEDInsertionFilter<A>(NextHopResolver<A>& next_hop_resolver) 
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
MEDRemovalFilter<A>::MEDRemovalFilter<A>() 
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
UnknownFilter<A>::UnknownFilter<A>() 
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
OriginateRouteFilter<A>::OriginateRouteFilter<A>(const AsNum &as_num,
						 const bool ibgp)
    :  _as_num(as_num), _ibgp(ibgp)
{
}

template<class A>
const InternalMessage<A>* 
OriginateRouteFilter<A>::filter(const InternalMessage<A> *rtmsg,
				 bool &modified) const
{
    debug_msg("Originate Route Filter\n");

    // If we didn't originate this route forget it.
    if (!rtmsg->origin_peer()->originate_route_handler())
	return rtmsg;

    // If this is an EBGP peering then assume the AS is already
    // present. Perhaps we should check.
    if (false == _ibgp)
	return rtmsg;

    //Create a new AS path with our AS number prepended to it.
    AsPath new_as_path(rtmsg->route()->attributes()->aspath());
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
FilterTable<A>::FilterTable(string table_name,  
			    Safi safi,
			    BGPRouteTable<A> *parent_table,
			    NextHopResolver<A>& next_hop_resolver)
    : BGPRouteTable<A>("FilterTable-" + table_name, safi),
    _next_hop_resolver(next_hop_resolver)
{
    this->_parent = parent_table;
}

template<class A>
FilterTable<A>::~FilterTable() {
    typename list <BGPRouteFilter<A> *>::iterator iter;
    iter = _filters.begin();
    while (iter != _filters.end()) {
	delete (*iter);
	++iter;
    }    
}

template<class A>
int
FilterTable<A>::add_route(const InternalMessage<A> &rtmsg, 
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

    XLOG_ASSERT(!rtmsg.changed());
    const InternalMessage<A> *filtered_msg = apply_filters(&rtmsg);
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

    const InternalMessage<A> *filtered_old_msg = apply_filters(&old_rtmsg);
    const InternalMessage<A> *filtered_new_msg = apply_filters(&new_rtmsg);

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

    const InternalMessage<A> *filtered_msg = apply_filters(&rtmsg);
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

    const InternalMessage<A> *filtered_msg = apply_filters(&rtmsg);
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

    //the filters MUST NOT modify the route
    XLOG_ASSERT(!filtered_msg->changed());
    XLOG_ASSERT(filtered_msg == &msg);
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
FilterTable<A>::add_simple_AS_filter(const AsNum& as_num)
{
    SimpleASFilter<A>* AS_filter;
    AS_filter = new SimpleASFilter<A>(as_num);
    _filters.push_back(AS_filter);
    return 0;
}

template<class A>
int
FilterTable<A>::add_AS_prepend_filter(const AsNum& as_num)
{
    ASPrependFilter<A>* AS_prepender;
    AS_prepender = new ASPrependFilter<A>(as_num);
    _filters.push_back(AS_prepender);
    return 0;
}

template<class A>
int
FilterTable<A>::add_nexthop_rewrite_filter(const A& nexthop)
{
    NexthopRewriteFilter<A>* nh_rewriter;
    nh_rewriter = new NexthopRewriteFilter<A>(nexthop);
    _filters.push_back(nh_rewriter);
    return 0;
}

template<class A>
int
FilterTable<A>::add_ibgp_loop_filter()
{
    IBGPLoopFilter<A>* ibgp_filter;
    ibgp_filter = new IBGPLoopFilter<A>;
    _filters.push_back(ibgp_filter);
    return 0;
}

template<class A>
int
FilterTable<A>::add_localpref_insertion_filter(uint32_t default_local_pref)
{
    LocalPrefInsertionFilter<A> *localpref_filter;
    localpref_filter = new LocalPrefInsertionFilter<A>(default_local_pref);
    _filters.push_back(localpref_filter);
    return 0;
}

template<class A>
int
FilterTable<A>::add_localpref_removal_filter()
{
    LocalPrefRemovalFilter<A> *localpref_filter;
    localpref_filter = new LocalPrefRemovalFilter<A>();
    _filters.push_back(localpref_filter);
    return 0;
}

template<class A>
int
FilterTable<A>::add_med_insertion_filter()
{
    MEDInsertionFilter<A> *med_filter;
    med_filter = new MEDInsertionFilter<A>(_next_hop_resolver);
    _filters.push_back(med_filter);
    return 0;
}

template<class A>
int
FilterTable<A>::add_med_removal_filter()
{
    MEDRemovalFilter<A> *med_filter;
    med_filter = new MEDRemovalFilter<A>();
    _filters.push_back(med_filter);
    return 0;
}

template<class A>
int
FilterTable<A>::add_unknown_filter()
{
    UnknownFilter<A> *unknown_filter;
    unknown_filter = new UnknownFilter<A>();
    _filters.push_back(unknown_filter);
    return 0;
}

template<class A>
int
FilterTable<A>::add_originate_route_filter(const AsNum &asn, const bool ibgp)
{
    OriginateRouteFilter<A> *originate_route_filter;
    originate_route_filter = new OriginateRouteFilter<A>(asn, ibgp);
    _filters.push_back(originate_route_filter);
    return 0;
}

template<class A>
const InternalMessage<A> *
FilterTable<A>::apply_filters(const InternalMessage<A> *rtmsg) const 
{
    const InternalMessage<A> *filtered_msg = rtmsg;
    bool modified_by_us = false;
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
    return filtered_msg;
}


/* mechanisms to implement flow control in the output plumbing */
template<class A>
void 
FilterTable<A>::output_state(bool busy, BGPRouteTable<A> *next_table)
{
    XLOG_ASSERT(this->_next_table == next_table);

    this->_parent->output_state(busy, this);
}

template<class A>
bool 
FilterTable<A>::get_next_message(BGPRouteTable<A> *next_table)
{
    XLOG_ASSERT(this->_next_table == next_table);

    return this->_parent->get_next_message(this);
}

template class FilterTable<IPv4>;
template class FilterTable<IPv6>;




