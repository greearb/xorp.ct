// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/route_table_nhlookup.cc,v 1.7 2003/05/23 00:02:07 mjh Exp $"

#include "route_table_nhlookup.hh"

template <class A>
MessageQueueEntry<A>::MessageQueueEntry(const InternalMessage<A>* add_msg,
					const InternalMessage<A>* delete_msg) :
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
MessageQueueEntry<A>::copy_in(const InternalMessage<A>* add_msg,
			      const InternalMessage<A>* delete_msg) 
{
    /* Note: this all depends on _added_route_ref and
       _deleted_route_ref, whose pupose is to maintain the reference
       count on the SubnetRoutes from the add and delete message, so
       that the original won't go away before we've finished using it */

    assert(add_msg != NULL);
    debug_msg("MessageQueueEntry: add_msg: %p\n%s\n", add_msg, add_msg->str().c_str());

    // Copy the add_msg.  We can't assume it will still be around.
    _add_msg = new InternalMessage<A>(add_msg->route(),
				      add_msg->origin_peer(),
				      add_msg->genid());
    // changed must be false - we don't store new routes here, so the
    // plumbing has to ensure that there's a cache upstream.
    assert(add_msg->changed() == false);

    if (delete_msg == NULL) {
	_delete_msg = NULL;
    } else {
	_delete_msg = new InternalMessage<A>(delete_msg->route(),
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
				NextHopResolver<A>* next_hop_resolver,
				BGPRouteTable<A> *parent)
    : BGPRouteTable<A>(tablename)
{
    _parent = parent;
    _next_hop_resolver = next_hop_resolver;
}

template <class A>
int
NhLookupTable<A>::add_route(const InternalMessage<A> &rtmsg,
			    BGPRouteTable<A> *caller) 
{
    assert(caller == _parent);

    if (_next_hop_resolver->register_nexthop(rtmsg.route()->nexthop(),
					     rtmsg.net(), this)) {
	return _next_table->add_route(rtmsg, this);
    }

    // we need to queue the add, pending nexthop resolution
    typename RefTrie<A, const MessageQueueEntry<A> >::iterator inserted;
    inserted = _queue_by_net.insert(rtmsg.net(),
				    MessageQueueEntry<A>(&rtmsg, NULL));
    const MessageQueueEntry<A>* mqe = &(inserted.payload());
    _queue_by_nexthop.insert(make_pair(rtmsg.nexthop(), mqe));

    // we don't know if it will ultimately be used, so err on the safe
    // side
    return ADD_USED;
}

template <class A>
int
NhLookupTable<A>::replace_route(const InternalMessage<A> &old_rtmsg,
				const InternalMessage<A> &new_rtmsg,
				BGPRouteTable<A> *caller) 
{
    assert(caller == _parent);
    debug_msg("NhLookupTable::replace_route\n");

    IPNet<A> net = new_rtmsg.net();

    // Are we still waiting for the old_rtmsg to resolve?
    bool old_msg_is_queued;
    const MessageQueueEntry<A>* mqe = NULL;
    typename RefTrie<A, const MessageQueueEntry<A> >::iterator i;
    i = _queue_by_net.lookup_node(net);
    if (i == _queue_by_net.end()) {
	old_msg_is_queued = false;
    } else {
	old_msg_is_queued = true;
	mqe = &(i.payload());
    }

    _next_hop_resolver->deregister_nexthop(old_rtmsg.route()->nexthop(),
					   old_rtmsg.net(), this);

    bool new_msg_needs_queuing;
    if (_next_hop_resolver->register_nexthop(new_rtmsg.nexthop(),
					     new_rtmsg.net(), this))
	new_msg_needs_queuing = false;
    else
	new_msg_needs_queuing = true;


    const InternalMessage<A>* real_old_msg = &old_rtmsg;
    bool propagate_as_add = false;
    if (old_msg_is_queued) {
	// there was an entry for this net in our queue awaiting
	// resolution of it's nexthop
	if (mqe->type() == MessageQueueEntry<A>::REPLACE) {
	    // preserve the old delete message and route
	    SubnetRoute<A>* preserve_route
		= new SubnetRoute<A>(*(mqe->deleted_route()));
	    InternalMessage<A>* preserve_msg
		= new InternalMessage<A>(preserve_route,
					 mqe->delete_msg()->origin_peer(),
					 mqe->delete_msg()->genid());
	    if (mqe->delete_msg()->changed())
	    	preserve_msg->set_changed();
	    real_old_msg = preserve_msg;
	} else if (mqe->type() == MessageQueueEntry<A>::ADD) {
	    // there was an ADD queued.  No-one downstream heard this
	    // add, so our REPLACE has now become an ADD
	    propagate_as_add = true;
	}

	// we can now remove the old queue entry, because it's no longer
	// needed
	A original_nexthop = mqe->added_route()->nexthop();
	typename multimap <A, const MessageQueueEntry<A>*>::iterator nh_iter
	    = _queue_by_nexthop.find(original_nexthop);
	while (nh_iter->second->net() != net) {
	    nh_iter++;
	    assert(nh_iter != _queue_by_nexthop.end());
	}
	assert(nh_iter->first == original_nexthop);
	_queue_by_nexthop.erase(nh_iter);
	_queue_by_net.erase(i);
    }

    if (new_msg_needs_queuing) {
	typename RefTrie<A, const MessageQueueEntry<A> >::iterator inserted;
	if (propagate_as_add) {
	    inserted
		= _queue_by_net.insert(net,
				       MessageQueueEntry<A>(&new_rtmsg,
							    NULL));
	} else {
	    inserted
		= _queue_by_net.insert(net,
				       MessageQueueEntry<A>(&new_rtmsg,
							    real_old_msg));
	}
	const MessageQueueEntry<A>* mqe = &(inserted.payload());
	_queue_by_nexthop.insert(make_pair(new_rtmsg.nexthop(), mqe));
	if (real_old_msg != &old_rtmsg) {
	    delete real_old_msg;
	}
	return ADD_USED;
    } else {
	bool success;
	if (propagate_as_add) {
	    success = _next_table->add_route(new_rtmsg, this);
	} else {
	    success = _next_table->replace_route(*real_old_msg,
						 new_rtmsg, this);
	}
	if (real_old_msg != &old_rtmsg) {
	    delete real_old_msg;
	}
	return success;
    }
}

template <class A>
int
NhLookupTable<A>::delete_route(const InternalMessage<A> &rtmsg,
			       BGPRouteTable<A> *caller) 
{
    assert(caller == _parent);
    IPNet<A> net = rtmsg.net();

    // Are we still waiting for the old_rtmsg to resolve?
    bool msg_is_queued;
    const MessageQueueEntry<A>* mqe = NULL;
    typename RefTrie<A, const MessageQueueEntry<A> >::iterator i;
    i = _queue_by_net.lookup_node(net);
    if (i == _queue_by_net.end()) {
	msg_is_queued = false;
    } else {
	msg_is_queued = true;
	mqe = &(i.payload());
    }

    _next_hop_resolver->deregister_nexthop(rtmsg.route()->nexthop(),
					   rtmsg.net(), this);

    const InternalMessage<A>* real_msg = &rtmsg;
    if (msg_is_queued == true) {
	// there was an entry for this net in our queue awaiting
	// resolution of it's nexthop

	bool dont_send_delete;
	if (mqe->type() == MessageQueueEntry<A>::REPLACE) {
	    // preserve the old delete message
	    InternalMessage<A>* preserve_msg
		= new InternalMessage<A>(mqe->delete_msg()->route(),
					 mqe->delete_msg()->origin_peer(),
					 mqe->delete_msg()->genid());
	    if (mqe->delete_msg()->changed())
		preserve_msg->set_changed();
	    real_msg = preserve_msg;
	    dont_send_delete = false;
	} else if (mqe->type() == MessageQueueEntry<A>::ADD) {
	    dont_send_delete = true;
	}

	// we can now remove the old queue entry, because it's no longer
	// needed
	A original_nexthop = mqe->added_route()->nexthop();
	typename multimap <A, const MessageQueueEntry<A>*>::iterator nh_iter
	    = _queue_by_nexthop.find(original_nexthop);
	while (nh_iter->second->net() != net) {
	    nh_iter++;
	    assert(nh_iter != _queue_by_nexthop.end());
	}
	assert(nh_iter->first == original_nexthop);
	_queue_by_nexthop.erase(nh_iter);
	_queue_by_net.erase(i);

	if (dont_send_delete) {
	    // there was an ADD in the queue - we just dequeued it, and
	    // don't need to propagate the delete further
	    return 0;
	}
    }

    bool success = _next_table->delete_route(*real_msg, this);
    if (real_msg != &rtmsg)
	delete real_msg;
    return success;
}

template <class A>
int
NhLookupTable<A>::push(BGPRouteTable<A> *caller) 
{
    assert(caller == _parent);

    // Always propagate a push - we'll add new pushes each time a
    // nexthop resolves.
    return _next_table->push(this);
}

template <class A>
const SubnetRoute<A> *
NhLookupTable<A>::lookup_route(const IPNet<A> &net) const 
{
    // Are we still waiting for the old_rtmsg to resolve?
    const MessageQueueEntry<A>* mqe = NULL;
    typename RefTrie<A, const MessageQueueEntry<A> >::iterator i;
    i = _queue_by_net.lookup_node(net);
    if (i == _queue_by_net.end()) {
	return _parent->lookup_route(net);
    } else {
	// we found an entry in the lookup pool
	mqe = &(i.payload());
    }

    switch (mqe->type()) {
    case MessageQueueEntry<A>::ADD:
	// although there is a route, we don't know the true nexthop
	// yet, so we act as though we don't know the answer
	return NULL;
    case MessageQueueEntry<A>::REPLACE:
	// although there is a route, we don't know the true nexthop
	// for it yet, so we act as though we only know the old answer
	return mqe->deleted_route();
    }
    XLOG_UNREACHABLE();
}

template <class A>
void
NhLookupTable<A>::route_used(const SubnetRoute<A>* route, bool in_use) 
{
    _parent->route_used(route, in_use);
}

template <class A>
void
NhLookupTable<A>::RIB_lookup_done(const A& nexthop,
				  const set <IPNet<A> >& nets,
				  bool /*lookup_succeeded*/) 
{
    typename multimap <A, const MessageQueueEntry<A>*>::iterator nh_iter, nh_iter2;
    nh_iter = _queue_by_nexthop.find(nexthop);
    while ((nh_iter != _queue_by_nexthop.end())
	   && (nh_iter->first == nexthop)) {
	const MessageQueueEntry<A>* mqe = nh_iter->second;
	switch (mqe->type()) {
	case MessageQueueEntry<A>::ADD:
	    _next_table->add_route(*(mqe->add_msg()), this);
	    break;
	case MessageQueueEntry<A>::REPLACE:
	    _next_table->replace_route(*(mqe->delete_msg()),
				       *(mqe->add_msg()), this);
	    break;
	}
	nh_iter2 = nh_iter;
	nh_iter++;
	_queue_by_nexthop.erase(nh_iter2);
    }

    typename set <IPNet<A> >::const_iterator net_iter;
    for (net_iter = nets.begin(); net_iter != nets.end(); net_iter++) {
	_queue_by_net.erase(*net_iter);
    }

    // force a push because the original push may be long gone
    _next_table->push(this);
}

template <class A>
string
NhLookupTable<A>::str() const 
{
    string s = "NhLookupTable<A>" + tablename();
    return s;
}

template class NhLookupTable<IPv4>;
template class NhLookupTable<IPv6>;
