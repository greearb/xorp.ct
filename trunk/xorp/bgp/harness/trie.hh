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

// $XORP: xorp/bgp/harness/trie.hh,v 1.8 2003/09/11 03:38:38 atanu Exp $

#ifndef __BGP_HARNESS_TRIE_HH__
#define __BGP_HARNESS_TRIE_HH__

#include <stdio.h>
#include "libxorp/timeval.hh"
#include "bgp/packet.hh"
#include "trie_payload.hh"
#include "real_trie.hh"

/**
 * The trie stores BGP update packets the trie index is the
 * NLRI. A BGP update packet can contain multiple NLRI's. To save
 * the overhead of storing an update packet multiple times in the
 * trie a single copy of the update packet is kept. The Payload is
 * a reference to this single copy. Each update packet is chained
 * together on a linked list. New update packets are added to the
 * end of the list. In theory this ordered chained list structure
 * should make is very simple to print out the current update
 * packets that constitute the routing table. The ordering is
 * important if the same NLRI is contained in two packets then the
 * later one should be used.
 */
class Trie {
public:
    Trie() : _first(0), _last(0), _update_cnt(0) {
    }

    const UpdatePacket *lookup(const string& net) const;
    const UpdatePacket *lookup(const IPv4Net& net) const;
    const UpdatePacket *lookup(const IPv6Net& net) const;
    void process_update_packet(const TimeVal& tv, const uint8_t *buf,
			       size_t len);

    typedef RealTrie<IPv4>::TreeWalker TreeWalker_ipv4;
    typedef RealTrie<IPv6>::TreeWalker TreeWalker_ipv6;

    void tree_walk_table(const TreeWalker_ipv4& tw) const;
    void tree_walk_table(const TreeWalker_ipv6& tw) const;

    typedef XorpCallback2<void, const UpdatePacket*,
			  const TimeVal&>::RefPtr ReplayWalker;
    
    /**
     * Generate the set of update packets that would totally populate
     * this trie.
     *
     * @param uw The callback function that is called.
     */
    void replay_walk(const ReplayWalker uw) const;

    uint32_t update_count() {
	return _update_cnt;
    }

private:
    template <class A> void add(IPNet<A> net, TriePayload&);
    template <class A> void del(IPNet<A> net, TriePayload&);

    template <class A> void get_heads(RealTrie<A>*&, RealTrie<A>*&);

    RealTrie<IPv4> _head_ipv4;
    RealTrie<IPv4> _head_ipv4_del;
    RealTrie<IPv6> _head_ipv6;
    RealTrie<IPv6> _head_ipv6_del;

    TrieData *_first;
    TrieData *_last;

    uint32_t _update_cnt;	// Number of update packets seen
};

#endif // __BGP_HARNESS_TRIE_HH__

