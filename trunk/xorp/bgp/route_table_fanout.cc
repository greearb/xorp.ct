// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/route_table_fanout.cc,v 1.62 2007/02/16 22:45:17 pavlin Exp $"

//#define DEBUG_LOGGING
//#define DEBUG_PRINT_FUNCTION_NAME

// #define DEBUG_QUEUE

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "route_table_fanout.hh"
#include "route_table_dump.hh"

template<class A> 
NextTableMap<A>::~NextTableMap()
{
    typename map<BGPRouteTable<A> *, PeerTableInfo<A>* >::iterator i;
    i = _next_tables.begin();
    while (i != _next_tables.end()) {
	delete i->second;
	_next_tables.erase(i); 
	i = _next_tables.begin();
   }
}

template<class A> 
void 
NextTableMap<A>::insert(BGPRouteTable<A> *next_table,
			const PeerHandler *ph, uint32_t genid) 
{
    PeerTableInfo<A>* prpair =
	new PeerTableInfo<A>(next_table, ph, genid);
    _next_tables[next_table] = prpair;

    //we have to choose a sort order so our results are repeatable
    //across different platforms.  we arbitrarily choose to sort on
    //BGP ID because it should be unique.

    //check we don't have two peers with the same address
    if (_next_table_order.find(ph->id().addr()) != _next_table_order.end())
	XLOG_WARNING("BGP: Two peers have same BGP ID: %s\n", 
		     ph->id().str().c_str());

    _next_table_order.insert(make_pair(ph->id().addr(),prpair));
}

template<class A> 
void 
NextTableMap<A>::erase(iterator& iter) 
{
    PeerTableInfo<A>* prpair = &(iter.second());
    typename map<BGPRouteTable<A> *, PeerTableInfo<A>* >::iterator i;
    i = _next_tables.find(prpair->route_table());
    XLOG_ASSERT(i != _next_tables.end());
    uint32_t id = i->second->peer_handler()->id().addr();
    _next_tables.erase(i);

    typename multimap<uint32_t, PeerTableInfo<A>* >::iterator j;
    j = _next_table_order.find(id);
    while (j->first == id  && j->second != prpair) {
	//find the right one.
	j++;
    }
    //if it's in _next_table, it must be in _next_table_order too.
    XLOG_ASSERT(j != _next_table_order.end());
    XLOG_ASSERT(j->second == prpair);
    _next_table_order.erase(j);
    delete prpair;
}

template<class A> 
typename NextTableMap<A>::iterator 
NextTableMap<A>::find(BGPRouteTable<A> *next_table)
{
    typename map<BGPRouteTable<A> *, PeerTableInfo<A>* >::iterator i;
    i = _next_tables.find(next_table);
    if (i == _next_tables.end())
	return end();
    PeerTableInfo<A>* prpair = i->second;
    typename multimap<uint32_t, PeerTableInfo<A>* >::iterator j;
    uint32_t id = i->second->peer_handler()->id().addr();
    j = _next_table_order.find(id);
    while (j->first == id  && j->second != prpair) {
	//find the right one.
	j++;
    }
    //if it's in _next_table, it must be in _next_table_order too.
    XLOG_ASSERT(j != _next_table_order.end());
    XLOG_ASSERT(j->second == prpair);
    return iterator(j);
}

template<class A> 
typename NextTableMap<A>::iterator 
NextTableMap<A>::begin() {
    return iterator(_next_table_order.begin());
}

template<class A> 
typename NextTableMap<A>::iterator 
NextTableMap<A>::end() {
    return iterator(_next_table_order.end());
};


template<class A>
FanoutTable<A>::FanoutTable(string table_name,
			    Safi safi,
			    BGPRouteTable<A> *init_parent,
			    PeerHandler *aggr_handler,
			    BGPRouteTable<A> *aggr_table)
    : BGPRouteTable<A>("FanoutTable-" + table_name, safi)
{
    this->_parent = init_parent;
    if (aggr_table != NULL && aggr_table != NULL)
	_aggr_peerinfo = new PeerTableInfo<A>(aggr_table,
					      aggr_handler,
					      GENID_UNKNOWN);
    else
	_aggr_peerinfo = NULL;
}

template<class A>
FanoutTable<A>::~FanoutTable()
{
    if (_aggr_peerinfo != NULL)
	delete _aggr_peerinfo;
}

template<class A>
int
FanoutTable<A>::add_next_table(BGPRouteTable<A> *new_next_table,
			       const PeerHandler *ph, uint32_t genid) 
{
    debug_msg("FanoutTable<IPv%u:%s>::add_next_table %p %s\n",
	      XORP_UINT_CAST(A::ip_version()),
	      pretty_string_safi(this->safi()),
	      new_next_table, new_next_table->tablename().c_str());

    if (_next_tables.find(new_next_table) != _next_tables.end()) {
	// the next_table is already in the set
	return -1;
    }
    _next_tables.insert(new_next_table, ph, genid);
    new_next_table->peering_came_up(ph, genid, this);
    return 0;
}

template<class A>
int
FanoutTable<A>::remove_next_table(BGPRouteTable<A> *ex_next_table) 
{
    debug_msg("removing: %s\n", ex_next_table->tablename().c_str());

    typename NextTableMap<A>::iterator iter;
    iter = _next_tables.find(ex_next_table);
    if (iter == _next_tables.end()) {
	// the next_table is not already in the set
	XLOG_FATAL("Attempt to remove table that is not in list: %s",
		   ex_next_table->tablename().c_str());
    }

    skip_entire_queue(ex_next_table);

    DumpTable<A> *dtp = dynamic_cast<DumpTable<A>*>(ex_next_table);
    if (dtp) {
	remove_dump_table(dtp);
	dtp->suspend_dump();
    }
    _next_tables.erase(iter);
    return 0;
}

template<class A>
int
FanoutTable<A>::replace_next_table(BGPRouteTable<A> *old_next_table,
				   BGPRouteTable<A> *new_next_table)
{
    typename NextTableMap<A>::iterator iter;
    iter = _next_tables.find(old_next_table);
    if (iter == _next_tables.end()) {
	// the next_table is not already in the set
	XLOG_FATAL("Attempt to remove table that is not in list: %s",
		   old_next_table->tablename().c_str());
    }

    const PeerHandler *peer = iter.second().peer_handler();
    uint32_t genid = iter.second().genid();

    _next_tables.erase(iter);
    _next_tables.insert(new_next_table, peer, genid);
    return 0;
}

template<class A>
int
FanoutTable<A>::add_route(const InternalMessage<A> &rtmsg,
			  BGPRouteTable<A> *caller) 
{
    debug_msg("\n         %s\n caller: %s\n rtmsg: %p route: %p\n%s\n",
	      this->tablename().c_str(),
	      caller ? caller->tablename().c_str() : "NULL",
	      &rtmsg,
	      rtmsg.route(),
	      rtmsg.str().c_str());

    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(rtmsg.route()->nexthop_resolved());

    const PeerHandler *origin_peer = rtmsg.origin_peer();

    log("add_route rcvd, net: " + rtmsg.route()->net().str()
	+ " peer: " + origin_peer->peername()
	+ c_format(" filters: %p,%p,%p",
		   rtmsg.route()->policyfilter(0).get(),
		   rtmsg.route()->policyfilter(1).get(),
		   rtmsg.route()->policyfilter(2).get()));

    typename NextTableMap<A>::iterator i = _next_tables.begin();
    list <PeerTableInfo<A>*> queued_peers;
    while (i != _next_tables.end()) {
	const PeerHandler *next_peer = i.second().peer_handler();
	if (origin_peer == next_peer) {
	    // don't send the route back to the peer it came from
	    debug_msg("FanoutTable<IPv%u%s>::add_route %p.\n  Don't send back to %s\n",
		      XORP_UINT_CAST(A::ip_version()),
		      pretty_string_safi(this->safi()),
		      &rtmsg, (i.first())->tablename().c_str());
	} else {
	    debug_msg("FanoutTable<IPv%u%s>::add_route %p to %s\n",
		      XORP_UINT_CAST(A::ip_version()),
		      pretty_string_safi(this->safi()),
		      &rtmsg, (i.first())->tablename().c_str());
	    //we now always add to the queue, and require a
	    //get_next_message from downstream to liberate a queued
	    //entry.
	    debug_msg("Fanout: queuing route, queue len is %u\n",
		      XORP_UINT_CAST(queued_peers.size()));
	    queued_peers.push_back(&(i.second()));

	    // we don't bother to return ADD_UNUSED or ADD_FILTERED,
	    // but if there's a failure, we'll return the last failure
	    // code.  Not clear how useful this is, but best not to
	    // hide the failure completely.
	}
	i++;
    }
    //queue it if anyone needs it.
    if (queued_peers.empty() == false) {
	add_to_queue(RTQUEUE_OP_ADD, rtmsg, queued_peers);
	wakeup_downstream(queued_peers);
    }

    return ADD_USED;
}

template<class A>
int
FanoutTable<A>::replace_route(const InternalMessage<A> &old_rtmsg,
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
    XLOG_ASSERT(old_rtmsg.route()->nexthop_resolved());
    XLOG_ASSERT(new_rtmsg.route()->nexthop_resolved());

    const PeerHandler *origin_peer = old_rtmsg.origin_peer();
    XLOG_ASSERT(origin_peer == new_rtmsg.origin_peer());

    log("replace_route rcvd, net: " + old_rtmsg.route()->net().str()
	+ " peer: " + origin_peer->peername());

    list <PeerTableInfo<A>*> queued_peers;
    typename NextTableMap<A>::iterator i;
    for (i = _next_tables.begin();  i != _next_tables.end();  i++) {
	const PeerHandler *next_peer = i.second().peer_handler();
	if (origin_peer == next_peer) {
	    // don't send the route back to the peer it came from
	} else {
	    debug_msg("FanoutTable<IPv%u:%s>::replace_route %p -> %p to %s\n",
		      XORP_UINT_CAST(A::ip_version()),
		      pretty_string_safi(this->safi()),
		      &old_rtmsg, &new_rtmsg,
		      (i.first())->tablename().c_str());

	    debug_msg("queueing replace_route\n");
	    queued_peers.push_back(&(i.second()));

	}
    }
    if (queued_peers.empty() == false) {
	add_replace_to_queue(old_rtmsg, new_rtmsg, queued_peers);
	wakeup_downstream(queued_peers);
    }

    return ADD_USED;
}

template<class A>
int
FanoutTable<A>::delete_route(const InternalMessage<A> &rtmsg,
			     BGPRouteTable<A> *caller) 
{
    debug_msg("\n         %s\n caller: %s\n rtmsg: %p route: %p\n%s\n",
	      this->tablename().c_str(),
	      caller ? caller->tablename().c_str() : "NULL",
	      &rtmsg,
	      rtmsg.route(),
	      rtmsg.str().c_str());
    
    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(rtmsg.route()->nexthop_resolved());

    const PeerHandler *origin_peer = rtmsg.origin_peer();

    log("delete_route rcvd, net: " + rtmsg.route()->net().str()
	+ " peer: " + origin_peer->peername()
	+ c_format(" filters: %p,%p,%p",
		   rtmsg.route()->policyfilter(0).get(),
		   rtmsg.route()->policyfilter(1).get(),
		   rtmsg.route()->policyfilter(2).get()));


    list <PeerTableInfo<A>*> queued_peers;
    typename NextTableMap<A>::iterator i;
    for (i = _next_tables.begin();  i != _next_tables.end();  i++) {
	const PeerHandler *next_peer = i.second().peer_handler();
	if (origin_peer == next_peer) {
	    debug_msg("FanoutTable<IPv%u:%s>::delete_route %p.\n  Don't send back to %s\n",
		      XORP_UINT_CAST(A::ip_version()),
		      pretty_string_safi(this->safi()),
		      &rtmsg, (i.first())->tablename().c_str());
	} else {
	    debug_msg("FanoutTable<IPv%u:%s>::delete_route %p to %s\n",
		      XORP_UINT_CAST(A::ip_version()),
		      pretty_string_safi(this->safi()),
		      &rtmsg, (i.first())->tablename().c_str());
	    queued_peers.push_back(&(i.second()));

	}
    }
    if (queued_peers.empty() == false) {
	add_to_queue(RTQUEUE_OP_DELETE, rtmsg, queued_peers);
	wakeup_downstream(queued_peers);
    }

    return 0;
}

template<class A>
int
FanoutTable<A>::route_dump(const InternalMessage<A> &rtmsg,
			   BGPRouteTable<A> *caller,
			   const PeerHandler *dump_peer) 
{
    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(rtmsg.route()->nexthop_resolved());

    log("route_dump, net: " + rtmsg.route()->net().str()
	+ " dump peer: " + dump_peer->peername());

    BGPRouteTable<A> *dump_child = 0;
    typename NextTableMap<A>::iterator i;
    for (i = _next_tables.begin();  i != _next_tables.end();  i++) {
	if (i.second().peer_handler() == dump_peer) {
	    dump_child = i.first();
	    break;
	}
    }
    XLOG_ASSERT(i != _next_tables.end());

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
    log("received push");
    XLOG_ASSERT(caller == this->_parent);
    list <PeerTableInfo<A>*> queued_peers;
    typename NextTableMap<A>::iterator i;
    for (i = _next_tables.begin();  i != _next_tables.end();  i++) {
	// a push needs to go to all peers because an add may cause a
	// delete (or vice versa) of a route that originated from a
	// different peer.
	queued_peers.push_back(&(i.second()));
    }

    if (queued_peers.empty() == false) {
	// if the origin peer we send to add_push_to_queue is NULL, the
	// push will go to all peers
	add_push_to_queue(queued_peers, NULL);
	wakeup_downstream(queued_peers);
    }

    return 0;
}

template<class A>
void
FanoutTable<A>::wakeup_downstream(list <PeerTableInfo<A>*>& queued_peers)
{
    //send a wakeup downstream if they're currently waiting
    typename list <PeerTableInfo<A>*>::iterator i;
    for (i = queued_peers.begin(); i != queued_peers.end(); i++) {
	if ((*i)->is_ready()) {
	    (*i)->wakeup_sent();
	    (*i)->route_table()->wakeup();
	}
    }
}

template<class A>
const SubnetRoute<A>*
FanoutTable<A>::lookup_route(const IPNet<A> &net, uint32_t& genid) const 
{
    return this->_parent->lookup_route(net, genid);
}

template<class A>
string
FanoutTable<A>::str() const 
{
    string s = "FanoutTable<A>" + this->tablename();
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
    debug_msg("remove: %s\n", dump_table->str().c_str());

    typename set <DumpTable<A>*>::iterator i;
    i = _dump_tables.find(dump_table);
    XLOG_ASSERT(i != _dump_tables.end());
    _dump_tables.erase(i);
}

template<class A>
void
FanoutTable<A>::peer_table_info(list<const PeerTableInfo<A>*>& peer_list) {
    typename NextTableMap<A>::iterator i;
    
    for (i = _next_tables.begin(); i != _next_tables.end(); i++) {
	if (i.second().peer_handler() != NULL)
	    peer_list.push_back(&(i.second()));
    }

}

template<class A>
int
FanoutTable<A>::dump_entire_table(BGPRouteTable<A> *child_to_dump_to,
				  Safi safi,
				  string ribname)
{
    XLOG_ASSERT(child_to_dump_to->type() != DUMP_TABLE);

    typename NextTableMap<A>::iterator i;
    PeerTableInfo<A> *peer_info = NULL;
    list <const PeerTableInfo<A>*> peer_list;
    for (i = _next_tables.begin(); i != _next_tables.end(); i++) {
	if (i.second().peer_handler() != NULL)
	    peer_list.push_back(&(i.second()));
	if (i.first() == child_to_dump_to)
	    peer_info = &(i.second());
    }

    // add the aggregation table / handler at the end of the list
    if (_aggr_peerinfo != NULL)
	peer_list.push_back(_aggr_peerinfo);

    XLOG_ASSERT(peer_info != NULL);
    const PeerHandler *peer_handler = peer_info->peer_handler();

    string tablename = string(ribname + "DumpTable" +
			      peer_handler->peername());
    DumpTable<A>* dump_table =
	new DumpTable<A>(tablename, peer_handler, peer_list,
			 (BGPRouteTable<A>*)this, safi);

    dump_table->set_next_table(child_to_dump_to);
    child_to_dump_to->set_parent(dump_table);
    replace_next_table(child_to_dump_to, dump_table);

    //find the new peer_info (we just deleted the old one)
    peer_info = NULL;
    for (i = _next_tables.begin(); i != _next_tables.end(); i++) {
	if (i.first() == dump_table)
	    peer_info = &(i.second());
    }
    XLOG_ASSERT(peer_info != NULL);

    add_dump_table(dump_table);

    dump_table->initiate_background_dump();
    return 0;
}

/* mechanisms to implement flow control in the output plumbing */
template<class A>
void
FanoutTable<A>::add_to_queue(RouteQueueOp operation,
			     const InternalMessage<A> &rtmsg,
			     const list<PeerTableInfo<A>*>& queued_peers) 
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
	queue_entry->set_push(true);

    // we are the end stop for this rtmsg so unref the route if needed
    if (rtmsg.changed())
	rtmsg.inactivate();
}

template<class A>
void
FanoutTable<A>::add_replace_to_queue(const InternalMessage<A> &old_rtmsg,
				     const InternalMessage<A> &new_rtmsg,
				     const list<PeerTableInfo<A>*>&
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
	    //add_push_to_queue(queued_peers, new_rtmsg.origin_peer());
	    queue_entry->set_push(true);
	else
	    // if the origin peer we send to add_push_to_queue, the push will
	    // go to all peers
	    add_push_to_queue(queued_peers, NULL);
    }

}

template<class A>
void
FanoutTable<A>::add_push_to_queue(const list<PeerTableInfo<A>*>&
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
FanoutTable<A>::set_queue_positions(const list<PeerTableInfo<A>*>&
				    queued_peers) 
{
    typename list<PeerTableInfo<A>*>::const_iterator i;
    for (i = queued_peers.begin(); i != queued_peers.end(); i++) {
	if ((*i)->has_queued_data() == false) {
	    /* set the queue position to the current last element */
	    (*i)->set_queue_position( --(_output_queue.end()) );
	    (*i)->set_has_queued_data(true);
	}
    }
}


template<class A>
bool
FanoutTable<A>::get_next_message(BGPRouteTable<A> *next_table) 
{
    debug_msg("next table: %s\n", next_table->tablename().c_str());
    print_queue();
    typename NextTableMap<A>::iterator i;
    i = _next_tables.find(next_table);
    XLOG_ASSERT(i != _next_tables.end());

    PeerTableInfo<A> *peer_info = &(i.second());
    peer_info->received_get();
    if (peer_info->has_queued_data() == false) {
	debug_msg("no data queued\n");
	peer_info->set_is_ready();
	log("get next message (non available): " 
	    + peer_info->peer_handler()->peername());
	return false;
    }

    log("get next message: " + peer_info->peer_handler()->peername());

    typename list<const RouteQueueEntry<A>*>::iterator queue_ptr;
    queue_ptr = peer_info->queue_position();
    bool discard_possible = false;

    switch ((*queue_ptr)->op()) {
    case RTQUEUE_OP_ADD: {
	debug_msg("OP_ADD, net=%s\n", 
		  (*queue_ptr)->route()->net().str().c_str());
	InternalMessage<A> rtmsg((*queue_ptr)->route(),
				 (*queue_ptr)->origin_peer(),
				 (*queue_ptr)->genid());
	if ((*queue_ptr)->push()) rtmsg.set_push();
	log("sending add_route: " + (*queue_ptr)->route()->net().str());
	next_table->add_route(rtmsg, (BGPRouteTable<A>*)this);
	break;
    }
    case RTQUEUE_OP_DELETE: {
	debug_msg("OP_DELETE\n");
	InternalMessage<A> rtmsg((*queue_ptr)->route(),
				 (*queue_ptr)->origin_peer(),
				 (*queue_ptr)->genid());
	if ((*queue_ptr)->push()) rtmsg.set_push();
	log("sending delete_route: " + (*queue_ptr)->route()->net().str());
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
	XLOG_ASSERT(queue_ptr != _output_queue.end());
	InternalMessage<A> new_rtmsg((*queue_ptr)->route(),
				     (*queue_ptr)->origin_peer(),
				     (*queue_ptr)->genid());
	if ((*queue_ptr)->push()) new_rtmsg.set_push();
	log("sending replace_route: " + (*queue_ptr)->route()->net().str());
	next_table->replace_route(old_rtmsg, new_rtmsg,
				  (BGPRouteTable<A>*)this);
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
	log("sending push");
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

	// sanity check
	if(queue_ptr == _output_queue.end()) {
	    // this shouldn't ever happen, as a REPLACE_NEW should
	    // follow a REPLACE_OLD
	    crash_dump();
	    XLOG_UNREACHABLE();
	}

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
	typename NextTableMap<A>::iterator nti;
	// iterating on a map isn't very efficient, but the map
	// shouldn't be all that large
	bool discard = true;
	for (nti = _next_tables.begin(); nti != _next_tables.end(); nti++) {
	    if (nti.second().has_queued_data()) {
		// if someone still references the queue head, we can't
		// discard.
		if (nti.second().queue_position() == _output_queue.begin())
		    discard = false;
	    }
	}
	if (discard) {
	    // check if the item to delete is a replace, in which case
	    // we need to delete both replace items
	    bool delete_two = false;
	    if (_output_queue.front()->op() == RTQUEUE_OP_REPLACE_OLD)
		delete_two = true;

	    delete _output_queue.front();
	    _output_queue.pop_front();

	    if (delete_two) {
		XLOG_ASSERT(_output_queue.front()->op() 
			    == RTQUEUE_OP_REPLACE_NEW);
		XLOG_ASSERT(!_output_queue.empty());
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
    typename NextTableMap<A>::iterator i;
    i = _next_tables.find(next_table);
    XLOG_ASSERT(i != _next_tables.end());

    PeerTableInfo<A> *peer_info = &(i.second());
    peer_info->peer_reset();
    if (peer_info->has_queued_data() == false)
	return;

    typename list<const RouteQueueEntry<A>*>::iterator queue_ptr;
    queue_ptr = peer_info->queue_position();
    bool more_queued_data = true;
    while (more_queued_data) {
	bool discard_possible = false;

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
	    queue_ptr++;
	    if (queue_ptr == _output_queue.end()) {
		break;
	    } else {
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
	    typename NextTableMap<A>::iterator nti;
	    // iterating on a map isn't very efficient, but the map
	    // shouldn't be all that large
	    bool discard = true;
	    for (nti = _next_tables.begin(); nti != _next_tables.end(); nti++) {
		if (nti.second().has_queued_data()) {
		    // if someone still references the queue head, we can't
		    // discard.
		    if (nti.second().queue_position() == _output_queue.begin())
			discard = false;
		}
	    }
	    if (discard) {
		// check if the item to delete is a replace, in which case
		// we need to delete both replace items
		bool delete_two = false;
		if (_output_queue.front()->op() == RTQUEUE_OP_REPLACE_OLD)
		    delete_two = true;

		delete _output_queue.front();
		_output_queue.pop_front();
		if (delete_two) {
		    XLOG_ASSERT(_output_queue.front()->op() 
				== RTQUEUE_OP_REPLACE_NEW);
		    XLOG_ASSERT(!_output_queue.empty());
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
    XLOG_ASSERT(this->_parent == caller);
    log("Peering went down: " + peer->peername());
    typename NextTableMap<A>::iterator i;
    for (i = _next_tables.begin();  i != _next_tables.end();  i++) {
	i.first()->peering_went_down(peer, genid, (BGPRouteTable<A>*)this);
    }
}

template<class A>
void
FanoutTable<A>::peering_down_complete(const PeerHandler *peer,
				      uint32_t genid,
				      BGPRouteTable<A> *caller) 
{
    XLOG_ASSERT(this->_parent == caller);
    log("Peering down complete: " + peer->peername());

    debug_msg("peer: %s genid: %u caller: %s\n",
	      peer->peername().c_str(), XORP_UINT_CAST(genid),
	      caller->tablename().c_str());

    print_queue();

    typename NextTableMap<A>::iterator i;
    for (i = _next_tables.begin();  i != _next_tables.end(); ) {
	BGPRouteTable<A>* next_table = i.first();
	//move the iterator on now, as peering_down_complete may cause
	//a dump table to unplumb itself.
	i++;
	next_table->peering_down_complete(peer, genid, 
					  (BGPRouteTable<A>*)this);
    }
}

template<class A>
void
FanoutTable<A>::peering_came_up(const PeerHandler *peer, uint32_t genid,
				BGPRouteTable<A> *caller) 
{
    XLOG_ASSERT(this->_parent == caller);
    log("Peering came up: " + peer->peername());
    typename NextTableMap<A>::iterator i;
    for (i = _next_tables.begin();  i != _next_tables.end();  i++) {
	i.first()->peering_came_up(peer, genid, (BGPRouteTable<A>*)this);
    }
}

template<class A>
void
FanoutTable<A>::print_queue() 
{
#ifdef DEBUG_QUEUE
    debug_msg("Rate control queue:\n");
    typename list <const RouteQueueEntry<A>*>::iterator i;
    int ctr = 0;
    for (i = _output_queue.begin(); i != _output_queue.end(); i++) {
	ctr++;
	debug_msg("%-5d %s\n", ctr, (*i)->str().c_str());
    }
#endif
}

template<class A>
string
FanoutTable<A>::dump_state() const {
    string s;
    s  = "=================================================================\n";
    s += "FanoutTable\n";
    s += "=================================================================\n";
    s += "Rate control queue:\n";
    typename list <const RouteQueueEntry<A>*>::const_iterator i;
    int ctr = 0;
    for (i = _output_queue.begin(); i != _output_queue.end(); i++) {
	ctr++;
	s += c_format("%-5d %s\n", ctr, (*i)->str().c_str());
	s += c_format("Parent now: %p\n", (*i)->route()->parent_route());
	s += c_format("Filters now: %p,%p,%p\n",
		      (*i)->route()->policyfilter(0).get(),
		      (*i)->route()->policyfilter(1).get(),
		      (*i)->route()->policyfilter(2).get());
		      
    }
    s += CrashDumper::dump_state();
    return s;
} 

template class PeerTableInfo<IPv4>;
template class PeerTableInfo<IPv6>;
template class FanoutTable<IPv4>;
template class FanoutTable<IPv6>;
