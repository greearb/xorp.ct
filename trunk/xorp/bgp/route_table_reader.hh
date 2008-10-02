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

// $XORP: xorp/bgp/route_table_reader.hh,v 1.14 2008/07/23 05:09:37 pavlin Exp $

#ifndef __BGP_ROUTE_TABLE_READER_HH__
#define __BGP_ROUTE_TABLE_READER_HH__

#include "libxorp/xorp.h"
#include "libxorp/ipnet.hh"

#include <map>

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
    RouteTableReader(const list <RibInTable<A>*>& ribins,
		     const IPNet<A>& prefix);
    bool get_next(const SubnetRoute<A>*& route, IPv4& peer_id);
private:
    set <ReaderIxTuple<A>*> _peer_readers;
};

#endif // __BGP_ROUTE_TABLE_READER_HH__
