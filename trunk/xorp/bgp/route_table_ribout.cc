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

#ident "$XORP: xorp/bgp/route_table_ribout.cc,v 1.36 2008/07/23 05:09:38 pavlin Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "route_table_ribout.hh"
#include "peer_handler.hh"

template<class A>
RibOutTable<A>::RibOutTable(string table_name,
			    Safi safi,
			    BGPRouteTable<A> *init_parent,
			    PeerHandler *peer)
    : BGPRouteTable<A>("RibOutTable-" + table_name, safi)
{
    this->_parent = init_parent;
    _peer = peer;
    _peer_busy = false;
    _peer_is_up = false;
}

template<class A>
RibOutTable<A>::~RibOutTable() 
{
    print_queue(_queue);
    typename list<const RouteQueueEntry<A>*>::iterator i;
    i = _queue.begin();
    while (i != _queue.end()) {
	delete (*i);
	++i;
    }
}

template<class A>
void
RibOutTable<A>::print_queue(const list<const RouteQueueEntry<A>*>& queue) 
    const 
{
#ifdef DEBUG_LOGGING
    debug_msg("Output Queue:\n");
    typename list<const RouteQueueEntry<A>*>::const_iterator i;
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
RibOutTable<A>::add_route(const InternalMessage<A> &rtmsg,
			  BGPRouteTable<A> *caller) 
{
    debug_msg("\n         %s\n caller: %s\n rtmsg: %p route: %p\n%s\n",
	      this->tablename().c_str(),
	      caller->tablename().c_str(),
	      &rtmsg,
	      rtmsg.route(),
	      rtmsg.str().c_str());
    
    print_queue(_queue);
    XLOG_ASSERT(caller == this->_parent);

    // check the queue to see if there's a matching delete - if so we
    // can replace the delete with an add.
    const RouteQueueEntry<A>* queued_entry = NULL;
    typename list<const RouteQueueEntry<A>*>::iterator i;
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
	entry = new RouteQueueEntry<A>(rtmsg.route(), 
				       RTQUEUE_OP_REPLACE_NEW);
	entry->set_origin_peer(rtmsg.origin_peer());
	_queue.push_back(entry);
	delete queued_entry;
    } else if (queued_entry->op() == RTQUEUE_OP_REPLACE_OLD) {
	// There was a replace in the queue.  The new part of the
	// replace must be updated.
	i++;
	queued_entry = *i;
	XLOG_ASSERT(queued_entry->op() == RTQUEUE_OP_REPLACE_NEW);
	entry = new RouteQueueEntry<A>(rtmsg.route(), 
				       RTQUEUE_OP_REPLACE_NEW);
	entry->set_origin_peer(rtmsg.origin_peer());
	_queue.insert(i, entry);
	_queue.erase(i);
	delete queued_entry;
    } else if (queued_entry->op() == RTQUEUE_OP_ADD) {
	XLOG_FATAL("RibOut: add_route called for a subnet already in the output queue\n");
    }

    // handle push
    if (rtmsg.push())
	push(this->_parent);
    debug_msg("After-->:\n");
    print_queue(_queue);
    debug_msg("<--After\n");
    return ADD_USED;
}

template<class A>
int
RibOutTable<A>::replace_route(const InternalMessage<A> &old_rtmsg,
			      const InternalMessage<A> &new_rtmsg,
			      BGPRouteTable<A> *caller) 
{
    debug_msg("%s::replace_route %p %p\n", this->tablename().c_str(),
	      &old_rtmsg, &new_rtmsg);
    XLOG_ASSERT(old_rtmsg.push() == false);

    delete_route(old_rtmsg, caller);
    return add_route(new_rtmsg, caller);
}

template<class A>
int
RibOutTable<A>::delete_route(const InternalMessage<A> &rtmsg,
			     BGPRouteTable<A> *caller) 
{
    debug_msg("\n         %s\n caller: %s\n rtmsg: %p route: %p\n%s\n",
	      this->tablename().c_str(),
	      caller->tablename().c_str(),
	      &rtmsg,
	      rtmsg.route(),
	      rtmsg.str().c_str());

    print_queue(_queue);
    XLOG_ASSERT(caller == this->_parent);

    // check the queue to see if there's a matching entry.

    const RouteQueueEntry<A>* queued_entry = NULL;
    typename list<const RouteQueueEntry<A>*>::iterator i;
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
	// The happens when a route with the same nexthop has been
	// introduced on two peers and the resolvability changes.
	// The RIB-IN sends a delete followed by an add. If this is
	// the better route then the other peer will send an add then
	// a delete.
	// RIB-IN				RIB-OUT
	// Peer A -> delete<A>
	//					add<B>
	// Peer A -> add<A>
	//					delete<B>
	
	_queue.erase(i);
	delete queued_entry;
    } else if (queued_entry->op() == RTQUEUE_OP_DELETE) {
	// This should not happen.
	XLOG_UNREACHABLE();
    } else if (queued_entry->op() == RTQUEUE_OP_REPLACE_OLD) {
	// The happens when a route with the same nexthop has been
	// introduced on two peers and the resolvability changes.

	i = _queue.erase(i);
	XLOG_ASSERT((*i)->op() == RTQUEUE_OP_REPLACE_NEW);
	delete *i;
	_queue.erase(i);

	entry = new RouteQueueEntry<A>(queued_entry->route(),
				       RTQUEUE_OP_DELETE);
	entry->set_origin_peer(queued_entry->origin_peer());
	_queue.push_back(entry);
	delete queued_entry;
    }

    // handle push
    if (rtmsg.push())
	push(this->_parent);
    return 0;
}

template<class A>
int
RibOutTable<A>::push(BGPRouteTable<A> *caller) 
{
    debug_msg("%s\n", this->tablename().c_str());
    XLOG_ASSERT(caller == this->_parent);

    // In push, we need to collect together all the SubnetRoutes that
    // have the same Path Attributes, and send them together in an
    // Update message.  We repeatedly do this until the queue is empty.

    while (_queue.empty() == false) {

	// go through _queue and move all of the queue elements that
	// have the same attribute list as the first element to
	// tmp_queue

	list <const RouteQueueEntry<A>*> tmp_queue;
	const PathAttributeList<A> *attributes = NULL;
	int ctr = 1;
	typedef typename list<const RouteQueueEntry<A>*>::iterator Iter;
	Iter i = _queue.begin();
	while (i != _queue.end()) {
	    if ((*i)->op() == RTQUEUE_OP_REPLACE_OLD) {
		// we have to handle replace differently from the rest
		// because replace uses two paired queue entries, and
		// we must move them together.  We only care about the
		// attributes on the new one of the pair.
		Iter i2 = i;
		i2++;
		XLOG_ASSERT((*i2)->op() == RTQUEUE_OP_REPLACE_NEW);
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
		    XLOG_ASSERT(i != _queue.end());
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
	debug_msg("%d elements with attr %p moved to tmp queue\n", ctr,
		  attributes);
	print_queue(tmp_queue);

	// at this point we pass the tmp_queue to the output BGP
	// session object for output
	debug_msg("************************************\n");
	debug_msg("* Outputting route to BGP peer\n");
	debug_msg("* Attributes: %s\n", attributes->str().c_str());
	i = tmp_queue.begin();
	_peer->start_packet();
	while (i != tmp_queue.end()) {
	    debug_msg("* Subnet: %s\n", (*i)->net().str().c_str());
	    if ((*i)->op() == RTQUEUE_OP_ADD ) {
		debug_msg("* Announce\n");
		// the sanity checking was done in add_route...
		_peer->add_route(*((*i)->route()), 
				 (*i)->origin_peer()->ibgp(), this->safi());
		delete (*i);
	    } else if ((*i)->op() == RTQUEUE_OP_DELETE ) {
		// the sanity checking was done in delete_route...
		debug_msg("* Withdraw\n");
		_peer->delete_route(*((*i)->route()), 
				    (*i)->origin_peer()->ibgp(), this->safi());
		delete (*i);
	    } else if ((*i)->op() == RTQUEUE_OP_REPLACE_OLD ) {
		debug_msg("* Replace\n");
		const SubnetRoute<A> *old_route = (*i)->route();
		bool old_ibgp = (*i)->origin_peer()->ibgp();
		const RouteQueueEntry<A> *old_queue_entry = (*i);
		i++;
		XLOG_ASSERT(i != tmp_queue.end());
		XLOG_ASSERT((*i)->op() == RTQUEUE_OP_REPLACE_NEW);
		const SubnetRoute<A> *new_route = (*i)->route();
		bool new_ibgp = (*i)->origin_peer()->ibgp();
		_peer->replace_route(*old_route, old_ibgp,
				     *new_route, new_ibgp,
				     this->safi());
		delete old_queue_entry;
		delete (*i);
	    } else {
		XLOG_UNREACHABLE();
	    }
	    ++i;
	}

	/* push the packet */
	/* if the peer is busy (a queue has built up), there's not
           much the RibOut can do for the things already in its
           output queue - we just keep sending those to the
           peer_handler.  But we want to not request any more data for now. */
	if (_peer->push_packet() == PEER_OUTPUT_BUSY)
	    _peer_busy = true;


	debug_msg("************************************\n");
    }

    return 0;
}

template<class A>
void 
RibOutTable<A>::wakeup()
{
    reschedule_self();
}

/* value is a tradeoff between not too many calls to timers, and not
   being away from eventloop for too long - probably needs tuning*/
#define MAX_MSGS_IN_BATCH 10

template<class A>
bool
RibOutTable<A>::pull_next_route()
{
    /* don't do anything if we can't sent to the peer handler yet */
    if (_peer_busy == true)
	return false;

    /* we're down, so ignore this wakeup */
    if (_peer_is_up == false)
	return false;

    for (int msgs = 0; msgs < MAX_MSGS_IN_BATCH; msgs++) {
	/* only request a limited about of messages, so we don't hog
	   the eventloop for too long */

	/* get_next_message will cause the upstream queue to send
           another update (add, delete, replace or push) through the
           normal RouteTable interfaces*/
	bool upstream_queue_exists = this->_parent->get_next_message(this);

	if (upstream_queue_exists == false) {
	    /*the queue upstream has now drained*/
	    /*there's nothing left to do here*/
	    return false;
	}
	if (_peer_busy == true) {
	    /*stop requesting messages because the output queue filled
              up again*/
	    return false;
	}
    }
    return true;
}

template<class A>
void
RibOutTable<A>::output_no_longer_busy() 
{
    debug_msg("%s: output_no_longer_busy\n", this->tablename().c_str());
    debug_msg("%s: _peer_busy true->false\n", this->tablename().c_str());
    _peer_busy = false;
    reschedule_self();
}

template<class A>
void
RibOutTable<A>::reschedule_self()
{
    /*call back immediately, but after network events or expired
      timers */    
    if (_pull_routes_task.scheduled())
	return;
    _pull_routes_task = _peer->eventloop().new_task(
	callback(this, &RibOutTable<A>::pull_next_route),
	XorpTask::PRIORITY_DEFAULT, XorpTask::WEIGHT_DEFAULT);
}


template<class A>
const SubnetRoute<A>*
RibOutTable<A>::lookup_route(const IPNet<A> &net, uint32_t& genid) const 
{
    return this->_parent->lookup_route(net, genid);
}

template<class A>
void
RibOutTable<A>::peering_went_down(const PeerHandler *peer, uint32_t genid,
				  BGPRouteTable<A> *caller) 
{
    XLOG_ASSERT(this->_parent == caller);
    UNUSED(genid);

    if (peer == _peer) {
	_peer_is_up = false;
    }
}

template<class A>
void
RibOutTable<A>::peering_down_complete(const PeerHandler *peer,
				      uint32_t genid,
				      BGPRouteTable<A> *caller) 
{
    XLOG_ASSERT(this->_parent == caller);
    UNUSED(genid);
    UNUSED(peer);
}

template<class A>
void
RibOutTable<A>::peering_came_up(const PeerHandler *peer, uint32_t genid,
				  BGPRouteTable<A> *caller) 
{
    XLOG_ASSERT(this->_parent == caller);
    UNUSED(genid);

    if (peer == _peer) {
	_peer_is_up = true;
	_peer_busy = false;
    }
}

template<class A>
string
RibOutTable<A>::str() const 
{
    string s = "RibOutTable<A>" + this->tablename();
    return s;
}

template class RibOutTable<IPv4>;
template class RibOutTable<IPv6>;




