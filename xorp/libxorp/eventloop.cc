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



#include "libxorp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxipc/finder.hh"
#include "eventloop.hh"
#include <sys/types.h>
#include <signal.h>

//
// Number of EventLoop instances.
//
int eventloop_instance_count = 0;

int xorp_do_run = 1;
char xorp_sig_msg_buffer[64];

//Trap some common signals to allow graceful exit.
// NOTE:  Cannot do logging here, that logic is not re-entrant.
// Copy msg into xorp-sig_msg_buffer instead..main program can check
// if they want, and we also register an atexit handler to print it out
// on exit.
void dflt_sig_handler(int signo) { 
    //reestablish signal handler
    signal(signo, (&dflt_sig_handler));

    switch (signo) {
    case SIGTERM:
	strncpy(xorp_sig_msg_buffer, "SIGTERM received", sizeof(xorp_sig_msg_buffer));
	goto do_terminate;
    case SIGINT:
	strncpy(xorp_sig_msg_buffer, "SIGINT received", sizeof(xorp_sig_msg_buffer));
	goto do_terminate;
    case SIGXCPU:
	strncpy(xorp_sig_msg_buffer, "SIGINT received", sizeof(xorp_sig_msg_buffer));
	goto do_terminate;
    case SIGXFSZ:
	strncpy(xorp_sig_msg_buffer, "SIGINT received", sizeof(xorp_sig_msg_buffer));
	goto do_terminate;
    default:
	// This is a coding error and we need to fix it.
	assert("WARNING:  Ignoring un-handled error in dflt_sig_handler." == NULL);
	return;
    }//switch

  do_terminate:
    xorp_do_run = 0;
	
    // Now, kick any selects that are blocking,
    // SIGURG seems harmless enough to use.
    kill(getpid(), SIGURG);

}//dflt_sig_handler


void xorp_sig_atexit() {
    if (xorp_sig_msg_buffer[0]) {
	cerr << "WARNING:  Process: " << getpid() << " has message from dflt_sig_handler: "
	     << xorp_sig_msg_buffer << endl;
    }
}

void setup_dflt_sighandlers() {
    memset(xorp_sig_msg_buffer, 0, sizeof(xorp_sig_msg_buffer));
    atexit(xorp_sig_atexit);

    signal(SIGTERM, dflt_sig_handler);
    signal(SIGINT, dflt_sig_handler);
    signal(SIGXCPU, dflt_sig_handler);
    signal(SIGXFSZ, dflt_sig_handler);
}



EventLoop::EventLoop()
    : _clock(new SystemClock), _timer_list(_clock), _aggressiveness(0),
      _last_ev_run(0), _last_warned(0), _is_debug(false),
      _selector_list(_clock)
{
    XLOG_ASSERT(eventloop_instance_count == 0);
    XLOG_ASSERT(_last_ev_run == 0);
    eventloop_instance_count++;

    // Totally unnecessary initialization should stop complaints from
    // clever tools.
    for (int i = 0; i < XorpTask::PRIORITY_INFINITY; i++)
	_last_ev_type[i] = true;

    //
    // XXX: Ignore SIGPIPE, because we always check the return code.
    // If the program needs to install a SIGPIPE signal handler, the
    // handler must be installed after the EventLoop instance is created.
    //
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif
}

EventLoop::~EventLoop()
{
    eventloop_instance_count--;
    XLOG_ASSERT(eventloop_instance_count == 0);
    //
    // XXX: The _clock pointer is invalidated for other EventLoop fields
    // that might be using it.
    //
    delete _clock;
    _clock = NULL;
}

void
EventLoop::run()
{
    static const time_t MAX_ALLOWED = (XORP_HELLO_TIMER_MS/1000) + 2;
    TimeVal t;

    _timer_list.advance_time();
    _timer_list.current_time(t);

    if (_last_ev_run == 0)
	_last_ev_run = t.sec();

    time_t now  = t.sec();
    time_t diff = now - _last_ev_run;

    if (now - _last_warned > 0 && (diff > MAX_ALLOWED)) {
	XLOG_WARNING("%d seconds between calls to EventLoop::run", (int)diff);
	_last_warned = now;
    }

    // standard eventloop run
    bool more = do_work(true);

    // Aggressive eventloop runs.  Going through the eventloop from the start is
    // expensive because we need to call time() and select().  If there's
    // additional work to do, do it now rather than going through all the
    // bureaucracy again.  Because we (sometimes) have an outdated time() and
    // select() we could run low priority tasks instead of higher ones simply
    // because we don't know about them yet (need to call time or select to
    // detect them).  For this reason, we only do a few "aggressive" runs.
    // -sorbo.
    int agressiveness = _aggressiveness;
    while (more && agressiveness--)
	more = do_work(false);

    // select() could cause a large delay and will advance_time.  Re-read
    // current time.  Maybe we can get rid of advance_time if timeout to select
    // is small, or get rid of advance_time at the top of event loop.  -sorbo
    _timer_list.current_time(t);
    _last_ev_run = t.sec();
}

bool
EventLoop::do_work(bool can_block)
{
    TimeVal t;
    UNUSED(can_block);

    _timer_list.current_time(t);
    _timer_list.get_next_delay(t);

    
    // Run timers if they need it.
    if (t == TimeVal::ZERO()) {
	_timer_list.run();
    }
    
    if (!_task_list.empty()) {
	_task_list.run();
	if (!_task_list.empty()) {
	    // Run task again as soon as possible.
	    t = TimeVal::ZERO();
	}
    }
    
#ifdef HOST_OS_WINDOWS
    _win_dispatcher.wait_and_dispatch(t);
#else
    _selector_list.wait_and_dispatch(t);
#endif
    
    return false; // don't use the 'aggressiveness' logic.
    
#if 0
    int timer_priority	  = XorpTask::PRIORITY_INFINITY;
    int selector_priority = XorpTask::PRIORITY_INFINITY;
    int task_priority	  = XorpTask::PRIORITY_INFINITY;

    if (t == TimeVal::ZERO())
	timer_priority = _timer_list.get_expired_priority();

    selector_priority = _selector_list.get_ready_priority(can_block);

    if (!_task_list.empty())
	task_priority = _task_list.get_runnable_priority();

    debug_msg("Priorities: timer = %d selector = %d task = %d\n",
	      timer_priority, selector_priority, task_priority);

    if ( (timer_priority != XorpTask::PRIORITY_INFINITY)
	 && (timer_priority <= selector_priority)
	 && (timer_priority <= task_priority)) {

	// the most important thing to run next is a timer
	_timer_list.run();

    } else if ( (selector_priority != XorpTask::PRIORITY_INFINITY)
		&& (selector_priority < task_priority) ) {

	// the most important thing to run next is a selector
	_selector_list.wait_and_dispatch(t);

    } else if ( (task_priority != XorpTask::PRIORITY_INFINITY)	
		 && (task_priority < selector_priority) ) {
		     
	// the most important thing to run next is a task
	_task_list.run();

    } else if ( (selector_priority != XorpTask::PRIORITY_INFINITY)
		&& (task_priority != XorpTask::PRIORITY_INFINITY) ) {
		    XLOG_ASSERT(selector_priority == task_priority);
		    XLOG_ASSERT(task_priority < XorpTask::PRIORITY_INFINITY);

		    // There is a task and selector event of the same
		    // priority to guard against starvation flip
		    // between the two.

		    if (_last_ev_type[task_priority]) {
			_task_list.run();
			_last_ev_type[task_priority] = false;
		    } else {
			_selector_list.wait_and_dispatch(t);
			_last_ev_type[task_priority] = true;
		    }
    } else {
	if (!can_block)
	    return false;

	// there's nothing immediate to run, so go to sleep until the
	// next selector or timer goes off
	_selector_list.wait_and_dispatch(t);
	// XXX return false if you want to be conservative.  -sorbo.
    }
#endif
    return true;
}

bool
EventLoop::add_ioevent_cb(XorpFd fd, IoEventType type, const IoEventCb& cb,
			  int priority)
{
    return _selector_list.add_ioevent_cb(fd, type, cb, priority);
}

bool
EventLoop::remove_ioevent_cb(XorpFd fd, IoEventType type)
{
    _selector_list.remove_ioevent_cb(fd, type);
    return true;
}

size_t
EventLoop::descriptor_count() const
{
    return _selector_list.descriptor_count();
}

XorpTask
EventLoop::new_oneoff_task(const OneoffTaskCallback& cb, int priority,
			   int weight)
{
    return _task_list.new_oneoff_task(cb, priority, weight);
}

XorpTask
EventLoop::new_task(const RepeatedTaskCallback& cb, int priority, int weight)
{
    return _task_list.new_task(cb, priority, weight);
}

void
EventLoop::set_aggressiveness(int num)
{
    _aggressiveness = num;
}
