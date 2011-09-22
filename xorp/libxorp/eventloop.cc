// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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

EnvTrace eloop_trace("ELOOPTRACE");

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
#ifndef	HOST_OS_WINDOWS
    case SIGXCPU:
	strncpy(xorp_sig_msg_buffer, "SIGINT received", sizeof(xorp_sig_msg_buffer));
	goto do_terminate;
    case SIGXFSZ:
	strncpy(xorp_sig_msg_buffer, "SIGINT received", sizeof(xorp_sig_msg_buffer));
	goto do_terminate;
#endif
    default:
	// This is a coding error and we need to fix it.
	assert("WARNING:  Ignoring un-handled error in dflt_sig_handler." == NULL);
	return;
    }//switch

  do_terminate:
    xorp_do_run = 0;

    // Now, kick any selects that are blocking,
#ifndef	HOST_OS_WINDOWS
    // SIGURG seems harmless enough to use.
    kill(getpid(), SIGURG);
#else
    // Maybe this will work on Windows
    raise(SIGINT);
#endif

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
#ifndef	HOST_OS_WINDOWS
    signal(SIGXCPU, dflt_sig_handler);
    signal(SIGXFSZ, dflt_sig_handler);
#endif
}



EventLoop::EventLoop()
    : _clock(new SystemClock), _timer_list(_clock), _aggressiveness(0),
      _last_ev_run(0), _last_warned(0), _is_debug(false),
#ifdef USE_WIN_DISPATCHER
      _win_dispatcher(_clock)
#else
      _selector_list(_clock)
#endif
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
    do_work();

    // select() could cause a large delay and will advance_time.  Re-read
    // current time.
    _timer_list.current_time(t);
    _last_ev_run = t.sec();
}

void
EventLoop::do_work()
{
    TimeVal t;
    TimeVal start;

    _timer_list.get_next_delay(t);
    
    // Run timers if they need it.
    if (t == TimeVal::ZERO()) {
	_timer_list.current_time(start);
	_timer_list.run();
	if (eloop_trace.on()) {
	    _timer_list.advance_time();
	    TimeVal n2;
	    _timer_list.current_time(n2);
	    if (n2.to_ms() > start.to_ms() + 20) {
		XLOG_INFO("timer-list run took too long to run: %lims\n",
			  (long)(n2.to_ms() - start.to_ms()));
	    }
	}
    }
    
    if (!_task_list.empty()) {
	_timer_list.current_time(start);
	_task_list.run();
	if (eloop_trace.on()) {
	    _timer_list.advance_time();
	    TimeVal n2;
	    _timer_list.current_time(n2);
	    if (n2.to_ms() > start.to_ms() + 20) {
		XLOG_INFO("task-list run took too long to run: %lims\n",
			  (long)(n2.to_ms() - start.to_ms()));
	    }
	}
	if (!_task_list.empty()) {
	    // Run task again as soon as possible.
	    t.set_ms(0);
	}
    }
    
    // If we are trying to shut down..make sure the event loop
    // doesn't hang forever.
    if (!xorp_do_run) {
	if ((t == TimeVal::MAXIMUM()) ||
	    (t.to_ms() > 1000)) {
	    t = TimeVal(1, 0); // one sec
	}
    }

    _timer_list.current_time(start);
#if USE_WIN_DISPATCHER

    // Seeing weird slownesses on windows..going to cap the timeout and see
    // if that helps.
    if (t.to_ms() > 100)
	t.set_ms(100);
    else if (t.to_ms() < 0)
	t.set_ms(0);

    _win_dispatcher.wait_and_dispatch(t);
#else
    _selector_list.wait_and_dispatch(t);
#endif
    if (eloop_trace.on()) {
	TimeVal n2;
	_timer_list.current_time(n2);
	if (n2.to_ms() > start.to_ms() + t.to_ms() + 20) {
	    XLOG_INFO("wait-and-dispatch took too long to run: %lims\n",
		      (long)(n2.to_ms() - start.to_ms()));
	}
    }
}

bool
EventLoop::add_ioevent_cb(XorpFd fd, IoEventType type, const IoEventCb& cb,
			  int priority)
{
#ifdef USE_WIN_DISPATCHER
    return _win_dispatcher.add_ioevent_cb(fd, type, cb, priority);
#else
    return _selector_list.add_ioevent_cb(fd, type, cb, priority);
#endif
}

bool
EventLoop::remove_ioevent_cb(XorpFd fd, IoEventType type)
{
#ifdef USE_WIN_DISPATCHER
    return _win_dispatcher.remove_ioevent_cb(fd, type);
#else
    _selector_list.remove_ioevent_cb(fd, type);
    return true;
#endif
}

size_t
EventLoop::descriptor_count() const
{
#ifdef USE_WIN_DISPATCHER
    return _win_dispatcher.descriptor_count();
#else
    return _selector_list.descriptor_count();
#endif
}

/** Remove timer from timer list. */
void EventLoop::remove_timer(XorpTimer& t) {
    _timer_list.remove_timer(t);
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
