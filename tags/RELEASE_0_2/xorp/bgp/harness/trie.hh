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

// $XORP: xorp/bgp/harness/trie.hh,v 1.2 2003/01/22 02:46:35 rizzo Exp $

#ifndef __BGP_HARNESS_TRIE_HH__
#define __BGP_HARNESS_TRIE_HH__

#include <stdio.h>
#include "bgp/packet.hh"

class Trie {
public:
    Trie() : _first(0), _last(0), _debug(true), _pretty(true) {
	_head.p._data = 0;
    }

    ~Trie();

    const UpdatePacket *lookup(const string& net) const;
    void process_update_packet(const timeval& tv, const uint8_t *buf,
			       size_t len);

    typedef XorpCallback3<void, const UpdatePacket*,
			  const IPv4Net&,
			  const timeval&>::RefPtr TreeWalker;

    void tree_walk_table(const TreeWalker& ) const;

    void save_routing_table(FILE *fp) const;
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
    struct Payload {
	Payload() : _data(0) {}

	Payload(const timeval& tv, const uint8_t *buf, size_t len,
		Trie *trie) {
	    _data = new Data(tv, buf, len, trie);
	}

	~Payload() {
	    zap();
	}

	Payload(const Payload& rhs) {
	    _data = 0;
	    copy(rhs);
	}

	Payload operator=(const Payload& rhs) {
	    if(&rhs == this)
		return *this;
	    zap();
	    copy(rhs);
	    
	    return *this;
	}

	void copy(const Payload& rhs) {
	    if(rhs._data) {
// 		debug_msg("refcnt: %d %#x\n", rhs._data->_refcnt + 1,
// 			  rhs._data);
		rhs._data->_refcnt++;
		_data = rhs._data;
	    }
	}

	const UpdatePacket *get() const {
	    if(0 == _data)
		return 0;
	    return _data->_packet;
	}

	const UpdatePacket *get(timeval& tv) const {
	    if(0 == _data)
		return 0;
	    tv = _data->_tv;
	    return _data->_packet;
	}

	void
	zap() {
// 	    if(_data)
// 		debug_msg("refcnt: %d %#x\n", _data->_refcnt - 1, _data);
	    if(_data && --_data->_refcnt == 0) {
		delete _data;
	    }
	    _data = 0;
	}

	struct Data {
	    Data(const timeval& tv, const uint8_t *buf, size_t len,
		 Trie *trie) : _tv(tv), _trie(trie) {
		_packet = new UpdatePacket(buf, len);
		_refcnt = 1;

		_next = 0;
		_prev = _trie->_last;		

		if(0 == _trie->_first)
		    _trie->_first = this;
		if(0 != _trie->_last) 
		    _trie->_last->_next = this;
		_trie->_last = this;
	    }
	    ~Data() {
		XLOG_ASSERT(0 == _refcnt);

		if(this == _trie->_first)
		    _trie->_first = _next;
		else
		    _prev->_next = _next;
		if(this == _trie->_last)
		    _trie->_last = _prev;
		if(0 != _next) {
		    _next->_prev = _prev;
		}
// 		debug_msg("Deleting: %s\n", _packet->str().c_str());
		delete _packet;
	    }

	    timeval _tv;
	    Trie *_trie;

	    int _refcnt;
	    UpdatePacket *_packet;
	    Data *_next;
	    Data *_prev;

	};
	Data *_data;
    };

    struct Tree {
	Tree *ptrs[2];
	Payload p;

	Tree() {
	    ptrs[0] = ptrs[1] = 0;
	}
    };
    
    friend class Payload::Data;

    Payload::Data *_first;
    Payload::Data *_last;

    bool _debug;
    bool _pretty;

    Tree _head;

    bool empty() const;
    void tree_walk_table(const TreeWalker&, const Tree *ptr, const char *st)
	const;
    void print(FILE *fp) const;
    void prt(FILE *fp, const Tree *ptr, const char *st) const;
    string bit_string_to_subnet(const char *st) const;
    bool insert(const IPv4Net& net, Payload& p);
    bool insert(uint32_t address, int mask_length, Payload& p);
    void del(Tree *ptr);
    bool del(const IPv4Net& net);
    bool del(uint32_t address, int mask_length);
    bool del(Tree *ptr, uint32_t address, int mask_length);
    Trie::Payload find(const IPv4Net& net) const;
    Trie::Payload find(uint32_t address, const int mask_length) const;
    Trie::Payload find(uint32_t address) const;
};

#endif // __BGP_HARNESS_TRIE_HH__
