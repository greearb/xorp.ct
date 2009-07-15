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



#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "route_table_reader.hh"
#include "route_table_ribin.hh"
#include "peer_handler.hh"

template <class A>
ReaderIxTuple<A>::ReaderIxTuple(const IPv4& peer_id,
				trie_iterator route_iter, 
				const RibInTable<A>* ribin)
    : _peer_id(peer_id), _route_iter(route_iter), _ribin(ribin) 
{
    _net = _route_iter.key();
}

template <class A>
bool
ReaderIxTuple<A>::operator<(const ReaderIxTuple& them) const 
{
    if (masked_addr() < them.masked_addr()) {
	return true;
    }
    if (them.masked_addr() < masked_addr()) {
	return false;
    }
#if 0
    //The MIB requires it to be least-specific first
    if (prefix_len() < them.prefix_len()) {
	return true;
    }
    if (them.prefix_len() < _prefix_len()) {
	return false;
    }
#else
    //XXX But we don't have a Trie iterator for that, so we do most
    //specific first for now.
    if (prefix_len() < them.prefix_len()) {
	return false;
    }
    if (them.prefix_len() < prefix_len()) {
	return true;
    }
#endif
    if (_peer_id < them.peer_id()) {
	return true;
    }
    if (them.peer_id() < _peer_id) {
	return false;
    }
    return false;
}

template <class A>
bool
ReaderIxTuple<A>::is_consistent() const {
    if (_route_iter == _ribin->trie().end()) {
	// the route we were pointing at has been deleted, and there
	// are no routes after this in the trie.
	return false;
    }
    if (_route_iter.key() != _net) {
	// the route we were pointing at has been deleted, and the
	// iterator now points to a later subnet in the trie.
	return false;
    }
    return true;
}

template <class A>
RouteTableReader<A>::RouteTableReader(const list <RibInTable<A>*>& ribins,
				      const IPNet<A>& /*prefix*/)
{
    typename list <RibInTable<A>*>::const_iterator i;
    for(i = ribins.begin(); i != ribins.end(); i++) {
	trie_iterator ti = (*i)->trie().begin();
	if (ti != (*i)->trie().end()) {
	    IPv4 peer_id = (*i)->peer_handler()->id();
	    _peer_readers.insert(new ReaderIxTuple<A>(peer_id, ti, (*i)));
	}
    }
}

template <class A>
bool
RouteTableReader<A>::get_next(const SubnetRoute<A>*& route, IPv4& peer_id) 
{
    typename set <ReaderIxTuple<A>*>::iterator i;
    while (1) {
	i = _peer_readers.begin();
	if (i == _peer_readers.end()) {
	    return false;
	}
	ReaderIxTuple<A>* reader = *i;
	
	if (reader->is_consistent()) {
	    //return the route and the peer_id from the reader
	    route = &(reader->route_iterator().payload());
	    peer_id = reader->peer_id();

	    //if necessary, prepare this peer's reader for next time
	    reader->route_iterator()++;
	    if (reader->route_iterator() != reader->ribin()->trie().end()) {
		_peer_readers.insert(new ReaderIxTuple<A>(reader->peer_id(),
							  reader->
							     route_iterator(),
							  reader->ribin()));
	    }

	    //remove the obsolete reader
	    _peer_readers.erase(i);
	    delete reader;
	    return true;
	}

	//the iterator was inconsistent, so we need to re-insert a
	//consistent version into the set and try again.
	if (reader->route_iterator() != reader->ribin()->trie().end()) {
	    _peer_readers.insert(new ReaderIxTuple<A>(reader->peer_id(),
						      reader->route_iterator(),
						      reader->ribin()));
	}
	//remove the obsolete reader
	_peer_readers.erase(i);
	delete reader;
    }
}

template class RouteTableReader<IPv4>;
template class RouteTableReader<IPv6>;
