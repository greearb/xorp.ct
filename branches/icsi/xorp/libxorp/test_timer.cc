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

#ident "$XORP: xorp/libxorp/test_timer.cc,v 1.5 2002/12/09 18:29:14 hodson Exp $"

//
// demo program to test timers and event loops (and show
// how to use them
//

#include <stdio.h>

#include "libxorp_module.h"

#include "libxorp/timer.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"

int fired = 0 ;
// callback for non-periodic timer. Does not need to return a value
static void some_foo(void*) {
    fired++;
    printf("O"); fflush(stdout);
}

// callback for a periodic timer. If true, the timer is rescheduled.
static bool print_dot(void*) {
    printf("."); fflush(stdout);
    return true;
}

void
test_many()
{
#define N 100

    int i;
    EventLoop e;
    XorpTimer a[N];

    fired = 0 ;
    fprintf(stderr, "++ create a bunch of timers to fire in about 2s\n");
    for (i=0; i<N ; i++) {
	a[i] = e.new_oneoff_after_ms(2110+1*i, some_foo);
    }
    fprintf(stderr, "++ move deadline of 1/3 of them by 5s\n");
    for (i=0; i<N ; i += 3) {
	a[i].reschedule_after_ms(5000) ;
    }
    fprintf(stderr, "++ create 100K timers which never fire because\n"
	"they go out of scope and are automatically deleted\n");
    for (i=0; i<100000 ; i++) {
	XorpTimer b = e.new_oneoff_after_ms(2110+1*i, some_foo);
    }
    fprintf(stderr, "++ wait for the two batches of events at 2 and 5s\n");
    while (e.timers_pending()) {
	fprintf(stdout, "-- fired %d\n", fired);
	fflush(stdout);
	e.run();
    }
    printf("\ndone with test_many\n"); fflush(stdout);
}

void
print_tv(FILE * s, timeval a)
{
    fprintf(s, "%ld.%06ld", a.tv_sec, a.tv_usec);
    fflush(s);
}

void
test_wrap()
{
    timeval a = {INT_MAX, 999998} ;
    timeval one_us = {0, 1} ;
    timeval b = a + one_us;
    timeval c = b + one_us;

    fprintf(stderr, "a is ");print_tv(stderr, a);fprintf(stderr, "\n");
    fprintf(stderr, "b is ");print_tv(stderr, b);fprintf(stderr, "\n");
    fprintf(stderr, "c is ");print_tv(stderr, c);fprintf(stderr, "\n");
    fprintf(stderr, "a < b is %d\n", (int)(a < b));
    fprintf(stderr, "b < c is %d\n", (int)(b < c));
}

void run_test()
{
    EventLoop e;

    test_wrap();

    XorpTimer show_stopper; 
    show_stopper = e.new_oneoff_after_ms(500, some_foo);
    assert(show_stopper.scheduled());

    XorpTimer zzz = e.new_periodic(3, print_dot, 0);
    assert(zzz.scheduled());

    while(show_stopper.scheduled()) {
	assert(zzz.scheduled());    
	e.run(); // run will return after one or more pending events
		 // have fired.
    }
    zzz.unschedule();
    test_many();
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

    run_test();

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
