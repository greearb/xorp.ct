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

// $XORP: xorp/libxorp/timer.hh,v 1.32 2002/12/09 18:29:15 hodson Exp $

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

//typedef void (*TimerHook)(XorpTimer&, void*);

/**
 * Deprecated callback types - use OneoffTimerCallback/PeriodicTimerCallback
 * instead.
 */
typedef void (*OneoffTimerHook)(void*);	  
typedef bool (*PeriodicTimerHook)(void*); 

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
     * @return true if XorpTimer is associated with a @ref TimerList and
     * has an expiry time in the future. 
     */
    bool scheduled() const;

    /**
     * @return the expiry time of the @ref XorpTimer
     */
    const timeval& expiry() const;

    /**
     * Expire the @ref XorpTimer object when the TimerList is next run.
     */
    void schedule_now();

    /**
     * Schedule the @ref XorpTimer object at a given time.
     */
    void schedule_at(const timeval& when);

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

    /**
     * @return true if @ref XorpTimer object has underlying state.
     */
    bool initialized() const		{ return _node != 0; }

    /**
     * Equivalent to @ref initialized.
     */
    operator bool() const		{ return initialized(); }

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
typedef void (*query_current_time)(timeval*);

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
    XorpTimer new_oneoff_at(const timeval& when, 
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
     * [Deprecated]
     * Create a XorpTimer that will be scheduled once.
     *
     * @param when the absolute time when the timer expires.
     * @param hook user function that is invoked when timer expires.
     * @param thunk argument supplied to user function when timer expires.
     *
     * @return the @ref XorpTimer created.
     */
    inline XorpTimer new_oneoff_at(const timeval& when, 
				   OneoffTimerHook hook, 
				   void* thunk = 0)
    {
	return new_oneoff_at(when, callback(hook, thunk));
    }

    /**
     * [Deprecated]
     * Create a XorpTimer that will invoke a callback once. 
     *
     * @param ms the relative time in milliseconds when the timer expires.
     * @param hook user function that is invoked when timer expires.
     * @param thunk argument supplied to user function when timer expires.
     *
     * @return the @ref XorpTimer created.
     */
    inline XorpTimer new_oneoff_after_ms(int ms, 
					 OneoffTimerHook hook, 
					 void* thunk = 0)
    {
	return new_oneoff_after_ms(ms, callback(hook, thunk));
    }

    /**
     * [Deprecated]
     * Create a XorpTimer that will invoke a callback periodically. 
     *
     * @param ms the period in milliseconds when the timer expires.
     *
     * @param hook user function that is invoked when timer expires.
     * If the hook returns false the periodic XorpTimer is unscheduled.
     *
     * @param thunk argument supplied to user function when timer
     * expires.
     *
     * @return the @ref XorpTimer created.  
     */
    inline XorpTimer new_periodic(int ms, 
				  PeriodicTimerHook hook, void* thunk = 0)
    {
	return new_periodic(ms, callback(hook, thunk));
    }

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
    XorpTimer set_flag_at(const timeval& when, bool* flag_ptr);

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
     * maximum timeval value.
     *
     * @return true if there is a XorpTimer awaiting expiry, false otherwise.  
     */
    bool get_next_delay(timeval& tv) const;

    /**
     * Read from clock used by @ref TimerList object.
     *
     * @param now the timeval structure to be assigned the current time.
     */
    inline void current_time(timeval& now) const { _current_time_proc(&now); }
    
private:
    void schedule_node(TimerNode* t);	// Put node in time ordered list pos.
    void unschedule_node(TimerNode* t); // Remove node from list.

    void acquire_lock() const		{ /* nothing, for now */ }
    bool attempt_lock() const		{ return true; }
    void release_lock() const		{ /* nothing, for now */ }

    query_current_time _current_time_proc;	// called to get time
    static void system_gettimeofday(timeval*);	// default time querier

    friend class TimerNode;
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

    bool scheduled() const	{ return _pos_in_heap >= 0; }
    const timeval& expiry() const	{ return _expires; }
    
    void schedule_at(const timeval&);
    void schedule_after_ms(int x_ms);
    void reschedule_after_ms(int x_ms);
    void unschedule();
    virtual void expire(XorpTimer&, void*);

    int		_ref_cnt;	// Number of referring XorpTimer objects

    timeval	_expires;	// Expiration time
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

inline const timeval&
XorpTimer::expiry() const
{
    assert(_node);
    return _node->expiry();
}

inline void
XorpTimer::schedule_at(const timeval& t)
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

TimerList::TimerList(query_current_time q = system_gettimeofday)
    : Heap(OFFSET_OF(*(TimerNode *)0, _pos_in_heap)),
      _current_time_proc(q)
{
}

#endif // __LIBXORP_TIMER_HH__
