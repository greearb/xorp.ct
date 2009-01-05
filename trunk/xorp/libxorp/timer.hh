// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/libxorp/timer.hh,v 1.40 2008/10/13 00:45:58 pavlin Exp $

#ifndef __LIBXORP_TIMER_HH__
#define __LIBXORP_TIMER_HH__

#include <assert.h>
#include <memory>
#include <list>
#include <map>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "timeval.hh"
#include "heap.hh"
#include "callback.hh"
#include "task.hh"

class XorpTimer;
class TimerNode;
class TimerList;
class ClockBase;

typedef XorpCallback0<void>::RefPtr OneoffTimerCallback;

// PeriodicTimerCallback methods should return true to reschedule
typedef XorpCallback0<bool>::RefPtr PeriodicTimerCallback;

typedef XorpCallback1<void, XorpTimer&>::RefPtr BasicTimerCallback;

/**
 * @short Abstract class used to receive TimerList notifications
 *
 * TimerListObserverBase is a class that can be subtyped to receive
 * notifications on when timers are created or expired.  All the methods in
 * this class are private, since they must only be invoked by the friend class,
 * TimerList
 *
 * @see TimerList
 */
class TimerListObserverBase {
public:
    virtual ~TimerListObserverBase();

private:
    /**
     * This function will get called when a timer is scheduled.  Periodic
     * timers will produce periodic notifications.
     */
    virtual void notify_scheduled(const TimeVal&) = 0;

   /**
     * This function will get called when a timer is unscheduled.
     */
    virtual void notify_unscheduled(const TimeVal&) = 0;

    TimerList * _observed;

    friend class TimerList;
};

/**
 * @short XorpTimer class
 *
 * Timers allow callbacks to be made at a specific time in the future.
 * They are ordinarily created via TimerList methods, and they
 * must be associated with an TimerList object in order to be
 * runnable.
 *
 * @see TimerList
 */
class XorpTimer {
public:

    /**
     * @return true if the timer has been scheduled, and the callback
     * associated with this timer has not been called yet.
     */
    bool scheduled() const;

    /**
     * @return the expiry time of the @ref XorpTimer
     */
    const TimeVal& expiry() const;

    /**
     * Get the remaining time until the timer expires.
     *
     * @param remain the return-by-reference value with the remaining
     * time until the timer expires. If the current time is beyond
     * the expire time (e.g., if we are behind schedule with the timer
     * processing), the return time is zero.
     * @return true if the remaining time has meaningful value (e.g.,
     * if timer was scheduled), otherwise false.
     */
    bool time_remaining(TimeVal& remain) const;

    /**
     * Expire the @ref XorpTimer object when the TimerList is next run.
     */
    void schedule_now(int priority = XorpTask::PRIORITY_DEFAULT);

    /**
     * Schedule the @ref XorpTimer object at a given time.
     */
    void schedule_at(const TimeVal& when,
		     int priority = XorpTask::PRIORITY_DEFAULT);

    /**
     * Schedule the @ref XorpTimer object to expire in @ref wait
     * after the current time.
     */
    void schedule_after(const TimeVal& wait,
			int priority = XorpTask::PRIORITY_DEFAULT);

    /**
     * Schedule the @ref XorpTimer object.
     *
     * @param ms milliseconds from the current time.
     */
    void schedule_after_ms(int ms,
			   int priority = XorpTask::PRIORITY_DEFAULT);

    /**
     * Reschedule the @ref XorpTimer object.
     *
     * @param wait time from the most recent expiry.
     */
    void reschedule_after(const TimeVal& wait);

    /**
     * Reschedule the @ref XorpTimer object.
     *
     * @param ms milliseconds from the most recent expiry.
     */
    void reschedule_after_ms(int ms);

    /**
     * Unschedule the @ref XorpTimer object.  The XorpTimer callback is not
     * invoked.
     */
    void unschedule();			// unschedule if scheduled

    /**
     * Release reference to underlying state.
     */
    void clear();			// erase timer

    XorpTimer()				: _node(0) { }
    XorpTimer(TimerList* list, BasicTimerCallback cb);
    XorpTimer(const XorpTimer&);
    ~XorpTimer();

    XorpTimer& operator=(const XorpTimer&);
    TimerNode* node() const		{ return _node; }

private:
    TimerNode* _node;

    XorpTimer(TimerNode* n);

    friend class TimerList;
};


/**
 * @short XorpTimer creation and scheduling entity
 *
 * A TimerList is a scheduling entity that provides a means to
 * create @ref XorpTimer objects and run them.
 *
 * XorpTimer objects created via TimerList methods contain pointers to
 * reference counted elements maintained in the TimerList.  The
 * elements on the list need to be referenced by XorpTimer objects or
 * the underlying timer callbacks are never made.  For instance:
 *
<pre>
TimerList timer_list;

XorpTimer t = timer_list.new_oneoff_after(TimeVal(0, 100000),
			callback(some_function, some_arg));

new_oneoff_after(TimeVal(0, 200000), my_callback_b, my_parameter_a);

while ( ! timer_list.empty() ) {
	timer_list.run();
}
</pre>
 *
 * <code>my_callback_a</code> is called 100000us after the @ref XorpTimer
 * object is created.

 * <code>my_callback_b</code> is never called
 * because no XorpTimer references the underlying element on the TimerList
 * after <code>TimerList::new_oneoff_after()</code> is called.
 */
class TimerList {
public:
    /**
     * @param clock clock object to use to query time.
     */
    TimerList(ClockBase* clock);

    ~TimerList();

    /**
     * Expire all pending @ref XorpTimer objects associated with @ref
     * TimerList.
     */
    void run();

    /**
     * Create a XorpTimer that will be scheduled once.
     *
     * @param when the absolute time when the timer expires.
     * @param ocb callback object that is invoked when timer expires.
     *
     * @return the @ref XorpTimer created.
     */
    XorpTimer new_oneoff_at(const TimeVal& when,
			    const OneoffTimerCallback& ocb,
			    int priority = XorpTask::PRIORITY_DEFAULT);

    /**
     * Create a XorpTimer that will be scheduled once.
     *
     * @param wait the relative time when the timer expires.
     * @param ocb callback object that is invoked when timer expires.
     *
     * @return the @ref XorpTimer created.
     */
    XorpTimer new_oneoff_after(const TimeVal& wait,
			       const OneoffTimerCallback& ocb,
			       int priority = XorpTask::PRIORITY_DEFAULT);

    /**
     * Create a XorpTimer that will invoke a callback periodically.
     *
     * @param wait the period when the timer expires.
     * @param pcb user callback object that is invoked when timer expires.
     * If the callback returns false the periodic XorpTimer is unscheduled.
     *
     * @return the @ref XorpTimer created.
     */
    XorpTimer new_periodic(const TimeVal& wait,
			   const PeriodicTimerCallback& pcb,
			   int priority = XorpTask::PRIORITY_DEFAULT);

    /**
     * Create a XorpTimer to set a flag.
     *
     * @param when the absolute time when the timer expires.
     *
     * @param flag_ptr pointer to a boolean variable that is set to
     * @ref to_value when the @ref XorpTimer expires.
     *
     * @return the @ref XorpTimer created.
     */
    XorpTimer set_flag_at(const TimeVal&	when,
			  bool*			flag_ptr,
			  bool			to_value = true,
			  int priority = XorpTask::PRIORITY_DEFAULT);

    /**
     * Create a XorpTimer to set a flag.
     *
     * @param wait the relative time when the timer expires.
     *
     * @param flag_ptr pointer to a boolean variable that is set to
     * @ref to_value when the @ref XorpTimer expires.
     *
     * @return the @ref XorpTimer created.
     */
    XorpTimer set_flag_after(const TimeVal&	wait,
			     bool*		flag_ptr,
			     bool		to_value = true,
			     int priority = XorpTask::PRIORITY_DEFAULT);

    /**
     * Custom XorpTimer creation method.  The @ref XorpTimer object created
     * needs to be explicitly scheduled with the available @ref XorpTimer
     * methods.
     *
     * @param hook user function to be invoked when XorpTimer expires.
     *
     * @param thunk user argument to be passed when user's function is
     * invoked.
     *
     * @return the @ref XorpTimer created.
     */
    XorpTimer new_timer(const BasicTimerCallback& cb) {
	return XorpTimer(this, cb);
    }

    /**
     * @return true if there no @ref XorpTimer objects currently scheduled on
     * list.
     */
    bool empty() const;

    /**
     * @return the number of scheduled objects.
     */
    size_t size() const;

    /**
     * Query the next XorpTimer Expiry time.
     *
     * @param tv reference that is assigned expiry time of next timer.
     * If there is no @ref XorpTimer pending, this value is assigned the
     * maximum @ref TimeVal::MAXIMUM().  The first function returns the
     * absolute time at which the timer expires, where the second returns the
     * difference between now and the expiry time.
     *
     * @return true if there is a XorpTimer awaiting expiry, false otherwise.
     */
    bool get_next_delay(TimeVal& tv) const;

    /**
     * Get the priority of the highest priority timer that has expired.
     *
     * @return the priority of the expired timer, or INFINITE_PRIORITY
     * if no timer has expired.
     */
    int get_expired_priority() const;

    /**
     * Read the latest known value from the clock used by @ref
     * TimerList object.
     *
     * @param now the return-by-reference value with the current time.
     */
    void current_time(TimeVal& now) const;

    /**
     * Advance time.  This method fetches the time from clock object
     * associated with the TimerList and sets the TimerList current
     * time to this value.
     */
    void advance_time();

    /**
     * Default time querier.
     *
     * Get the current time.  This method is analogous to calling
     * the underlying operating system's 'get current system time'
     * function and is implemented as a call to advance_time()
     * followed by a call to current_time().
     *
     * @param tv a pointer to the @ref TimeVal storage to store the current
     * time.
     */
    static void system_gettimeofday(TimeVal* tv);

    /**
     * Suspend process execution for a defined interval.
     *
     * This methid is analogous to calling sleep(3) or usleep(3),
     * and is implemented as a call to sleep(3) and/or usleep(3)
     * followed by a call to advance_time().
     *
     * @param tv the period of time to suspend execution.
     */
    static void system_sleep(const TimeVal& tv);

    /**
     * Register an observer object with this class
     *
     * @param obs an observer object derived from @ref TimerListObserverBase
     */
    void set_observer(TimerListObserverBase& obs);

    /**
     * Unregister the current observer
     */
    void remove_observer();

    /**
     * Get pointer to sole TimerList instance.
     *
     * @return pointer if TimerList has been constructed, NULL otherwise.
     */
    static TimerList* instance();

private:
    void schedule_node(TimerNode* t);		// insert in time ordered pos.
    void unschedule_node(TimerNode* t);		// remove from list

    void acquire_lock() const		{ /* nothing, for now */ }
    bool attempt_lock() const		{ return true; }
    void release_lock() const		{ /* nothing, for now */ }


    // find or create the heap assoicated with this priority level
    Heap* find_heap(int priority);    

    // expire the highest priority timer
    bool expire_one(int worst_priority);

private:
    TimerList(const TimerList&);		// not implemented
    TimerList& operator=(const TimerList&);	// not implemented

private:
    // we need one heap for each priority level
    map<int, Heap*>		_heaplist;

    ClockBase* 			_clock;
    TimerListObserverBase* 	_observer;
#ifdef HOST_OS_WINDOWS
    HANDLE			_hirestimer;
#endif

    friend class TimerNode;
    friend class TimerListObserverBase;
};


class TimerNode : public HeapBase {
protected:
    TimerNode(TimerList*, BasicTimerCallback);
    virtual ~TimerNode();
    void add_ref();
    void release_ref();

    // we want this even if it is never called, to override the
    // default supplied by the compiler.
    TimerNode(const TimerNode&);	// never called
    TimerNode& operator=(const TimerNode&);

    bool scheduled()		const	{ return _pos_in_heap >= 0; }
    int priority()		const	{ return _priority; }
    const TimeVal& expiry()	const	{ return _expires; }
    bool time_remaining(TimeVal& remain) const;

    void schedule_at(const TimeVal&, int priority);
    void schedule_after(const TimeVal& wait, int priority);
    void reschedule_after(const TimeVal& wait);
    void unschedule();
    virtual void expire(XorpTimer&, void*);

    int		_ref_cnt;	// Number of referring XorpTimer objects

    TimeVal	_expires;	// Expiration time
    BasicTimerCallback _cb;

    int		_priority;	// Scheduling priority

    TimerList*	_list;		// TimerList this node is associated w.

    friend class XorpTimer;
    friend class TimerList;
};


// ----------------------------------------------------------------------------
// inline Timer methods

inline XorpTimer::XorpTimer(TimerNode* n)
    : _node(n)
{
    if (_node)
	_node->add_ref();
}

inline XorpTimer::XorpTimer(TimerList* tlist, BasicTimerCallback cb)
    : _node(new TimerNode(tlist, cb))
{
    if (_node)
	_node->add_ref();
}

inline XorpTimer::XorpTimer(const XorpTimer& t)
    : _node(t._node)
{
    if (_node)
	_node->add_ref();
}

inline XorpTimer::~XorpTimer()
{
    if (_node)
	_node->release_ref();
}

inline XorpTimer&
XorpTimer::operator=(const XorpTimer& t)
{
    if (t._node)
	t._node->add_ref();
    if (_node)
	_node->release_ref();
    _node = t._node;
    return *this;
}

inline bool
XorpTimer::scheduled() const
{
    return _node && _node->scheduled();
}

inline const TimeVal&
XorpTimer::expiry() const
{
    assert(_node);
    return _node->expiry();
}

inline bool
XorpTimer::time_remaining(TimeVal& remain) const
{
    if (_node == NULL) {
	remain = TimeVal::ZERO();
	return (false);
    }
    return (_node->time_remaining(remain));
}

inline void
XorpTimer::schedule_at(const TimeVal& t, int priority)
{
    assert(_node);
    _node->schedule_at(t, priority);
}

inline void
XorpTimer::schedule_after(const TimeVal& wait, int priority)
{
    assert(_node);
    _node->schedule_after(wait, priority);
}

inline void
XorpTimer::schedule_after_ms(int ms, int priority)
{
    assert(_node);
    TimeVal wait(ms / 1000, (ms % 1000) * 1000);
    _node->schedule_after(wait, priority);
}

inline void
XorpTimer::schedule_now(int priority)
{
    schedule_after(TimeVal::ZERO(), priority);
}

inline void
XorpTimer::reschedule_after(const TimeVal& wait)
{
    assert(_node);
    _node->reschedule_after(wait);
}

inline void
XorpTimer::reschedule_after_ms(int ms)
{
    assert(_node);
    TimeVal wait(ms / 1000, (ms % 1000) * 1000);
    _node->reschedule_after(wait);
}

inline void
XorpTimer::unschedule()
{
    if (_node)
	_node->unschedule();
}

inline void
XorpTimer::clear()
{
    if (_node)
	_node->release_ref();
    _node = 0;
}

#endif // __LIBXORP_TIMER_HH__
