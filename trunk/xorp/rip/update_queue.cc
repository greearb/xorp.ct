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

#ident "$XORP: xorp/rip/update_queue.cc,v 1.3 2003/07/08 15:45:27 hodson Exp $"

#include <vector>

#include "rip_module.h"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/xlog.h"

#include "update_queue.hh"

/**
 * Class used to manage read iterators into update queue buffer.
 */
class UpdateQueueReaderPool {
public:
    UpdateQueueReaderPool()
	: _icnt (0)
    {
    }

    ~UpdateQueueReaderPool()
    {
	for (uint32_t i = 0; i < _pos.size(); i++)
	    assert(_pos[i] == npos);
    }

    inline uint32_t make_reader()
    {
	// Claim an empty slot if available
	for (uint32_t i = 0; i < _pos.size(); i++) {
	    if (_pos[i] == npos) {
		_pos[i] = 0;
		_icnt++;
		return i;
	    }
	}
	// Create and use new slot.
	_pos.push_back(0);
	_icnt++;
	return (uint32_t)(_pos.size() - 1);
    }

    inline void remove_reader(uint32_t reader)
    {
	XLOG_ASSERT(reader < _pos.size() && _pos[reader] != npos);
	_pos[reader] = npos;
	_icnt--;

	// Clean up tail entries
	reader = _pos.size();
	while (reader > 1 && _pos[reader - 1] == npos) {
	    reader--;
	}
	_pos.resize(reader);
    }

    inline void incr_reader(uint32_t reader)
    {
	XLOG_ASSERT(reader < _pos.size() && _pos[reader] != npos);
	_pos[reader]++;
    }

    inline uint32_t reader_position(uint32_t reader) const
    {
	XLOG_ASSERT(reader < _pos.size() && _pos[reader] != npos);
	return _pos[reader];
    }

    void reset_all_positions()
    {
	for (uint32_t i = 0; i < _pos.size(); i++) {
	    if (_pos[i] == npos)
		continue;
	    _pos[i] = 0;
	}
    }

    void shift_all_positions(uint32_t reduction)
    {
	for (uint32_t i = 0; i < _pos.size(); i++) {
	    if (_pos[i] == npos)
		continue;
	    XLOG_ASSERT(_pos[i] >= reduction);
	    _pos[i] -= reduction;
	}
    }

    uint32_t minimum_position() const
    {
	uint32_t m = npos;
	for (uint32_t i = 0; i < _pos.size(); i++) {
	    if (_pos[i] < m) {
		m = _pos[i];
	    }
	}
	return m;
    }

    uint32_t reader_count() const
    {
	return _icnt;
    }

protected:
    static const uint32_t npos = ~0;	// Invalid iterator position rep.

    vector<uint32_t> _pos;		// Reader positions in updatequeue
    uint32_t	     _icnt;		// Number of valid iterators
};


/* ------------------------------------------------------------------------- */
/* UpdateQueueReader methods */

template <typename A>
UpdateQueueReader<A>::UpdateQueueReader(UpdateQueueReaderPool* p)
    : _pool(p)
{
    _reader_no = _pool->make_reader();
}

template <typename A>
UpdateQueueReader<A>::~UpdateQueueReader()
{
    _pool->remove_reader(_reader_no);
}

template <typename A>
inline void
UpdateQueueReader<A>::incr()
{
    _pool->incr_reader(_reader_no);
}

template <typename A>
inline uint32_t
UpdateQueueReader<A>::position() const
{
    return (uint32_t)_pool->reader_position(_reader_no);
}


/* ------------------------------------------------------------------------- */
/* UpdateQueue methods */

template <typename A>
UpdateQueue<A>::UpdateQueue()
{
    _pool = new ReaderPool;
}

template <typename A>
UpdateQueue<A>::~UpdateQueue()
{
    delete _pool;
    _pool = 0;
}

template <typename A>
typename UpdateQueue<A>::ReadIterator
UpdateQueue<A>::create_reader()
{
    Reader* r = new Reader(_pool);
    return ReadIterator(r);
}

template <typename A>
void
UpdateQueue<A>::destroy_reader(ReadIterator& r)
{
    r = 0;
}

template <typename A>
const RouteEntry<A>*
UpdateQueue<A>::get(ReadIterator& r) const
{
    uint32_t pos = r->position();
    if (pos < _queue.size()) {
	return _queue[pos].get();
    }
    return 0;
}

template <typename A>
const RouteEntry<A>*
UpdateQueue<A>::next(ReadIterator& r) const
{
    if (r->position() < _queue.size()) {
	r->incr();
	return get(r);
    }
    return 0;
}

template <typename A>
void
UpdateQueue<A>::flush()
{
    _pool->reset_all_positions();
    _queue.resize(0);
}

template <typename A>
void
UpdateQueue<A>::push_back(const RouteUpdate& u)
{
    // Only enqueue if readers are present, no interested parties otherwise
    if (_pool->reader_count())
	_queue.push_back(u);
}

template class UpdateQueue<IPv4>;
template class UpdateQueue<IPv6>;
