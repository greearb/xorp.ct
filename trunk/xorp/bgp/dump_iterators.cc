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

#ident "$XORP: xorp/bgp/dump_iterators.cc,v 1.10 2004/03/04 03:35:24 pavlin Exp $"

// #define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "dump_iterators.hh"
#include "internal_message.hh"

template <class A>
DownedPeer<A>::DownedPeer(const PeerHandler* peer,
			     bool routes_dumped,
			     const IPNet<A>& last_net,
			     uint32_t genid) 
{
    _peer = peer;
    _routes_dumped = routes_dumped;
    if (routes_dumped)
	_last_net_before_down = last_net;
    _genid = genid;
    _delete_complete = false;

    debug_msg("%s", str().c_str());
}

template <class A>
string
DownedPeer<A>::str() const
{
    return c_format("peer: %p routes_dumped: %s last_net: %s, genid: %d\n",
		    _peer, _routes_dumped ? "true" : "false",
		    _last_net_before_down.str().c_str(), _genid);

}

template <class A>
DownedPeer<A>::~DownedPeer()
{
}

template <class A>
DumpIterator<A>::DumpIterator(const PeerHandler* peer,
			 const list <const PeerHandler*>& peers_to_dump)
{
    debug_msg("peer: %p\n", peer);

    _peer = peer;
    list <const PeerHandler*>::const_iterator i;
    int ctr = 0;
    for (i = peers_to_dump.begin(); i != peers_to_dump.end(); i++) {
	if (*i != peer) {
	    debug_msg("adding peer %p to dump list\n", *i);
	    _peers_to_dump.push_back(*i);
	    ctr++;
	}
    }
    debug_msg("Peers to dump has %d members\n", ctr);
    _current_peer = _peers_to_dump.begin();
    _route_iterator_is_valid = false;
    //    _rib_version = 0;
    _routes_dumped_on_current_peer = false;
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
    _routes_dumped_on_current_peer = true;
    _last_dumped_net = rtmsg.net();
}

template <class A>
bool
DumpIterator<A>::is_valid() const
{
    // stupid hack to get around inability to compare const and
    // non-const iterators
    list <const PeerHandler*> *peer_lst =
	const_cast<list <const PeerHandler*> *>(&_peers_to_dump);

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

    _current_peer++;
    // Make sure the iterator no longer points at a trie that may go away.
    typename BgpTrie<A>::iterator empty;
    _route_iterator = empty;	
    _route_iterator_is_valid = false;
    _routes_dumped_on_current_peer = false;
    if (_current_peer == _peers_to_dump.end())
	return false;
    return true;
}

template <class A>
void
DumpIterator<A>::peering_is_down(const PeerHandler *peer, uint32_t genid)
{
    debug_msg("peering_is_down %p genid %d\n", peer, genid);
    _peers_to_dump.push_back(peer);
    _downed_peers.push_back(DownedPeer<A>(peer, false, IPNet<A>(), genid));
}

template <class A>
void
DumpIterator<A>::peering_went_down(const PeerHandler *peer, uint32_t genid)
{
    debug_msg("peering_went_down %p current_peer %p\n", peer, *_current_peer);
    if (_current_peer == _peers_to_dump.end()
	&& waiting_for_deletion_completion()) {
	debug_msg("waiting for deletion completion\n");
	/* we don't care about this anymore*/
	return;
    }

    if (*_current_peer == peer) {
	/* The peer we're dumping went down */
	if (_routes_dumped_on_current_peer) {
	    /* This is a pain because we're going to get deletes for all the
	       routes from this peer, but we only want the ones for Subnets
	       that are before the one in the current route iterator.

	       We need to store the peer and last subnet seen from this
	       peer so we can correctly filter future deletes or replaces.
	       But the peering can come back up, and go back down again
	       causing more adds and more deletes, all of which might reach
	       the DumpTable before the dump has completed.  To distinguish
	       this, we use the genid from the route message - if we've got a
	       valid iterator, then we'll have received a dump, and so we'll
	       have a valid genid for the ribin.  We can filter using this
	       information */
	    _downed_peers.push_back(DownedPeer<A>(peer,
						  true,
						  _last_dumped_net,
						  genid));
	} else {
	    /* we hadn't yet dumped anything from this peer - make
               sure we don't propagate any deletes from it */
	    _downed_peers.push_back(DownedPeer<A>(peer, false, IPNet<A>(),
						  genid));
	}
	/*skip to the next peer*/
	next_peer();
    } else {
	list <const PeerHandler*>::iterator pi;
	for (pi = _current_peer; pi != _peers_to_dump.end(); pi++) {
	    if (*pi == peer) {
		/* the peer that went down hasn't been dumped yet */
		/* move it up the list so we don't dump it later */
		_peers_to_dump.erase(pi);
		_peers_to_dump.insert(_current_peer, peer);
		/* add it to the downed peers list so we prevent
                   unnecessary deletions for this genid getting
                   though. */
		_downed_peers.push_back(DownedPeer<A>(peer, false, IPNet<A>(),
						      genid));
		return;
	    }
	}
	/* If we get to here, we've already dumped this peer, or it's
           not in our original dumplist because it came up after our
           dump started.  Either way, we don't need to do anything */
    }
}

template <class A>
void
DumpIterator<A>::peering_down_complete(const PeerHandler *peer,
				       uint32_t genid)
{
    debug_msg("peering_down_complete %p genid %d\n", peer, genid);

    typename list <DownedPeer<A> >::iterator i;
    bool delete_complete = true;
    for (i = _downed_peers.begin(); i != _downed_peers.end(); i++) {
	if ((i->peer_handler() == peer) && (i->genid() == genid)){
	    i->set_delete_complete();
	}
	if (i->delete_complete() == false)
	    delete_complete = false;
    }
}

template <class A>
bool
DumpIterator<A>::waiting_for_deletion_completion() const
{
    typename list <DownedPeer<A> >::const_iterator i;
    bool wait = false;
    for (i = _downed_peers.begin(); i != _downed_peers.end(); i++) {
	debug_msg("downed peers: %s\n", i->str().c_str());
	if (i->delete_complete() == false)
	    wait = true;
    }
    return wait;
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
    debug_msg("peer:%p, pa:%s , gi:%d, op:%d\n",
	      origin_peer, net.str().c_str(), genid, op);
    switch (op) {
    case RTQUEUE_OP_DELETE:
	debug_msg("DELETE\n");
	break;
    case RTQUEUE_OP_REPLACE_OLD:
	debug_msg("REPLACE_OLD\n");
	break;
    case RTQUEUE_OP_REPLACE_NEW:
	debug_msg("REPLACE_NEW\n");
	break;
    case RTQUEUE_OP_ADD:
	debug_msg("ADD\n");
	break;
    default:
	debug_msg("OTHER\n");
    }

    if (origin_peer == *_current_peer) {
	debug_msg("it's the current peer\n");
	/* the change is from the peer we're currently dumping */
	if (_routes_dumped_on_current_peer == false) {
	    /* we haven't dumped anything from this peer yet*/
	    debug_msg("route iterator is not valid\n");
	    return false;
	}
	if (net <= _last_dumped_net) {
	    debug_msg("we've dumped this route: %s last dumped: %s\n",
		      net.str().c_str(), _last_dumped_net.str().c_str());
	    /*we've already dumped this route*/
	    return true;
	} else {
	    debug_msg("we'll dump this route later. last dumped: %s\n",
		      _last_dumped_net.str().c_str());
	    /*we'll dump this later */
	    return false;
	}
    }
    list <const PeerHandler*>::const_iterator pi;
    for (pi = _peers_to_dump.begin(); pi != _current_peer; pi++) {
	if (*pi == origin_peer) {
	    /* this peer has already finished being dumped */
	    /* we need to check whether the peering went down while we
	       were dumping it */
	    typename list <DownedPeer<A> >::iterator dpi;
	    for (dpi = _downed_peers.begin();
		 dpi != _downed_peers.end(); dpi++) {
		if (origin_peer == dpi->peer_handler()) {
		    if (dpi->genid() == 0) {
			// should no longer happen
			XLOG_UNREACHABLE();
		    }

		    /*the peer had gone down when we were in the
                      middle of dumping it */
		    if (genid == dpi->genid()) {
			DownedPeer<A>* tmp = &(*dpi);
			UNUSED(tmp);
			switch (op) {
			case RTQUEUE_OP_DELETE:
			case RTQUEUE_OP_REPLACE_OLD:
			    if (dpi->routes_dumped() == false) {
				/* we'd not started dumping this
                                   peering when it went down */
				debug_msg("we'd not started dumping this peer\n");
				return false;
			    }
			    if (net <= dpi->last_net()) {
				/* we'd already dumped this route, and now
				   it's being deleted */
		debug_msg("we've dumped this route: %s last dumped: %s\n",
			  net.str().c_str(), dpi->last_net().str().c_str());
				return true;
			    } else {
				/* we'd not dumped this one yet */
				debug_msg("we'd not yet dumped this one\n");
				return false;
			    }
			    break;
			case RTQUEUE_OP_ADD:
			    /* this would be if we'd got an add for the
			       same genid as when the peering went down -
			       doesn't make much sense */
			    XLOG_UNREACHABLE();
			default:
			    // shouldn't get anything else
			    XLOG_UNREACHABLE();
			}
		    }
		    assert(genid != dpi->genid());
		    /* the change comes from a later rib version
		       than the one we partially dumped */
		    debug_msg("change comes from later genid\n");
		    return true;
		}
	    }
	    /* the peer didn't go down in mid dump */
	    debug_msg("peer didn't go down in mid dump\n");
	    return true;
	}
    }
    for (pi == _current_peer; pi != _peers_to_dump.end(); pi++) {
	if (*pi == origin_peer) {
	    /* we haven't dumped this peer yet */
	    debug_msg("haven't dumped this peer yet\n");
	    return false;
	}
    }

    /* if we got this far the peer isn't in our list of peers to dump,
       so it must have appeared since we started the dump */
    debug_msg("must be a new peer\n");
    return true;
}

template class DumpIterator<IPv4>;
template class DumpIterator<IPv6>;
