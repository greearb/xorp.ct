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

#ident "$XORP: xorp/bgp/route_table_ribin.cc,v 1.2 2002/12/14 00:51:07 mjh Exp $"

//#define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "route_table_ribin.hh"
#include "route_table_deletion.hh"
#include "rib_ipc_handler.hh"


template<class A>
BGPRibInTable<A>::BGPRibInTable(string table_name, const PeerHandler *peer)
    : BGPRouteTable<A>("BGPRibInTable-" + table_name), _peer(peer)
{
    _route_table = new BgpTrie<A>;
    _peer_is_up = true;
    _genid = 1; /*zero is not a valid genid*/
    _table_version = 1;
    _parent = NULL;

    _nexthop_push_active = false;
}

template<class A>
BGPRibInTable<A>::~BGPRibInTable()
{
    delete _route_table;
}

template<class A>
void
BGPRibInTable<A>::peering_went_down()
{
    _peer_is_up = false;

    /* When the peering goes down we unhook our entire RibIn routing
       table and give it to a new deletion table.  We plumb the
       deletion table in after the RibIn, and it can then delete the
       routes as a background task while filtering the routes we give
       it so that the later tables get the correct deletes at the
       correct time. This means that we don't have to concern
       ourselves with holding deleted and new routes if the peer comes
       back up before we've finished the background deletion process -
       credits to Atanu Ghosh for this neat idea */

    string tablename = "Deleted" + _tablename;

    BGPDeletionTable<A>* deletion_table =
	new BGPDeletionTable<A>(tablename, _route_table, _peer, _genid, this);

    _route_table = new BgpTrie<A>;

    deletion_table->set_next_table(_next_table);
    _next_table->set_parent(deletion_table);
    _next_table = deletion_table;

    deletion_table->initiate_background_deletion();
    _next_table->peering_went_down(_peer, _genid, this);
}

template<class A>
void
BGPRibInTable<A>::peering_came_up()
{
    _peer_is_up = true;
    _genid++;

    // cope with wrapping genid without using zero which is reserved
    if (_genid == 0) {
	_genid = 1;
    }

    _table_version = 1;
}

template<class A>
int
BGPRibInTable<A>::add_route(const InternalMessage<A> &rtmsg,
			    BGPRouteTable<A> *caller)
{
    const ChainedSubnetRoute<A> *new_route;
    const SubnetRoute<A> *existing_route;
    assert(caller == NULL);
    assert(rtmsg.origin_peer() == _peer);
    assert(_peer_is_up);
    assert(_next_table != NULL);

    existing_route = lookup_route(rtmsg.net());
    int response;
    if (existing_route != NULL) {
	assert(existing_route->net() == rtmsg.net());
#if 0
	if (rtmsg.route()->attributes() == existing_route->attributes()) {
	    // this route is the same as before.
	    // no need to do anything.
	    return ADD_UNUSED;
	}
#endif
	// preserve the information
	SubnetRoute<A> route_copy(*existing_route);
	deletion_nexthop_check(existing_route);

	// delete from the Trie
	_route_table->erase(rtmsg.net());
	_table_version++;

	InternalMessage<A> old_rt_msg(&route_copy, _peer, _genid);

	// Store it locally.  The BgpTrie will copy it into a ChainedSubnetRoute
	BgpTrie<A>::iterator iter =
	    _route_table->insert(rtmsg.net(), *(rtmsg.route()));
	new_route = &(iter.payload());

	// progogate downstream
	InternalMessage<A> new_rt_msg(new_route, _peer, _genid);
	if (rtmsg.push()) new_rt_msg.set_push();

	response = _next_table->replace_route(old_rt_msg, new_rt_msg,
					      (BGPRouteTable<A>*)this);
    } else {
	// Store it locally.  The BgpTrie will copy it into a ChainedSubnetRoute
	BgpTrie<A>::iterator iter =
	    _route_table->insert(rtmsg.net(), *(rtmsg.route()));
	new_route = &(iter.payload());

	// progogate downstream
	InternalMessage<A> new_rt_msg(new_route, _peer, _genid);
	if (rtmsg.push()) new_rt_msg.set_push();
	response = _next_table->add_route(new_rt_msg, (BGPRouteTable<A>*)this);
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
BGPRibInTable<A>::delete_route(const InternalMessage<A> &rtmsg,
			       BGPRouteTable<A> *caller)
{
    assert(caller == NULL);
    assert(rtmsg.origin_peer() == _peer);
    assert(_peer_is_up);

    const SubnetRoute<A> *existing_route;
    existing_route = lookup_route(rtmsg.net());

    if (existing_route != NULL) {
	// preserve the information
	SubnetRoute<A> route_copy(*existing_route);
	deletion_nexthop_check(existing_route);

	// remove from the Trie
	_route_table->erase(rtmsg.net());
	_table_version++;

	// propogate downstream
	InternalMessage<A> old_rt_msg(&route_copy, _peer, _genid);
	if (rtmsg.push()) old_rt_msg.set_push();
	if (_next_table != NULL)
	    _next_table->delete_route(old_rt_msg, (BGPRouteTable<A>*)this);
    } else {
	// we received a delete, but didn't have anything to delete.
	// It's debatable whether we should silently ignore this, or
	// drop the peering.  If we don't hold input-filtered routes in
	// the RIB-In, then this would be commonplace, so we'd have to
	// silently ignore it.  Currently (Sept 2002) we do still hold
	// filtered routes in the RIB-In, so this should be an error.
	// But we'll just ignore this error, and log a warning.
	string s = "Attempt to delete route for net " + rtmsg.net().str()
	    + " that wasn't in RIB-In\n";
	XLOG_WARNING(s.c_str());
	return -1;
    }
    return 0;
}

template<class A>
int
BGPRibInTable<A>::push(BGPRouteTable<A> *caller)
{
    debug_msg("BGPRibInTable<A>::push\n");
    assert(caller == NULL);
    assert(_peer_is_up);
    assert(_next_table != NULL);

    return _next_table->push((BGPRouteTable<A>*)this);
}

template<class A>
const SubnetRoute<A>*
BGPRibInTable<A>::lookup_route(const IPNet<A> &net) const
{
    if (_peer_is_up == false)
	return NULL;

    BgpTrie<A>::iterator iter = _route_table->lookup_node(net);
    if (iter != _route_table->end()) {
	// assert(iter.payload().net() == net);
	return &(iter.payload());
    } else
	return NULL;
}

template<class A>
void
BGPRibInTable<A>::route_used(const SubnetRoute<A>* used_route,
			     bool in_use)
{
    // we look this up rather than modify used_route itself because
    // it's possible that used_route originates the other side of a
    // cache, and modifying it might not modify the version we have
    // here in the RibIn
    if (_peer_is_up == false)
	return;
    const SubnetRoute<A> *rt;
    rt = lookup_route(used_route->net());
    if (rt == NULL) abort();
    rt->set_in_use(in_use);
}

template<class A>
string
BGPRibInTable<A>::str() const
{
    string s = "BGPRibInTable<A>" + tablename();
    return s;
}

template<class A>
bool
BGPRibInTable<A>::dump_next_route(DumpIterator<A>& dump_iter)
{
    BgpTrie<A>::iterator route_iterator;
    debug_msg("dump_next_route\n");

    if (dump_iter.route_iterator_is_valid()) {
	debug_msg("route_iterator is valid\n");
	if (_table_version == dump_iter.rib_version()) {
	    debug_msg("no change to RIB\n");
	    // no deletions have occured since we were last here so the
	    // chain iterator is still valid
	    route_iterator = dump_iter.route_iterator();
	} else {
	    debug_msg("RIB changed from version %d to %d\n",
		   dump_iter.rib_version(), _table_version);
	    // deletions have occured, so we don't know for sure that
	    // the chain iterator is still valid - look it up again in
	    // the pathmap to be safe
	    route_iterator = _route_table->lower_bound(dump_iter.net());
	    dump_iter.set_rib_version(_table_version);
	}
    } else {
	debug_msg("route_iterator is not valid\n");
	route_iterator = _route_table->begin();
	dump_iter.set_rib_version(_table_version);
    }

    if (route_iterator == _route_table->end()) {
	return false;
    }
    const ChainedSubnetRoute<A>* chained_rt;
    while (route_iterator != _route_table->end()) {
	chained_rt = &(route_iterator.payload());
	route_iterator++;
	debug_msg("chained_rt: %s\n", chained_rt->str().c_str());
	// propagate downstream
	if (!chained_rt->is_filtered()) {
	    // route is not filtered
	    InternalMessage<A> rt_msg(chained_rt, _peer, _genid);
	    rt_msg.set_push();
	    _next_table->route_dump(rt_msg, (BGPRouteTable<A>*)this,
				    dump_iter.peer_to_dump_to());
	    break;
	}
    }
    dump_iter.set_route_iterator(route_iterator);
    if (route_iterator == _route_table->end())
	return false;
    dump_iter.set_route_iterator_net(route_iterator.payload().net());
    return true;
}

template<class A>
void
BGPRibInTable<A>::igp_nexthop_changed(const A& bgp_nexthop)
{
    debug_msg("igp_nexthop_changed for bgp_nexthop %s on table %s\n",
	   bgp_nexthop.str().c_str(), _tablename.c_str());

    set <A>::const_iterator i;
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
	PathAttributeList<A> dummy_pa_list;
	NextHopAttribute<A> nh_att(bgp_nexthop);
	dummy_pa_list.add_path_attribute(nh_att);
	dummy_pa_list.rehash();
	BgpTrie<A>::PathmapType::const_iterator pmi;
#ifdef NOTDEF
	pmi = _route_table->pathmap().begin();
	while (pmi != _route_table->pathmap().end()) {
	    debug_msg("Route: %s\n", pmi->second->str().c_str());
	    pmi++;
	}
#endif

	pmi = _route_table->pathmap().lower_bound(&dummy_pa_list);
	if (pmi == _route_table->pathmap().end()) {
	    // no route in this trie has this Nexthop
	    debug_msg("no matching routes - do nothing\n");
	    return;
	}
	if (pmi->second->nexthop() != bgp_nexthop) {
	    debug_msg("no matching routes (2)- do nothing\n");
	    return;
	}
	_current_changed_nexthop = bgp_nexthop;
	_nexthop_push_active = true;
	_current_chain = pmi;
	const SubnetRoute<A>* next_route_to_push = _current_chain->second;
	debug_msg("Found route with nexthop %s:\n%s\n",
		  bgp_nexthop.str().c_str(),
		  next_route_to_push->str().c_str());

	// _next_table->push((BGPRouteTable<A>*)this);
	_push_timer = get_eventloop()->
	    new_oneoff_after_ms(0 /*call back immediately, but after
				    network events or expired timers */,
				callback(this,
					 &BGPRibInTable<A>::push_next_changed_nexthop));
    }
}

template<class A>
void
BGPRibInTable<A>::push_next_changed_nexthop()
{
    const ChainedSubnetRoute<A>* chained_rt, *first_rt;
    first_rt = chained_rt = _current_chain->second;
    while (1) {
	// Replace the route with itself.  This will cause filters to
	// be re-applied, and decision to re-evaluate the route.
	InternalMessage<A> old_rt_msg(chained_rt, _peer, _genid);
	InternalMessage<A> new_rt_msg(chained_rt, _peer, _genid);

	_next_table->replace_route(old_rt_msg,
				   new_rt_msg, (BGPRouteTable<A>*)this);

	if (chained_rt->next() == first_rt) {
	    debug_msg("end of chain\n");
	    break;
	} else {
	    debug_msg("chain continues\n");
	}
	chained_rt = chained_rt->next();
    }
    debug_msg("****RibIn Sending push***\n");
    _next_table->push((BGPRouteTable<A>*)this);

    next_chain();

    if (_nexthop_push_active == false)
	return;

    _push_timer = get_eventloop()->
	new_oneoff_after_ms(0 /*call back immediately, but after
				network events or expired timers */,
			    callback(this,
				     &BGPRibInTable<A>::push_next_changed_nexthop));
}


template<class A>
void
BGPRibInTable<A>::deletion_nexthop_check(const SubnetRoute<A>* route)
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
BGPRibInTable<A>::next_chain()
{
    _current_chain++;
    if (_current_chain != _route_table->pathmap().end()
	&& _current_chain->first->nexthop() == _current_changed_nexthop) {
	// there's another chain with the same nexthop
	return;
    }

    // that's it for this nexthop - try the next
    if (_changed_nexthops.empty()) {
	// no more nexthops to push
	_nexthop_push_active = false;
	return;
    }

    set <A>::iterator i = _changed_nexthops.begin();
    _current_changed_nexthop = *i;
    _changed_nexthops.erase(i);

    PathAttributeList<A> dummy_pa_list;
    NextHopAttribute<A> nh_att(_current_changed_nexthop);
    dummy_pa_list.add_path_attribute(nh_att);
    dummy_pa_list.rehash();
    BgpTrie<A>::PathmapType::const_iterator pmi;

    pmi = _route_table->pathmap().lower_bound(&dummy_pa_list);
    if (pmi == _route_table->pathmap().end()) {
	// no route in this trie has this Nexthop, try the next nexthop
	next_chain();
	return;
    }
    if (pmi->second->nexthop() != _current_changed_nexthop) {
	// no route in this trie has this Nexthop, try the next nexthop
	next_chain();
	return;
    }

    _current_chain = pmi;
}

template class BGPRibInTable<IPv4>;
template class BGPRibInTable<IPv6>;











