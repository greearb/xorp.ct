// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/dump_iterators.cc,v 1.23 2004/06/10 22:40:29 hodson Exp $"

//#define DEBUG_LOGGING
//#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "dump_iterators.hh"
#include "internal_message.hh"

//#define DEBUG_CODEPATH
#ifdef DEBUG_CODEPATH
#define cp(x) printf("DumpItCodePath: %2d\n", x)
#else
#define cp(x) {}
#endif

template <class A>
PeerDumpState<A>::PeerDumpState(const PeerHandler* peer,
				PeerDumpStatus status,
				uint32_t genid) 
{
    _peer = peer;
    _genid = genid;
    _status = status;
    _routes_dumped = false;

    debug_msg("%s", str().c_str());
}

#ifdef NOTDEF
template <class A>
PeerDumpState<A>::PeerDumpState(const PeerHandler* peer, uint32_t genid) 
{
    _peer = peer;
    _routes_dumped = false;
    _genid = genid;
}
#endif

template <class A>
string
PeerDumpState<A>::str() const
{
    return c_format("peer: %p routes_dumped: %s last_net: %s, genid: %d\n",
		    _peer, _routes_dumped ? "true" : "false",
		    _last_net_before_down.str().c_str(), _genid);

}

template <class A>
PeerDumpState<A>::~PeerDumpState()
{
}

template <class A>
void
PeerDumpState<A>::set_delete_complete(uint32_t genid)
{
    debug_msg("set_delete_complete: Peer: %p genid: %d\n", _peer, genid);
    typename set <uint32_t>::iterator i;
    i = _deleting_genids.find(genid);
    if (i != _deleting_genids.end()) {
	_deleting_genids.erase(i);
	return;
    } else {
	switch(_status) {
	case STILL_TO_DUMP:
	case CURRENTLY_DUMPING:
	    // this should never happen because we'd have recorded the
	    // genid when the peering went down, or at DumpTable
	    // startup.
	    XLOG_UNREACHABLE();
	    break;
	case DOWN_DURING_DUMP:
	case DOWN_BEFORE_DUMP:
	case COMPLETELY_DUMPED:
	case NEW_PEER:
	case FIRST_SEEN_DURING_DUMP:
	    // this can happen, because in these states we don't
	    // bother to record when the peering goes down again.
	    return;
	}
    }
}



template <class A>
DumpIterator<A>::DumpIterator(const PeerHandler* peer,
			 const list <const PeerTableInfo<A>*>& peers_to_dump)
{
    debug_msg("peer: %p\n", peer);

    _peer = peer;
    typename list <const PeerTableInfo<A>*>::const_iterator i;
    int ctr = 0;
    for (i = peers_to_dump.begin(); i != peers_to_dump.end(); i++) {
	if ((*i)->peer_handler() != peer) {
	    // Store it in the list of peers to dump.
	    // This determines the dump order.
	    debug_msg("adding peer %p to dump list\n", *i);
	    _peers_to_dump.push_back(**i);

	    // Also store it in the general peer state data
	    _peers[(*i)->peer_handler()] = 
		new PeerDumpState<A>((*i)->peer_handler(), STILL_TO_DUMP,
				     (*i)->genid());
	    ctr++;
	}
    }

    debug_msg("Peers to dump has %d members\n", ctr);
    _current_peer = _peers_to_dump.begin();
    if (_current_peer != _peers_to_dump.end()) {
	_current_peer_debug = &(*_current_peer);
	typename map <const PeerHandler*, 
		  PeerDumpState<A>* >::iterator state_i;
	state_i = _peers.find(_current_peer->peer_handler());
	XLOG_ASSERT(state_i != _peers.end());
	state_i->second->start_dump();
    } else {
	_current_peer_debug = NULL;
    }
    _route_iterator_is_valid = false;
    _routes_dumped_on_current_peer = false;
}

template <class A>
DumpIterator<A>::~DumpIterator()
{
    if (_route_iterator.cur() != NULL)
	debug_msg("refcnt: %d\n", _route_iterator.cur()->references());
    else 
	debug_msg("iterator not valid\n");
    //delete the payloads of the map
    typename map <const PeerHandler*, PeerDumpState<A>* >::iterator i;
    for (i = _peers.begin(); i!= _peers.end(); i++) {
	delete i->second;
    }
}


template <class A>
string
DumpIterator<A>::str() const
{
    return c_format("peer: %p last dumped net %s", _peer,
		    _last_dumped_net.str().c_str());
}

template <class A>
void
DumpIterator<A>::route_dump(const InternalMessage<A> &rtmsg)
{
    // XXX inefficient sanity checks
    XLOG_ASSERT(rtmsg.origin_peer() == _current_peer->peer_handler());
    typename map <const PeerHandler*, 
		  PeerDumpState<A>* >::iterator state_i;
    state_i = _peers.find(_current_peer->peer_handler());
    XLOG_ASSERT(state_i != _peers.end());
    debug_msg("route_dump: rtmsg.genid():%d state_i->second->genid():%d\n",
	   rtmsg.genid(), state_i->second->genid());
    switch (state_i->second->status()) {
    case STILL_TO_DUMP:
	debug_msg("STILL_TO_DUMP\n");
	break;
    case CURRENTLY_DUMPING:
	debug_msg("CURRENTLY_DUMPING\n");
	break;
    case DOWN_BEFORE_DUMP:
	debug_msg("DOWN_BEFORE_DUMP\n");
	break;
    case DOWN_DURING_DUMP:
    case COMPLETELY_DUMPED:
    case NEW_PEER:
    case FIRST_SEEN_DURING_DUMP:
	debug_msg("OTHER\n");
	break;
    }
    XLOG_ASSERT(rtmsg.genid() == state_i->second->genid());
    // end sanity checks

    _routes_dumped_on_current_peer = true;
    _last_dumped_net = rtmsg.net();
}

template <class A>
bool
DumpIterator<A>::is_valid() const
{
    // stupid hack to get around inability to compare const and
    // non-const iterators
    list <PeerTableInfo<A> > *peer_lst =
	const_cast<list <PeerTableInfo<A> > *>(&_peers_to_dump);

    if (_current_peer == peer_lst->end()) {
	debug_msg("Iterator is not valid\n");
	return false;
    }
    debug_msg("Iterator is valid\n");
    return true;
}

template <class A>
bool
DumpIterator<A>::next_peer()
{
    typename map <const PeerHandler*, 
		  PeerDumpState<A>* >::iterator state_i;
    state_i = _peers.find(_current_peer->peer_handler());
    XLOG_ASSERT(state_i != _peers.end());
    
    //we've finished with the old peer
    if (state_i->second->status() == CURRENTLY_DUMPING)
	state_i->second->set_dump_complete();

    //move to next undumped peer, if any remain
    while (state_i->second->status() != STILL_TO_DUMP) {
	_current_peer++;
	if (_current_peer == _peers_to_dump.end()) {
	    _current_peer_debug = NULL;
	    break;
	}
	_current_peer_debug = &(*_current_peer);

	state_i = _peers.find(_current_peer->peer_handler());
    }

    //record that we've started
    if (_current_peer != _peers_to_dump.end()) {
	state_i->second->start_dump();
    }


    // Make sure the iterator no longer points at a trie that may go away.
    typename BgpTrie<A>::iterator empty;
    _route_iterator = empty;	
    _route_iterator_is_valid = false;
    _routes_dumped_on_current_peer = false;
    if (_current_peer == _peers_to_dump.end())
	return false;
    return true;
}

/**
 * peering_is_down is called on DumpTable startup to indicate peerings
 * that already had DeletionTables in progress.
 */

template <class A>
void
DumpIterator<A>::peering_is_down(const PeerHandler *peer, uint32_t genid)
{
    XLOG_ASSERT(peer != _peer);

    debug_msg("peering_is_down %p genid %d\n", peer, genid);
    /*
     * first we must locate the appropriate state for this peer
     */
    typename map <const PeerHandler*, 
		  PeerDumpState<A>* >::iterator state_i;
    state_i = _peers.find(peer);

    if (state_i == _peers.end()) {
	_peers[peer] = new PeerDumpState<A>(peer,
					    DOWN_BEFORE_DUMP,
					    genid);
	_peers[peer]->set_delete_occurring(genid);
	return;
    }
   
    /*
     * what we do depends on the peer state
     */
    switch (state_i->second->status()) {

    case STILL_TO_DUMP:
    case CURRENTLY_DUMPING:
    case DOWN_BEFORE_DUMP:
	state_i->second->set_delete_occurring(genid);
	return;

    case DOWN_DURING_DUMP:
    case COMPLETELY_DUMPED:
    case NEW_PEER:
    case FIRST_SEEN_DURING_DUMP:
	XLOG_UNREACHABLE(); //only called at startup
	return;
    }
    XLOG_UNREACHABLE();
    
}

template <class A>
void
DumpIterator<A>::peering_went_down(const PeerHandler *peer, uint32_t genid)
{
    XLOG_ASSERT(peer != _peer);

    /*
     * first we must locate the appropriate state for this peer
     */
    typename map <const PeerHandler*, 
		  PeerDumpState<A>* >::iterator state_i;
    state_i = _peers.find(peer);
    XLOG_ASSERT(state_i != _peers.end());

    /*
     * what we do depends on the peer state
     */
    switch (state_i->second->status()) {

    case STILL_TO_DUMP:
	state_i->second->set_down(genid);
	return;

    case CURRENTLY_DUMPING:
	if (_routes_dumped_on_current_peer) {
	    state_i->second->set_down_during_dump(_last_dumped_net, genid);
	} else {
	    state_i->second->set_down(genid);
	}
	next_peer();
	return;

    case DOWN_DURING_DUMP:
    case DOWN_BEFORE_DUMP:
	// it went down before - we don't care about it going down again.
	return;

    case COMPLETELY_DUMPED:
	// we've finished with it - we don't care about it going down now.
	return;

    case NEW_PEER:
    case FIRST_SEEN_DURING_DUMP:
	// nothing to do here.
	return;
    }
    XLOG_UNREACHABLE();
}

template <class A>
void
DumpIterator<A>::peering_down_complete(const PeerHandler *peer,
				       uint32_t genid)
{
    XLOG_ASSERT(peer != _peer);

    /*
     * first we must locate the appropriate state for this peer
     */
    typename map <const PeerHandler*, 
		  PeerDumpState<A>* >::iterator state_i;
    state_i = _peers.find(peer);
    XLOG_ASSERT(state_i != _peers.end());

    state_i->second->set_delete_complete(genid);
    return;
}

template <class A>
bool
DumpIterator<A>::waiting_for_deletion_completion() const
{
    typename map <const PeerHandler*, PeerDumpState<A>* >::const_iterator i;
    bool wait = false;
    for (i = _peers.begin(); i != _peers.end() && wait == false; i++) {
	if (i->second->delete_complete() == false) {
	    wait = true;
	    break;
	}
	switch (i->second->status()) {
	case STILL_TO_DUMP:
	case CURRENTLY_DUMPING:
	    wait = true;
	    break;
	case DOWN_DURING_DUMP:
	case DOWN_BEFORE_DUMP:
	case COMPLETELY_DUMPED:
	case NEW_PEER:
	case FIRST_SEEN_DURING_DUMP:
	    //don't care
	    break;
	}
    }
    return wait;
}

template <class A>
void
DumpIterator<A>::peering_came_up(const PeerHandler *peer, uint32_t genid)
{
    XLOG_ASSERT(peer != _peer);

    /*
     * first we must locate the appropriate state for this peer
     */
    typename map <const PeerHandler*, 
		  PeerDumpState<A>* >::iterator state_i;
    state_i = _peers.find(peer);

    if (state_i == _peers.end()) {
	// we've not heard about this one.
	_peers[peer] = new PeerDumpState<A>(peer, NEW_PEER, genid);
	return;
    }

    
    switch (state_i->second->status()) {
    case STILL_TO_DUMP:
    case CURRENTLY_DUMPING:
	XLOG_UNREACHABLE();
    case DOWN_DURING_DUMP:
    case DOWN_BEFORE_DUMP:
    case COMPLETELY_DUMPED:
    case NEW_PEER:
	// We don't care about these.
	return;
    case FIRST_SEEN_DURING_DUMP:
	// Anything prior to this must be obsolete data, but now the
	// peer has actually come up properly.  We need to record it as
	// a new peer now.
	_peers.erase(state_i);
	_peers[peer] = new PeerDumpState<A>(peer, NEW_PEER, genid);
	return;
    }
}


template <class A>
bool
DumpIterator<A>::route_change_is_valid(const PeerHandler* origin_peer,
				       const IPNet<A>& net,
				       uint32_t genid,
				       RouteQueueOp op)
{
    /* route_change_is_valid is called to determine whether or not a
       route add, delete, or replace should be passed through to the
       peer.  The answer depends on how far we've got in the route
       dump process.  true indicates that the change should be
       propagated downstream; false indicates it shouldn't because the
       dump will get to this eventually */

    /* If we've not yet dumped this peer, but it is in our list of
       peers to dump, then we return false, because we'll get to it
       later.
 
       If this route change is on the peer we're currently dumping,
       then the change is valid if we've passed that point in the
       route space, otherwise we'll get to it in due course.

       If we've already dumped this peer, then the change is if it's
       genid is at least as new as the genid when we dumped the peer.
       Otherwise the change may be an old background deletion finally
       getting round to deleting the route.
    */

    /* This is complicated by peers that come up and down during the
       dump process.

       If the peer is one that came up after we started dumping, then
       normally the route change will be valid, but it could be
       invalid if it has an older GenID that the GenID of the peer
       when it came up.  This might happen because there's a
       DeletionTable doing a background deletion for routes from
       before we started the dump.

       If the peer is one we've never heard of, then the change must
       be from a DeletionTable doing a background deletion for routes from
       before we started the dump.
    */


    debug_msg("peer:%p, pa:%s , gi:%d, op:%d\n",
	      origin_peer, net.str().c_str(), genid, op);
    switch (op) {
    case RTQUEUE_OP_DELETE:
	debug_msg("DELETE\n");
	cp(1);
	break;
    case RTQUEUE_OP_REPLACE_OLD:
	debug_msg("REPLACE_OLD\n");
	cp(2);
	break;
    case RTQUEUE_OP_REPLACE_NEW:
	debug_msg("REPLACE_NEW\n");
	cp(3);
	break;
    case RTQUEUE_OP_ADD:
	debug_msg("ADD\n");
	cp(4);
	break;
    default:
	XLOG_UNREACHABLE();
    }
    
    /*
     * first we must locate the appropriate state for this peer
     */
    typename map <const PeerHandler*, 
		  PeerDumpState<A>* >::iterator state_i;
    state_i = _peers.find(origin_peer);
    if (state_i == _peers.end()) {
	// We have never seen this peer before.  this can only happen
	// for peers that were down when we started the dump, but had
	// stuff on background tasks in DeletionTable or NextHopLookup.
	// We never pass these routes downstream.
	_peers[origin_peer] = new PeerDumpState<A>(origin_peer,
						   FIRST_SEEN_DURING_DUMP,
						   genid);
	// Don't pass downstream.
	return false;
    }


    if (genid < state_i->second->genid()) {
	// The route predates anything we know about.  We definitely
	// don't want to pass this downstream - it's an obsolete
	// route.
	return false;
    }

    switch (state_i->second->status()) {

    case STILL_TO_DUMP:
	debug_msg("STILL_TO_DUMP\n");
	XLOG_ASSERT(genid == state_i->second->genid());

	// Don't pass downstream - we'll dump it later
	return false;

    case CURRENTLY_DUMPING:
	debug_msg("CURRENTLY_DUMPING\n");
	XLOG_ASSERT(genid == state_i->second->genid());

	// Depends on whether we've dumped this route already.
	if (_routes_dumped_on_current_peer) {
	    if (net == _last_dumped_net
		|| net < _last_dumped_net) {
		//We have dumped this route already, so pass downstream.
		return true;
	    }
	}
	// Don't pass downstream - we'll dump it later
	return false;

    case DOWN_DURING_DUMP:
	debug_msg("DOWN_DURING_DUMP\n");
	if (genid == state_i->second->genid()) {
	    // The change is from the version of the rib we'd half dumped.
	    debug_msg("last_net: %s, net: %s\n",
		   state_i->second->last_net().str().c_str(),
		   net.str().c_str());
	    if (net ==state_i->second->last_net()
		|| net < state_i->second->last_net()) {
		//We have dumped this route already, so pass downstream.
		return true;
	    }
	    // Don't pass downstream - we hadn't dumped this before it
	    // went down.
	    return false;
	}
	// The change is from a later version of the rib, so we must
	// pass it on downstream.
	return true;

    case DOWN_BEFORE_DUMP:
	debug_msg("DOWN_BEFORE_DUMP\n");
	// This peer went down before we'd had a chance to dump it.
	if (genid == state_i->second->genid()) {
	    // We hadn't dumped these - don't pass downstream any changes.
	    return false;
	}
	// The change is from a later version of the rib, so we must
	// pass it on downstream.
	return true;

    case COMPLETELY_DUMPED:
	debug_msg("COMPLETELY_DUMPED\n");
	// We have already dumped this peer, so all changes are valid.
	return true;

    case NEW_PEER:
	debug_msg("NEW_PEER\n");
	// This peer came up while we were dumping, and we'd not
	// previously heard of it.  All changes are valid.
	return true;

    case FIRST_SEEN_DURING_DUMP:
	debug_msg("FIRST_SEEN_DURING_DUMP\n");
	XLOG_ASSERT(genid == state_i->second->genid());
	return false;
    }
    XLOG_UNREACHABLE();
}



template class DumpIterator<IPv4>;
template class DumpIterator<IPv6>;
