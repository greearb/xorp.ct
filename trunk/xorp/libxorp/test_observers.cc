// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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



//
// Test program to the Observer classes for TimerList and SelectorList
//

#include "libxorp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/xorpfd.hh"

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "timer.hh"
#include "eventloop.hh"


#ifndef HOST_OS_WINDOWS

static int fired(0);

static int add_rem_fd_counter(0);
static int add_rem_mask_counter(0);

static bool fd_read_notification_called = false;
static bool fd_write_notification_called = false;
static bool fd_exeption_notification_called = false;
static bool fd_removal_notification_called = false;
static bool schedule_notification_called = false;
static bool unschedule_notification_called = false;
static bool one_fd_rem_per_each_fd_add_notif = false;
static bool no_notifications_beyond_this_point = false;




void
print_tv(FILE * s, TimeVal a)
{
    fprintf(s, "%lu.%06lu", (unsigned long)a.sec(), (unsigned long)a.usec());
    fflush(s);
}

// callback for non-periodic timer. Does not need to return a value
static void some_foo() {
    fired++;
    printf("#"); fflush(stdout);
}

// callback for a periodic timer. If true, the timer is rescheduled.
static bool print_dot() {
    printf("."); fflush(stdout);
    return true;
}

// callback for selector.  
static void do_the_twist(XorpFd fd, IoEventType event) {
    printf("fd: %s event: %0x is here\n", fd.str().c_str(), event);
}

// Implement the SelectorList observer and notification functions
class mySelectorListObserver : public SelectorListObserverBase {
    void notify_added(XorpFd, const SelectorMask&); 
    void notify_removed(XorpFd, const SelectorMask&); 
};

void 
mySelectorListObserver::notify_added(XorpFd fd, const SelectorMask& mask)
{
    fprintf(stderr, "notif added fd: %s mask: %#0x\n", fd.str().c_str(), mask);
    add_rem_fd_counter+=fd;
    add_rem_mask_counter+=mask;
    switch (mask) {
	case SEL_RD:	fd_read_notification_called = true; break;
	case SEL_WR:	fd_write_notification_called = true; break;
	case SEL_EX:	fd_exeption_notification_called = true; break;
	default:	fprintf(stderr, "invalid mask notification\n"); exit(1);
    }
} 

void 
mySelectorListObserver::notify_removed(XorpFd fd, const SelectorMask& mask)
{
    fprintf(stderr, "notif removed fd: %s mask: %#0x\n", fd.str().c_str(),
	    mask);
    add_rem_fd_counter-=fd;
    add_rem_mask_counter-=mask;
    if (no_notifications_beyond_this_point) {
	fprintf(stderr, "duplicate removal!!\n");
	exit(1);
    }
    fd_removal_notification_called = true;   
}

// Implement TimerList observer and notification functions
class myTimerListObserver : public TimerListObserverBase {
    void notify_scheduled(const TimeVal&);
    void notify_unscheduled(const TimeVal&);
};

void myTimerListObserver::notify_scheduled(const TimeVal& tv) 
{
    fprintf(stderr, "notif sched ");print_tv(stderr, tv);fprintf(stderr, "\n");
    schedule_notification_called = true;
}

void myTimerListObserver::notify_unscheduled(const TimeVal& tv) 
{
    fprintf(stderr, "notif unsch "); print_tv(stderr, tv);fprintf(stderr, "\n");
    unschedule_notification_called = true;
}

void run_test()
{
    EventLoop e;
    mySelectorListObserver sel_obs;
    myTimerListObserver timer_obs;

    e.selector_list().set_observer(sel_obs);
    e.timer_list().set_observer(timer_obs);

    XorpTimer show_stopper; 
    show_stopper = e.new_oneoff_after_ms(500, callback(some_foo));
    assert(show_stopper.scheduled());

    XorpTimer zzz = e.new_periodic_ms(30, callback(print_dot));
    assert(zzz.scheduled());

    XorpFd fd[2];
    int pipefds[2];

#ifndef HOST_OS_WINDOWS
    if (pipe(pipefds)) {
	fprintf(stderr, "unable to generate file descriptors for test\n");
	exit(2);
    }
    fd[0] = XorpFd(pipefds[0]);
    fd[1] = XorpFd(pipefds[1]);
#else // HOST_OS_WINDOWS
    if (_pipe(pipefds, 65536, _O_BINARY)) {
	fprintf(stderr, "unable to generate file descriptors for test\n");
	exit(2);
    }
    fd[0] = XorpFd(_get_osfhandle(pipefds[0]));
    fd[1] = XorpFd(_get_osfhandle(pipefds[1]));
#endif // HOST_OS_WINDOWS

    XorpCallback2<void,XorpFd,IoEventType>::RefPtr cb = callback(do_the_twist);
    e.selector_list().add_ioevent_cb(fd[0], IOT_READ, cb);
    e.selector_list().add_ioevent_cb(fd[1], IOT_WRITE, cb);
    e.selector_list().add_ioevent_cb(fd[0], IOT_EXCEPTION, cb);

    while(show_stopper.scheduled()) {
	assert(zzz.scheduled());    
	e.run(); // run will return after one or more pending events
		 // have fired.
	e.selector_list().remove_ioevent_cb(fd[0], IOT_READ);
	e.selector_list().remove_ioevent_cb(fd[1], IOT_WRITE);
	e.selector_list().remove_ioevent_cb(fd[0], IOT_EXCEPTION);
    }
    zzz.unschedule();
    // these should not raise notifications since they have already been removed
    no_notifications_beyond_this_point = true;
    e.selector_list().remove_ioevent_cb(fd[0], IOT_READ);
    e.selector_list().remove_ioevent_cb(fd[1], IOT_WRITE);
#ifndef HOST_OS_WINDOWS
    close(fd[0]);
    close(fd[1]);
#else
    _close(fd[0]);
    _close(fd[1]);
#endif
    
    one_fd_rem_per_each_fd_add_notif =  (add_rem_fd_counter == 0) &&
					(add_rem_mask_counter == 0);
    if (!one_fd_rem_per_each_fd_add_notif) {
	fprintf(stderr, "cumulative fd and mask should both be 0 but are ");
	fprintf(stderr, "fd: %d mask: %d\n", add_rem_fd_counter,
		add_rem_mask_counter);
	exit(1);
    }
    if (!(fd_read_notification_called && fd_write_notification_called &&
	fd_exeption_notification_called &&  fd_removal_notification_called &&
	schedule_notification_called && unschedule_notification_called)) {
	fprintf(stderr, "Some notifications not called correctly\n");
	exit(1);  
    }
    fprintf(stderr, "Test passed\n");
}

#endif // !HOST_OS_WINDOWS

int main(int /* argc */, const char* argv[]) 
{
    // TODO: enable the test for Windows
#ifndef HOST_OS_WINDOWS

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);         // Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    
    xlog_add_default_output();
    xlog_start();

    fprintf(stderr, "------------------------------------------------------\n");
    run_test();
    fprintf(stderr, "------------------------------------------------------\n");

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

#else // HOST_OS_WINDOWS
    UNUSED(argv);
#endif // HOST_OS_WINDOWS
    return 0;
}
