// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

// $XORP: xorp/libxorp/eventloop.hh,v 1.27 2007/02/16 22:46:18 pavlin Exp $

#ifndef __LIBXORP_EVENTLOOP_HH__
#define __LIBXORP_EVENTLOOP_HH__

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "xorpfd.hh"
#include "clock.hh"
#include "timer.hh"
#include "task.hh"
#include "callback.hh"
#include "ioevents.hh"

#ifdef HOST_OS_WINDOWS
#include "win_dispatcher.hh"
#else
#include "selector.hh"
#endif

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
     * Constructor
     */
    EventLoop();

    /**
     * Destructor.
     */
    ~EventLoop();

    /**
     * Invoke all pending callbacks relating to XorpTimer and file
     * descriptor activity.  This function may block if there are no
     * selectors ready.  It may block forever if there are no timers
     * pending.  The @ref timers_pending method can be used to detect
     * whether there are timers pending, while the @ref events_pending
     * method can be used to detect whether there any events pending.
     * An event can be either timer or task.
     * The @ref descriptor_count method can be used to see if there are
     * any select'able file descriptors.
     *
     * <pre>
     EventLoop e;

     ...

     while(e.events_pending() || e.descriptor_count() > 0) {
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
	XorpTimer wakeywakey = e.new_periodic_ms(100, callback(wakeup_hook, 1));

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

#ifndef HOST_OS_WINDOWS
    /**
     * @return reference to the @ref SelectorList used by the @ref
     * EventLoop instance.
     * XXX: Deprecated.
     */
    SelectorList& selector_list()	{ return _selector_list; }
#endif

    /**
     * Add a new one-off timer to the EventLoop.
     *
     * @param when the absolute time when the timer expires.
     * @param ocb callback object that is invoked when timer expires.
     * @return a @ref XorpTimer object that must be assigned to remain
     * scheduled.
     */
    XorpTimer new_oneoff_at(const TimeVal& when,
			    const OneoffTimerCallback& ocb,
			    int priority = XorpTask::PRIORITY_DEFAULT);

    /**
     * Add a new one-off timer to the EventLoop.
     *
     * @param wait the relative time when the timer expires.
     * @param ocb callback object that is invoked when timer expires.
     * @return a @ref XorpTimer object that must be assigned to remain
     * scheduled.
     */
    XorpTimer new_oneoff_after(const TimeVal& wait,
			       const OneoffTimerCallback& ocb,
			       int priority = XorpTask::PRIORITY_DEFAULT);

    /**
     * Add a new one-off timer to the EventLoop.
     *
     * @param ms the relative time in milliseconds when the timer expires.
     * @param ocb callback object that is invoked when timer expires.
     * @return a @ref XorpTimer object that must be assigned to remain
     * scheduled.
     */
    XorpTimer new_oneoff_after_ms(int ms, const OneoffTimerCallback& ocb,
				  int priority = XorpTask::PRIORITY_DEFAULT);

    /**
     * Add periodic timer to the EventLoop.
     *
     * @param wait the period when the timer expires.
     * @param pcb user callback object that is invoked when timer expires.
     * If the callback returns false the periodic XorpTimer is unscheduled.
     * @return a @ref XorpTimer object that must be assigned to remain
     * scheduled.
     */
    XorpTimer new_periodic(const TimeVal& wait,
			   const PeriodicTimerCallback& pcb,
			   int priority = XorpTask::PRIORITY_DEFAULT);

    /**
     * Add periodic timer to the EventLoop.
     *
     * @param ms the period in milliseconds when the timer expires.
     * @param pcb user callback object that is invoked when timer expires.
     * If the callback returns false the periodic XorpTimer is unscheduled.
     * @return a @ref XorpTimer object that must be assigned to remain
     * scheduled.
     */
    XorpTimer new_periodic_ms(int ms, const PeriodicTimerCallback& pcb,
			      int priority = XorpTask::PRIORITY_DEFAULT);

    /**
     * Add a flag setting timer to the EventLoop.
     *
     * @param when the absolute time when the timer expires.
     * @param flag_ptr pointer to a boolean variable that is to be set
     * when the timer expires.
     * @param to_value value to set the boolean variable to.  Default value
     * is true.
     * @return a @ref XorpTimer object that must be assigned to remain
     * scheduled.
     */
    XorpTimer set_flag_at(const TimeVal& 	when,
			  bool* 		flag_ptr,
			  bool 			to_value = true);

    /**
     * Add a flag setting timer to the EventLoop.
     *
     * @param wait the relative time when the timer expires.
     * @param flag_ptr pointer to a boolean variable that is to be set
     * when the timer expires.
     * @param to_value value to set the boolean variable to.  Default value
     * is true.
     * @return a @ref XorpTimer object that must be assigned to remain
     * scheduled.
     */
    XorpTimer set_flag_after(const TimeVal& 	wait,
			     bool* 		flag_ptr,
			     bool 		to_value = true);

    /**
     * Add a flag setting timer to the EventLoop.
     *
     * @param ms the relative time in millisecond when the timer expires.
     * @param flag_ptr pointer to a boolean variable that is set to
     * false when this function is called and will be set to true when
     * the @ref XorpTimer expires.
     * @param flag_ptr pointer to a boolean variable that is to be set
     * when the timer expires.
     * @param to_value value to set the boolean variable to.  Default value
     * is true.
     * @return a @ref XorpTimer object that must be assigned to remain
     * scheduled.
     */
    XorpTimer set_flag_after_ms(int ms, bool* flag_ptr, bool to_value = true);

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
     * Create a new one-time task to be scheduled with the timers and
     * file handlers.
     *
     * @param cb callback object that is invoked when task is run.
     * @param priority the scheduling priority for the task.  
     * @param scheduler_class the scheduling class within the priority level.
     * @return a @ref XorpTask object that must be assigned to remain
     * scheduled.
     */
    XorpTask new_oneoff_task(const OneoffTaskCallback& cb,
			     int priority = XorpTask::PRIORITY_DEFAULT,
			     int weight = XorpTask::WEIGHT_DEFAULT);

    /** 
     * Create a new repeated task to be scheduled with the timers and file
     * handlers.
     *
     * @param cb callback object that is invoked when task is run.
     * If the callback returns true, the task will continue to run,
     * otherwise it will be unscheduled.
     * @param priority the scheduling priority for the task.  
     * @param scheduler_class the scheduling class within the priority level.
     * @return a @ref XorpTask object that must be assigned to remain
     * scheduled.
     */
    XorpTask new_task(const RepeatedTaskCallback& cb,
		      int priority = XorpTask::PRIORITY_DEFAULT,
		      int weight = XorpTask::WEIGHT_DEFAULT);

    /**
     * Add a file descriptor and callback to be invoked when
     * descriptor is ready for input or output.  An IoEventType
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
     * @param type the @ref IoEventType of the event.
     * @param cb object to be invoked when file descriptor has I/O
     * pending.
     * @return true on success, false if any error occurred.
     */
    bool add_ioevent_cb(XorpFd fd, IoEventType type, const IoEventCb& cb,
			int priority = XorpTask::PRIORITY_DEFAULT);

    /**
     * Remove callbacks associated with file descriptor.
     *
     * @param fd the file descriptor.
     * @param type the event type to clear.
     * The special value IOT_ANY means clear any kind of callback.
     * @return true on success, false if any error occurred.
     */
    bool remove_ioevent_cb(XorpFd fd, IoEventType type = IOT_ANY);

    /**
     * @return true if any XorpTimers are present on EventLoop's TimerList.
     */
    bool timers_pending() const;

    /**
     * @return true if any XorpTimers are present on EventLoop's
     * TimerList or any XorpTasks are present on the TaskList.
     */
    bool events_pending() const;

    /**
     * @return the number of XorpTimers present on EventLoop's TimerList.
     */
    size_t timer_list_length() const;

    /**
     * Get current time according to EventLoop's TimerList
     */
    void current_time(TimeVal& now) const;

    /**
     * Get the count of the descriptors that have been added.
     *
     * @return the count of the descriptors that have been added.
     */
    size_t descriptor_count() const;

private:
    EventLoop(const EventLoop&);		// not implemented
    EventLoop& operator=(const EventLoop&);	// not implemented

private:
    ClockBase*		_clock;
    TimerList		_timer_list;
    TaskList		_task_list;
#ifdef HOST_OS_WINDOWS
    WinDispatcher	_win_dispatcher;
#else
    SelectorList	_selector_list;
#endif
};

// ----------------------------------------------------------------------------
// Deferred definitions

inline XorpTimer
EventLoop::new_timer(const BasicTimerCallback& cb)
{
    return _timer_list.new_timer(cb);
}

inline XorpTimer
EventLoop::new_oneoff_at(const TimeVal& tv, const OneoffTimerCallback& ocb,
			 int priority)
{
    return _timer_list.new_oneoff_at(tv, ocb, priority);
}

inline XorpTimer
EventLoop::new_oneoff_after(const TimeVal&	       wait,
			    const OneoffTimerCallback& ocb,
			    int priority)
{
    return _timer_list.new_oneoff_after(wait, ocb, priority);
}

inline XorpTimer
EventLoop::new_oneoff_after_ms(int ms, const OneoffTimerCallback& ocb,
			       int priority)
{
    return _timer_list.new_oneoff_after_ms(ms, ocb, priority);
}

inline XorpTimer
EventLoop::new_periodic(const TimeVal& wait, const PeriodicTimerCallback& pcb,
			int priority)
{
    return _timer_list.new_periodic(wait, pcb, priority);
}

inline XorpTimer
EventLoop::new_periodic_ms(int period_ms, const PeriodicTimerCallback& pcb,
			   int priority)
{
    return _timer_list.new_periodic_ms(period_ms, pcb, priority);
}

inline XorpTimer
EventLoop::set_flag_at(const TimeVal& tv, bool *flag_ptr, bool to_value)
{
    return _timer_list.set_flag_at(tv, flag_ptr, to_value);
}

inline XorpTimer
EventLoop::set_flag_after(const TimeVal& wait, bool *flag_ptr, bool to_value)
{
    return _timer_list.set_flag_after(wait, flag_ptr, to_value);
}

inline XorpTimer
EventLoop::set_flag_after_ms(int ms, bool *flag_ptr, bool to_value)
{
    return _timer_list.set_flag_after_ms(ms, flag_ptr, to_value);
}

inline bool
EventLoop::timers_pending() const
{
    return !_timer_list.empty();
}

inline bool
EventLoop::events_pending() const
{
    return ((!_timer_list.empty()) || (!_task_list.empty()));
}

inline size_t
EventLoop::timer_list_length() const
{
    return _timer_list.size();
}

inline void
EventLoop::current_time(TimeVal& t) const
{
    _timer_list.current_time(t);
}

#endif // __LIBXORP_EVENTLOOP_HH__
