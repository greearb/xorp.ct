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

// $XORP: xorp/rip/update_queue.hh,v 1.8 2003/10/07 17:24:28 hodson Exp $

#ifndef __RIP_UPDATE_QUEUE__
#define __RIP_UPDATE_QUEUE__

#include <vector>
#include "route_db.hh"

template <typename A>
class UpdateQueueImpl;


/**
 * @short Reader for @ref UpdateQueue class.
 *
 * Hooks and unhooks read iterators in update queue.  The opaque
 * UpdateQueueReaderPool actually tracks the position of each iterator,
 * this class just maintains a token that the reader pool uses.
 */
template <typename A>
class UpdateQueueReader {
public:
    UpdateQueueReader(UpdateQueueImpl<A>* i);
    ~UpdateQueueReader();

    uint32_t id() const;
    inline bool parent_is(const UpdateQueueImpl<A>* o) const;

private:
    UpdateQueueImpl<A>* _impl;
    uint32_t		_id;
};


/**
 * @short Update Queue for RIP Route entries.
 *
 * The Update Queue has is conceptually a single writer multi-reader
 * queue.  It is used to store state for triggered updates and may be
 * used unsolicited responses (routing table announcements).
 */
template <typename A>
class UpdateQueue {
protected:
    typedef UpdateQueueReader<A>	Reader;

public:
    typedef ref_ptr<Reader>		ReadIterator;
    typedef RouteEntryRef<A>		RouteUpdate;

public:
    UpdateQueue();
    ~UpdateQueue();

    /**
     * Add update to back of queue.
     */
    void push_back(const RouteUpdate& ru);

    /**
     * Remove all queued entries and reset all read iterators to the front
     * of the queue.
     */
    void flush();

    /**
     * Create a read iterator.  These are reference counted entities that
     * need to be stored in order to operate.  The newly created reader is
     * set to the end of the update queue.
     */
    ReadIterator create_reader();

    /**
     * Destroy read iterator.  This method detaches the iterator from the
     * update queue.  Use of the iterator after this call is unsafe.
     */
    void destroy_reader(ReadIterator& r);

    /**
     * Check ReadIterator's validity.
     * @param r reader to be checked.
     * @return true if r is an active read iterator, false if iterator does
     * not belong to this instance or has been destroyed.
     */
    bool reader_valid(const ReadIterator& r);

    /**
     * Increment iterator and return pointer to entry if available.
     *
     * @return A pointer to a RouteEntry if available, 0 otherwise.
     */
    const RouteEntry<A>* next(ReadIterator& r);

    /**
     * Get the RouteEntry associated with the read iterator.
     *
     * @return A pointer to a RouteEntry if available, 0 otherwise.
     */
    const RouteEntry<A>* get(ReadIterator& r) const;

    /**
     * Advance read iterator to end of update queue.  Calls to
     * @ref next and @ref get will return 0 until further
     * updates occur.
     */
    void ffwd(ReadIterator& r);

    /**
     * Move read iterator to first entry of update queue.
     */
    void rwd(ReadIterator& r);

    /**
     * Return number of updates held.  Note: this may be more than are
     * available for reading since there is internal buffering and
     * UpdateQueue iterators attach at the end of the UpdateQueue.
     *
     * @return number of updates queued.
     */
    uint32_t updates_queued() const;

protected:
    UpdateQueueImpl<A>*	_impl;
};

#endif // __RIP_UPDATE_QUEUE__
