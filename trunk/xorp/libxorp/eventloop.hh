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

// $XORP: xorp/libxorp/eventloop.hh,v 1.5 2003/03/27 17:01:07 hodson Exp $

#ifndef __LIBXORP_EVENTLOOP_HH__
#define __LIBXORP_EVENTLOOP_HH__

#include <sys/time.h>

#include "timer.hh"
#include "selector.hh"

/**
 * @short Event Loop.
 *
 * Co-ordinates interactions between a TimerList and a SelectorList
 * for Xorp processes.  All XorpTimer and select operations should be
 * co-ordinated through this interface.
 */
class EventLoop {
public:
    /**
     * Invoke all pending callbacks relating to XorpTimer and file
     * descriptor activity.  This function may block if there are no
     * selectors ready.  It may block forever if there are no timers
     * pending.  The @ref timers_pending method can be used to detect
     * whether there are timers pending, and the @ref descriptor_count
     * method can be used to see if there are any select'able file
     * descriptors.
     *
     * <pre>
     EventLoop e;

     ...

     while(e.timers_pending() || e.descriptor_count() > 0) {
         e.run();
     }

     * </pre>
     *
     * Non-xorp processes which use Xorp code should create a periodic
     * timer to prevent the run() function from blocking indefinitely
     * when there are no pending XorpTimer or SelectList objects. The
     * period of the timer will depend on the application's existing
     * needs.
     *
     * <pre>
     static bool wakeup_hook(int n) {
        static int count = 0;
	count += n;
	printf("count = %d\n", n);
     	return true;  
     }

     int main() {
     	... // Program initialization

    	// Add a Xorp EventLoop
	EventLoop e;
	XorpTimer wakeywakey = e.new_periodic(100, callback(wakeup_hook, 1));

	// Program's main loop
	for(;;) {
		... do what program does in its main loop ...
		e.run(); // process events
	}
     }
     * </pre> 
     */
    void run();

    /**
     * @return reference to the @ref TimerList used by the @ref
     * EventLoop instance.
     */
    TimerList&    timer_list()		{ return _timer_list; }

    /**
     * @return reference to the @ref SelectorList used by the @ref
     * EventLoop instance.
     */
    SelectorList& selector_list()	{ return _selector_list; }

    /**
     * Add a new one-off timer to the EventLoop.
     * 
     * @param when the absolute time when the timer expires.
     * @param ocb callback object that is invoked when timer expires.
     * @return a @ref XorpTimer object that must be assigned to remain 
     * scheduled.
     */
    XorpTimer new_oneoff_at(const timeval& when, 
			    const OneoffTimerCallback& ocb);

    /**
     * Add a new one-off timer to the EventLoop.
     * 
     * @param wait the relative time when the timer expires.
     * @param ocb callback object that is invoked when timer expires.
     * @return a @ref XorpTimer object that must be assigned to remain 
     * scheduled.
     */
    XorpTimer new_oneoff_after(const TimeVal& wait, 
			       const OneoffTimerCallback& ocb);

    /**
     * Add a new one-off timer to the EventLoop.
     * 
     * @param ms the relative time in milliseconds when the timer expires.
     * @param ocb callback object that is invoked when timer expires.
     * @return a @ref XorpTimer object that must be assigned to remain 
     * scheduled.
     */
    XorpTimer new_oneoff_after_ms(int ms, const OneoffTimerCallback& ocb);

    /**
     * Add periodic timer to the EventLoop.
     * 
     * @param ms the period in milliseconds when the timer expires.
     * 
     * @param pcb user callback object that is invoked when timer expires.
     * If the callback returns false the periodic XorpTimer is unscheduled.
     * 
     * @return a @ref XorpTimer object that must be assigned to remain 
     * scheduled.
     */
    XorpTimer new_periodic(int ms, const PeriodicTimerCallback& pcb);

    /**
     * Add a flag setting timer to the EventLoop.
     * 
     * @param when the absolute time when the timer expires.
     * 
     * @param flag_ptr pointer to a boolean variable that is set to
     * false when this function is called and will be set to true when
     * the @ref XorpTimer expires.
     * 
     * @return a @ref XorpTimer object that must be assigned to remain 
     * scheduled.
     */
    XorpTimer set_flag_at(const struct timeval& when, bool* flag_ptr);

    /**
     * Add a flag setting timer to the EventLoop.
     * 
     * @param wait the relative time when the timer expires.
     * 
     * @param flag_ptr pointer to a boolean variable that is set to
     * false when this function is called and will be set to true when
     * the @ref XorpTimer expires.
     * 
     * @return a @ref XorpTimer object that must be assigned to remain 
     * scheduled.
     */
    XorpTimer set_flag_after(const TimeVal& wait, bool* flag_ptr);
    
    /**
     * Add a flag setting timer to the EventLoop.
     * 
     * @param ms the relative time in millisecond when the timer expires.
     * 
     * @param flag_ptr pointer to a boolean variable that is set to
     * false when this function is called and will be set to true when
     * the @ref XorpTimer expires.
     * 
     * @return a @ref XorpTimer object that must be assigned to remain 
     * scheduled.
     */
    XorpTimer set_flag_after_ms(int ms, bool* flag_ptr);

    /**
     * Create a custom timer associated with the EventLoop.
     * 
     * The @ref XorpTimer object created needs to be explicitly
     * scheduled with the available @ref XorpTimer methods.
     * 
     * @param cb user callback object that is invoked when timer expires.
     * @return a @ref XorpTimer object that must be assigned to remain 
     * scheduled.
     */
    XorpTimer new_timer(const BasicTimerCallback& cb);

    /**
     * Add a file descriptor and callback to be invoked when
     * descriptor is ready for input or output.  A SelectorMask
     * determines what type of I/O event will cause the callback to be
     * invoked.  
     * 
     * Only one callback may be associated with each event
     * type, e.g. one callback for read pending, one callback for
     * write pending.
     * 
     * If multiple event types in are associated with the same
     * callback, the callback is only invoked once, but the mask
     * argument passed to the callback shows multiple event types.
     * 
     * @param fd the file descriptor.  
     * @param mask the @ref SelectorMask of the event types to that
     * will invoke hook.  
     * @param cb object to be invoked when file descriptor has I/O 
     * pending.
     * @return true on success.  
     */
    inline bool EventLoop::add_selector(int fd, 
					SelectorMask mask,
					const SelectorCallback& cb);

    /**
     * Remove hooks associated with file descriptor.
     * 
     * @param fd the file descriptor.
     * @param event_mask mask of event types to clear.
     */
    void remove_selector(int fd, SelectorMask event_mask = SEL_ALL);

    /**
     * @return the number of file descriptors that have registered
     * callbacks with the EventLoop @ref SelectorList.
     */
    size_t descriptor_count() const;

    /**
     * @return true if any XorpTimers are present on EventLoop's TimerList.
     */
    bool timers_pending() const;

    /**
     * @return the number of XorpTimers present on EventLoop's TimerList.
     */
    size_t timer_list_length() const;

    /**
     * Get current time according to EventLoop's TimerList
     */
    inline void current_time(timeval& now) const;

private:
    TimerList    _timer_list;
    SelectorList _selector_list;
};

// ----------------------------------------------------------------------------
// Deferred definitions

inline XorpTimer
EventLoop::new_timer(const BasicTimerCallback& cb)
{
    return _timer_list.new_timer(cb);
}

inline XorpTimer
EventLoop::new_oneoff_at(const timeval& tv, const OneoffTimerCallback& ocb)
{
    return _timer_list.new_oneoff_at(tv, ocb);
}

inline XorpTimer
EventLoop::new_oneoff_after(const TimeVal&	       wait, 
			    const OneoffTimerCallback& ocb)
{
    return _timer_list.new_oneoff_after(wait, ocb);
}

inline XorpTimer
EventLoop::new_oneoff_after_ms(int ms, const OneoffTimerCallback& ocb)
{
    return _timer_list.new_oneoff_after_ms(ms, ocb);
}

inline XorpTimer
EventLoop::new_periodic(int period_ms, const PeriodicTimerCallback& pcb)
{
    return _timer_list.new_periodic(period_ms, pcb);
}

inline XorpTimer
EventLoop::set_flag_at(const timeval& tv, bool *flag_ptr)
{
    return _timer_list.set_flag_at(tv, flag_ptr);
}

inline XorpTimer
EventLoop::set_flag_after(const TimeVal& wait, bool *flag_ptr)
{
    return _timer_list.set_flag_after(wait, flag_ptr);
}

inline XorpTimer
EventLoop::set_flag_after_ms(int ms, bool *flag_ptr)
{
    return _timer_list.set_flag_after_ms(ms, flag_ptr);
}

inline bool
EventLoop::add_selector(int			fd,
			SelectorMask		mask, 
			const SelectorCallback&	cb)
{
    return _selector_list.add_selector(fd, mask, cb);
}

inline void
EventLoop::remove_selector(int fd, SelectorMask event_mask)
{
    _selector_list.remove_selector(fd, event_mask);
}

inline size_t
EventLoop::descriptor_count() const {
    return _selector_list.descriptor_count();
}

inline bool
EventLoop::timers_pending() const
{
    return !_timer_list.empty();
}

inline size_t
EventLoop::timer_list_length() const
{
    return _timer_list.size();
}

inline void
EventLoop::current_time(timeval& t) const 
{
    _timer_list.current_time(t);
}


#endif // __LIBXORP_EVENTLOOP_HH__
