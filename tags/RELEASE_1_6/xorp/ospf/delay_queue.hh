// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/ospf/delay_queue.hh,v 1.10 2008/10/02 21:57:47 bms Exp $

#ifndef __OSPF_DELAY_QUEUE_HH__
#define __OSPF_DELAY_QUEUE_HH__

/**
 * Entries can be added to the queue at any rate. The callback is
 * invoked at the specified period to remove an entry from the queue.
 */
template <typename _Entry>
class DelayQueue {
public:
    typedef typename XorpCallback1<void, _Entry>::RefPtr DelayCallback;

    DelayQueue(EventLoop& eventloop, uint32_t delay, DelayCallback forward)
	: _eventloop(eventloop), _delay(delay), _forward(forward)
    {}

    /**
     * Add an entry to the queue. If the entry is already on the queue
     * it is not added again.
     */
    void add(_Entry entry);

    /**
     * Start the timer running but don't add anything to the queue.
     */
    void fire();

private:
    EventLoop& _eventloop;
    deque<_Entry> _queue;
    const uint32_t _delay;	// Delay in seconds.
    DelayCallback _forward;	// Invoked to forward an entry from the queue.
    XorpTimer	_timer;		// Timer that services the queue.

    /**
     * Invoked from the timer to take the next entry from the queue.
     */
    void next();
};

template <typename _Entry>
void
DelayQueue<_Entry>::add(_Entry entry)
{
    // If this entry is already on the queue just return.
    if (_queue.end() != find(_queue.begin(), _queue.end(), entry))
	return;

    // If the timer is running push this entry to the back of the
    // queue and return.
    if (_timer.scheduled()) {
	_queue.push_back(entry);
	return;
    }

    // If the timer isn't running then we have been idle for more than
    // delay seconds. Forward this entry immediately and start the
    // timer. Start the timer first in case this code is re-entered.

    _timer = _eventloop.new_oneoff_after(TimeVal(_delay, 0),
					 callback(this, &DelayQueue::next));

    _forward->dispatch(entry);
}

template <typename _Entry>
void
DelayQueue<_Entry>::fire()
{
    if (_timer.scheduled())
	return;
    
    _timer = _eventloop.new_oneoff_after(TimeVal(_delay, 0),
					 callback(this, &DelayQueue::next));
}

template <typename _Entry>
void
DelayQueue<_Entry>::next()
{
    if (_queue.empty())
	return;

    _timer = _eventloop.new_oneoff_after(TimeVal(_delay, 0),
					 callback(this, &DelayQueue::next));
    
    _Entry entry = _queue.front();
    _queue.pop_front();

    _forward->dispatch(entry);
}

#endif // __OSPF_DELAY_QUEUE_HH__
