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

// $XORP: xorp/bgp/harness/trie.hh,v 1.5 2003/06/26 02:15:26 atanu Exp $

#ifndef __BGP_HARNESS_TRIE_HH__
#define __BGP_HARNESS_TRIE_HH__

#include <stdio.h>
#include "libxorp/timeval.hh"
#include "bgp/packet.hh"

class TrieData {
public:
    TrieData(const TimeVal& tv, const uint8_t *buf, size_t len,
	     TrieData* &first, TrieData* &last) : _tv(tv),
						_first(first),
						_last(last) {
	_packet = new UpdatePacket(buf, len);
	_refcnt = 1;

	_next = 0;
	_prev = _last;		

	if(0 == _first)
	    _first = this;
	if(0 != _last) 
	    _last->_next = this;
	_last = this;
    }

    void
    incr_refcnt() {
	_refcnt++;
    }

    bool
    decr_refcnt() {
	_refcnt--;

	XLOG_ASSERT(_refcnt >= 0);

	return 0 == _refcnt;
    }

    const UpdatePacket *data() const {
	XLOG_ASSERT(_refcnt > 0);

	return _packet;
    }

    const TimeVal& tv() const {
	return _tv;
    }

    ~TrieData() {
	XLOG_ASSERT(0 == _refcnt);

	if(this == _first)
	    _first = _next;
	else
	    _prev->_next = _next;
	if(this == _last)
	    _last = _prev;
	if(0 != _next) {
	    _next->_prev = _prev;
	}
// 	debug_msg("Deleting: %s\n", _packet->str().c_str());
	delete _packet;
    }

private:
    TimeVal _tv;
    TrieData *_first;
    TrieData *_last;

    int _refcnt;
    UpdatePacket *_packet;
    TrieData *_next;
    TrieData *_prev;
};

struct TriePayload {
    TriePayload() : _data(0) {}

    TriePayload(const TimeVal& tv, const uint8_t *buf, size_t len,
	    TrieData* &first, TrieData* &last) {
	_data = new TrieData(tv, buf, len, first, last);
    }

    ~TriePayload() {
	zap();
    }

    TriePayload(const TriePayload& rhs) {
	_data = 0;
	copy(rhs);
    }

    TriePayload operator=(const TriePayload& rhs) {
	if(&rhs == this)
	    return *this;
	zap();
	copy(rhs);
	    
	return *this;
    }

    void copy(const TriePayload& rhs) {
	if(rhs._data) {
// 	    debug_msg("refcnt: %d %#x\n", rhs._data->_refcnt + 1, rhs._data);
	    rhs._data->incr_refcnt();
	    _data = rhs._data;
	}
    }

    const UpdatePacket *get() const {
	if(0 == _data)
	    return 0;
	return _data->data();
    }

    const UpdatePacket *get(TimeVal& tv) const {
	if(0 == _data)
	    return 0;
	tv = _data->tv();
	return _data->data();
    }

    void
    zap() {
// 	if(_data)
// 	    debug_msg("refcnt: %d %#x\n", _data->_refcnt - 1, _data);
	if(_data && _data->decr_refcnt()) {
	    delete _data;
	}
	_data = 0;
    }

    TrieData *_data;
};

class Trie {
public:
    Trie() : _first(0), _last(0), _debug(true), _pretty(true), _update_cnt(0) {
	_head.p._data = 0;
    }

    ~Trie();

    const UpdatePacket *lookup(const string& net) const;
    void process_update_packet(const TimeVal& tv, const uint8_t *buf,
			       size_t len);

    typedef XorpCallback3<void, const UpdatePacket*,
			  const IPv4Net&,
			  const TimeVal&>::RefPtr TreeWalker;

    void tree_walk_table(const TreeWalker& ) const;

    void save_routing_table(FILE *fp) const;

    uint32_t update_count() {
	return _update_cnt;
    }
private:

    /*
    ** The trie stores BGP update packets the trie index is the
    ** NLRI. A BGP update packet can contain multiple NLRI's. To save
    ** the overhead of storing an update packet multiple times in the
    ** trie a single copy of the update packet is kept. The Payload is
    ** a reference to this single copy. Each update packet is chained
    ** together on a linked list. New update packets are added to the
    ** end of the list. In theory this ordered chained list structure
    ** should make is very simple to print out the current update
    ** packets that constitute the routing table. The ordering is
    ** important if the same NLRI is contained in two packets then the
    ** later one should be used. One possible problem is that update
    ** packets with withdraws only will not be stored.
    */

    struct Tree {
	Tree *ptrs[2];
	TriePayload p;

	Tree() {
	    ptrs[0] = ptrs[1] = 0;
	}
    };
    
    TrieData *_first;
    TrieData *_last;

    bool _debug;
    bool _pretty;

    Tree _head;

    uint32_t _update_cnt;	// Number of update packets seen

    bool empty() const;
    void tree_walk_table(const TreeWalker&, const Tree *ptr, const char *st)
	const;
    void print(FILE *fp) const;
    void prt(FILE *fp, const Tree *ptr, const char *st) const;
    string bit_string_to_subnet(const char *st) const;
    bool insert(const IPv4Net& net, TriePayload& p);
    bool insert(uint32_t address, int mask_length, TriePayload& p);
    void del(Tree *ptr);
    bool del(const IPv4Net& net);
    bool del(uint32_t address, int mask_length);
    bool del(Tree *ptr, uint32_t address, int mask_length);
    TriePayload find(const IPv4Net& net) const;
    TriePayload find(uint32_t address, const int mask_length) const;
    TriePayload find(uint32_t address) const;
};

#endif // __BGP_HARNESS_TRIE_HH__
