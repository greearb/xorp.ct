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

#ident "$XORP: xorp/bgp/dump_iterators.cc,v 1.17 2004/05/08 16:46:32 mjh Exp $"

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
PeerDumpState<A>::PeerDumpState(const PeerHandler* peer, uint32_t genid) 
{
    _peer = peer;
    _routes_dumped = false;
    _genid = genid;
    _delete_complete = false;
}

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
DumpIterator<A>::DumpIterator(const PeerHandler* peer,
			 const list <const PeerTableInfo<A>*>& peers_to_dump)
{
    debug_msg("peer: %p\n", peer);

    _peer = peer;
    list <const PeerTableInfo<A>*>::const_iterator i;
    int ctr = 0;
    for (i = peers_to_dump.begin(); i != peers_to_dump.end(); i++) {
	if ((*i)->peer_handler() != peer) {
	    debug_msg("adding peer %p to dump list\n", *i);
	    _peers_to_dump.push_back(**i);
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
    // record the genid of peer we just finished dumping
    if (_routes_dumped_on_current_peer)
	_dumped_peers.push_back(
              PeerDumpState<A>(_current_peer->peer_handler(), 
			       _current_peer->genid()));

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
    _peers_to_dump.push_front(PeerTableInfo<A>(NULL, peer, genid));
    _downed_peers.push_back(PeerDumpState<A>(peer, genid));
}

template <class A>
void
DumpIterator<A>::peering_went_down(const PeerHandler *peer, uint32_t genid)
{
    debug_msg("peering_went_down %p current_peer %p\n", peer, 
	      _current_peer->peer_handler());
    if (_current_peer == _peers_to_dump.end()
	&& waiting_for_deletion_completion()) {
	debug_msg("waiting for deletion completion\n");
	/* we don't care about this anymore*/
	return;
    }

    if (_current_peer->peer_handler() == peer) {
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
	    _downed_peers.push_back(PeerDumpState<A>(peer,
						  true,
						  _last_dumped_net,
						  genid));
	} else {
	    /* we hadn't yet dumped anything from this peer - make
               sure we don't propagate any deletes from it */
	    _downed_peers.push_back(PeerDumpState<A>(peer, genid));
	}
	/*skip to the next peer*/
	next_peer();
    } else {
	list <PeerTableInfo<A> >::iterator pi;
	for (pi = _current_peer; pi != _peers_to_dump.end(); pi++) {
	    if (pi->peer_handler() == peer) {
		/* the peer that went down hasn't been dumped yet */
		/* move it up the list so we don't dump it later */
		_peers_to_dump.insert(_current_peer, *pi);
		_peers_to_dump.erase(pi);
		/* add it to the downed peers list so we prevent
                   unnecessary deletions for this genid getting
                   though. */
		_downed_peers.push_back(PeerDumpState<A>(peer, genid));
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

    typename list <PeerDumpState<A> >::iterator i;
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
    typename list <PeerDumpState<A> >::const_iterator i;
    bool wait = false;
    for (i = _downed_peers.begin(); i != _downed_peers.end(); i++) {
	debug_msg("downed peers: %s\n", i->str().c_str());
	if (i->delete_complete() == false)
	    wait = true;
    }
    return wait;
}

template <class A>
void
DumpIterator<A>::peering_came_up(const PeerHandler *peer, uint32_t genid)
{
    debug_msg("peering_came_up %p genid %d\n", peer, genid);
    
    /* A peering came up.  Now, this peering could have been one that
       we know went down, because it went down after we started the
       down, or it could be one we've never heard of before.  In the
       later case, there could still be routes in a DeletionTable from
       before, so we need to record the GenID so we don't get confused
       between old and new routes */

    typename list <PeerDumpState<A> >::iterator i;
    for (i = _downed_peers.begin(); i != _downed_peers.end(); i++) {
	if (i->peer_handler() == peer) {
	    // OK, we've heard about this one before, so nothing more needed.
	    return;
	}
    }

    // perhaps it came up, went down, and came up again.  In which
    // case it may be in our new_peers list.
    for (i = _new_peers.begin(); i != _new_peers.end(); i++) {
	if (i->peer_handler() == peer) {
	    // OK, we've heard about this one before, so nothing more needed.
	    return;
	}
    }

    //sanity check.  If the peering came up, and it wasn't in the
    //downed_peers list, the we must have never have heard of it,
    list <PeerTableInfo<A> >::iterator pi;
    for (pi = _current_peer; pi != _peers_to_dump.end(); pi++) {
	XLOG_ASSERT(pi->peer_handler() != peer);
    }
    
    // this peer is a new one for us, so record it.
    _new_peers.push_back(PeerDumpState<A>(peer, false, IPNet<A>(), genid));
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
	cp(5);
	debug_msg("OTHER\n");
    }

    if (origin_peer == _current_peer->peer_handler()) {
	cp(6);
	debug_msg("it's the current peer\n");
	/* the change is from the peer we're currently dumping */
	if (_routes_dumped_on_current_peer == false) {
	    cp(7);
	    /* we haven't dumped anything from this peer yet*/
	    debug_msg("route iterator is not valid\n");
	    return false;
	}
	if (net <= _last_dumped_net) {
	    cp(8);
	    debug_msg("we've dumped this route: %s last dumped: %s\n",
		      net.str().c_str(), _last_dumped_net.str().c_str());
	    /*we've already dumped this route*/

	    if (_current_peer->genid() != genid) {
		cp(9);
		// the route comes from an old version of the Rib,
		// probably from a DeletionTable.
		debug_msg("Route %s genid %d, expected genid %d\n",
			  net.str().c_str(), _current_peer->genid(), genid);
		return false;
	    }
	    cp(10);
	    return true;
	} else {
	    cp(11);
	    debug_msg("we'll dump this route later. last dumped: %s\n",
		      _last_dumped_net.str().c_str());
	    /*we'll dump this later */
	    return false;
	}
    }
    list <PeerTableInfo<A> >::const_iterator pi;
    for (pi = _peers_to_dump.begin(); pi != _current_peer; pi++) {
	cp(12);
	if (pi->peer_handler() == origin_peer) {
	    cp(13);
	    /* this peer has already finished being dumped */
	    /* we need to check whether the peering went down while we
	       were dumping it */
	    typename list <PeerDumpState<A> >::iterator dpi;
	    for (dpi = _downed_peers.begin();
		 dpi != _downed_peers.end(); dpi++) {
		cp(14);
		if (origin_peer == dpi->peer_handler()) {
		    cp(15);
		    if (dpi->genid() == 0) {
			// should no longer happen
			XLOG_UNREACHABLE();
		    }

		    /*the peer had gone down when we were in the
                      middle of dumping it */
		    if (genid <= dpi->genid()) {
			cp(16);
			PeerDumpState<A>* tmp = &(*dpi);
			UNUSED(tmp);
			switch (op) {
			case RTQUEUE_OP_DELETE:
			case RTQUEUE_OP_REPLACE_OLD:
			    cp(17);
			    if (dpi->routes_dumped() == false) {
				cp(18);
				/* we'd not started dumping this
                                   peering when it went down */
				debug_msg("we'd not started dumping this peer\n");
				return false;
			    }
			    cp(19);
			    if (net <= dpi->last_net()) {
				cp(20);
				/* we'd already dumped this route, and now
				   it's being deleted */
		debug_msg("we've dumped this route: %s last dumped: %s\n",
			  net.str().c_str(), dpi->last_net().str().c_str());
				return true;
			    } else {
				cp(21);
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
		    cp(22);
		    assert(genid > dpi->genid());
		    /* the change comes from a later rib version
		       than the one we partially dumped */
		    debug_msg("change comes from later genid\n");
		    return true;
		}
	    }
	    cp(23);
	    /* the peer didn't go down in mid dump */
	    debug_msg("peer didn't go down in mid dump\n");

	    /* we need to check the genid of the routes we dumped in
	       case this is older */
	    for (dpi = _dumped_peers.begin();
		 dpi != _dumped_peers.end(); dpi++) {
		cp(24);
		if (origin_peer == dpi->peer_handler()) {
		    cp(25);
		    if (genid >= dpi->genid()) {
			cp(26);
			// it's a new change, so it's valid
			return true;
		    } else {
			cp(27);
			// change must have come from a DeletionTable
			XLOG_ASSERT(op != RTQUEUE_OP_REPLACE_NEW);
			XLOG_ASSERT(op != RTQUEUE_OP_ADD);
			return false;
		    }
		}
	    }
	    cp(28);
	    /* We don't have this peer in the _dumped_peers list. We
	       must have not dumped any routes from it */
	    switch (op) {
	    case RTQUEUE_OP_DELETE:
	    case RTQUEUE_OP_REPLACE_OLD:
		cp(29);
		/* if we didn't dump any routes from it, a delete
		   cannot be valid */
		return false;
	    case RTQUEUE_OP_REPLACE_NEW:
	    case RTQUEUE_OP_ADD:
		cp(30); 
		/* if we didn't dump any routes from it, an add must
		   be valid */
		/* make sure we keep track of the genid this time */
		_dumped_peers.push_back(PeerDumpState<A>(origin_peer, genid));
		return true;
	    default:
		XLOG_UNREACHABLE();
	    }
	    XLOG_UNREACHABLE();
	}
    }
    cp(32);
    for (pi == _current_peer; pi != _peers_to_dump.end(); pi++) {
	cp(33);
	if (pi->peer_handler() == origin_peer) {
	    cp(34); 
	    /* we haven't dumped this peer yet */
	    debug_msg("haven't dumped this peer yet\n");
	    return false;
	}
    }

    cp(35);
    /* if we got this far the peer isn't in our list of peers to dump,
       so it must have appeared since we started the dump, or it must
       be down */
    debug_msg("must be a new peer or must be down\n");

    typename list <PeerDumpState<A> >::const_iterator npi;
    for (npi = _new_peers.begin(); npi != _new_peers.end(); npi++) {
	cp(36);
	if (npi->peer_handler() == origin_peer) {
	    cp(37);
	    //It is a new peer.  Check the GenID in case this came
	    //from a DeletionTable on this peer.
	    if (genid >= npi->genid()) {
		cp(38);
		// the route change is not from before the peer came up
		return true;
	    } else {
		cp(39);
		// it must be from a DeletionTable, so it cannot be an add 
		XLOG_ASSERT(op != RTQUEUE_OP_REPLACE_NEW);
		XLOG_ASSERT(op != RTQUEUE_OP_ADD);
		return false;
	    }
	}
    }

    cp(40);
    debug_msg("we've never heard of the peer - it must be down\n");
    XLOG_ASSERT(op != RTQUEUE_OP_REPLACE_NEW);
    XLOG_ASSERT(op != RTQUEUE_OP_ADD);
    
    return false;
}

template class DumpIterator<IPv4>;
template class DumpIterator<IPv6>;
