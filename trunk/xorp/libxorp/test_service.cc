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

#ident "$XORP: xorp/devnotes/template.cc,v 1.2 2003/01/16 19:08:48 mjh Exp $"

#include "config.h"

#include <string>

#include "eventloop.hh"
#include "service.hh"

#include "libxorp_module.h"
#include "xlog.h"
#include "exceptions.hh"

//
// This test is fairly straightforward.  We implement two classes
// TestService and TestServiceChangeObserver.  TestService is derived
// from ServiceBase and implements ServiceBase::startup() and
// ServiceBase::shutdown().  With both methods, invocation sets the
// current status to the relevant intermediate state (STARTING_UP or
// SHUTTING_DOWN) and also sets a timer.  When the timer expires it
// sets the current state to the requested state (RUNNING or
// SHUTDOWN).  The timer expiry time is after TRANS_MS milliseconds.
//
// TestServiceChangeObserver assumes it is going to be informed of
// these changes and verifies that the origin and order of changes
// matches those it expects.
//
// The main body of the test is in test_main.  It instantiates and
// associates instances of TestService and TestServiceChangeObserver.
// It uses timers to trigger the calling of TestService::startup() and
// TestService::shutdown().
//

// ----------------------------------------------------------------------------
// Constants

static const uint32_t TRANS_MS = 100;		// Transition time (millisecs)
static const uint32_t EXIT_MS = 5 * TRANS_MS;	// Time to exit (millisecs)

// ----------------------------------------------------------------------------
// Verbose output

static bool s_verbose = false;

inline bool verbose() 		{ return s_verbose; }
inline void set_verbose(bool v)	{ s_verbose = v; }

#define verbose_log(x...) 						      \
do {									      \
    if (verbose()) {							      \
	printf("From %s:%d: ", __FILE__, __LINE__);			      \
	printf(x);							      \
    }									      \
} while(0)


// ----------------------------------------------------------------------------
// TestService implementation

class TestService : public ServiceBase
{
public:
    TestService(EventLoop& e)
	: _e(e)
    {}

    void
    startup()
    {
	set_status(STARTING, "Waiting for timed start event");
	_xt = _e.new_oneoff_after_ms(TRANS_MS,
				     callback(this, &TestService::go_running));

    }

    void
    shutdown()
    {
	set_status(SHUTTING_DOWN, "Waiting for timed shutdown event");
	_xt = _e.new_oneoff_after_ms(TRANS_MS,
				     callback(this, &TestService::go_shutdown));
    }

protected:
    void go_running()
    {
	set_status(RUNNING);
    }

    void go_shutdown()
    {
	set_status(SHUTDOWN);
    }

protected:
    EventLoop&	_e;
    XorpTimer	_xt;	// Timer for simulated status transitions
};


// ----------------------------------------------------------------------------
// TestServiceChangeObserver implementation

class TestServiceChangeObserver : public ServiceChangeObserverBase
{
public:
    TestServiceChangeObserver(const TestService* expected_service)
	: _s(expected_service), _cc(0), _bc(0)
    {
    }

    void
    status_change(ServiceBase*	service,
		  ServiceStatus	old_status,
		  ServiceStatus	new_status)
    {
	if (service != _s) {
	    verbose_log("Wrong service argument\n");
	    _bc++;
	    return;
	}

	ServiceStatus e_old, e_new;
	e_old = e_new = FAILED; // pessimism is...
	switch (_cc++) {
	case 0:
	    // First change expected READY -> STARTING
	    e_old = READY;
	    e_new = STARTING;
	    break;

	case 1:
	    // Second change expected STARTING -> RUNNING
	    e_old = STARTING;
	    e_new = RUNNING;
	    break;

	case 2:
	    // Third change expected RUNNING -> SHUTTING_DOWN
	    e_old = RUNNING;
	    e_new = SHUTTING_DOWN;
	    break;

	case 3:
	    // Fourth change expected SHUTTING_DOWN -> SHUTDOWN
	    e_old = SHUTTING_DOWN;
	    e_new = SHUTDOWN;
	    break;
	default:
	    verbose_log("%d. Too many changes.\n", _cc);
	}

	if (e_old == old_status && e_new == new_status) {
	    verbose_log("%d. Good transition: %s -> %s (%s)\n",
			_cc,
			service_status_name(e_old),
			service_status_name(e_new),
			service->status_note().c_str());
	    return;
	}
	verbose_log("%d. Bad transition: Got %s -> %s (%s) "
		    "Expected %s -> %s\n",
		    _cc,
		    service_status_name(old_status),
		    service_status_name(new_status),
		    service_status_name(e_old),
		    service_status_name(e_new),
		    service->status_note().c_str());
	// Record bad change
	_bc++;
    }

    bool changes_okay() const
    {
	return _cc == 4 && _bc == 0;
    }

protected:
    const ServiceBase*		_s;	// Expected service
    uint32_t	 		_cc;	// Change count
    uint32_t	 		_bc;	// number of bad changes
};


// ----------------------------------------------------------------------------
// Guts of Test

static void
startup_test_service(TestService* ts)
{
    ts->startup();
}

static void
shutdown_test_service(TestService* ts)
{
    ts->shutdown();
}

static int
run_test()
{
    EventLoop e;

    TestService ts(e);
    TestServiceChangeObserver o(&ts);
    ts.set_observer(&o);

    XorpTimer startup = e.new_oneoff_after_ms(TRANS_MS,
					      callback(&startup_test_service,
						       &ts));
    XorpTimer shutdown = e.new_oneoff_after_ms(EXIT_MS,
					       callback(&shutdown_test_service,
							&ts));

    bool timed_out = false;
    XorpTimer timeout = e.set_flag_after_ms(EXIT_MS + 3 * TRANS_MS, &timed_out);

    while (timed_out == false && ts.status() != SHUTDOWN) {
	e.run();
    }

    if (timed_out) {
	verbose_log("Test timed out.\n");
	return -1;
    }

    if (o.changes_okay() == false) {
	verbose_log("Changes not okay.\n");
	return -1;
    }
    return 0;
}

static void
usage(const char* argv0)
{
    fprintf(stderr, "usage: %s [-v]\n", argv0);
    fprintf(stderr, "A test program for XORP service classes\n");
}

int
main(int argc, char* const* argv)
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

    int ch;
    while ((ch = getopt(argc, argv, "hv")) != -1) {
	switch (ch) {
	case 'v':
	    set_verbose(true);
	    break;
	case 'h':
	case '?':
	default:
	    usage(argv[0]);
	    xlog_stop();
	    xlog_exit();
	    return -1;
	}
    }
    argc -= optind;
    argv += optind;

    int r = 0;
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    try {
	r = run_test();
    } catch (...) {
	xorp_catch_standard_exceptions();
    }
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return r;
}
