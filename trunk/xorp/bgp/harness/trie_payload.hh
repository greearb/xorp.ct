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

// $XORP: xorp/bgp/harness/trie_payload.hh,v 1.1 2003/09/10 03:19:26 atanu Exp $

#ifndef __BGP_HARNESS_TRIE_PAYLOAD_HH__
#define __BGP_HARNESS_TRIE_PAYLOAD_HH__

/**
 * A BGP update packet can have many NLRIs. Each NLRI is stored in a
 * trie node. Rather than keep multiple copies of a BGP update
 * packet. A single reference counted copy is kept in TrieData. A
 * TriePayload is stored in the trie and holds a pointer to the TrieData.
 */
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

    void incr_refcnt() {
	_refcnt++;
    }

    bool decr_refcnt() {
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

/**
 * The paylaod of a RealTrie.
 */
class TriePayload {
public:
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

    void zap() {
// 	if(_data)
// 	    debug_msg("refcnt: %d %#x\n", _data->_refcnt - 1, _data);
	if(_data && _data->decr_refcnt()) {
	    delete _data;
	}
	_data = 0;
    }

private:
    TrieData *_data;
};

#endif // __BGP_HARNESS_TRIE_PAYLOAD_HH__
