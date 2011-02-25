// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2010 XORP, Inc and Others
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

// $XORP: xorp/libxorp/eventloop.hh,v 1.41 2008/11/08 05:31:25 atanu Exp $

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


// The default signal handler logic will catch:
// SIGTERM, SIGINT, SIGXFSZ, SIGXCPU
// and set xorp_do_run to 0.  Control loops can use this
// to bail out and gracefully exit.
extern int xorp_do_run;
extern char xorp_sig_msg_buffer[64];
void setup_dflt_sighandlers();
void dflt_sig_handler(int signo);


/**
 * @short Event Loop.
 *
 * Co-ordinates interactions between a TimerList and a SelectorList
 * for Xorp processes.  All XorpTimer and select operations should be
 * co-ordinated through this interface.
 */
class EventLoop :
    public NONCOPYABLE
{
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

    void set_debug(bool v) {
	_is_debug = v;
#ifndef HOST_OS_WINDOWS
	_selector_list.set_debug(v);
#endif
    }

    bool is_debug() const { return (_is_debug); }

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

    void set_aggressiveness(int num);

private:
    bool do_work(bool can_block);

private:
    ClockBase*		_clock;
    TimerList		_timer_list;
    TaskList		_task_list;
    int			_aggressiveness;
    time_t 		_last_ev_run;	// 0 - Means run not called yet.
    time_t 		_last_warned;
    bool		_is_debug;	// If true, debug enabled
    // Was the last event at this priority a selector or a task.
    bool		_last_ev_type[XorpTask::PRIORITY_INFINITY]; 
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
EventLoop::new_oneoff_after(const TimeVal& wait,
			    const OneoffTimerCallback& ocb,
			    int priority)
{
    return _timer_list.new_oneoff_after(wait, ocb, priority);
}

inline XorpTimer
EventLoop::new_oneoff_after_ms(int ms, const OneoffTimerCallback& ocb,
			       int priority)
{
    TimeVal wait(ms / 1000, (ms % 1000) * 1000);
    return _timer_list.new_oneoff_after(wait, ocb, priority);
}

inline XorpTimer
EventLoop::new_periodic(const TimeVal& wait, const PeriodicTimerCallback& pcb,
			int priority)
{
    return _timer_list.new_periodic(wait, pcb, priority);
}

inline XorpTimer
EventLoop::new_periodic_ms(int ms, const PeriodicTimerCallback& pcb,
			   int priority)
{
    TimeVal wait(ms / 1000, (ms % 1000) * 1000);
    return _timer_list.new_periodic(wait, pcb, priority);
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
    TimeVal wait(ms / 1000, (ms % 1000) * 1000);
    return _timer_list.set_flag_after(wait, flag_ptr, to_value);
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
