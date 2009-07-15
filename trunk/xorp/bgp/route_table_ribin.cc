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



// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "route_table_ribin.hh"
#include "route_table_deletion.hh"
#include "rib_ipc_handler.hh"
#include "bgp.hh"

template<class A>
RibInTable<A>::RibInTable(string table_name,
			  Safi safi,
			  const PeerHandler *peer)
    : 	BGPRouteTable<A>("RibInTable-" + table_name, safi),
	_peer(peer)
{
    _route_table = new BgpTrie<A>;
    _peer_is_up = true;
    _genid = 1; /*zero is not a valid genid*/
    _table_version = 1;
    this->_parent = NULL;

    _nexthop_push_active = false;
}

template<class A>
RibInTable<A>::~RibInTable()
{
    delete _route_table;
}

template<class A>
void
RibInTable<A>::flush()
{
    debug_msg("%s\n", this->tablename().c_str());
    _route_table->delete_all_nodes();
}

template<class A>
void
RibInTable<A>::ribin_peering_went_down()
{
    log("Peering went down");
    _peer_is_up = false;

    // Stop pushing changed nexthops.
    stop_nexthop_push();

    /* When the peering goes down we unhook our entire RibIn routing
       table and give it to a new deletion table.  We plumb the
       deletion table in after the RibIn, and it can then delete the
       routes as a background task while filtering the routes we give
       it so that the later tables get the correct deletes at the
       correct time. This means that we don't have to concern
       ourselves with holding deleted and new routes if the peer comes
       back up before we've finished the background deletion process -
       credits to Atanu Ghosh for this neat idea */

    if (_route_table->route_count() > 0) {

	string tablename = "Deleted" + this->tablename();

	DeletionTable<A>* deletion_table =
	    new DeletionTable<A>(tablename, this->safi(), _route_table, _peer, 
				 _genid, this);

	_route_table = new BgpTrie<A>;

	deletion_table->set_next_table(this->_next_table);
	this->_next_table->set_parent(deletion_table);
	this->_next_table = deletion_table;

	this->_next_table->peering_went_down(_peer, _genid, this);
	deletion_table->initiate_background_deletion();
    } else {
	//nothing to delete - just notify everyone
	this->_next_table->peering_went_down(_peer, _genid, this);
	this->_next_table->push(this);
	this->_next_table->peering_down_complete(_peer, _genid, this);
    }
}

template<class A>
void
RibInTable<A>::ribin_peering_came_up()
{
    log("Peering came up");
    _peer_is_up = true;
    _genid++;

    // cope with wrapping genid without using zero which is reserved
    if (_genid == 0) {
	_genid = 1;
    }

    _table_version = 1;

    this->_next_table->peering_came_up(_peer, _genid, this);
}

template<class A>
int
RibInTable<A>::add_route(const IPNet<A>& net, 
			 FPAListRef& fpa_list,
			 const PolicyTags& policy_tags)
{
    const ChainedSubnetRoute<A> *new_route;
    const SubnetRoute<A> *existing_route;
    XLOG_ASSERT(_peer_is_up);
    XLOG_ASSERT(this->_next_table != NULL);
    XLOG_ASSERT(!fpa_list->is_locked());
    log("add route: " + net.str());

    int response;
    typename BgpTrie<A>::iterator iter = _route_table->lookup_node(net);
    if (iter != _route_table->end()) {
	existing_route = &(iter.payload());
	XLOG_ASSERT(existing_route->net() == net);
#if 0
	if (rtmsg.route()->attributes() == existing_route->attributes()) {
	    // this route is the same as before.
	    // no need to do anything.
	    return ADD_UNUSED;
	}
#endif
	// Preserve the route.  Taking a reference will prevent the
	// route being deleted when it's erased from the Trie.
	// Deletion will occur when the reference goes out of scope.
	SubnetRouteConstRef<A> route_reference(existing_route);
	deletion_nexthop_check(existing_route);

	PAListRef<A> old_pa_list = existing_route->attributes();   
	FPAListRef old_fpa_list = new FastPathAttributeList<A>(old_pa_list);

	// delete from the Trie
	_route_table->erase(net);
	_table_version++;

	old_pa_list.deregister_with_attmgr();

	InternalMessage<A> old_rt_msg(existing_route, old_fpa_list, 
				      _peer, _genid);

	// Create the concise format PA list.
	fpa_list->canonicalize();
	PAListRef<A> pa_list = new PathAttributeList<A>(fpa_list);
	pa_list.register_with_attmgr();

	// Store it locally.  The BgpTrie will copy it into a ChainedSubnetRoute
	SubnetRoute<A>* tmp_route = new SubnetRoute<A>(net, pa_list, NULL);
	tmp_route->set_policytags(policy_tags);
	A nexthop = fpa_list->nexthop_att()->nexthop();
	typename BgpTrie<A>::iterator iter =
	    _route_table->insert(net, *tmp_route);
	tmp_route->unref();
	new_route = &(iter.payload());

	// propagate downstream
	InternalMessage<A> new_rt_msg(new_route, fpa_list, _peer, _genid);
	response = this->_next_table->replace_route(old_rt_msg, new_rt_msg,
					      (BGPRouteTable<A>*)this);
    } else {
	// Create the concise format PA list.
	fpa_list->canonicalize();
	PAListRef<A> pa_list = new PathAttributeList<A>(fpa_list);
	pa_list.register_with_attmgr();

	SubnetRoute<A>* tmp_route = new SubnetRoute<A>(net, pa_list, NULL);
	tmp_route->set_policytags(policy_tags);
	typename BgpTrie<A>::iterator iter =
	    _route_table->insert(net, *tmp_route);
	tmp_route->unref();
	new_route = &(iter.payload());

	// progogate downstream
	InternalMessage<A> new_rt_msg(new_route, fpa_list, _peer, _genid);
	response = this->_next_table->add_route(new_rt_msg, (BGPRouteTable<A>*)this);
    }

    switch (response) {
    case ADD_UNUSED:
	new_route->set_in_use(false);
	new_route->set_filtered(false);
	break;
    case ADD_FILTERED:
	new_route->set_in_use(false);
	new_route->set_filtered(true);
	break;
    case ADD_USED:
    case ADD_FAILURE:
	// the default if we don't know for sure that a route is unused
	// should be that it is used.
	new_route->set_in_use(true);
	new_route->set_filtered(false);
	break;
    }

    return response;
}

template<class A>
int
RibInTable<A>::delete_route(const IPNet<A> &net)
{
    XLOG_ASSERT(_peer_is_up);
    log("delete route: " + net.str());


    typename BgpTrie<A>::iterator iter = _route_table->lookup_node(net);
    if (iter != _route_table->end()) {
	const SubnetRoute<A> *existing_route = &(iter.payload());

	// Preserve the route.  Taking a reference will prevent the
	// route being deleted when it's erased from the Trie.
	// Deletion will occur when the reference goes out of scope.
	SubnetRouteConstRef<A> route_reference(existing_route);
	deletion_nexthop_check(existing_route);

	PAListRef<A> old_pa_list = iter.payload().attributes();   
	FPAListRef old_fpa_list = new FastPathAttributeList<A>(old_pa_list);

	// remove from the Trie
	_route_table->erase(net);
	_table_version++;

	old_pa_list.deregister_with_attmgr();

	// propogate downstream
	InternalMessage<A> old_rt_msg(existing_route, old_fpa_list, _peer, _genid);
	if (this->_next_table != NULL)
	    this->_next_table->delete_route(old_rt_msg, (BGPRouteTable<A>*)this);
    } else {
	// we received a delete, but didn't have anything to delete.
	// It's debatable whether we should silently ignore this, or
	// drop the peering.  If we don't hold input-filtered routes in
	// the RIB-In, then this would be commonplace, so we'd have to
	// silently ignore it.  Currently (Sept 2002) we do still hold
	// filtered routes in the RIB-In, so this should be an error.
	// But we'll just ignore this error, and log a warning.
	string s = "Attempt to delete route for net " + net.str()
	    + " that wasn't in RIB-In\n";
	XLOG_WARNING("%s", s.c_str());
	return -1;
    }
    return 0;
}

template<class A>
int
RibInTable<A>::push(BGPRouteTable<A> *caller)
{
    debug_msg("RibInTable<A>::push\n");
    XLOG_ASSERT(caller == NULL);
    XLOG_ASSERT(_peer_is_up);
    XLOG_ASSERT(this->_next_table != NULL);

    return this->_next_table->push((BGPRouteTable<A>*)this);
}

template<class A>
const SubnetRoute<A>*
RibInTable<A>::lookup_route(const IPNet<A> &net, uint32_t& genid,
			    FPAListRef& fpa_list_ref) const
{
    if (_peer_is_up == false)
	return NULL;

    typename BgpTrie<A>::iterator iter = _route_table->lookup_node(net);
    if (iter != _route_table->end()) {
	// assert(iter.payload().net() == net);
	genid = _genid;
	PAListRef<A> pa_list = iter.payload().attributes();   
	FastPathAttributeList<A>* fpa_list =
	    new FastPathAttributeList<A>(pa_list);
	fpa_list_ref = fpa_list;
	return &(iter.payload());
    } else
	fpa_list_ref = NULL;
	return NULL;
}

template<class A>
void
RibInTable<A>::route_used(const SubnetRoute<A>* used_route, bool in_use)
{
    // we look this up rather than modify used_route itself because
    // it's possible that used_route originates the other side of a
    // cache, and modifying it might not modify the version we have
    // here in the RibIn
    if (_peer_is_up == false)
	return;
    typename BgpTrie<A>::iterator iter = _route_table->lookup_node(used_route->net());
    XLOG_ASSERT(iter != _route_table->end());
    iter.payload().set_in_use(in_use);
}

template<class A>
string
RibInTable<A>::str() const
{
    string s = "RibInTable<A>" + this->tablename();
    return s;
}

template<class A>
bool
RibInTable<A>::dump_next_route(DumpIterator<A>& dump_iter)
{
    typename BgpTrie<A>::iterator route_iterator;
    debug_msg("dump iter: %s\n", dump_iter.str().c_str());
   
    if (dump_iter.route_iterator_is_valid()) {
	debug_msg("route_iterator is valid\n");
 	route_iterator = dump_iter.route_iterator();
	// Make sure the iterator is valid. If it is pointing at a
	// deleted node this comparison will move it forward.
	if (route_iterator == _route_table->end()) {
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
	route_iterator = _route_table->begin();
    }

    if (route_iterator == _route_table->end()) {
	return false;
    }

    const ChainedSubnetRoute<A>* chained_rt;
    for ( ; route_iterator != _route_table->end(); route_iterator++) {
	chained_rt = &(route_iterator.payload());
	debug_msg("chained_rt: %s\n", chained_rt->str().c_str());

	// propagate downstream
	// only dump routes that actually won
	// XXX: or if its a policy route dump

	if (chained_rt->is_winner() || dump_iter.peer_to_dump_to() == NULL) {
	    InternalMessage<A> rt_msg(chained_rt, _peer, _genid);
	   
	    log("dump route: " + rt_msg.net().str());
	    int res = this->_next_table->route_dump(rt_msg, (BGPRouteTable<A>*)this,
				    dump_iter.peer_to_dump_to());
	    if(res == ADD_FILTERED) 
		chained_rt->set_filtered(true);
	    else
		chained_rt->set_filtered(false);
	    
	    break;
	}
    }

    if (route_iterator == _route_table->end())
	return false;

    // Store the new iterator value as its valid.
    dump_iter.set_route_iterator(route_iterator);

    return true;
}

template<class A>
void
RibInTable<A>::igp_nexthop_changed(const A& bgp_nexthop)
{
    debug_msg("igp_nexthop_changed for bgp_nexthop %s on table %s\n",
	   bgp_nexthop.str().c_str(), this->tablename().c_str());

    log("igp nexthop changed: " + bgp_nexthop.str());
    typename set <A>::const_iterator i;
    i = _changed_nexthops.find(bgp_nexthop);
    if (i != _changed_nexthops.end()) {
	debug_msg("nexthop already queued\n");
	// this nexthop is already queued to be pushed again
	return;
    }

    if (_nexthop_push_active) {
	debug_msg("push active, adding to queue\n");
	_changed_nexthops.insert(bgp_nexthop);
    } else {
	debug_msg("push was inactive, activating\n");

	// this is more work than it should be - we need to create a
	// canonicalized path attribute list containing only the
	// nexthop.  When we call lower bound, this will find the
	// first pathmap chain containing this nexthop.

	FPAListRef dummy_fpa_list = new FastPathAttributeList<A>();
	NextHopAttribute<A> nh_att(bgp_nexthop);
	dummy_fpa_list->add_path_attribute(nh_att);
	dummy_fpa_list->canonicalize();
	PAListRef<A> dummy_pa_list = new PathAttributeList<A>(dummy_fpa_list);

	typename BgpTrie<A>::PathmapType::const_iterator pmi;
	pmi = _route_table->pathmap().lower_bound(dummy_pa_list);
	if (pmi == _route_table->pathmap().end()) {
	    // no route in this trie has this Nexthop
	    debug_msg("no matching routes - do nothing\n");
	    return;
	}
	PAListRef<A> pa_list = pmi->first;
	FPAListRef fpa_list = new FastPathAttributeList<A>(pa_list);
	if (fpa_list->nexthop() != bgp_nexthop) {
	    debug_msg("no matching routes (2)- do nothing\n");
	    return;
	}
	_current_changed_nexthop = bgp_nexthop;
	_nexthop_push_active = true;
	_current_chain = pmi;
	const SubnetRoute<A>* next_route_to_push = _current_chain->second;
	UNUSED(next_route_to_push);
	debug_msg("Found route with nexthop %s:\n%s\n",
		  bgp_nexthop.str().c_str(),
		  next_route_to_push->str().c_str());

	// _next_table->push((BGPRouteTable<A>*)this);
	// call back immediately, but after network events or expired timers
	_push_task = eventloop().new_task(
	    callback(this, &RibInTable<A>::push_next_changed_nexthop),
	    XorpTask::PRIORITY_DEFAULT, XorpTask::WEIGHT_DEFAULT);
    }
}

template<class A>
bool
RibInTable<A>::push_next_changed_nexthop()
{
    if (_nexthop_push_active == false) {
	//
	// XXX: No more nexthops to push, probably because routes have
	// been just deleted.
	//
	return false;
    }

    XLOG_ASSERT(_peer_is_up);

    const ChainedSubnetRoute<A>* chained_rt, *first_rt;
    first_rt = chained_rt = _current_chain->second;
    while (1) {
	// Replace the route with itself.  This will cause filters to
	// be re-applied, and decision to re-evaluate the route.
	InternalMessage<A> old_rt_msg(chained_rt, _peer, _genid);
	InternalMessage<A> new_rt_msg(chained_rt, _peer, _genid);

	//we used to send this as a replace route, but replacing a
	//route with itself isn't safe in terms of preserving the
	//flags.
	log("push next changed nexthop: " + old_rt_msg.net().str());
	this->_next_table->delete_route(old_rt_msg, (BGPRouteTable<A>*)this);
	this->_next_table->add_route(new_rt_msg, (BGPRouteTable<A>*)this);

	if (chained_rt->next() == first_rt) {
	    debug_msg("end of chain\n");
	    break;
	} else {
	    debug_msg("chain continues\n");
	}
	chained_rt = chained_rt->next();
    }
    debug_msg("****RibIn Sending push***\n");
    this->_next_table->push((BGPRouteTable<A>*)this);

    next_chain();

    if (_nexthop_push_active == false)
	return false;

    return true;
}


template<class A>
void
RibInTable<A>::deletion_nexthop_check(const SubnetRoute<A>* route)
{
    // checks to make sure that the deletion doesn't make
    // _next_route_to_push invalid
    if (!_nexthop_push_active)
	return;
    const ChainedSubnetRoute<A>* next_route_to_push = _current_chain->second;
    if (*route == *next_route_to_push) {
	if (next_route_to_push == next_route_to_push->next()) {
	    // this is the last route in the chain, so we need to bump
	    // the iterator before we delete it.
	    next_chain();
	}
    }
}

template<class A>
void
RibInTable<A>::next_chain()
{
    _current_chain++;
    if (_current_chain != _route_table->pathmap().end()) {
	PAListRef<A> pa_list =_current_chain->first;
	FPAListRef fpa_list = new FastPathAttributeList<A>(pa_list);
	XLOG_ASSERT(fpa_list->nexthop_att() );
	if (fpa_list->nexthop() == _current_changed_nexthop) {
	    // there's another chain with the same nexthop
	    return;
	}
    }

    while (1) {
	// that's it for this nexthop - try the next
	if (_changed_nexthops.empty()) {
	    // no more nexthops to push
	    _nexthop_push_active = false;
	    return;
	}

	typename set <A>::iterator i = _changed_nexthops.begin();
	_current_changed_nexthop = *i;
	_changed_nexthops.erase(i);

	FPAListRef dummy_fpa_list = new FastPathAttributeList<A>();
	NextHopAttribute<A> nh_att(_current_changed_nexthop);
	dummy_fpa_list->add_path_attribute(nh_att);
	dummy_fpa_list->canonicalize();
	PAListRef<A> dummy_pa_list = new PathAttributeList<A>(dummy_fpa_list);

	typename BgpTrie<A>::PathmapType::const_iterator pmi;

	pmi = _route_table->pathmap().lower_bound(dummy_pa_list);
	if (pmi == _route_table->pathmap().end()) {
	    // no route in this trie has this Nexthop, try the next nexthop
	    continue;
	}
	PAListRef<A> pa_list = pmi->first;
	FPAListRef fpa_list = new FastPathAttributeList<A>(pa_list);
	if (fpa_list->nexthop() != _current_changed_nexthop) {
	    // no route in this trie has this Nexthop, try the next nexthop
	    continue;
	}

	_current_chain = pmi;
	return;
    }
}

template<class A>
void
RibInTable<A>::stop_nexthop_push()
{
    // When a peering goes down tear out all the nexthop change state

    _changed_nexthops.clear();
    _nexthop_push_active = false;
    _current_changed_nexthop = A::ZERO();
//     _current_chain = _route_table->pathmap().end();
    _push_task.unschedule();
}

template<class A>
string
RibInTable<A>::dump_state() const {
    string s;
    s  = "=================================================================\n";
    s += "RibInTable\n";
    s += str() + "\n";
    s += "=================================================================\n";
    if (_peer_is_up)
	s += "Peer is UP\n";
    else
	s += "Peer is DOWN\n";
    s += _route_table->str();
    s += CrashDumper::dump_state();  
    return s;
} 

template<class A>
EventLoop& 
RibInTable<A>::eventloop() const 
{
    return _peer->eventloop();
}


template class RibInTable<IPv4>;
template class RibInTable<IPv6>;
