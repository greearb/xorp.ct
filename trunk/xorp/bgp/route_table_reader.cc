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

#ident "$XORP: xorp/devnotes/template.cc,v 1.2 2003/01/16 19:08:48 mjh Exp $"

#include "bgp_module.h"
#include "route_table_reader.hh"

template <class A>
ReaderIxTable<A>::ReaderIxTuple(const IPv4& peer_id,
				BGPTrie<A>::iterator route_iter, 
				const RibInTable* ribin)
    : _peer_id(peer_id), _route_iter(route_iter), _ribin(ribin) 
{
}

template <class A>
IPNet<A>
ReaderIxTable<A>::net() const
{
    assert(_route_iter != _ribin->trie().end());
}

template <class A>
ReaderIxTable<A>::operator<(const ReaderIxTuple& them) const 
{
    if (base_addr() < them.base_addr()) {
	return true;
    }
    if (them.base_addr() < _base_addr()) {
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
RouteTableReader<A>::RouteTableReader(const list <RibInTable*>& ribins) 
{
    typename list <RibInTable*>::const_iterator i;
    for(i = ribins.begin(); i != ribins.end(); i++) {
	if ((*i)->rib_empty()==false) {
	    uint32_t table_version = (*i)->table_version();
	    BGPTrie<A>::iterator ti = (*i)->trie().begin();
	    IPNet<A> net = ti.net();
	    IPv4 peer_id = (*i)->peer_handler()->peer()->peer_id();
	    _peer_readers.insert(ReaderIxTuple<A>(net, peer_id, ti, (*i),
						  table_version));
	}
    }
}

template <class A>
bool
RouteTableReader<A>::get_next(SubnetRoute<A>*& route, IPv4& peer_id) 
{
    typename set <ReaderIxTuple>::iterator i;
    i = _peer_readers.begin();
    if (i == _peer_readers.end()) {
	return false;
    }
    ReaderIxTuple<A>* reader = &(*i);
    //XXX
}







