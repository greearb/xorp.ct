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

// $XORP: xorp/libxorp/timer.hh,v 1.15 2003/06/12 23:58:16 jcardona Exp $

#ifndef __LIBXORP_TIMER_HH__
#define __LIBXORP_TIMER_HH__

#include <sys/time.h>

#include <assert.h>
#include <memory>

#include "timeval.hh"
#include "heap.hh"
#include "callback.hh"

class XorpTimer;
class TimerNode;
class TimerList;

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
    void schedule_now();

    /**
     * Schedule the @ref XorpTimer object at a given time.
     */
    void schedule_at(const TimeVal& when);

    /**
     * Schedule the @ref XorpTimer object to expire in @ref wait
     * after the current time.
     */
    void schedule_after(const TimeVal& wait);

    /**
     * Schedule the @ref XorpTimer object.
     * @param ms milliseconds from the current time.
     */
    void schedule_after_ms(int ms);

    /**
     * Reschedule the @ref XorpTimer object.
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


// TimerList can use alternate clock that uses this prototype
typedef void (*query_current_time)(TimeVal*);

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

XorpTimer t = timer_list.new_oneoff_after_ms(100,
			callback(some_function, some_arg));

new_oneoff_after_ms(200, my_callback_b, my_parameter_a);

while ( ! timer_list.empty() ) {
	timer_list.run();
}
</pre>
 *
 * <code>my_callback_a</code> is called 100ms after the @ref XorpTimer
 * object is created.

 * <code>my_callback_b</code> is never called
 * because no XorpTimer references the underlying element on the TimerList
 * after <code>TimerList::new_oneoff_after_ms()</code> is called.
 */
class TimerList : public Heap {
public:
    /**
     * @param query_current_time specifiable current time function.
     * If no argument is supplied, gettimeofday is used.
     */
    inline TimerList(query_current_time q = system_gettimeofday);
    inline ~TimerList() { }

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
			    const OneoffTimerCallback& ocb);

    /**
     * Create a XorpTimer that will be scheduled once.
     *
     * @param wait the relative time when the timer expires.
     * @param ocb callback object that is invoked when timer expires.
     *
     * @return the @ref XorpTimer created.
     */
    XorpTimer new_oneoff_after(const TimeVal& wait,
			       const OneoffTimerCallback& ocb);

    /**
     * Create a XorpTimer that will be scheduled once.
     *
     * @param ms the relative time in milliseconds when the timer expires.
     * @param ocb callback object that is invoked when timer expires.
     *
     * @return the @ref XorpTimer created.
     */
    XorpTimer new_oneoff_after_ms(int ms, const OneoffTimerCallback& ocb);

    /**
     * Create a XorpTimer that will invoke a callback periodically.
     *
     * @param ms the period in milliseconds when the timer expires.
     * @param pcb user callback object that is invoked when timer expires.
     * If the callback returns false the periodic XorpTimer is unscheduled.
     *
     * @return the @ref XorpTimer created.
     */
    XorpTimer new_periodic(int ms, const PeriodicTimerCallback& pcb);

    /**
     * Create a XorpTimer to set a flag.
     *
     * @param when the absolute time when the timer expires.
     *
     * @param flag_ptr pointer to a boolean variable that is set to
     * false when this function is called and will be set to true when
     * the @ref XorpTimer expires.
     *
     * @return the @ref XorpTimer created.
     */
    XorpTimer set_flag_at(const TimeVal& when, bool *flag_ptr);

    /**
     * Create a XorpTimer to set a flag.
     *
     * @param wait the relative time when the timer expires.
     *
     * @param flag_ptr pointer to a boolean variable that is set to
     * false when this function is called and will be set to true when
     * the @ref XorpTimer expires.
     *
     * @return the @ref XorpTimer created.
     */
    XorpTimer set_flag_after(const TimeVal& wait, bool *flag_ptr);

    /**
     * Create a XorpTimer to set a flag.
     *
     * @param ms the relative time in milliseconds when the timer expires.
     *
     * @param flag_ptr pointer to a boolean variable that is set to
     * false when this function is called and will be set to true when
     * the @ref XorpTimer expires.
     *
     * @return the @ref XorpTimer created.
     */
    XorpTimer set_flag_after_ms(int ms, bool* flag_ptr);

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
    inline XorpTimer new_timer(const BasicTimerCallback& cb) {
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
    bool get_next_expire(TimeVal& tv) const;

    /**
     * Read from clock used by @ref TimerList object.
     *
     * @param now the return-by-reference value with the current time.
     */
    inline void current_time(TimeVal& now) const { _current_time_proc(&now); }

    /**
     * Default time querier.
     *
     * Get the current time by using the default time querier.
     * E.g., in non-simulation environment, this typically would
     * be gettimeofday(2).
     *
     * @param tv a pointer to the @ref TimeVal storage to store the current
     * time.
     */
    static void system_gettimeofday(TimeVal *tv);	// default time querier

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

private:
    void schedule_node(TimerNode* t);	// Put node in time ordered list pos.
    void unschedule_node(TimerNode* t); // Remove node from list.


    void acquire_lock() const		{ /* nothing, for now */ }
    bool attempt_lock() const		{ return true; }
    void release_lock() const		{ /* nothing, for now */ }

    query_current_time _current_time_proc;	// called to get time
    TimerListObserverBase * _observer;

    friend class TimerNode;
    friend class TimerListObserverBase;

    static TimerNode _dummy_timer_node;
};


class TimerNode {
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
    const TimeVal& expiry()	const	{ return _expires; }
    bool time_remaining(TimeVal& remain) const;

    void schedule_at(const TimeVal&);
    void schedule_after(const TimeVal& wait);
    void schedule_after_ms(int x_ms);
    void reschedule_after_ms(int x_ms);
    void unschedule();
    virtual void expire(XorpTimer&, void*);

    int		_ref_cnt;	// Number of referring XorpTimer objects

    TimeVal	_expires;	// Expiration time
    BasicTimerCallback _cb;

    TimerList*	_list;		// TimerList this node is associated w.
    int		_pos_in_heap;	// position of this node in heap

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
    return(_node->time_remaining(remain));
}

inline void
XorpTimer::schedule_at(const TimeVal& t)
{
    assert(_node);
    _node->schedule_at(t);
}

inline void
XorpTimer::schedule_after_ms(int x_ms)
{
    assert(_node);
    _node->schedule_after_ms(x_ms);
}

inline void
XorpTimer::schedule_now()
{
    schedule_after_ms(0);
}

inline void
XorpTimer::reschedule_after_ms(int x_ms)
{
    assert(_node);
    _node->reschedule_after_ms(x_ms);
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

TimerList::TimerList(query_current_time q)
    : Heap(OFFSET_OF(_dummy_timer_node, _pos_in_heap)),
      _current_time_proc(q), _observer(NULL)
{
}

#endif // __LIBXORP_TIMER_HH__
