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

#ident "$XORP: xorp/libxorp/test_observers.cc,v 1.1 2003/06/11 19:15:19 jcardona Exp $"

//
// test program to the Observer classes for TimerList and SelectorList
//

#include <stdio.h>

#include "libxorp_module.h"

#include "libxorp/timer.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"

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
static void do_the_twist(int fd, SelectorMask mask) {
    printf("fd: %d mask: %0x are here\n", fd, mask);
}

// Implement the SelectorList observer and notification functions
class mySelectorListObserver : public SelectorListObserverBase {
    void notify_added(int, const SelectorMask&); 
    void notify_removed(int, const SelectorMask&); 
};

void 
mySelectorListObserver::notify_added(int fd, const SelectorMask& mask)
{
    fprintf(stderr, "notif added fd:%d mask:%#0x\n", fd, mask);
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
mySelectorListObserver::notify_removed(int fd, const SelectorMask& mask)
{
    fprintf(stderr, "notif removed fd:%d mask:%#0x\n", fd, mask);
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

    XorpTimer zzz = e.new_periodic(30, callback(print_dot));
    assert(zzz.scheduled());

    int fd[2];
    if (pipe(fd)) {
	fprintf(stderr, "unable to generate file descriptors for test\n");
	exit(2);
    }
    XorpCallback2<void,int,SelectorMask>::RefPtr cb = callback(do_the_twist);
    e.selector_list().add_selector(fd[0], SEL_RD, cb);
    e.selector_list().add_selector(fd[1], SEL_WR, cb);
    e.selector_list().add_selector(fd[0], SEL_EX, cb);

    while(show_stopper.scheduled()) {
	assert(zzz.scheduled());    
	e.run(); // run will return after one or more pending events
		 // have fired.
	e.selector_list().remove_selector(fd[0]);
	e.selector_list().remove_selector(fd[1]);
    }
    zzz.unschedule();
    // these should not raise notifications since they have already been removed
    no_notifications_beyond_this_point = true;
    e.selector_list().remove_selector(fd[0]);
    e.selector_list().remove_selector(fd[1]);
    close(fd[0]); close(fd[1]);
    
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

int main(int /* argc */, const char* argv[]) 
{

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

    return 0;
}
