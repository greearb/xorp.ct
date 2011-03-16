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



//
// Demo program to test tasks and event loops.
//

#include "libxorp_module.h"
#include "libxorp/xorp.h"

#include "libxorp/timer.hh"
#include "libxorp/task.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"

static bool s_verbose = false;
bool verbose()			{ return s_verbose; }
void set_verbose(bool v)	{ s_verbose = v; }

static int s_failures = 0;
bool failures()			{ return (s_failures)? (true) : (false); }
void incr_failures()		{ s_failures++; }
void reset_failures()		{ s_failures = 0; }

#include "libxorp/xorp_tests.hh"

class TestTask {
public:
    TestTask(EventLoop& e)
	: _eventloop(e) {}

    bool test_weights() {
	_counter1 = 0;
	_counter2 = 0;
	_counter3 = 0;
	_t1 = _eventloop.new_task(callback(this, &TestTask::handler1),
				  XorpTask::PRIORITY_DEFAULT, 1);
	_t2 = _eventloop.new_task(callback(this, &TestTask::handler2),
				  XorpTask::PRIORITY_DEFAULT, 2);
	_t3 = _eventloop.new_oneoff_task(callback(this, &TestTask::handler3),
					 XorpTask::PRIORITY_DEFAULT, 3);
	for (int i = 0; i < 31; i++) {
	    _eventloop.run();
	}
	_t1.unschedule();
	_t2.unschedule();
	XLOG_ASSERT(_t3.scheduled() == false);
	XLOG_ASSERT(_eventloop.events_pending() == false);
	debug_msg("counter1 = %d counter2 = %d counter3 = %d\n",
		  _counter1, _counter2, _counter3);

	// handler1 must be run exactly once
	if (_counter3 != 1 || _counter1 == 0 || _counter2 == 0)
	    return false;

	// handler2 must run twice as frequently as handler1.  There could be an
	// error of at most 2 because handler1 could have run and we could have
	// stopped before getting a chance of running handler2.
	int err = _counter1 * 2 - _counter2;
	if (err > 2 || err < 0)
	    return false;

	// XXX could return true, but lets hardcode values for safety.
	return (_counter1 == 10 && _counter2 == 20 && _counter3 == 1);
    }

    bool test_priority1() {
	_counter1 = 0;
	_counter2 = 0;
	_counter3 = 0;
	_t1 = _eventloop.new_task(callback(this, &TestTask::handler1), 3, 1);
	_t2 = _eventloop.new_task(callback(this, &TestTask::handler2), 4, 1);
	_t3 = _eventloop.new_oneoff_task(callback(this, &TestTask::handler3),
					 5, 1);
	for (int i = 0; i < 11; i++) {
	    _eventloop.run();
	}
	_t1.unschedule();
	_t2.unschedule();
	_t3.unschedule();
	XLOG_ASSERT(_eventloop.events_pending() == false);
	debug_msg("counter1 = %d counter2 = %d counter3 = %d\n",
		  _counter1, _counter2, _counter3);

	// task 3 and 2 must be starved
	if (_counter2 > 0 || _counter3 > 0)
	    return false;

	// task 1 must get all runs
	if (_counter1 == 0)
	    return false;

	// XXX could return true.
	return (_counter1 == 11 && _counter2 == 0 && _counter3 == 0);
    }

    bool test_priority2() {
	_counter1 = 0;
	_counter2 = 0;
	_counter3 = 0;
	_t1 = _eventloop.new_task(callback(this, &TestTask::handler1b), 3, 1);
	_t2 = _eventloop.new_task(callback(this, &TestTask::handler2), 4, 1);
	_t3 = _eventloop.new_oneoff_task(callback(this, &TestTask::handler3),
					 2, 1);
	for (int i = 0; i < 16; i++) {
	    _eventloop.run();
	}
	XLOG_ASSERT(_t1.scheduled() == false);
	_t2.unschedule();
	XLOG_ASSERT(_t3.scheduled() == false);
	XLOG_ASSERT(_eventloop.events_pending() == false);
	debug_msg("counter1 = %d counter2 = %d counter3 = %d\n",
		  _counter1, _counter2, _counter3);

	// task 1 and 3 must complete
	if (_counter1 != 10 || _counter3 != 1)
	    return false;

	// task 2 must get the rest of the cycles
	int aggressiveness = 0; // eventloop's aggressiveness
	int cycles = 16;
	cycles    *= 1 + aggressiveness; // runs per event loop
	cycles    -= _counter1 + _counter3;

	if (_counter2 != cycles)
	    return false;

	// XXX could return true
	return (_counter1 == 10 && _counter2 == 5 && _counter3 == 1);
    }

    bool handler1() {
	_counter1++;
	return true;
    }

    bool handler2() {
	_counter2++;
	return true;
    }

    void handler3() {
	_counter3++;
    }

    bool handler1b() {
	_counter1++;
	if (_counter1 < 10)
	    return true;
	else
	    return false;
    }

private:
    EventLoop& _eventloop;
    XorpTask _t1;
    XorpTask _t2;
    XorpTask _t3;
    int _counter1;
    int _counter2;
    int _counter3;
};

static void
run_test()
{
    EventLoop e;

    TestTask test1(e);
    if (test1.test_weights()) {
	print_passed("test weights");
    } else {
	print_failed("test weights");
	exit(1);
    }
    if (test1.test_priority1()) {
	print_passed("test priority 1");
    } else {
	print_failed("test priority 1");
	exit(1);
    }
    if (test1.test_priority2()) {
	print_passed("test priority 2");
    } else {
	print_failed("test priority 2");
	exit(1);
    }
}

int
main(int /* argc */, const char* argv[])
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
