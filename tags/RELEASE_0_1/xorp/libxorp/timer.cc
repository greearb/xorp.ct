// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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
//
// Portions of this code originally derived from:
// timer.{cc,hh} -- portable timers. Linux kernel module uses Linux timers
// Eddie Kohler
//
// Copyright (c) 1999-2000 Massachusetts Institute of Technology
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, subject to the conditions
// listed in the Click LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the Click LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/libxorp/timer.cc,v 1.29 2002/12/09 18:29:15 hodson Exp $"

#include "timer.hh"

// Implementation Notes:
//
// Event scheduling happens through the TimerList.  The TimerList is
// comprised of TimerNode's that contain an expiry time, a callback to
// make when the timer expires, and a thunk value to pass with the
// callback.  TimerNode's that are not scheduled are not part of the
// TimerList, but remain "associated" with their originating list so
// can be rescheduled by calling TimerNode methods (eg
// reschedule_after_ms()).
//
// User applications deal with XorpTimer objects that are pointers to
// TimerNodes.  There is some magic with XorpTimer objects: they enforce
// reference counting of TimerNodes.  When there are no XorpTimers
// referring to a particular TimerNode it is de-scheduled and
// destroyed.  This makes it difficult, though not impossible, for
// timer event callbacks to be invoked that pass invalid stack or heap
// memory into the callback.  Under normal usage we expect XorpTimer
// objects and the associated thunk values to have similar scope so
// they both exist and disappear at the same time.

//-----------------------------------------------------------------------------
// Constants

// ----------------------------------------------------------------------------
// TimerNode methods

TimerNode::TimerNode(TimerList* l, BasicTimerCallback cb) 
    : _ref_cnt(0), _cb(cb), _list(l), _pos_in_heap(NOT_IN_HEAP)
{
    assert(_list);
}

TimerNode::~TimerNode()
{
    unschedule();
}

void
TimerNode::add_ref() 
{
    _ref_cnt++; 
}

void
TimerNode::release_ref()
{
    if (--_ref_cnt <= 0)
	delete this;
}

void
TimerNode::expire(XorpTimer&, void*) 
{
    // Implemented by children
}

void
TimerNode::unschedule()
{
    if (scheduled())
	_list->unschedule_node(this);
}

void
TimerNode::schedule_at(const timeval& t)
{
    assert(_list);
    unschedule();
    _expires = t;
    _list->schedule_node(this);
}

void
TimerNode::schedule_after_ms(int ms)
{
    assert(_list);
    unschedule();

    timeval interval;
    interval.tv_sec  = ms / 1000;
    interval.tv_usec = (ms % 1000) * 1000;

    timeval now;
    _list->current_time(now);
    _expires = now + interval ;
    _list->schedule_node(this);
}

void
TimerNode::reschedule_after_ms(int ms)
{
    assert(_list);
    unschedule();

    timeval interval;
    interval.tv_sec  = ms / 1000;
    interval.tv_usec = (ms % 1000) * 1000;

    _expires = _expires + interval;
    _list->schedule_node(this);
}

// ----------------------------------------------------------------------------
// Specialized Timers.  These are what the XorpTimer objects returned by
// the TimerList XorpTimer creation methods (e.g. TimerList::new_oneoff_at(), etc)
// actually refer to/

class OneoffTimerNode2 : public TimerNode {
public:
    OneoffTimerNode2(TimerList *l, const OneoffTimerCallback& cb) 
	: TimerNode (l, callback(this, &OneoffTimerNode2::expire, (void*)0)), 
	_cb(cb) {}
private:
    OneoffTimerCallback _cb;

    void expire(XorpTimer&, void*) {
	_cb->dispatch();
    }
};

class PeriodicTimerNode2 : public TimerNode {
public:
    PeriodicTimerNode2(TimerList *l, const PeriodicTimerCallback& cb, int ms)
	: TimerNode(l, callback(this, &PeriodicTimerNode2::expire, (void*)0)),
		    _cb(cb), _period_ms(ms) { }

private:
    PeriodicTimerCallback _cb;
    int _period_ms;

    void expire(XorpTimer& t, void*) {
	if (_cb->dispatch()) 
	    t.reschedule_after_ms(_period_ms);
    }
};

// ----------------------------------------------------------------------------
// XorpTimer factories

XorpTimer
TimerList::new_oneoff_at(const timeval &tv, const OneoffTimerCallback& cb)
{
    TimerNode* n = new OneoffTimerNode2(this, cb);
    n->schedule_at(tv);
    return XorpTimer(n);
}

XorpTimer
TimerList::new_oneoff_after_ms(int ms, const OneoffTimerCallback& cb)
{
    TimerNode* n = new OneoffTimerNode2(this, cb);
    n->schedule_after_ms(ms);
    return XorpTimer(n);
}

XorpTimer
TimerList::new_periodic(int interval, const PeriodicTimerCallback& cb)
{
    TimerNode* n = new PeriodicTimerNode2(this, cb, interval);
    n->schedule_after_ms(interval);
    return XorpTimer(n);
}

static void
set_flag_hook(bool* flag_ptr)
{
    assert(flag_ptr);
    *flag_ptr = true;
}

XorpTimer
TimerList::set_flag_at(const timeval &tv, bool *flag_ptr)
{
    assert(flag_ptr);
    *flag_ptr = false;
    return new_oneoff_at(tv, callback(set_flag_hook, flag_ptr));
}

XorpTimer
TimerList::set_flag_after_ms(int ms, bool *flag_ptr)
{
    assert(flag_ptr);
    *flag_ptr = false;
    return new_oneoff_after_ms(ms, callback(set_flag_hook, flag_ptr));
}

// Default XorpTimer and TimerList clock
void
TimerList::system_gettimeofday(timeval *t)
{
    gettimeofday(t, NULL);
}

// XXX locking need some care here. Worry about it when we have locks.
void
TimerList::run()
{
    timeval now;
    _current_time_proc(&now);
    struct heap_entry *n;
    while ((n = top()) != 0 && n->key <= now) {
	TimerNode *t = (TimerNode *)n->object;
	pop();
	// _hook() requires a XorpTimer as first argument, we have
	// only a timernode, so we have to create a temporary
	// timer to invoke the hook.
	XorpTimer placeholder(t);
	t->expire(placeholder, 0);
    }
}

bool
TimerList::empty() const
{
    acquire_lock();
    bool result = (top() == 0) ;
    release_lock();
    return result;
}

size_t
TimerList::size() const
{
    acquire_lock();
    size_t result = Heap::size();
    release_lock();
    return result;
}

bool
TimerList::get_next_delay(timeval &tv) const
{
    acquire_lock();
    struct heap_entry *t = top();
    release_lock();
    if (t == 0) {
	tv.tv_sec  = 1000000;
	tv.tv_usec = 0;
	return false;
    } else {
	timeval now ;
	_current_time_proc(&now);
	if ( t->key > now) // next event is in the future
	    tv = t->key - now ;
	else // next event is already in the past, return 0.0
	    tv = (timeval){0,0};
	return true;
    }
}

void
TimerList::schedule_node(TimerNode* n)
{
    acquire_lock();
    push(n->expiry(), n);
    release_lock();
}

void
TimerList::unschedule_node(TimerNode *n)
{
    acquire_lock();
    pop_obj((void *)n);
    release_lock();
}

