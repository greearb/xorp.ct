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

#ident "$XORP: xorp/rip/update_queue.cc,v 1.8 2004/02/20 01:22:04 hodson Exp $"

#include <vector>
#include <list>

#include "rip_module.h"

#include "libxorp/debug.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/xlog.h"

#include "update_queue.hh"

/**
 * @short Store for a fixed block of update.
 *
 * Stores a group of updates in a block.  They are stored like this
 * for two reasons.  Firstly, we can track readers entering and
 * leaving blocks which makes garbage collection decisions easy.
 * Secondly, it involves minimal copying of ref_ptrs (compared to say
 * a scrolling vector of ref_ptrs).
 */
template <typename A>
struct UpdateBlock {
public:
    typedef typename UpdateQueue<A>::RouteUpdate RouteUpdate;
    static const size_t MAX_UPDATES = 100;

public:
    UpdateBlock()
	: _updates(MAX_UPDATES), _update_cnt(0), _refs(0)
    {}

    ~UpdateBlock()
    {
	XLOG_ASSERT(_refs == 0);
    }

    bool full() const
    {
	return _update_cnt == MAX_UPDATES;
    }

    bool empty() const
    {
	return _update_cnt == 0;
    }

    size_t count() const
    {
	return _update_cnt;
    }

    bool add_update(const RouteUpdate& u)
    {
	XLOG_ASSERT(u.get() != 0);

	if ( full() ) {
	    XLOG_WARNING("Attempting to add update to full block");
	    return false;
	}
	_updates[_update_cnt++] = u;
	return true;
    }

    const RouteUpdate& get(uint32_t pos) const
    {
	XLOG_ASSERT(pos < MAX_UPDATES);

	return _updates[pos];
    }

    void ref()
    {
	_refs++;
    }

    void unref()
    {
	XLOG_ASSERT( _refs > 0 );
	_refs--;
    }

    uint32_t ref_cnt() const
    {
	return _refs;
    }

private:
    vector<RouteUpdate> _updates;
    size_t		_update_cnt;
    uint32_t		_refs;
};


// ----------------------------------------------------------------------------
// UpdateQueueImpl

/**
 * @short Internal implementation of UpdateQueue class.
 */
template <typename A>
class UpdateQueueImpl
{
private:
    typedef typename UpdateQueue<A>::RouteUpdate RouteUpdate;
    typedef list<UpdateBlock<A> >		 UpdateBlockList;

    /**
     * @short State associated with an UpdateQueue reader.
     */
    struct ReaderPos {
	typename UpdateBlockList::iterator _bi;
	uint32_t		  	   _pos;

	ReaderPos(const typename UpdateBlockList::iterator& bi, uint32_t pos)
	    : _bi(bi), _pos(pos)
	{
	    _bi->ref();
	}

	~ReaderPos()
	{
	    _bi->unref();
	}

	typename UpdateBlockList::const_iterator
	block() const
	{
	    return _bi;
	}

	uint32_t
	position() const
	{
	    return _pos;
	}

	void advance_position()
	{
	    if (_pos < _bi->count())
		_pos++;
	}

	void advance_block()
	{
	    _bi->unref();
	    _bi++;
	    _bi->ref();
	    _pos = 0;
	}

	void move_to(const typename UpdateBlockList::iterator& bi,
		     uint32_t pos)
	{
	    _bi->unref();
	    _bi = bi;
	    _bi->ref();
	    _pos = pos;
	}
    };

private:
    UpdateBlockList	_update_blocks;
    vector<ReaderPos*>	_readers;
    uint32_t		_num_readers;

public:
    UpdateQueueImpl()
	: _num_readers(0)
    {
	_update_blocks.push_back(UpdateBlock<A>());
    }

    ~UpdateQueueImpl()
    {
    }

    /**
     * Create state for a new reader.
     * @return id for reader.
     */
    uint32_t add_reader()
    {
	typename UpdateBlockList::iterator lb = --_update_blocks.end();
	ReaderPos* new_reader = new ReaderPos(lb, lb->count());
	_num_readers++;

	for (uint32_t i = 0; i < _readers.size(); ++i) {
	    if (_readers[i] == 0) {
		_readers[i] = new_reader;
		return i;
	    }
	}
	_readers.push_back(new_reader);
	return _readers.size() - 1;
    }

    /**
     * Destroy state for reader.
     * @param id unique id of reader.
     */
    void remove_reader(uint32_t id)
    {
	if (id < _readers.size() && _readers[id] != 0) {
	    delete _readers[id];
	    _readers[id] = 0;
	    _num_readers--;
	    if (_num_readers == 0 && _update_blocks.back().empty() == false) {
		_update_blocks.push_back(UpdateBlock<A>());
	    }
	    garbage_collect();
	}
    }

    /**
     * Remove blocks at front of queue who have no referrers.
     */
    void garbage_collect()
    {
	typename UpdateBlockList::iterator last = --_update_blocks.end();
	while (_update_blocks.begin() != last &&
	       _update_blocks.front().ref_cnt() == 0) {
	    _update_blocks.erase(_update_blocks.begin());
	}
    }

    /**
     * Advance position of reader by 1 update.
     * @param id unique id of reader.
     * @return true if reader advanced, false if reader has reached end.
     */
    bool advance_reader(uint32_t id)
    {
	XLOG_ASSERT(id < _readers.size());
	XLOG_ASSERT(_readers[id] != 0);

	ReaderPos* rp = _readers[id];
	if (rp->position() != rp->block()->count()) {
	    debug_msg("Advancing position\n");
	    rp->advance_position();
	}

	if (rp->position() == rp->block()->count() &&
	    rp->block()->count() != 0) {
	    if (rp->block() == --_update_blocks.end()) {
		_update_blocks.push_back(UpdateBlock<A>());
	    }
	    debug_msg("Advancing block\n");
	    rp->advance_block();
	    garbage_collect();
	}
	return true;
    }

    /**
     * Fast forward reader to end of updates.
     * @param id unique id of reader.
     */
    void ffwd_reader(uint32_t id)
    {
	XLOG_ASSERT(id < _readers.size());
	XLOG_ASSERT(_readers[id] != 0);

	typename UpdateBlockList::iterator bi = --_update_blocks.end();
	_readers[id]->move_to(bi, bi->count());
	advance_reader(id);

	garbage_collect();
    }

    void rwd_reader(uint32_t id)
    {
	XLOG_ASSERT(id < _readers.size());
	XLOG_ASSERT(_readers[id] != 0);

	_readers[id]->move_to(_update_blocks.begin(), 0);
    }

    /**
     * Fast forward all readers to end of updates.
     */
    void
    flush()
    {
	_update_blocks.push_back(UpdateBlock<A>());
	for (size_t i = 0; i < _readers.size(); i++) {
	    if (_readers[i] != 0) ffwd_reader(i);
	}
    }

    /**
     * Get data associated with reader.
     */
    const RouteEntry<A>*
    read(uint32_t id)
    {
	XLOG_ASSERT(id < _readers.size());
	XLOG_ASSERT(_readers[id] != 0);

	ReaderPos* rp = _readers[id];
	debug_msg("Reading from %d %u/%u\n",
		  id, rp->position(), rp->block()->count());

	// Reader may have reached end of last block and stopped, but
	// more data may now have been added.
	if (rp->position() == rp->block()->count()) {
	    advance_reader(id);
	}

	if (rp->position() < rp->block()->count()) {
	    const RouteUpdate& u = rp->block()->get(rp->position());
	    debug_msg("Got route\n");
	    return u.get();
	}
	debug_msg("Nothing available\n");
	return 0;
    }

    bool
    push_back(const RouteUpdate& u)
    {
	if (_num_readers == 0) {
	    return true;
	}
	if (_update_blocks.back().full() == true) {
	    _update_blocks.push_back(UpdateBlock<A>());
	}
	return _update_blocks.back().add_update(u);
    }

    uint32_t updates_queued() const
    {
	UpdateBlockList::const_iterator ci = _update_blocks.begin();
	uint32_t total = 0;
	while (ci != _update_blocks.end()) {
	    total += ci->count();
	    ++ci;
	}
	return total;
    }
};


/* ------------------------------------------------------------------------- */
/* UpdateQueueReader methods */

template <typename A>
UpdateQueueReader<A>::UpdateQueueReader(UpdateQueueImpl<A>* impl)
    : _impl(impl)
{
    _id = _impl->add_reader();
}

template <typename A>
UpdateQueueReader<A>::~UpdateQueueReader()
{
    _impl->remove_reader(_id);
}

template <typename A>
inline uint32_t
UpdateQueueReader<A>::id() const
{
    return _id;
}

template <typename A>
bool
UpdateQueueReader<A>::parent_is(const UpdateQueueImpl<A>* o) const
{
    return _impl == o;
}


/* ------------------------------------------------------------------------- */
/* UpdateQueue methods */

template <typename A>
UpdateQueue<A>::UpdateQueue()
{
    _impl = new UpdateQueueImpl<A>();
}

template <typename A>
UpdateQueue<A>::~UpdateQueue()
{
    delete _impl;
}

template <typename A>
typename UpdateQueue<A>::ReadIterator
UpdateQueue<A>::create_reader()
{
    Reader* r = new UpdateQueueReader<A>(_impl);
    return ReadIterator(r);
}

template <typename A>
void
UpdateQueue<A>::destroy_reader(ReadIterator& r)
{
    r = 0;
}

template <typename A>
bool
UpdateQueue<A>::reader_valid(const ReadIterator& r)
{
    if (r.get() == 0)
	return false;
    return r->parent_is(this->_impl);
}

template <typename A>
const RouteEntry<A>*
UpdateQueue<A>::get(ReadIterator& r) const
{
    return _impl->read(r->id());
}

template <typename A>
const RouteEntry<A>*
UpdateQueue<A>::next(ReadIterator& r)
{
    if (_impl->advance_reader(r->id())) {
	return get(r);
    }
    return 0;
}

template <typename A>
void
UpdateQueue<A>::ffwd(ReadIterator& r)
{
    _impl->ffwd_reader(r->id());
}

template <typename A>
void
UpdateQueue<A>::rwd(ReadIterator& r)
{
    _impl->rwd_reader(r->id());
}

template <typename A>
void
UpdateQueue<A>::flush()
{
    _impl->flush();
}

template <typename A>
void
UpdateQueue<A>::push_back(const RouteUpdate& u)
{
    _impl->push_back(u);
}

template <typename A>
uint32_t
UpdateQueue<A>::updates_queued() const
{
    return _impl->updates_queued();
}


// ----------------------------------------------------------------------------
// Instantiations

#ifdef INSTANTIATE_IPV4
template class UpdateQueue<IPv4>;
#endif

#ifdef INSTANTIATE_IPV6
template class UpdateQueue<IPv6>;
#endif
