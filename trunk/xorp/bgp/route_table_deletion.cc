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

#ident "$XORP: xorp/bgp/route_table_deletion.cc,v 1.15 2004/05/14 18:30:07 mjh Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "route_table_deletion.hh"

template<class A>
DeletionTable<A>::DeletionTable(string table_name,
				Safi safi,
				BgpTrie<A>* route_table,
				const PeerHandler *peer,
				uint32_t genid,
				BGPRouteTable<A> *parent_table)
    : BGPRouteTable<A>("DeletionTable-" + table_name, safi)
{
    this->_parent = parent_table;
    _genid = genid;
    _route_table = route_table;
    _peer = peer;
    this->_next_table = 0;
}

template<class A>
DeletionTable<A>::~DeletionTable()
{
    _route_table->delete_self();
}

template<class A>
int
DeletionTable<A>::add_route(const InternalMessage<A> &rtmsg,
			    BGPRouteTable<A> *caller)
{
    debug_msg("DeletionTable<A>::add_route %x on %s\n",
	      (u_int)(&rtmsg), this->tablename().c_str());
    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(this->_next_table != NULL);

    IPNet<A> net = rtmsg.net();

    // check if we have this route in our deletion cache
    typename BgpTrie<A>::iterator iter;
    iter = _route_table->lookup_node(net);

    if (iter == _route_table->end()) {
	return this->_next_table->add_route(rtmsg, (BGPRouteTable<A>*)this);
    } else {
	const SubnetRoute<A> *existing_route = &(iter.payload());
	// We have a copy of this route in our deletion cache.

    	XLOG_ASSERT(existing_route->net() == rtmsg.net());

	// Preserve the route.  Taking a reference will prevent the
	// route being deleted when it's erased from the Trie.
	// Deletion will occur when the reference goes out of scope.
	SubnetRouteConstRef<A> route_reference(existing_route);

	// delete from the Trie
	if ((_del_sweep->second->net() == rtmsg.net()) &&
	    (_del_sweep->second->prev() == _del_sweep->second)) {
	    // we're about to delete the chain that's pointed to by our
	    // iterator, so move the iterator on now.
	    _del_sweep++;
	}
	_route_table->erase(rtmsg.net());

	// propogate downstream
	InternalMessage<A> old_rt_msg(existing_route, _peer, _genid);
	old_rt_msg.set_from_previous_peering();
	return this->_next_table->replace_route(old_rt_msg, rtmsg,
					  (BGPRouteTable<A>*)this);
    }
    XLOG_UNREACHABLE();
}

template<class A>
int
DeletionTable<A>::replace_route(const InternalMessage<A> &old_rtmsg,
				const InternalMessage<A> &new_rtmsg,
				BGPRouteTable<A> *caller)
{
    debug_msg("DeletionTable<A>::replace_route %x -> %x on %s\n",
	      (u_int)(&old_rtmsg), (u_int)(&new_rtmsg), this->tablename().c_str());
    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(this->_next_table != NULL);
    XLOG_ASSERT(old_rtmsg.net() == new_rtmsg.net());
    // we should never see a replace for a net that's in the deletion cache
    XLOG_ASSERT(_route_table->lookup_node(old_rtmsg.net()) ==
	   _route_table->end());

    return this->_next_table->replace_route(old_rtmsg, new_rtmsg,
				      (BGPRouteTable<A>*)this);
}

template<class A>
int
DeletionTable<A>::route_dump(const InternalMessage<A> &rtmsg,
			     BGPRouteTable<A> *caller,
			     const PeerHandler *dump_peer)
{
    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(this->_next_table != NULL);

    /* A route dump must have been initiated after this table was
       created (because the creation of this table would terminate any
       previous route dump).  So the contents of this dump MUST NOT be
       in our table */
    XLOG_ASSERT(_route_table->lookup_node(rtmsg.net()) ==
	   _route_table->end());

    return this->_next_table->route_dump(rtmsg, (BGPRouteTable<A>*)this, dump_peer);
}

template<class A>
int
DeletionTable<A>::delete_route(const InternalMessage<A> &rtmsg,
			       BGPRouteTable<A> *caller)
{
    debug_msg("DeletionTable<A>::delete_route %x on %s\n",
	      (u_int)(&rtmsg), this->tablename().c_str());
    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(this->_next_table != NULL);
    // we should never see a delete for a net that's in the deletion cache
    XLOG_ASSERT(_route_table->lookup_node(rtmsg.net()) ==
	   _route_table->end());

    return this->_next_table->delete_route(rtmsg, (BGPRouteTable<A>*)this);
}

template<class A>
int
DeletionTable<A>::push(BGPRouteTable<A> *caller)
{
    XLOG_ASSERT(caller == this->_parent);
    return this->_next_table->push((BGPRouteTable<A>*)this);
}

template<class A>
const SubnetRoute<A>*
DeletionTable<A>::lookup_route(const IPNet<A> &net, 
			       uint32_t& genid) const
{
    // Even though the peering has gone down, we still need to answer
    // lookup requests.  This is because we need to be internally
    // consistent - the route is treated as still being active until we
    // explicitly tell the downstream tables that it has been deleted.
    typename BgpTrie<A>::iterator iter = _route_table->lookup_node(net);
    if (iter != _route_table->end()) {
	genid = _genid;
	return &(iter.payload());
    } else
	return this->_parent->lookup_route(net, genid);
}

template<class A>
void
DeletionTable<A>::route_used(const SubnetRoute<A>* rt, bool in_use)
{
    this->_parent->route_used(rt, in_use);
}

template<class A>
string
DeletionTable<A>::str() const
{
    string s = "DeletionTable<A>" + this->tablename();
    return s;
}

template<class A>
void
DeletionTable<A>::initiate_background_deletion()
{
    XLOG_ASSERT(this->_next_table != NULL);
    _del_sweep = _route_table->pathmap().begin();
    _deleted = 0;
    _chains = 0;

    // Make sure that anything previously sent by this peer has been
    // pushed from the output queue in the RibOut tables.
    this->_next_table->push(this);

    _deletion_timer = eventloop().
	new_oneoff_after_ms(0 /*call back immediately, but after
				network events or expired timers */,
			    callback(this,
				     &DeletionTable<A>::delete_next_chain));
}

template<class A>
void
DeletionTable<A>::delete_next_chain()
{
    debug_msg("deleted %d routes in %d chains\n", _deleted, _chains);
    if (_del_sweep == _route_table->pathmap().end()) {
	unplumb_self();
	delete this;
	return;
    }

    const ChainedSubnetRoute<A>* chained_rt, *first_rt, *next_rt;
    first_rt = chained_rt = _del_sweep->second;

    // increment the iterator here, before we delete the node, as
    // deletion may invalidate the iterator
    _del_sweep++;

    // erase the first_rt last
    chained_rt = chained_rt->next();

    while (1) {
	// preserve the information
	next_rt = chained_rt->next();
	// Preserve the route.  Taking a reference will prevent the
	// route being deleted when it's erased from the Trie.
	// Deletion will occur when the reference goes out of scope.
	SubnetRouteConstRef<A> route_reference(chained_rt);

	// delete from the Trie
	_route_table->erase(chained_rt->net());

	// propagate downstream
	InternalMessage<A> rt_msg(chained_rt, _peer, _genid);
	rt_msg.set_from_previous_peering();
	if (this->_next_table != NULL)
	    this->_next_table->delete_route(rt_msg, (BGPRouteTable<A>*)this);
	_deleted++;
	if (chained_rt == first_rt) {
	    debug_msg("end of chain\n");
	    break;
	} else {
	    debug_msg("chain continues\n");
	}
	chained_rt = next_rt;
    }
    if (this->_next_table != NULL)
	this->_next_table->push((BGPRouteTable<A>*)this);
    _chains++;

    _deletion_timer = eventloop().
	new_oneoff_after_ms(0 /*call back immediately, but after
				network events or expired timers */,
			    callback(this,
				     &DeletionTable<A>::delete_next_chain));
}

template<class A>
void
DeletionTable<A>::unplumb_self()
{
    debug_msg("unplumbing self\n");
    XLOG_ASSERT(this->_next_table != NULL);
    XLOG_ASSERT(this->_parent != NULL);
    XLOG_ASSERT(0 == _route_table->route_count());

    // signal downstream that we finished deleting routes from this
    // version of this RibIn.
    this->_next_table->peering_down_complete(_peer, _genid, this);

    this->_parent->set_next_table(this->_next_table);
    this->_next_table->set_parent(this->_parent);

    // ensure we can't continue to operate
    this->_next_table = (BGPRouteTable<A>*)0xd0d0;
    this->_parent = (BGPRouteTable<A>*)0xd0d0;
}

template class DeletionTable<IPv4>;
template class DeletionTable<IPv6>;




