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

#ident "$XORP: xorp/bgp/route_table_fanout.cc,v 1.12 2003/05/15 16:37:54 hodson Exp $"

//#define DEBUG_LOGGING
//#define DEBUG_PRINT_FUNCTION_NAME


#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "route_table_fanout.hh"
#include "route_table_dump.hh"

template <class A>
struct TypeName {
    static const char* get() { return "Unknown"; }
};

template<> 
inline const char* 
TypeName<IPv4>::get() 
{ 
    return "IPv4";
}

template<> 
inline const char*
TypeName<IPv6>::get() 
{
    return "IPv6";
}
template<class A>
FanoutTable<A>::FanoutTable(string table_name,
			    BGPRouteTable<A> *init_parent)
    : BGPRouteTable<A>("FanoutTable-" + table_name)
{
    _parent = init_parent;
}

template<class A>
int
FanoutTable<A>::add_next_table(BGPRouteTable<A> *new_next_table,
			       const PeerHandler *ph) 
{
    debug_msg("FanoutTable<%s>::add_next_table %x %s\n",
	      TypeName<A>::get(),
	      (u_int)new_next_table, new_next_table->tablename().c_str());

    if (_next_tables.find(new_next_table) != _next_tables.end()) {
	// the next_table is already in the set
	return -1;
    }
    PeerRoutePair<A> next_peer_table(new_next_table, ph);
    _next_tables.insert(make_pair(new_next_table, next_peer_table));
    return 0;
}


template<class A>
int
FanoutTable<A>::remove_next_table(BGPRouteTable<A> *ex_next_table) 
{
    typename map<BGPRouteTable<A>*, PeerRoutePair<A> >::iterator iter;
    iter = _next_tables.find(ex_next_table);
    if (iter == _next_tables.end()) {
	// the next_table is not already in the set
	debug_msg("FanoutTable<A>::remove_next_table\
 attempt to delete a table that doesn't exist\n");
	abort();
    }
    skip_entire_queue(ex_next_table);
    if (ex_next_table->type() == DUMP_TABLE) {
	remove_dump_table((DumpTable<A>*)ex_next_table);
	((DumpTable<A>*)ex_next_table)->suspend_dump();
    }
    _next_tables.erase(iter);
    return 0;
}


template<class A>
int
FanoutTable<A>::add_route(const InternalMessage<A> &rtmsg,
			  BGPRouteTable<A> *caller) 
{
    debug_msg("%s\n", _tablename.c_str());
    debug_msg("FanoutTable::add_route %x\n",
	      (u_int)(&rtmsg));

    if (caller != _parent) abort();

    const PeerHandler *origin_peer = rtmsg.origin_peer();

    typedef typename map<BGPRouteTable<A>*, PeerRoutePair<A> >::iterator Iter;
    Iter i = _next_tables.begin();
    int result = ADD_USED, r;
    list <PeerRoutePair<A>*> queued_peers;
    while (i != _next_tables.end()) {
	const PeerHandler *next_peer = i->second.peer_handler();
	if (origin_peer == next_peer) {
	    // don't send the route back to the peer it came from
	    debug_msg("FanoutTable<%s>::add_route %x.\n  Don't send back to %s\n",
		      TypeName<A>::get(),
		      (u_int)(&rtmsg), (i->first)->tablename().c_str());
	} else {
	    debug_msg("FanoutTable<%s>::add_route %x to %s\n",
		      TypeName<A>::get(),
		      (u_int)(&rtmsg), (i->first)->tablename().c_str());

	    if (i->second.busy()) {
		debug_msg("Fanout: queuing route, queue len is %u\n",
			  (uint32_t)queued_peers.size());
		queued_peers.push_back(&(i->second));
		r = ADD_USED;
	    } else {
		debug_msg("Fanout: passing route on\n");
		r = i->first->add_route(rtmsg, (BGPRouteTable<A>*)this);
	    }

	    // we don't bother to return ADD_UNUSED or ADD_FILTERED,
	    // but if there's a failure, we'll return the last failure
	    // code.  Not clear how useful this is, but best not to
	    // hide the failure completely.
	    if (r != ADD_USED && r != ADD_UNUSED && r != ADD_FILTERED)
		result = r;
	}
	++i;
    }
    if (queued_peers.empty() == false)
	add_to_queue(RTQUEUE_OP_ADD, rtmsg, queued_peers);
    return result;
}

template<class A>
int
FanoutTable<A>::replace_route(const InternalMessage<A> &old_rtmsg,
			      const InternalMessage<A> &new_rtmsg,
			      BGPRouteTable<A> *caller) 
{
    debug_msg("%s\n", _tablename.c_str());
    debug_msg("FanoutTable::replace_route %x -> %x\n",
	      (u_int)(&old_rtmsg), (u_int)(&new_rtmsg));

    assert(caller == _parent);

    const PeerHandler *origin_peer = old_rtmsg.origin_peer();
    assert(origin_peer == new_rtmsg.origin_peer());

    int result = ADD_USED, r;
    list <PeerRoutePair<A>*> queued_peers;
    typename map<BGPRouteTable<A>*, PeerRoutePair<A> >::iterator i;
    for (i = _next_tables.begin();  i != _next_tables.end();  i++) {
	const PeerHandler *next_peer = i->second.peer_handler();
	if (origin_peer == next_peer) {
	    // don't send the route back to the peer it came from
	} else {
	    debug_msg("FanoutTable<%s>::replace_route %x -> %x to %s\n",
		      TypeName<A>::get(),
		      (u_int)(&old_rtmsg), (u_int)(&new_rtmsg),
		      (i->first)->tablename().c_str());

	    if (i->second.busy()) {
		debug_msg("queueing replace_route\n");
		queued_peers.push_back(&(i->second));
		r = ADD_USED;
	    } else {
		debug_msg("not queuing replace_route\n");
		r = i->first->replace_route(old_rtmsg, new_rtmsg,
					    (BGPRouteTable<A>*)this);
	    }

	    // we don't bother to return ADD_UNUSED, but if there's a
	    // failure, we'll return the last failure code.  Not clear how
	    // useful this is, but best not to hide the failure completely.
	    if (r != ADD_USED && r != ADD_UNUSED && r != ADD_FILTERED)
		result = r;
	}
    }
    if (queued_peers.empty() == false)
	add_replace_to_queue(old_rtmsg, new_rtmsg, queued_peers);
    return result;
}

template<class A>
int
FanoutTable<A>::delete_route(const InternalMessage<A> &rtmsg,
			     BGPRouteTable<A> *caller) 
{
    if (caller != _parent) abort();

    const PeerHandler *origin_peer = rtmsg.origin_peer();

    list <PeerRoutePair<A>*> queued_peers;
    typename map<BGPRouteTable<A>*, PeerRoutePair<A> >::iterator i;
    for (i = _next_tables.begin();  i != _next_tables.end();  i++) {
	const PeerHandler *next_peer = i->second.peer_handler();
	if (origin_peer == next_peer) {
	    debug_msg("FanoutTable<%s>::delete_route %x.\n  Don't send back to %s\n",
		      TypeName<A>::get(),
		      (u_int)(&rtmsg), (i->first)->tablename().c_str());
	} else {
	    debug_msg("FanoutTable<%s>::delete_route %x to %s\n",
		      TypeName<A>::get(),
		      (u_int)(&rtmsg), (i->first)->tablename().c_str());
	    if (i->second.busy()) {
		queued_peers.push_back(&(i->second));
	    } else {
		i->first->delete_route(rtmsg, (BGPRouteTable<A>*)this);
	    }
	}
    }
    if (queued_peers.empty() == false)
	add_to_queue(RTQUEUE_OP_DELETE, rtmsg, queued_peers);
    return 0;
}

template<class A>
int
FanoutTable<A>::route_dump(const InternalMessage<A> &rtmsg,
			   BGPRouteTable<A> *caller,
			   const PeerHandler *dump_peer) 
{
    if (caller != _parent) abort();
    BGPRouteTable<A> *dump_child;
    typename map<BGPRouteTable<A>*, PeerRoutePair<A> >::iterator i;
    for (i = _next_tables.begin();  i != _next_tables.end();  i++) {
	if (i->second.peer_handler() == dump_peer) {
	    dump_child = i->first;
	    break;
	}
    }
    assert(i->second.busy());

    int result;
    result = dump_child->route_dump(rtmsg, (BGPRouteTable<A>*)this, dump_peer);
    if (result == ADD_USED || result == ADD_UNUSED || result == ADD_FILTERED)
	return 0;
    return result;
}

template<class A>
int
FanoutTable<A>::push(BGPRouteTable<A> *caller) 
{
    debug_msg("Push\n");
    if (caller != _parent) abort();
    list <PeerRoutePair<A>*> queued_peers;
    typename map<BGPRouteTable<A>*, PeerRoutePair<A> >::iterator i;
    for (i = _next_tables.begin();  i != _next_tables.end();  i++) {
	// a push needs to go to all peers because an add may cause a
	// delete (or vice versa) of a route that originated from a
	// different peer.
	if (i->second.busy()) {
	    queued_peers.push_back(&(i->second));
	} else {
	    i->first->push((BGPRouteTable<A>*)this);
	}
    }

    if (queued_peers.empty() == false)
	// if the origin peer we send to add_push_to_queue is NULL, the
	// push will go to all peers
	add_push_to_queue(queued_peers, NULL);

    return 0;
}

template<class A>
const SubnetRoute<A>*
FanoutTable<A>::lookup_route(const IPNet<A> &net) const 
{
    return _parent->lookup_route(net);
}

template<class A>
string
FanoutTable<A>::str() const 
{
    string s = "FanoutTable<A>" + tablename();
    return s;
}

template<class A>
void
FanoutTable<A>::add_dump_table(DumpTable<A> *dump_table) 
{
    _dump_tables.insert(dump_table);
}

template<class A>
void
FanoutTable<A>::remove_dump_table(DumpTable<A> *dump_table) 
{
    typename set <DumpTable<A>*>::iterator i;
    i = _dump_tables.find(dump_table);
    assert(i != _dump_tables.end());
    _dump_tables.erase(i);
}

template<class A>
int
FanoutTable<A>::dump_entire_table(BGPRouteTable<A> *child_to_dump_to) 
{
    assert(child_to_dump_to->type() != DUMP_TABLE);

    typename map<BGPRouteTable<A> *, PeerRoutePair<A> >::iterator i;
    PeerRoutePair<A> *peer_info = NULL;
    list <const PeerHandler*> peer_list;
    for (i = _next_tables.begin(); i != _next_tables.end(); i++) {
	if (i->second.peer_handler() != NULL)
	    peer_list.push_back(i->second.peer_handler());
	if (i->first == child_to_dump_to)
	    peer_info = &(i->second);
    }
    XLOG_ASSERT(peer_info != NULL);
    const PeerHandler *peer_handler = peer_info->peer_handler();

    string tablename = string("DumpTable") + TypeName<A>::get();
    DumpTable<A>* dump_table =
	new DumpTable<A>(tablename, peer_handler, peer_list,
			    (BGPRouteTable<A>*)this);

    dump_table->set_next_table(child_to_dump_to);
    child_to_dump_to->set_parent(dump_table);
    remove_next_table(child_to_dump_to);
    add_next_table(dump_table, peer_handler);
    peer_info->set_busy(true);
    add_dump_table(dump_table);

    dump_table->initiate_background_dump();
    return 0;
}

/* mechanisms to implement flow control in the output plumbing */
template<class A>
void
FanoutTable<A>::add_to_queue(RouteQueueOp operation,
			     const InternalMessage<A> &rtmsg,
			     const list<PeerRoutePair<A>*>& queued_peers) 
{
    debug_msg("FanoutTable<A>::add_to_queue, op=%d, net=%s\n", 
	      operation, rtmsg.net().str().c_str());
    RouteQueueEntry<A> *queue_entry;
    queue_entry = new RouteQueueEntry<A>(rtmsg.route(), operation);
    queue_entry->set_origin_peer(rtmsg.origin_peer());
    queue_entry->set_genid(rtmsg.genid());
    _output_queue.push_back(queue_entry);
    set_queue_positions(queued_peers);

    if (rtmsg.push())
	add_push_to_queue(queued_peers, rtmsg.origin_peer());
}


template<class A>
void
FanoutTable<A>::add_replace_to_queue(const InternalMessage<A> &old_rtmsg,
				     const InternalMessage<A> &new_rtmsg,
				     const list<PeerRoutePair<A>*>&
				     queued_peers) 
{
    debug_msg("FanoutTable<A>::add_replace_to_queue\n");
    // replace entails two queue entries, but they're always paired up
    // in the order OLD then NEW
    RouteQueueEntry<A> *queue_entry;
    queue_entry = new RouteQueueEntry<A>(old_rtmsg.route(),
					 RTQUEUE_OP_REPLACE_OLD);
    queue_entry->set_origin_peer(old_rtmsg.origin_peer());
    queue_entry->set_genid(old_rtmsg.genid());
    _output_queue.push_back(queue_entry);

    // set queue positions now, before we add the second queue entry
    set_queue_positions(queued_peers);

    queue_entry = new RouteQueueEntry<A>(new_rtmsg.route(),
					 RTQUEUE_OP_REPLACE_NEW);
    queue_entry->set_origin_peer(new_rtmsg.origin_peer());
    queue_entry->set_genid(new_rtmsg.genid());
    _output_queue.push_back(queue_entry);


    if (new_rtmsg.push()) {
	if (new_rtmsg.origin_peer() == old_rtmsg.origin_peer())
	    add_push_to_queue(queued_peers, new_rtmsg.origin_peer());
	else
	    // if the origin peer we send to add_push_to_queue, the push will
	    // go to all peers
	    add_push_to_queue(queued_peers, NULL);
    }
}


template<class A>
void
FanoutTable<A>::add_push_to_queue(const list<PeerRoutePair<A>*>&
				  queued_peers,
				  const PeerHandler *origin_peer) 
{
    debug_msg("FanoutTable<A>::add_push_to_queue\n");
    RouteQueueEntry<A> *queue_entry;
    queue_entry = new RouteQueueEntry<A>(RTQUEUE_OP_PUSH, origin_peer);
    _output_queue.push_back(queue_entry);
    set_queue_positions(queued_peers);
}

template<class A>
void
FanoutTable<A>::set_queue_positions(const list<PeerRoutePair<A>*>&
				    queued_peers) 
{
    typename list<PeerRoutePair<A>*>::const_iterator i;
    for (i = queued_peers.begin(); i != queued_peers.end(); i++) {
	if ((*i)->busy() && ((*i)->has_queued_data() == false)) {
	    /* set the queue position to the current last element */
	    (*i)->set_queue_position( --(_output_queue.end()) );
	    (*i)->set_has_queued_data(true);
	}
    }
}

template<class A>
void
FanoutTable<A>::output_state(bool busy, BGPRouteTable<A> *next_table) 
{
    typename map<BGPRouteTable<A> *, PeerRoutePair<A> >::iterator i;
    i = _next_tables.find(next_table);
    XLOG_ASSERT(i != _next_tables.end());

    PeerRoutePair<A> *peer_info = &(i->second);

    if (busy)
	debug_msg("Fanout: Peer output state is BUSY\n");
    else {
	// downstream must drain the queue using get_next_message or
	// skip_entire_queue before we reset to output state to NOT BUSY
	assert(peer_info->has_queued_data() == false);
	debug_msg("Fanout: Peer output state is not busy\n");
    }
    peer_info->set_busy(busy);
    //    peer_info->set_has_queued_data(false);
}

template<class A>
bool
FanoutTable<A>::get_next_message(BGPRouteTable<A> *next_table) 
{
    debug_msg("FanoutTable<A>::get_next_message: %p\n", next_table);
    print_queue();
    typename map<BGPRouteTable<A> *, PeerRoutePair<A> >::iterator i;
    i = _next_tables.find(next_table);
    XLOG_ASSERT(i != _next_tables.end());

    PeerRoutePair<A> *peer_info = &(i->second);
    if (peer_info->has_queued_data() == false) {
	debug_msg("no data queued\n");
	return false;
    }

    typename list<const RouteQueueEntry<A>*>::iterator queue_ptr;
    queue_ptr = peer_info->queue_position();
    bool discard_possible = false;
    int skipped = 1;

    switch ((*queue_ptr)->op()) {
    case RTQUEUE_OP_ADD: {
	debug_msg("OP_ADD, net=%s\n", 
		  (*queue_ptr)->route()->net().str().c_str());
	InternalMessage<A> rtmsg((*queue_ptr)->route(),
				 (*queue_ptr)->origin_peer(),
				 (*queue_ptr)->genid());
	next_table->add_route(rtmsg, (BGPRouteTable<A>*)this);
	break;
    }
    case RTQUEUE_OP_DELETE: {
	debug_msg("OP_DELETE\n");
	InternalMessage<A> rtmsg((*queue_ptr)->route(),
				 (*queue_ptr)->origin_peer(),
				 (*queue_ptr)->genid());
	next_table->delete_route(rtmsg, (BGPRouteTable<A>*)this);
	break;
    }
    case RTQUEUE_OP_REPLACE_OLD: {
	debug_msg("OP_REPLACE_OLD\n");
	InternalMessage<A> old_rtmsg((*queue_ptr)->route(),
				     (*queue_ptr)->origin_peer(),
				     (*queue_ptr)->genid());
	if (queue_ptr == _output_queue.begin())
	    discard_possible = true;
	queue_ptr++;
	assert(queue_ptr != _output_queue.end());
	InternalMessage<A> new_rtmsg((*queue_ptr)->route(),
				     (*queue_ptr)->origin_peer(),
				     (*queue_ptr)->genid());
	next_table->replace_route(old_rtmsg, new_rtmsg,
				  (BGPRouteTable<A>*)this);
	skipped = 2;
	break;
    }
    case RTQUEUE_OP_REPLACE_NEW: {
	debug_msg("OP_REPLACE_NEW\n");
	// this can't happen because the replace queue entries are
	// always old, then new, and the RTQUEUE_OP_REPLACE_OLD steps
	// over both entries
	XLOG_FATAL("illegal route queue state");
	break;
    }
    case RTQUEUE_OP_PUSH: {
	debug_msg("OP_PUSH\n");
	next_table->push(this);
	break;
    }
    }

    if (queue_ptr == _output_queue.begin())
	discard_possible = true;
    queue_ptr++;

    /* skip past anything that came from the peer that called us */
    while ((queue_ptr != _output_queue.end())
	   && ((*queue_ptr)->origin_peer() != NULL)
	   && ((*queue_ptr)->origin_peer() == peer_info->peer_handler())) {
	debug_msg("queued_peer: %p\n", (*queue_ptr)->origin_peer());
	debug_msg("our peer: %p\n", peer_info->peer_handler());
	switch ((*queue_ptr)->op()) {
	case RTQUEUE_OP_ADD:
	    debug_msg("skipping OP_ADD\n");
	    break;
	case RTQUEUE_OP_DELETE:
	    debug_msg("skipping OP_DELETE\n");
	    break;
	case RTQUEUE_OP_REPLACE_OLD:
	    debug_msg("skipping OP_REPLACE_OLD\n");
	    break;
	case RTQUEUE_OP_REPLACE_NEW:
	    debug_msg("skipping OP_REPLACE_NEW\n");
	    break;
	case RTQUEUE_OP_PUSH:
	    debug_msg("skipping OP_PUSH\n");
	    break;
	}
	if ((*queue_ptr)->op() == RTQUEUE_OP_REPLACE_OLD)
	    queue_ptr++;
	queue_ptr++;
    }
    if (queue_ptr == _output_queue.end()) {
	debug_msg("no more data queued for this peer\n");
	peer_info->set_has_queued_data(false);
    }

    if (peer_info->has_queued_data()) {
	peer_info->set_queue_position(queue_ptr);
    }

    /* now we have to deal with freeing up the head of the queue if
       no-one else needs it now*/
    while(discard_possible) {
	typename map<BGPRouteTable<A> *, PeerRoutePair<A> >::iterator nti;
	// iterating on a map isn't very efficient, but the map
	// shouldn't be all that large
	bool discard = true;
	for (nti = _next_tables.begin(); nti != _next_tables.end(); nti++) {
	    if (nti->second.has_queued_data()) {
		// if someone still references the queue head, we can't
		// discard.
		if (nti->second.queue_position() == _output_queue.begin())
		    discard = false;
		// if we're considering popping the first two entries,
		// we need to check no-one needs the second one either
		if (skipped == 2 &&
		    nti->second.queue_position() == (_output_queue.begin())++ )
		    discard = false;
	    }
	}
	if (discard) {
	    delete _output_queue.front();
	    _output_queue.pop_front();
	    if (skipped == 2) {
		delete _output_queue.front();
		_output_queue.pop_front();
	    }
	    if (_output_queue.empty())
		discard_possible = false;
	} else {
	    discard_possible = false;
	}
    }


    return peer_info->has_queued_data();
}


template<class A>
void
FanoutTable<A>::skip_entire_queue(BGPRouteTable<A> *next_table) 
{
    typename map<BGPRouteTable<A> *, PeerRoutePair<A> >::iterator i;
    i = _next_tables.find(next_table);
    XLOG_ASSERT(i != _next_tables.end());

    PeerRoutePair<A> *peer_info = &(i->second);
    if (peer_info->has_queued_data() == false)
	return;

    typename list<const RouteQueueEntry<A>*>::iterator queue_ptr;
    queue_ptr = peer_info->queue_position();
    bool more_queued_data = true;
    while (more_queued_data) {
	fprintf(stderr, "skip_entire_queue\n");
	bool discard_possible = false;
	int skipped = 1;

	switch ((*queue_ptr)->op())
	    {
	    case RTQUEUE_OP_ADD:
	    case RTQUEUE_OP_DELETE:
	    case RTQUEUE_OP_PUSH:
		break;
	    case RTQUEUE_OP_REPLACE_OLD:
		if (queue_ptr == _output_queue.begin())
		    discard_possible = true;
		queue_ptr++;
		skipped = 2;
		break;
	    case RTQUEUE_OP_REPLACE_NEW:
		XLOG_FATAL("illegal route queue state");
		break;
	    }

	if (queue_ptr == _output_queue.begin())
	    discard_possible = true;
	queue_ptr++;


	// skip over stuff that's not for this peer
	while ((queue_ptr != _output_queue.end()) &&
	       ((*queue_ptr)->origin_peer() == peer_info->peer_handler())) {
	    if (queue_ptr == _output_queue.end()) {
		break;
	    } else {
		queue_ptr++;
		if ((*queue_ptr)->op() == RTQUEUE_OP_REPLACE_NEW)
		    queue_ptr++;
	    }
	}

	if (queue_ptr == _output_queue.end()) {
	    more_queued_data = false;
	    peer_info->set_has_queued_data(false);
	} else {
	    peer_info->set_queue_position(queue_ptr);
	}

	/* now we have to deal with freeing up the head of the queue if
	   no-one else needs it now*/
	while (discard_possible) {
	    typename map<BGPRouteTable<A> *, PeerRoutePair<A> >::iterator nti;
	    // iterating on a map isn't very efficient, but the map
	    // shouldn't be all that large
	    bool discard = true;
	    for (nti = _next_tables.begin(); nti != _next_tables.end(); nti++) {
		if (nti->second.has_queued_data()) {
		    // if someone still references the queue head, we can't
		    // discard.
		    if (nti->second.queue_position() == _output_queue.begin())
			discard = false;
		    // if we're considering popping the first two entries,
		    // we need to check no-one needs the second one either
		    if (skipped == 2 &&
			(nti->second.queue_position() 
			 == (_output_queue.begin())++ ))
			discard = false;
		}
	    }
	    if (discard) {
		delete _output_queue.front();
		_output_queue.pop_front();
		if (skipped == 2) {
		    delete _output_queue.front();
		    _output_queue.pop_front();
		}
		if (_output_queue.empty())
		    discard_possible = false;
	    } else {
		discard_possible = false;
	    }
	}
    }
    return;
}

template<class A>
void
FanoutTable<A>::peering_went_down(const PeerHandler *peer, uint32_t genid,
				  BGPRouteTable<A> *caller) 
{
    XLOG_ASSERT(_parent == caller);
    typename map<BGPRouteTable<A>*, PeerRoutePair<A> >::iterator i;
    for (i = _next_tables.begin();  i != _next_tables.end();  i++) {
	i->first->peering_went_down(peer, genid, (BGPRouteTable<A>*)this);
    }
}

template<class A>
void
FanoutTable<A>::peering_down_complete(const PeerHandler *peer,
				      uint32_t genid,
				      BGPRouteTable<A> *caller) 
{
    XLOG_ASSERT(_parent == caller);
    typename map<BGPRouteTable<A>*, PeerRoutePair<A> >::iterator i;
    for (i = _next_tables.begin();  i != _next_tables.end();  i++) {
	i->first->peering_down_complete(peer, genid, (BGPRouteTable<A>*)this);
    }
}



template<class A>
void
FanoutTable<A>::print_queue() 
{
#ifdef DEBUG_LOGGING
    // leave this as printf please - mjh
    printf("Fanout Table rate control queue:\n");
    typename list <const RouteQueueEntry<A>*>::iterator i;
    int ctr = 0;
    for (i = _output_queue.begin(); i != _output_queue.end(); i++) {
	ctr++;
	printf("Queue Entry %d\n", ctr);
	printf("%s\n", (*i)->str().c_str());
    }
#endif
}

template class PeerRoutePair<IPv4>;
template class PeerRoutePair<IPv6>;
template class FanoutTable<IPv4>;
template class FanoutTable<IPv6>;




