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

// $XORP: xorp/bgp/harness/trie.hh,v 1.18 2008/07/23 05:09:44 pavlin Exp $

#ifndef __BGP_HARNESS_TRIE_HH__
#define __BGP_HARNESS_TRIE_HH__

#include <stdio.h>
#include "libxorp/timeval.hh"
#include "libxorp/callback.hh"
#include "bgp/packet.hh"
#include "trie_payload.hh"
#include "real_trie.hh"
class BGPPeerData;

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
    Trie() : _first(0), _last(0), _update_cnt(0), _changes(0), 
	     _warning(true) {
    }

    const UpdatePacket *lookup(const string& net) const;
    const UpdatePacket *lookup(const IPv4Net& net) const;
    const UpdatePacket *lookup(const IPv6Net& net) const;
    void process_update_packet(const TimeVal& tv, const uint8_t *buf,
			       size_t len, const BGPPeerData *peerdata);

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
    void replay_walk(const ReplayWalker uw, const BGPPeerData *peerdata) const;

    uint32_t update_count() {
	return _update_cnt;
    }

    uint32_t changes() {
	return _changes;
    }

    void set_warning(bool warning) {
	_warning = warning;
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
    uint32_t _changes;		// Number of trie entry changes.

    bool _warning;		// Print warning messages;
};

#endif // __BGP_HARNESS_TRIE_HH__

