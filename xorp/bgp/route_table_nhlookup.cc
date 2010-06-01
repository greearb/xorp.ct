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



// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "route_table_nhlookup.hh"

template <class A>
MessageQueueEntry<A>::MessageQueueEntry(InternalMessage<A>* add_msg,
					InternalMessage<A>* delete_msg) :
    _added_route_ref(add_msg->route()), 
    _deleted_route_ref(delete_msg ? delete_msg->route() : NULL)
{
    copy_in(add_msg, delete_msg);
}

template <class A>
MessageQueueEntry<A>::MessageQueueEntry(const MessageQueueEntry<A>& them) :
    _added_route_ref(them.add_msg()->route()),
    _deleted_route_ref(them.delete_msg() ? them.delete_msg()->route() : NULL)
{ 
    copy_in(them.add_msg(), them.delete_msg());
}

template <class A>
void
MessageQueueEntry<A>::copy_in(InternalMessage<A>* add_msg,
			      InternalMessage<A>* delete_msg) 
{
    /* Note: this all depends on _added_route_ref and
       _deleted_route_ref, whose pupose is to maintain the reference
       count on the SubnetRoutes from the add and delete message, so
       that the original won't go away before we've finished using it */

    XLOG_ASSERT(add_msg != NULL);
    debug_msg("MessageQueueEntry: add_msg: %p\n%s\n", add_msg, add_msg->str().c_str());

    // Copy the add_msg.  We can't assume it will still be around.
    _add_msg = new InternalMessage<A>(add_msg->route(),
				      add_msg->attributes(),
				      add_msg->origin_peer(),
				      add_msg->genid());
    // changed must be false - we don't store new routes here, so the
    // plumbing has to ensure that there's a cache upstream.
    XLOG_ASSERT(add_msg->copied() == false);

    if (delete_msg == NULL) {
	_delete_msg = NULL;
    } else {
	_delete_msg = new InternalMessage<A>(delete_msg->route(),
					     delete_msg->attributes(),
					     delete_msg->origin_peer(),
					     delete_msg->genid());
    }
}

template <class A>
MessageQueueEntry<A>::~MessageQueueEntry() 
{
    delete _add_msg;
    if (_delete_msg != NULL) {
	delete _delete_msg;
    }
}

template <class A>
string
MessageQueueEntry<A>::str() const 
{
    string s;
    s += c_format("add_msg: %p\n", _add_msg);
    s += c_format("delete_msg: %p\n", _delete_msg);
    return s;
}

template <class A>
NhLookupTable<A>::NhLookupTable(string tablename,
				Safi safi,
				NextHopResolver<A>* next_hop_resolver,
				BGPRouteTable<A> *parent)
    : BGPRouteTable<A>(tablename, safi)
{
    this->_parent = parent;
    _next_hop_resolver = next_hop_resolver;
}

template <class A>
int
NhLookupTable<A>::add_route(InternalMessage<A> &rtmsg,
			    BGPRouteTable<A> *caller) 
{
    debug_msg("\n         %s\n caller: %s\n rtmsg: %p route: %p\n%s\n",
	      this->tablename().c_str(),
	      caller ? caller->tablename().c_str() : "NULL",
	      &rtmsg,
	      rtmsg.route(),
	      rtmsg.str().c_str());

    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(0 == lookup_in_queue(rtmsg.nexthop(), rtmsg.net()));

    debug_msg("register_nexthop %s %s\n", cstring(rtmsg.nexthop()),
	      cstring(rtmsg.net()));
    if (_next_hop_resolver->register_nexthop(rtmsg.nexthop(), rtmsg.net(),
					     this)) {
	bool resolvable;
	uint32_t metric;
	_next_hop_resolver->lookup(rtmsg.nexthop(), resolvable, metric);
	rtmsg.route()->set_nexthop_resolved(resolvable);
	return this->_next_table->add_route(rtmsg, this);
    }

    add_to_queue(rtmsg.nexthop(), rtmsg.net(), &rtmsg, NULL);

    // we don't know if it will ultimately be used, so err on the safe
    // side
    return ADD_USED;
}

template <class A>
int
NhLookupTable<A>::replace_route(InternalMessage<A> &old_rtmsg,
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
	      caller == 0 ? "NULL" : caller->tablename().c_str(),
	      &old_rtmsg,
	      &new_rtmsg,
	      old_rtmsg.route(),
	      new_rtmsg.route(),
	      old_rtmsg.str().c_str(),
	      new_rtmsg.str().c_str());

    XLOG_ASSERT(caller == this->_parent);
    debug_msg("NhLookupTable::replace_route\n");

    IPNet<A> net = new_rtmsg.net();

    // Are we still waiting for the old_rtmsg to resolve?
    bool old_msg_is_queued;
    MessageQueueEntry<A>* mqe =
	lookup_in_queue(old_rtmsg.nexthop(), net);
    old_msg_is_queued = (mqe != NULL);

    // The correct behaviour is to deregister interest in this
    // nexthop. If the nexthop for the old route is the same as in the
    // new route then we may have done a horrible thing. We have told
    // the resolver that we are no longer interested in this nexthop,
    // if this is the last reference we will remove all state related
    // to this nexthop. The next thing we do is register interest in
    // this nexthop which will not resolve, thus requiring us to
    // queue the request. Not deregistering saves the extra queing.
    //
    // If the nexthops of the old and new route are different we need
    // to keep the old nexthop resolution around so that lookups can
    // be satisfied.
    //

//     _next_hop_resolver->deregister_nexthop(old_rtmsg.route()->nexthop(),
// 					   old_rtmsg.net(), this);

    bool new_msg_needs_queuing;
    debug_msg("register_nexthop %s %s\n", cstring(new_rtmsg.nexthop()),
	      cstring(new_rtmsg.net()));
    if (_next_hop_resolver->register_nexthop(new_rtmsg.nexthop(),
					     new_rtmsg.net(), this)) {
	new_msg_needs_queuing = false;
	bool resolvable = false;
	uint32_t metric;
	_next_hop_resolver->lookup(new_rtmsg.nexthop(), resolvable, metric);
	new_rtmsg.route()->set_nexthop_resolved(resolvable);
    } else {
	new_msg_needs_queuing = true;
    }

    debug_msg("need queuing %s\n", bool_c_str(new_msg_needs_queuing));

    InternalMessage<A>* real_old_msg = &old_rtmsg;
    SubnetRoute<A>* preserve_route = NULL;
    bool propagate_as_add = false;
    if (old_msg_is_queued) {
	// there was an entry for this net in our queue awaiting
	// resolution of it's nexthop
	if (mqe->type() == MessageQueueEntry<A>::REPLACE) {
	    // preserve the old delete message and route
	    preserve_route = new SubnetRoute<A>(*(mqe->deleted_route()));
	    InternalMessage<A>* preserve_msg
		= new InternalMessage<A>(preserve_route,
					 mqe->delete_msg()->attributes(),
					 mqe->delete_msg()->origin_peer(),
					 mqe->delete_msg()->genid());
#if 0
	    // re-enable me!
	    if (mqe->delete_msg()->changed())
	    	preserve_msg->set_changed();
#endif
	    if (mqe->delete_msg()->copied())
	    	preserve_msg->set_copied();
	    real_old_msg = preserve_msg;
	} else if (mqe->type() == MessageQueueEntry<A>::ADD) {
	    // there was an ADD queued.  No-one downstream heard this
	    // add, so our REPLACE has now become an ADD
	    propagate_as_add = true;
	}

	// we can now remove the old queue entry, because it's no longer
	// needed
	remove_from_queue(mqe->add_msg()->nexthop(), net);
    }

    bool deregister = true;
    int retval;
    if (new_msg_needs_queuing) {
	if (propagate_as_add) {
	    add_to_queue(new_rtmsg.nexthop(), net, &new_rtmsg, NULL);
	} else {
	    add_to_queue(new_rtmsg.nexthop(), net, &new_rtmsg, real_old_msg);
	    deregister = false;
	}
	if (real_old_msg != &old_rtmsg) {
	    delete real_old_msg;
	    preserve_route->unref();
	}
	retval = ADD_USED;
    } else {
	bool success;
	if (propagate_as_add) {
	    success = this->_next_table->add_route(new_rtmsg, this);
	} else {
	    success = this->_next_table->replace_route(*real_old_msg,
						 new_rtmsg, this);
	}
	if (real_old_msg != &old_rtmsg) {
	    delete real_old_msg;
	    preserve_route->unref();
	}
	retval = success;
    }

    if (deregister) {
	debug_msg("deregister_nexthop %s %s\n", cstring(old_rtmsg.nexthop()),
	      cstring(old_rtmsg.net()));
	_next_hop_resolver->deregister_nexthop(old_rtmsg.nexthop(),
					       old_rtmsg.net(), this);
    } else {
	debug_msg("Deferring deregistration\n");
    }

    return retval;
}

template <class A>
int
NhLookupTable<A>::delete_route(InternalMessage<A> &rtmsg,
			       BGPRouteTable<A> *caller) 
{
    debug_msg("\n         %s\n caller: %s\n rtmsg: %p route: %p\n%s\n",
	      this->tablename().c_str(),
	      caller ? caller->tablename().c_str() : "NULL",
	      &rtmsg,
	      rtmsg.route(),
	      rtmsg.str().c_str());

    XLOG_ASSERT(caller == this->_parent);
    IPNet<A> net = rtmsg.net();

    // Are we still waiting for the old_rtmsg to resolve?
    bool msg_is_queued;
    MessageQueueEntry<A>* mqe = lookup_in_queue(rtmsg.nexthop(), net);
    msg_is_queued = 0 != mqe;

    debug_msg("deregister_nexthop %s %s\n", cstring(rtmsg.nexthop()),
	      cstring(rtmsg.net()));
    _next_hop_resolver->deregister_nexthop(rtmsg.nexthop(), rtmsg.net(), this);

    InternalMessage<A>* real_msg = &rtmsg;
    if (msg_is_queued == true) {
	// there was an entry for this net in our queue awaiting
	// resolution of it's nexthop

	bool dont_send_delete = true;
	switch (mqe->type()) {
	case MessageQueueEntry<A>::REPLACE: {
	    // preserve the old delete message
	    InternalMessage<A>* preserve_msg
		= new InternalMessage<A>(mqe->delete_msg()->route(),
					 mqe->delete_msg()->attributes(),
					 mqe->delete_msg()->origin_peer(),
					 mqe->delete_msg()->genid());
#if 0
	    if (mqe->delete_msg()->changed())
		preserve_msg->set_changed();
#endif
	    if (mqe->delete_msg()->copied())
		preserve_msg->set_copied();
	    real_msg = preserve_msg;
	    dont_send_delete = false;
	    break;
	    }

	case MessageQueueEntry<A>::ADD:
	    dont_send_delete = true;
	    break;
	}

	if (dont_send_delete) {
	    // we can now remove the old queue entry, because it's no longer
	    // needed
	    remove_from_queue(mqe->add_msg()->nexthop(), net);
	    // there was an ADD in the queue - we just dequeued it, and
	    // don't need to propagate the delete further
	    return 0;
	}
    }

    bool success = this->_next_table->delete_route(*real_msg, this);
    if (real_msg != &rtmsg) {
	delete real_msg;
	// we can now remove the old queue entry, because it's no longer
	// needed
	remove_from_queue(mqe->add_msg()->nexthop(), net);
    }
    return success;
}

template <class A>
int
NhLookupTable<A>::push(BGPRouteTable<A> *caller) 
{
    XLOG_ASSERT(caller == this->_parent);

    // Always propagate a push - we'll add new pushes each time a
    // nexthop resolves.
    return this->_next_table->push(this);
}

template <class A>
const SubnetRoute<A> *
NhLookupTable<A>::lookup_route(const IPNet<A> &net, uint32_t& genid,
			       FPAListRef& pa_list) const 
{
    debug_msg("net: %s\n", cstring(net));

    // Are we still waiting for the old_rtmsg to resolve?
    const MessageQueueEntry<A>* mqe = lookup_in_queue(A::ZERO(), net);
    if (0 == mqe)
	return this->_parent->lookup_route(net, genid, pa_list);

    switch (mqe->type()) {
    case MessageQueueEntry<A>::ADD:
	debug_msg("ADD\n");
	// although there is a route, we don't know the true nexthop
	// yet, so we act as though we don't know the answer
	return NULL;
    case MessageQueueEntry<A>::REPLACE:
	debug_msg("********** JACKPOT **********\n");
	debug_msg("REPLACE\n");
	// although there is a route, we don't know the true nexthop
	// for it yet, so we act as though we only know the old answer.
	genid = mqe->delete_msg()->genid();
	pa_list = mqe->delete_msg()->attributes();
	return mqe->deleted_route();
    }
    XLOG_UNREACHABLE();
}

template <class A>
void
NhLookupTable<A>::route_used(const SubnetRoute<A>* route, bool in_use) 
{
    this->_parent->route_used(route, in_use);
}

template <class A>
void
NhLookupTable<A>::RIB_lookup_done(const A& nexthop,
				  const set <IPNet<A> >& nets,
				  bool lookup_succeeded) 
{
    typename set <IPNet<A> >::const_iterator net_iter;

    for (net_iter = nets.begin(); net_iter != nets.end(); net_iter++) {
	const MessageQueueEntry<A>* mqe = lookup_in_queue(nexthop, *net_iter);
	XLOG_ASSERT(0 != mqe);

	switch (mqe->type()) {
	case MessageQueueEntry<A>::ADD: {
	    mqe->add_msg()->route()->set_nexthop_resolved(lookup_succeeded);
	    this->_next_table->add_route(*(mqe->add_msg()), this);
	    break;
	}
	case MessageQueueEntry<A>::REPLACE:
            mqe->add_msg()->route()->set_nexthop_resolved(lookup_succeeded);
	    this->_next_table->replace_route(*(mqe->delete_msg()),
				       *(mqe->add_msg()), this);
	    // Perform the deferred deregistration.
	    debug_msg("Performing deferred deregistration\n");
	    debug_msg("deregister_nexthop %s %s\n",
		      cstring(mqe->deleted_attributes()->nexthop()),
		      cstring(mqe->delete_msg()->net()));
	    _next_hop_resolver->
		deregister_nexthop(mqe->deleted_attributes()->nexthop(),
				   mqe->delete_msg()->net(), this);
	    break;
	}

    }

    for (net_iter = nets.begin(); net_iter != nets.end(); net_iter++) {
	remove_from_queue(nexthop, *net_iter);
    }
    
    // force a push because the original push may be long gone
    this->_next_table->push(this);
}

template <class A>
void
NhLookupTable<A>::add_to_queue(const A& nexthop, const IPNet<A>& net,
			       InternalMessage<A>* new_msg,
			       InternalMessage<A>* old_msg)
{
    typename RefTrie<A, MessageQueueEntry<A> >::iterator inserted;
    inserted = _queue_by_net.insert(net, MessageQueueEntry<A>(new_msg, old_msg));
    MessageQueueEntry<A>* mqep = &(inserted.payload());
    _queue_by_nexthop.insert(make_pair(nexthop, mqep));
}

template <class A>
MessageQueueEntry<A> *
NhLookupTable<A>::lookup_in_queue(const A& nexthop, const IPNet<A>& net) const
{
    MessageQueueEntry<A>* mqe = NULL;
    typename RefTrie<A, MessageQueueEntry<A> >::iterator i;
    i = _queue_by_net.lookup_node(net);
    if (i != _queue_by_net.end()) {
	mqe = &(i.payload());
	if (A::ZERO() != nexthop)
	    XLOG_ASSERT(mqe->added_attributes()->nexthop() == nexthop);
    }

    return mqe;
}

template <class A>
void
NhLookupTable<A>::remove_from_queue(const A& nexthop, const IPNet<A>& net)
{
    // Find the queue entry in the queue by net
    MessageQueueEntry<A>* mqe;
    typename RefTrie<A, MessageQueueEntry<A> >::iterator net_iter;
    net_iter = _queue_by_net.lookup_node(net);
    XLOG_ASSERT(net_iter != _queue_by_net.end());
    mqe = &(net_iter.payload());

    // Find the queue entry in the queue by nexthop
    typename multimap <A, MessageQueueEntry<A>*>::iterator nh_iter
	= _queue_by_nexthop.find(nexthop);
    for (; nh_iter != _queue_by_nexthop.end(); nh_iter++)
	if (nh_iter->second->net() == net)
	    break;

    XLOG_ASSERT(nh_iter != _queue_by_nexthop.end());
    XLOG_ASSERT(nh_iter->first == nexthop);

    // Verify that both queues point at the same entry.
    XLOG_ASSERT(mqe == nh_iter->second);

    _queue_by_nexthop.erase(nh_iter);
    _queue_by_net.erase(net_iter);
}

template <class A>
string
NhLookupTable<A>::str() const 
{
    string s = "NhLookupTable<A>" + this->tablename();
    return s;
}

template class NhLookupTable<IPv4>;
template class NhLookupTable<IPv6>;
