
// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/bgp/route_table_cache.cc,v 1.43 2008/10/02 21:56:19 bms Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "route_table_cache.hh"
#include "bgp.hh"

template<class A> typename DeleteAllNodes<A>::RouteTables
	DeleteAllNodes<A>::_route_tables;
template<class A> int DeleteAllNodes<A>::_deletions_per_call = 1000;

template<class A>
CacheTable<A>::CacheTable(string table_name,  
			  Safi safi,
			  BGPRouteTable<A> *parent_table,
			  const PeerHandler *peer)
    : BGPRouteTable<A>("CacheTable-" + table_name, safi),
      _peer(peer),
      _unchanged_added(0), _unchanged_deleted(0),
      _changed_added(0), _changed_deleted(0)
{
    this->_parent = parent_table;
    _route_table = new RefTrie<A, const CacheRoute<A> >;
}

template<class A>
CacheTable<A>::~CacheTable()
{
    if (_route_table->begin() != _route_table->end()) {
	XLOG_WARNING("CacheTable trie was not empty on deletion\n");
    }
    delete _route_table;
}

template<class A>
void
CacheTable<A>::flush_cache()
{
    debug_msg("%s\n", this->tablename().c_str());
//     _route_table->delete_all_nodes();
    new DeleteAllNodes<A>(this->_peer, _route_table);
    _route_table = new RefTrie<A, const CacheRoute<A> >;
}

template<class A>
int
CacheTable<A>::add_route(InternalMessage<A> &rtmsg,
			 BGPRouteTable<A> *caller)
{
    debug_msg("\n         %s\n caller: %s\n rtmsg: %p route: %p\n%s\n",
	      this->tablename().c_str(),
	      caller ? caller->tablename().c_str() : "NULL",
	      &rtmsg,
	      rtmsg.route(),
	      rtmsg.str().c_str());

    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(this->_next_table != NULL);
    XLOG_ASSERT(!rtmsg.attributes()->is_locked());

    //a cache table is never going to be the last table
    IPNet<A> net = rtmsg.net();

    //XXXX
    //check we don't already have this cached
    if (_route_table->lookup_node(net) != _route_table->end()) {
	    crash_dump();
	    XLOG_UNREACHABLE();
    }

    log(c_format("add_route (changed): %s filters: %p,%p,%p",
		 net.str().c_str(),
		 rtmsg.route()->policyfilter(0).get(),
		 rtmsg.route()->policyfilter(1).get(),
		 rtmsg.route()->policyfilter(2).get()));

    // We used to store only changed routes.  Now we store all routes
    // here.  This is a half-way step to going pull-based, and putting
    // all this stuff in DecisionTable.

    const SubnetRoute<A> *msg_route = rtmsg.route();
    typename RefTrie<A, const CacheRoute<A> >::iterator iter;
    typename RefTrie<A, const CacheRoute<A> >::iterator ti;
    iter = _route_table->lookup_node(net);
    if (iter == _route_table->end()) {
	//it's not stored

	// need to regenerate the PA list to store it in the CacheRoute
	rtmsg.attributes()->canonicalize();
	PAListRef<A> pa_list_ref = new PathAttributeList<A>(rtmsg.attributes());
	pa_list_ref.register_with_attmgr();
	SubnetRoute<A>* new_route =
	    new SubnetRoute<A>(msg_route->net(),
			       pa_list_ref,
			       msg_route,
			       msg_route->igp_metric());
	new_route->set_nexthop_resolved(msg_route->nexthop_resolved());
	ti = _route_table->insert(msg_route->net(), 
				  CacheRoute<A>(new_route, rtmsg.genid()));
	new_route->unref();
    } else {
	// we can't get an add for a route that's stored - we'd expect
	// a replace instead.
	XLOG_UNREACHABLE();
    }

    debug_msg("Cache Table: %s\n", this->tablename().c_str());
    debug_msg("Caching route: %p net: %s %s\n", msg_route,
	      msg_route->net().str().c_str(), 
	      msg_route->str().c_str());
    //assert(ti.payload().route() == msg_route);

    //progogate downstream
    // we create a new message, but reuse the FPAList from the old one.
    InternalMessage<A> new_rt_msg(ti.payload().route(), 
				  rtmsg.attributes(),
				  rtmsg.origin_peer(),
				  rtmsg.genid());
    if (rtmsg.push()) new_rt_msg.set_push();
    int result = this->_next_table->add_route(new_rt_msg, 
					      (BGPRouteTable<A>*)this);

    rtmsg.inactivate();
    switch (result) {
    case ADD_USED:
      ti.payload().route()->set_in_use(true);
#ifdef DUMP_FROM_CACHE
	//Our cached copy was used, but we need to tell upstream
	//that their unmodified version was unused.
	return ADD_UNUSED;
#else
	//XXX see comment in dump_entire_table - disable dumping
	//from the cache, so we need to return ADD_USED for now.
	return ADD_USED;
#endif
    case ADD_UNUSED:
	ti.payload().route()->set_in_use(false);
	return ADD_UNUSED;
    default:
	//In the case of a failure, we don't know what to believe.
	//To be safe we set in_use to true, although it may reduce
	//efficiency.
	msg_route->set_in_use(true);
	return result;
    }
}

template<class A>
int
CacheTable<A>::replace_route(InternalMessage<A> &old_rtmsg, 
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
	      caller ? caller->tablename().c_str() : "NULL",
	      &old_rtmsg,
	      &new_rtmsg,
	      old_rtmsg.route(),
	      new_rtmsg.route(),
	      old_rtmsg.str().c_str(),
	      new_rtmsg.str().c_str());

    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(this->_next_table != NULL);

    IPNet<A> net = old_rtmsg.net();
    XLOG_ASSERT(net == new_rtmsg.net());
    log("replace_route: " + net.str());

    SubnetRouteConstRef<A> *old_route_reference = NULL;
    InternalMessage<A> *old_rtmsg_ptr = &old_rtmsg;
    int result = ADD_USED;

    typename RefTrie<A, const CacheRoute<A> >::iterator iter;
    iter = _route_table->lookup_node(net);
    if (iter == _route_table->end()) {
	//We don't flush the cache, so this should not happen
	crash_dump();
	XLOG_UNREACHABLE();
    }

    // Preserve the route.  Taking a reference will prevent
    // the route being immediately deleted when it's erased
    // from the RefTrie.  Deletion will occur later when the
    // reference is deleted.
    const SubnetRoute<A> *old_route = iter.payload().route();
    old_route_reference = new SubnetRouteConstRef<A>(old_route);

    // take the attribute list from the stored version to get the correct MED.
    PAListRef<A> old_pa_list = old_route->attributes();
    FPAListRef old_fpa_list = 
	new FastPathAttributeList<A>(old_pa_list);

    old_rtmsg_ptr = new InternalMessage<A>(old_route,
					   old_fpa_list,
					   old_rtmsg.origin_peer(),
					   iter.payload().genid());

    //delete it from our cache, 
    _route_table->erase(old_rtmsg.net());

    old_pa_list.deregister_with_attmgr();

    //It's the responsibility of the recipient of a changed
    //route to store it or free it.
    //Free the route from the message.
    old_rtmsg.inactivate();

    const SubnetRoute<A> *msg_new_route = new_rtmsg.route();
    //store it locally
    typename RefTrie<A, const CacheRoute<A> >::iterator ti;

    // need to regenerate the PA list to store it in the CacheRoute
    new_rtmsg.attributes()->canonicalize();
    PAListRef<A> pa_list_ref = new PathAttributeList<A>(new_rtmsg.attributes());
    pa_list_ref.register_with_attmgr();
    SubnetRoute<A>* new_route = 
	new SubnetRoute<A>(msg_new_route->net(),
			   pa_list_ref,
			   msg_new_route,
			   msg_new_route->igp_metric());
    new_route->set_nexthop_resolved(msg_new_route->nexthop_resolved());

    ti = _route_table->insert(net, CacheRoute<A>(new_route, 
						 new_rtmsg.genid()));
    new_route->unref();
    //progogate downstream
    InternalMessage<A> new_rtmsg_copy(ti.payload().route(),
				      new_rtmsg.attributes(),
				      new_rtmsg.origin_peer(),
				      new_rtmsg.genid());

    if (new_rtmsg.push()) new_rtmsg_copy.set_push();
    result = this->_next_table->replace_route(*old_rtmsg_ptr,
					      new_rtmsg_copy, 
					      (BGPRouteTable<A>*)this);
    new_rtmsg.inactivate();

    switch (result) {
    case ADD_USED:
	ti.payload().route()->set_in_use(true);
	break;
    case ADD_UNUSED:
	ti.payload().route()->set_in_use(false);
	break;
    default:
	//In the case of a failure, we don't know what to believe.
	//To be safe we set in_use to true, although it may reduce
	//efficiency.
	ti.payload().route()->set_in_use(true);
    }

    if (old_rtmsg_ptr != &old_rtmsg) {
	delete old_rtmsg_ptr;
	XLOG_ASSERT(old_route_reference != NULL);
	delete old_route_reference;
    }

    return result;
}

template<class A>
int
CacheTable<A>::delete_route(InternalMessage<A> &rtmsg, 
			    BGPRouteTable<A> *caller)
{
    int result = 0;
    debug_msg("\n         %s\n caller: %s\n rtmsg: %p route: %p\n%s\n",
	      this->tablename().c_str(),
	      caller ? caller->tablename().c_str() : "NULL",
	      &rtmsg,
	      rtmsg.route(),
	      rtmsg.str().c_str());

    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(this->_next_table != NULL);
    IPNet<A> net = rtmsg.net();
    log(c_format("delete_route (changed): %s filters: %p,%p,%p",
		 net.str().c_str(),
		 rtmsg.route()->policyfilter(0).get(),
		 rtmsg.route()->policyfilter(1).get(),
		 rtmsg.route()->policyfilter(2).get()));

    typename RefTrie<A, const CacheRoute<A> >::iterator iter;
    iter = _route_table->lookup_node(net);
    XLOG_ASSERT(iter != _route_table->end());

    const SubnetRoute<A> *existing_route = iter.payload().route();
    uint32_t existing_genid = iter.payload().genid();
    XLOG_ASSERT(rtmsg.genid() == existing_genid);
    debug_msg("Found cached route: %s\n", existing_route->str().c_str());

    PAListRef<A> old_pa_list = existing_route->attributes();   

    // Delete it from our cache trie.  The actual deletion will
    // only take place when iter goes out of scope, so
    // existing_route remains valid til then.
    _route_table->erase(iter);

    old_pa_list.deregister_with_attmgr();

    // Fix the parent route in case it has changed.  This is also
    // important if the parent route has been zeroed somewhere
    // upstream to avoid unwanted side effects.
    const_cast<SubnetRoute<A>*>(existing_route)
	->set_parent_route(rtmsg.route()->parent_route());

    // create the FPA list from the stored version
    FPAListRef fpa_list = 
	new FastPathAttributeList<A>(old_pa_list);

    InternalMessage<A> old_rt_msg(existing_route,
				  fpa_list,
				  rtmsg.origin_peer(),
				  existing_genid);
    if (rtmsg.push()) old_rt_msg.set_push();
    
    result = this->_next_table->delete_route(old_rt_msg, 
					     (BGPRouteTable<A>*)this);

    if (rtmsg.copied()) {
	//It's the responsibility of the recipient of a changed
	//route to store it or free it.
	//Free the route from the message.
	rtmsg.inactivate();
    }
    return result;
}

template<class A>
int
CacheTable<A>::push(BGPRouteTable<A> *caller)
{
    XLOG_ASSERT(caller == this->_parent);
    return this->_next_table->push((BGPRouteTable<A>*)this);
}

template<class A>
int 
CacheTable<A>::route_dump(InternalMessage<A> &rtmsg,
			  BGPRouteTable<A> *caller,
			  const PeerHandler *dump_peer)
{
    XLOG_ASSERT(caller == this->_parent);
    //Check we've got it cached.  Clear the changed bit so we
    //don't confuse anyone downstream.
    IPNet<A> net = rtmsg.route()->net();
    typename RefTrie<A, const CacheRoute<A> >::iterator iter;
    iter = _route_table->lookup_node(net);
    XLOG_ASSERT(iter != _route_table->end());
    XLOG_ASSERT(rtmsg.genid() == iter.payload().genid());
    const SubnetRoute<A> *existing_route = iter.payload().route();

    //It's the responsibility of the recipient of a changed route
    //to store or delete it.  We don't need it anymore (we found
    //the cached copy) so we delete it.
    if (rtmsg.copied())
	rtmsg.route()->unref();

    // create the FPA list from the stored version
    PAListRef<A> pa_list = existing_route->attributes();
    FPAListRef fpa_list = 
	new FastPathAttributeList<A>(pa_list);

    //the message we pass on needs to contain our cached
    //route, because the MED info in it may not be in the original
    //version of the route.
    InternalMessage<A> new_msg(existing_route,
			       fpa_list,
			       rtmsg.origin_peer(),
			       rtmsg.genid());
    return this->_next_table->route_dump(new_msg, (BGPRouteTable<A>*)this, 
					 dump_peer);
}

template<class A>
const SubnetRoute<A>*
CacheTable<A>::lookup_route(const IPNet<A> &net,
			    uint32_t& genid,
			    FPAListRef& fpa_list) const
{
    // we cache all requests, so if we don't have it, it doesn't exist
    typename RefTrie<A, const CacheRoute<A> >::iterator iter;
    iter = _route_table->lookup_node(net);
    if (iter != _route_table->end()) {
	genid = iter.payload().genid();

	// create the FPA list from the stored version
	PAListRef<A> pa_list = iter.payload().route()->attributes();
	fpa_list = new FastPathAttributeList<A>(pa_list);
	return iter.payload().route();
    }
    return 0;
}

template<class A>
void
CacheTable<A>::route_used(const SubnetRoute<A>* rt, bool in_use)
{
    this->_parent->route_used(rt, in_use);
}

template<class A>
string
CacheTable<A>::str() const {
    string s = "CacheTable<A>" + this->tablename();
    return s;
}

template<class A>
bool
CacheTable<A>::get_next_message(BGPRouteTable<A> *next_table)
{
    XLOG_ASSERT(this->_next_table == next_table);

    return this->_parent->get_next_message(this);
}

template<class A>
string
CacheTable<A>::dump_state() const {
    string s;
    s  = "=================================================================\n";
    s += "CacheTable\n";
    s += str() + "\n";
    s += "=================================================================\n";
    s += "Unchanged added: " + c_format("%d\n", _unchanged_added);
    s += "Unchanged deleted: " + c_format("%d\n", _unchanged_deleted);
    s += "Changed added: " + c_format("%d\n", _changed_added); 
    s += "Changed deleted: " + c_format("%d\n", _changed_deleted);
    //    s += "In cache: " + c_format("%d\n", _route_table->size());
    s += _route_table->str();
    s += CrashDumper::dump_state(); 
    return s;
}

template<class A>
EventLoop& 
CacheTable<A>::eventloop() const 
{
    return _peer->eventloop();
}

template class CacheTable<IPv4>;
template class CacheTable<IPv6>;
