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

// $XORP: xorp/bgp/route_table_reader.hh,v 1.2 2003/01/24 18:52:25 mjh Exp $

#ifndef __ROUTE_TABLE_READER_HH__
#define __ROUTE_TABLE_READER_HH__

#include "config.h"
#include "libxorp/xorp.h"
#include <map>
#include "libxorp/ipnet.hh"
#include "bgp_trie.hh"

template <class A>
class RibInTable;

template <class A>
class ReaderIxTuple {
public:
    typedef typename BgpTrie<A>::iterator trie_iterator;
    ReaderIxTuple(const IPv4& peer_id,
		  trie_iterator route_iter, 
		  const RibInTable<A>* _ribin);
    const A& masked_addr() const {return _net.masked_addr();}
    uint32_t prefix_len() const {return _net.prefix_len();}
    IPNet<A> net() const {return _net;}
    bool is_consistent() const;
    const IPv4& peer_id() const {return _peer_id;}
    const RibInTable<A>* ribin() const {return _ribin;}
    bool operator<(const ReaderIxTuple& them) const;
    trie_iterator& route_iterator()
    {
	return _route_iter;
    }
private:
    IPNet<A> _net;
    IPv4 _peer_id;
    trie_iterator _route_iter;
    const RibInTable<A>* _ribin;
};

template <class A>
class RouteTableReader {
public:
    typedef typename BgpTrie<A>::iterator trie_iterator;
    RouteTableReader(const list <RibInTable<A>*>& ribins);
    bool get_next(const SubnetRoute<A>*& route, IPv4& peer_id);
private:
    set <ReaderIxTuple<A>*> _peer_readers;
};

#endif // __ROUTE_TABLE_READER_HH__
