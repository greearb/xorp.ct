// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/route_table_cache.cc,v 1.8 2003/02/07 22:55:20 mjh Exp $"

//#define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "route_table_cache.hh"
#include <map>

template<class A>
CacheTable<A>::CacheTable(string table_name,  
			  BGPRouteTable<A> *parent_table) 
    : BGPRouteTable<A>("CacheTable-" + table_name)
{
    _parent = parent_table;
}

template<class A>
void
CacheTable<A>::flush_cache() {
    debug_msg("CacheTable<A>::flush_cache on %s\n",
	      tablename().c_str());
    _route_table.delete_all_nodes();
}

template<class A>
int
CacheTable<A>::add_route(const InternalMessage<A> &rtmsg, 
			 BGPRouteTable<A> *caller) {
    debug_msg("CacheTable<A>::add_route %x on %s\n",
	   (u_int)(&rtmsg), tablename().c_str());
    debug_msg("add route: %s\n", rtmsg.net().str().c_str());
    assert(caller == _parent);
    assert(_next_table != NULL);

    //a cache table is never going to be the last table
    IPNet<A> net = rtmsg.net();

    //check we don't already have this cached
    assert(_route_table.lookup_node(net) == _route_table.end());

    if (rtmsg.changed()==false) {
	return _next_table->add_route(rtmsg, (BGPRouteTable<A>*)this);
    } else {
	//The route was changed.  

	//It's the responsibility of the recipient of a changed route
	//to store it or free it.  We're the final recipient, so we
	//can rely on rtmsg.route() not to go away.
	const SubnetRoute<A> *msg_route = rtmsg.route();
	//store it locally
	typename Trie<A, const SubnetRoute<A> >::iterator ti;
	ti = _route_table.insert(msg_route->net(), *msg_route);
	debug_msg("Caching route: %x net: %s atts: %x  %s\n", (uint)msg_route,
	       msg_route->net().str().c_str(), 
	       (uint)(msg_route->attributes()), 
	       msg_route->str().c_str());

	//progogate downstream
	InternalMessage<A> new_rt_msg(&(ti.payload()), rtmsg.origin_peer(),
				      rtmsg.genid());
	if (rtmsg.push()) new_rt_msg.set_push();
	int result = _next_table->add_route(new_rt_msg, 
					    (BGPRouteTable<A>*)this);

	rtmsg.inactivate();

	switch (result) {
	case ADD_USED:
	    ti.payload().set_in_use(true);
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
	    ti.payload().set_in_use(false);
	    return ADD_UNUSED;
	default:
	    //In the case of a failure, we don't know what to believe.
	    //To be safe we set in_use to true, although it may reduce
	    //efficiency.
	    msg_route->set_in_use(true);
	    return result;
	}
    }
}

template<class A>
int
CacheTable<A>::replace_route(const InternalMessage<A> &old_rtmsg, 
			     const InternalMessage<A> &new_rtmsg, 
			     BGPRouteTable<A> *caller) {
    debug_msg("CacheTable<A>::replace_route %x -> %x on %s\n",
	   (u_int)(&old_rtmsg), (u_int)(&new_rtmsg), tablename().c_str());
    debug_msg("replace route: %s\n", old_rtmsg.net().str().c_str());
    assert(caller == _parent);
    assert(_next_table != NULL);

    IPNet<A> net = old_rtmsg.net();
    assert(net == new_rtmsg.net());

    SubnetRoute<A> *old_route_copy = NULL;
    const InternalMessage<A> *old_rtmsg_ptr = &old_rtmsg;
    int result = ADD_USED;


    //do we have the old route cached?
    if (old_rtmsg.changed()==true) {
	typename Trie<A, const SubnetRoute<A> >::iterator iter;
	iter = _route_table.lookup_node(net);
	if (iter == _route_table.end()) {
	    //We don't flush the cache, so this should not happen
	    abort();
	} else {
	    //preserve the information
	    old_route_copy = new SubnetRoute<A>(iter.payload());

	    //set the parent route of the copy to one that still
	    //exists, because the parent_route pointer of our cached
	    //version is probably now invalid.
	    old_route_copy->
		set_parent_route(old_rtmsg.route()->parent_route());

	    old_rtmsg_ptr = new InternalMessage<A>(old_route_copy,
						   old_rtmsg.origin_peer(),
						   old_rtmsg.genid());

	    //delete it from our cache, 
	    _route_table.erase(old_rtmsg.net());

	    //It's the responsibility of the recipient of a changed
	    //route to store it or free it.
	    //Free the route from the message.
	    old_rtmsg.inactivate();
	}
    }

    //do we need to cache the new route?
    if (new_rtmsg.changed()==false) {
	result = _next_table->replace_route(*old_rtmsg_ptr, 
					    new_rtmsg, 
					    (BGPRouteTable<A>*)this);
    } else {
	//Route was changed.

	//It's the responsibility of the recipient of a changed route
	//to store it or free it.  We're the final recipient, so we
	//can rely on rtmsg.route() not to go away.
	const SubnetRoute<A> *new_route = new_rtmsg.route();
	//store it locally
	typename Trie<A, const SubnetRoute<A> >::iterator ti;
	ti = _route_table.insert(net, *new_route);

	//progogate downstream
	InternalMessage<A> new_rtmsg_copy(&(ti.payload()),
					  new_rtmsg.origin_peer(),
					  new_rtmsg.genid());
	if (new_rtmsg.push()) new_rtmsg_copy.set_push();
	result = _next_table->replace_route(*old_rtmsg_ptr,
					    new_rtmsg_copy, 
					    (BGPRouteTable<A>*)this);
	new_rtmsg.inactivate();

	switch (result) {
	case ADD_USED:
	    ti.payload().set_in_use(true);
	    break;
	case ADD_UNUSED:
	    ti.payload().set_in_use(false);
	    break;
	default:
	    //In the case of a failure, we don't know what to believe.
	    //To be safe we set in_use to true, although it may reduce
	    //efficiency.
	    ti.payload().set_in_use(true);
	}
    }

    if (old_rtmsg_ptr != &old_rtmsg) {
	delete old_rtmsg_ptr;
	assert(old_route_copy != NULL);
	old_route_copy->unref();
    }

    return result;
}

template<class A>
int
CacheTable<A>::delete_route(const InternalMessage<A> &rtmsg, 
			    BGPRouteTable<A> *caller) {

    debug_msg("table %s route = %s\n", _tablename.c_str(),
	      rtmsg.str().c_str());

    assert(caller == _parent);
    assert(_next_table != NULL);
    IPNet<A> net = rtmsg.net();

    //do we already have this cached?
    typename Trie<A, const SubnetRoute<A> >::iterator iter;
    iter = _route_table.lookup_node(net);
    if (iter != _route_table.end()) {
	const SubnetRoute<A> *existing_route = &(iter.payload());
	debug_msg("Found cached route: %s\n", existing_route->str().c_str());
	//preserve the information
	SubnetRoute<A>* route_copy = new SubnetRoute<A>(*existing_route);

	//set the copy's parent route to one that still exists, because
	//the parent_route pointer of our cached version is
	//probably now invalid
	route_copy->set_parent_route(rtmsg.route()->parent_route());

	//delete it from our cache trie 
	_route_table.erase(net);

	InternalMessage<A> old_rt_msg(route_copy,
				      rtmsg.origin_peer(),
				      rtmsg.genid());
	if (rtmsg.push()) old_rt_msg.set_push();

	int result = 0;
	result = _next_table->delete_route(old_rt_msg, 
					   (BGPRouteTable<A>*)this);

	if (rtmsg.changed()) {
	    //It's the responsibility of the recipient of a changed
	    //route to store it or free it.
	    //Free the route from the message.
	    rtmsg.inactivate();
	}
	route_copy->unref();
	return result;
    }

    if (rtmsg.changed()) {
	//we don't flush the cache, so this should simply never happen.
	abort();
    }

    //If we get here, route was not cached and was not modified.

    return _next_table->delete_route(rtmsg, (BGPRouteTable<A>*)this);
}

template<class A>
int
CacheTable<A>::push(BGPRouteTable<A> *caller) {
    assert(caller == _parent);
    return _next_table->push((BGPRouteTable<A>*)this);
}

template<class A>
int 
CacheTable<A>::route_dump(const InternalMessage<A> &rtmsg,
			  BGPRouteTable<A> *caller,
			  const PeerHandler *dump_peer) {
    assert(caller == _parent);
    if (rtmsg.changed()) {
	//Check we've got it cached.  Clear the changed bit so we
	//don't confuse anyone downstream.
	IPNet<A> net = rtmsg.route()->net();
	typename Trie<A, const SubnetRoute<A> >::iterator iter;
	iter = _route_table.lookup_node(net);
	assert(iter != _route_table.end());

	//the message we pass on needs to contain our cached
	//route, because the MED info in it may not be in the original
	//version of the route.
	InternalMessage<A> new_msg(&(iter.payload()), rtmsg.origin_peer(),
				   rtmsg.genid());
	return _next_table->route_dump(new_msg, (BGPRouteTable<A>*)this, 
				       dump_peer);
    } else {
	return _next_table->route_dump(rtmsg, (BGPRouteTable<A>*)this, 
				       dump_peer);
    }
}

template<class A>
const SubnetRoute<A>*
CacheTable<A>::lookup_route(const IPNet<A> &net) const {
    //return our cached copy if there is one, otherwise ask our parent
    typename Trie<A, const SubnetRoute<A> >::iterator iter;
    iter = _route_table.lookup_node(net);
    if (iter != _route_table.end())
	return &(iter.payload());
    else
	return _parent->lookup_route(net);
}

template<class A>
void
CacheTable<A>::route_used(const SubnetRoute<A>* rt, bool in_use){
    _parent->route_used(rt, in_use);
}

template<class A>
string
CacheTable<A>::str() const {
    string s = "CacheTable<A>" + tablename();
    return s;
}

/* mechanisms to implement flow control in the output plumbing */
template<class A>
void 
CacheTable<A>::output_state(bool busy, BGPRouteTable<A> *next_table) {
    XLOG_ASSERT(_next_table == next_table);

    _parent->output_state(busy, this);
}

template<class A>
bool 
CacheTable<A>::get_next_message(BGPRouteTable<A> *next_table) {
    XLOG_ASSERT(_next_table == next_table);

    return _parent->get_next_message(this);
}

template class CacheTable<IPv4>;
template class CacheTable<IPv6>;





