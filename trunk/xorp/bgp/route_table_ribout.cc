// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the Xorp LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the Xorp LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/bgp/route_table_ribout.cc,v 1.1.1.1 2002/12/11 23:55:50 hodson Exp $"

//#define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "route_table_ribout.hh"
#include "peer_handler.hh"

template <class A>
struct TypeName {
    static const char* get() { return "Unknown"; }
};
template<> const char* TypeName<IPv4>::get() { return "IPv4"; }
template<> const char* TypeName<IPv6>::get() { return "IPv6"; }

template<class A>
BGPRibOutTable<A>::BGPRibOutTable(string table_name,
				  BGPRouteTable<A> *init_parent,
				  PeerHandler *peer)
    : BGPRouteTable<A>("BGPRibOutTable-" + table_name)
{
    _parent = init_parent;
    _peer = peer;
    _peer_busy = false;
    _upstream_queue_exists = false;
#ifdef UNNECESSARY
    if (_parent->type() != CACHE_TABLE) {
	fprintf(stderr, "Misconfiguration: a RibOutTable MUST be immediately \
preceded by a CacheTable to store routes that were modified by the final \
filterbank\n");
	abort();
    }
#endif
}

template<class A>
BGPRibOutTable<A>::~BGPRibOutTable() 
{
    print_queue(_queue);
    list<const RouteQueueEntry<A>*>::iterator i;
    i = _queue.begin();
    while (i != _queue.end()) {
	delete (*i);
	++i;
    }
}

template<class A>
void
BGPRibOutTable<A>::print_queue(const list<const RouteQueueEntry<A>*>& queue) 
    const 
{
#ifdef DEBUG_LOGGING
    debug_msg("Output Queue:\n");
    list<const RouteQueueEntry<A>*>::const_iterator i;
    i = queue.begin();
    while (i != queue.end()) {
	string s;
	switch ((*i)->op()) {
	case RTQUEUE_OP_ADD:
	    s = "ADD";
	    break;
	case RTQUEUE_OP_DELETE:
	    s = "DEL";
	    break;
	case RTQUEUE_OP_REPLACE_OLD:
	    s = "R_O";
	    break;
	case RTQUEUE_OP_REPLACE_NEW:
	    s = "R_N";
	    break;
	case RTQUEUE_OP_PUSH:
	    break;
	}
	debug_msg("  Entry: %s (%s)\n", (*i)->net().str().c_str(),
	    s.c_str());
	++i;
    }
#else
    UNUSED(queue);
#endif
}

template<class A>
int
BGPRibOutTable<A>::add_route(const InternalMessage<A> &rtmsg,
			    BGPRouteTable<A> *caller) 
{
    debug_msg("BGPRibOutTable<%s>::add_route %x for %s ", TypeName<A>::get(),
	   (u_int)(&rtmsg), rtmsg.net().str().c_str());
    debug_msg("on %s\n", _tablename.c_str());
    print_queue(_queue);
    if (caller != _parent) abort();

    // check the queue to see if there's a matching delete - if so we
    // can replace the delete with an add.
    const RouteQueueEntry<A>* queued_entry = NULL;
    list<const RouteQueueEntry<A>*>::iterator i;
    for (i = _queue.begin(); i != _queue.end(); i++) {
	if ( (*i)->net() == rtmsg.net()) {
	    debug_msg("old entry %s matches new entry %s\n",
		   (*i)->net().str().c_str(), rtmsg.net().str().c_str());
	    queued_entry = *i;
	    break;
	}
    }

    RouteQueueEntry<A>* entry;
    if (queued_entry == NULL) {
	// just add the route to the queue.
	entry = new RouteQueueEntry<A>(rtmsg.route(), RTQUEUE_OP_ADD);
	entry->set_origin_peer(rtmsg.origin_peer());
	_queue.push_back(entry);
    } else if (queued_entry->op() == RTQUEUE_OP_DELETE) {
	// There was a delete in the queue.  The delete must become a replace.
	debug_msg("removing delete entry from output queue to become replace\n");
	_queue.erase(i);
	entry = new RouteQueueEntry<A>(queued_entry->route(),
				       RTQUEUE_OP_REPLACE_OLD);
	entry->set_origin_peer(queued_entry->origin_peer());
	_queue.push_back(entry);
	entry = new RouteQueueEntry<A>(rtmsg.route(), RTQUEUE_OP_REPLACE_NEW);
	entry->set_origin_peer(rtmsg.origin_peer());
	_queue.push_back(entry);
	delete queued_entry;
    } else if (queued_entry->op() == RTQUEUE_OP_REPLACE_OLD) {
	// There was a replace in the queue.  The new part of the
	// replace must be updated.
	i++;
	queued_entry = *i;
	assert(queued_entry->op() == RTQUEUE_OP_REPLACE_NEW);
	entry = new RouteQueueEntry<A>(rtmsg.route(), RTQUEUE_OP_REPLACE_NEW);
	entry->set_origin_peer(rtmsg.origin_peer());
	_queue.insert(i, entry);
	_queue.erase(i);
	delete queued_entry;
    } else if (queued_entry->op() == RTQUEUE_OP_ADD) {
	XLOG_ERROR("RibOut: add_route called for a subnet already in the output queue\n");
	abort();
    }

    // handle push
    if (rtmsg.push())
	push(_parent);
    debug_msg("After-->:\n");
    print_queue(_queue);
    debug_msg("<--After\n");
    return ADD_USED;
}

template<class A>
int
BGPRibOutTable<A>::replace_route(const InternalMessage<A> &old_rtmsg,
				 const InternalMessage<A> &new_rtmsg,
				 BGPRouteTable<A> *caller) 
{
    debug_msg("BGPRibOutTable<%s>::replace_route %x %x\n", TypeName<A>::get(),
	   (u_int)(&old_rtmsg), (u_int)(&new_rtmsg));
    assert(old_rtmsg.push() == false);

    delete_route(old_rtmsg, caller);
    return add_route(new_rtmsg, caller);
}

template<class A>
int
BGPRibOutTable<A>::delete_route(const InternalMessage<A> &rtmsg,
			       BGPRouteTable<A> *caller) 
{
    debug_msg("%s\n", _tablename.c_str());
    debug_msg("BGPRibOutTable<%s>::delete_route %x\n", TypeName<A>::get(),
	   (u_int)(rtmsg.route()));
    debug_msg("Attribute: %x\n", (u_int)(rtmsg.route()->attributes()));
    print_queue(_queue);
    if (caller != _parent) abort();

    // check the queue to see if there's a matching entry.

    const RouteQueueEntry<A>* queued_entry = NULL;
    list<const RouteQueueEntry<A>*>::iterator i;
    for (i = _queue.begin(); i != _queue.end(); i++) {
	if ( (*i)->net() == rtmsg.net()) {
	    queued_entry = *i;
	    break;
	}
    }

    RouteQueueEntry<A>* entry;
    if (queued_entry == NULL) {
	// add the route delete operation to the queue.
	entry = new RouteQueueEntry<A>(rtmsg.route(), RTQUEUE_OP_DELETE);
	entry->set_origin_peer(rtmsg.origin_peer());
	_queue.push_back(entry);
    } else if (queued_entry->op() == RTQUEUE_OP_ADD) {
	// XXX this probably shouldn't happen.
	abort();

	// if it does, then here's how to handle it:
	_queue.erase(i);
	delete queued_entry;
    } else if (queued_entry->op() == RTQUEUE_OP_DELETE) {
	// This should not happen.
	abort();
    } else if (queued_entry->op() == RTQUEUE_OP_REPLACE_OLD) {
	// XXX this probably shouldn't happen.
	abort();

	// if it does, then here's how to handle it:
	list<const RouteQueueEntry<A>*>::iterator i2 = i;
	i++;
	_queue.erase(i2);
	assert((*i)->op() == RTQUEUE_OP_REPLACE_NEW);
	_queue.erase(i);
	delete *i;

	entry = new RouteQueueEntry<A>(queued_entry->route(),
				       RTQUEUE_OP_DELETE);
	entry->set_origin_peer(queued_entry->origin_peer());
	_queue.push_back(entry);
	delete queued_entry;
    }

    // handle push
    if (rtmsg.push())
	push(_parent);
    return 0;
}

template<class A>
int
BGPRibOutTable<A>::push(BGPRouteTable<A> *caller) 
{
    debug_msg("%s\n", _tablename.c_str());
    debug_msg("BGPRibOutTable<%s>::push\n", TypeName<A>::get());
    if (caller != _parent) abort();
    // In push, we need to collect together all the SubnetRoutes that
    // have the same Path Attributes, and send them together in an
    // Update message.  We repeatedly do this until the queue is empty.

    bool peer_busy = false;
    while (_queue.empty() == false) {

	// go through _queue and move all of the queue elements that
	// have the same attribute list as the first element to
	// tmp_queue

	list <const RouteQueueEntry<A>*> tmp_queue;
	const PathAttributeList<A> *attributes = NULL;
	int ctr = 1;
	typedef list<const RouteQueueEntry<A>*>::iterator Iter;
	Iter i = _queue.begin();
	while (i != _queue.end()) {
	    if ((*i)->op() == RTQUEUE_OP_REPLACE_OLD) {
		// we have to handle replace differently from the rest
		// because replace uses two paired queue entries, and
		// we must move them together.  We only care about the
		// attributes on the new one of the pair.
		Iter i2 = i;
		i2++;
		assert((*i2)->op() == RTQUEUE_OP_REPLACE_NEW);
		if (attributes == NULL)
		    attributes = (*i2)->attributes();
		if ((*i2)->attributes() == attributes) {
		    tmp_queue.push_back(*i);
		    tmp_queue.push_back(*i2);
		    _queue.erase(i);
		    i = _queue.erase(i2);
		    ctr++;
		} else {
		    ++i;
		    assert(i != _queue.end());
		    ++i;
		}
	    } else {
		if (attributes == NULL)
		    attributes = (*i)->attributes();
		if ((*i)->attributes() == attributes) {
		    tmp_queue.push_back((*i));
		    i = _queue.erase(i);
		    ctr++;
		} else {
		    ++i;
		}
	    }
	}
	debug_msg("%d elements with attr %x moved to tmp queue\n", ctr,
	       (u_int)attributes);
	print_queue(tmp_queue);

	// at this point we pass the tmp_queue to the output BGP
	// session object for output
	debug_msg("************************************\n");
	debug_msg("* Outputting route to BGP peer\n");
	debug_msg("* Attributes: \n%s", attributes->str().c_str());
	i = tmp_queue.begin();
	bool ibgp = (*i)->origin_peer()->ibgp();
	_peer->start_packet(ibgp);
	while (i != tmp_queue.end()) {
	    debug_msg("* Subnet: %s\n", (*i)->net().str().c_str());
	    if ((*i)->op() == RTQUEUE_OP_ADD ) {
		debug_msg("* Announce\n");
		// the sanity checking was done in add_route...
		_peer->add_route(*((*i)->route()));
		delete (*i);
	    } else if ((*i)->op() == RTQUEUE_OP_DELETE ) {
		// the sanity checking was done in delete_route...
		debug_msg("* Withdraw\n");
		_peer->delete_route(*((*i)->route()));
		delete (*i);
	    } else if ((*i)->op() == RTQUEUE_OP_REPLACE_OLD ) {
		debug_msg("* Replace\n");
		const SubnetRoute<A> *old_route = (*i)->route();
		const RouteQueueEntry<A> *old_queue_entry = (*i);
		i++;
		assert(i != tmp_queue.end());
		assert((*i)->op() == RTQUEUE_OP_REPLACE_NEW);
		const SubnetRoute<A> *new_route = (*i)->route();
		_peer->replace_route(*old_route, *new_route);
		delete old_queue_entry;
		delete (*i);
	    } else {
		abort();
	    }
	    ++i;
	}

	/* push the packet */
	/* if the peer is busy (a queue has built up), there's not
           much the RibOut can do for the things already in it's
           output queue - we just keep sending those to the
           peer_handler.  But we want to signal back upstream to stop
           sending us new data */
	if (_peer->push_packet() == PEER_OUTPUT_BUSY)
	    peer_busy = true;


	debug_msg("************************************\n");
    }

    /* signal our state back upstream */
    if ((peer_busy == true) && (_peer_busy == false)) {
	debug_msg("RibOut signalling peer busy upstream\n");
	_parent->output_state(true, this);

	// we now have to assume that a queue will build upstream
	_upstream_queue_exists = true;
    }
    _peer_busy = peer_busy;

    return 0;
}


template<class A>
void
BGPRibOutTable<A>::output_no_longer_busy() 
{
    debug_msg("RibOut: output_no_longer_busy\n");
    if (_peer_busy == false) return;
    debug_msg("RibOut: _peer_busy true->false\n");

    _peer_busy = false;
    while (1) {
	/* get_next_message will cause the upstream queue to send
           another update (add, delete, replace or push) through the
           normal RouteTable interfaces*/

	bool upstream_queue_exists = _parent->get_next_message(this);

	if (upstream_queue_exists == false) {
	    /*the queue upstream has now drained*/
	    if (_peer_busy == false) {
		/*if the queue downstream is no longer busy then go
                  back to normal operation*/
		_upstream_queue_exists = false;
		_parent->output_state(false, this);
	    }
	    /*there's nothing left to do here*/
	    return;
	}
	if (_peer_busy == true) {
	    /*stop requesting messages because the output queue filled
              up again*/
	    return;
	}
    }
}


template<class A>
const SubnetRoute<A>*
BGPRibOutTable<A>::lookup_route(const IPNet<A> &net) const 
{
    return _parent->lookup_route(net);
}

template<class A>
void
BGPRibOutTable<A>::peering_went_down(const PeerHandler *peer, uint32_t genid,
				     BGPRouteTable<A> *caller) 
{
    XLOG_ASSERT(_parent == caller);
    UNUSED(genid);
    UNUSED(peer);
}

template<class A>
void
BGPRibOutTable<A>::peering_down_complete(const PeerHandler *peer,
					 uint32_t genid,
					 BGPRouteTable<A> *caller) 
{
    XLOG_ASSERT(_parent == caller);
    UNUSED(genid);
    UNUSED(peer);
}

template<class A>
string
BGPRibOutTable<A>::str() const 
{
    string s = "BGPRibOutTable<A>" + tablename();
    return s;
}

template class BGPRibOutTable<IPv4>;
template class BGPRibOutTable<IPv6>;




