// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/test_service.cc,v 1.14 2007/10/13 01:50:03 pavlin Exp $"

#include "libxorp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/eventloop.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "service.hh"


//
// This test is fairly straightforward.  We implement two classes
// TestService and TestServiceChangeObserver.  TestService is derived
// from ServiceBase and implements ServiceBase::startup() and
// ServiceBase::shutdown().  With both methods, invocation sets the
// current status to the relevant intermediate state (SERVICE_STARTING or
// SERVICE_SHUTTING_DOWN) and also sets a timer.  When the timer expires it
// sets the current state to the requested state (SERVICE_RUNNING or
// SERVICE_SHUTDOWN).  The timer expiry time is after TRANS_MS milliseconds.
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

bool verbose()			{ return s_verbose; }
void set_verbose(bool v)	{ s_verbose = v; }

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

    int
    startup()
    {
	set_status(SERVICE_STARTING, "Waiting for timed start event");
	_xt = _e.new_oneoff_after_ms(TRANS_MS,
				     callback(this, &TestService::go_running));
	return (XORP_OK);
    }

    int
    shutdown()
    {
	set_status(SERVICE_SHUTTING_DOWN, "Waiting for timed shutdown event");
	_xt = _e.new_oneoff_after_ms(TRANS_MS,
				     callback(this, &TestService::go_shutdown));
	return (XORP_OK);
    }

protected:
    void go_running()
    {
	set_status(SERVICE_RUNNING);
    }

    void go_shutdown()
    {
	set_status(SERVICE_SHUTDOWN);
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
	e_old = e_new = SERVICE_FAILED; // pessimism is...
	switch (_cc++) {
	case 0:
	    // First change expected SERVICE_READY -> SERVICE_STARTING
	    e_old = SERVICE_READY;
	    e_new = SERVICE_STARTING;
	    break;

	case 1:
	    // Second change expected SERVICE_STARTING -> SERVICE_RUNNING
	    e_old = SERVICE_STARTING;
	    e_new = SERVICE_RUNNING;
	    break;

	case 2:
	    // Third change expected SERVICE_RUNNING -> SERVICE_SHUTTING_DOWN
	    e_old = SERVICE_RUNNING;
	    e_new = SERVICE_SHUTTING_DOWN;
	    break;

	case 3:
	    // Fourth change expected SERVICE_SHUTTING_DOWN -> SERVICE_SHUTDOWN
	    e_old = SERVICE_SHUTTING_DOWN;
	    e_new = SERVICE_SHUTDOWN;
	    break;
	default:
	    verbose_log("%u. Too many changes.\n", XORP_UINT_CAST(_cc));
	}

	if (e_old == old_status && e_new == new_status) {
	    verbose_log("%u. Good transition: %s -> %s (%s)\n",
			XORP_UINT_CAST(_cc),
			service_status_name(e_old),
			service_status_name(e_new),
			service->status_note().c_str());
	    return;
	}
	verbose_log("%u. Bad transition: Got %s -> %s (%s) "
		    "Expected %s -> %s\n",
		    XORP_UINT_CAST(_cc),
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

    while (timed_out == false && ts.status() != SERVICE_SHUTDOWN) {
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
