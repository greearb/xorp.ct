// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2006 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/test_timer.cc,v 1.15 2006/06/22 00:16:22 pavlin Exp $"

//
// demo program to test tasks and event loops
//

#include <stdio.h>

#include "libxorp_module.h"

#include "libxorp/timer.hh"
#include "libxorp/task.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"



class TestTask {
public:
    TestTask(EventLoop &e)
	: _eventloop(e) {}

    bool test_weights() {
	_counter1 = 0;
	_counter2 = 0;
	_t1 = _eventloop.new_task(callback(this, &TestTask::handler1),
				  DEFAULT_PRIORITY, 1);
	_t2 = _eventloop.new_task(callback(this, &TestTask::handler2),
				  DEFAULT_PRIORITY, 2);
	for (int i = 0; i < 30; i++) {
	    _eventloop.run();
	}
	_t1.unschedule();
	_t2.unschedule();
	assert(!_eventloop.events_pending());
	//printf("%d %d\n", _counter1, _counter2);
	return (_counter1 == 10 && _counter2 == 20);
    }

    bool test_priority1() {
	_counter1 = 0;
	_counter2 = 0;
	_t1 = _eventloop.new_task(callback(this, &TestTask::handler1),
				  3, 1);
	_t2 = _eventloop.new_task(callback(this, &TestTask::handler2),
				  4, 1);
	for (int i = 0; i < 10; i++) {
	    //printf("run\n");
	    _eventloop.run();
	}
	_t1.unschedule();
	_t2.unschedule();
	assert(!_eventloop.events_pending());
	//printf("%d %d\n", _counter1, _counter2);
	return (_counter1 == 10 && _counter2 == 0);
    }

    bool test_priority2() {
	_counter1 = 0;
	_counter2 = 0;
	_t1 = _eventloop.new_task(callback(this, &TestTask::handler1b),
				  3, 1);
	_t2 = _eventloop.new_task(callback(this, &TestTask::handler2),
				  4, 1);
	for (int i = 0; i < 15; i++) {
	    //printf("run\n");
	    _eventloop.run();
	}
	assert(!_t1.scheduled());
	_t2.unschedule();
	assert(!_eventloop.events_pending());
	//printf("%d %d\n", _counter1, _counter2);
	return (_counter1 == 10 && _counter2 == 5);
    }

    bool handler1() {
	//printf("h1\n");
	_counter1++;
	return true;
    }

    bool handler2() {
	//printf("h2\n");
	_counter2++;
	return true;
    }

    bool handler1b() {
	//printf("h1b\n");
	_counter1++;
	if (_counter1 < 10)
	    return true;
	else
	    return false;
    }
private:
    EventLoop& _eventloop;
    XorpTask _t1, _t2;
    int _counter1, _counter2;
};

static void
run_test()
{
    EventLoop e;

    TestTask test1(e);
    if (test1.test_weights()) {
	printf("test weights: PASS\n");
    } else {
	printf("test weights: FAIL\n");
	exit(1);
    }
    if (test1.test_priority1()) {
	printf("test priority 1: PASS\n");
    } else {
	printf("test priority 1: FAIL\n");
	exit(1);
    }
    if (test1.test_priority2()) {
	printf("test priority 2: PASS\n");
    } else {
	printf("test priority 2: FAIL\n");
	exit(1);
    }
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
