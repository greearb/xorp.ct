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

// $XORP: xorp/devnotes/template.hh,v 1.2 2003/01/16 19:08:48 mjh Exp $

#ifndef __ROUTE_TABLE_READER_HH__
#define __ROUTE_TABLE_READER_HH__

#include "config.h"
#include "libxorp/xorp.h"
#include <map>
#include "libxorp/ipnet.hh"
#include "bgp_trie.hh"

template <class A>
class ReaderIxTuple {
public:
    ReaderIxTuple(const IPv4& peer_id,
		  BGPTrie<A>::iterator route_iter, const RibInTable* _ribin);
    const A& base_addr() const;
    uint32_t prefix_len() const;
    IPNet<A> net() const;
    const IPv4& peer_id() const {return _peer_id;}
    RibInTable* ribin() const {return _ribin;}
    bool operator<(const ReaderIxTuple& them) const;
    const BGPTrie<A>::iterator& iterator() const {return _route_iter;}
private:
    IPv4 _peer_id;
    BGPTrie<A>::iterator _route_iter;
    RibInTable* _ribin;
};

template <class A>
class RouteTableReader {
public:
    RouteTableReader(const list <RibInTable*>& ribins);
private:
    set <ReaderIxTuple<A> > _peer_readers;
};

#endif // __ROUTE_TABLE_READER_HH__
