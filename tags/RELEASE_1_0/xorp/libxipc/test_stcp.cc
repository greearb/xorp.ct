// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/libxipc/test_stcp.cc,v 1.13 2003/09/17 03:27:21 hodson Exp $"

/*
#define DEBUG_LOGGING
*/

#include "xrl_module.h"

#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "xrl_error.hh"
#include "xrl_pf_stcp.hh"
#include "xrl_router.hh"

static bool g_trace = false;
#define tracef(args...) \
do { \
    if (g_trace) { printf(args) ; fflush(stdout); } \
} while (0)

// ----------------------------------------------------------------------------
// Hello message handlers (zero arguments to Xrl)

static bool hello_done = false;

static const XrlCmdError
hello_recv_handler(const XrlArgs& inputs,
		   XrlArgs*	  outputs)
{
    tracef("hello_recv_handler: inputs %s outputs %p\n",
	   inputs.str().c_str(), outputs);
    return XrlCmdError::OKAY();
}

static void
hello_reply_handler(const XrlError&	e,
		    XrlArgs*		response,
		    Xrl			request)
{
    if (e != XrlError::OKAY()) {
	fprintf(stderr, "hello failed: %s\n", e.str().c_str());
	exit(-1);
    }
    tracef("hello_reply_handler: request %s response %p\n",
	   request.str().c_str(), response);
    hello_done = true;
}

static void
test_hello(EventLoop& e, XrlPFSTCPSender &s)
{
    Xrl x("anywhere", "hello");

    debug_msg("test_hello\n");
    s.send(x, callback(hello_reply_handler, x));
    while (hello_done == false) {
	e.run();
    }
    hello_done = false;
}

// ----------------------------------------------------------------------------
// Hello message handlers (zero arguments to Xrl)

static bool int32_done = false;

static const XrlCmdError
int32_recv_handler(const XrlArgs& inputs,
		   XrlArgs*       outputs)
{
    tracef("int32_recv_handler: inputs %s outputs %p\n",
	   inputs.str().c_str(), outputs);
    if (outputs) {
	outputs->add_int32("an_int32", 123456);
    }
    return XrlCmdError::OKAY();
}

static void
int32_reply_handler(const XrlError& e,
		    XrlArgs*	    response,
		    Xrl		    request)

{
    if (e != XrlError::OKAY()) {
	fprintf(stderr, "get_int32 failed: %s\n", e.str().c_str());
	exit(-1);
    }
    tracef("int32_reply_handler: request %s response %p\n",
	   request.str().c_str(), response);
    tracef("int32 -> %s\n", response->str().c_str());
    int32_done = true;
}

static void
test_int32(EventLoop& e, XrlPFSTCPSender& s)
{
    Xrl x("anywhere", "get_int32");

    debug_msg("test_int32\n");
    s.send(x, callback(int32_reply_handler, x));

    while (int32_done == 0) {
	e.run();
    }
    int32_done = false;
}

static const char* NOISE = "Random arbitrary noise";

static const XrlCmdError
no_execute_recv_handler(const XrlArgs&  /* inputs*/,
			XrlArgs*    	/* outputs */,
			const char* noise)
{
    return XrlCmdError::COMMAND_FAILED(noise);
}

static void
no_execute_reply_handler(const XrlError& e,
			 XrlArgs*	 /* response */,
			 bool*		 done)
{
    if (e != XrlError::COMMAND_FAILED()) {
	fprintf(stderr, "no_execute_handler failed: %s\n", e.str().c_str());
	exit(-1);
    }
    if (e.note() != string(NOISE)) {
	fprintf(stderr, "no_execute_handler failed different reasons:"
		"expected:\t%s\ngot:\t\t%s\n", NOISE, e.note().c_str());
	exit(-1);
    }
    *done = true;
}

static void
test_xrlerror_note(EventLoop&e, XrlPFSTCPListener& l)
{
    Xrl x("anywhere", "no_execute");

    XrlPFSTCPSender s(e, l.address());

    bool done = false;
    s.send(x, callback(no_execute_reply_handler, &done));

    while (done == false) {
	e.run();
    }
}

static bool
print_twirl()
{
    static const char t[] = { '\\', '|', '/', '-' };
    static const size_t nt = sizeof(t) / sizeof(t[0]);
    static size_t n = 0;
    static char erase = '\0';

    printf("%c%c", erase, t[n % nt]); fflush(stdout);
    n++;
    erase = '\b';
    return true;
}

static bool
toggle_flag(bool* flag)
{
    *flag = !(*flag);
    return true;
}

static void
run_test()
{
    static const uint32_t KEEPALIVE_MS = 2500;
    EventLoop eventloop;

    XrlDispatcher cmd_dispatcher("tester");
    cmd_dispatcher.add_handler("hello", callback(hello_recv_handler));
    cmd_dispatcher.add_handler("get_int32", callback(int32_recv_handler));
    cmd_dispatcher.add_handler("no_execute",
			callback(no_execute_recv_handler, NOISE));

    XrlPFSTCPListener listener(eventloop, &cmd_dispatcher);
    XrlPFSTCPSender s(eventloop, listener.address());
    s.set_keepalive_ms(KEEPALIVE_MS);

    tracef("listener address: %s\n", listener.address());

    XorpTimer dp = eventloop.new_periodic(500, callback(&print_twirl));

    bool run_tests = true;
    XorpTimer rt = eventloop.new_periodic(5 * KEEPALIVE_MS / 4,
					  callback(&toggle_flag, &run_tests));

    tracef("Testing XrlPFSTCP\n");
    bool stop = false;
    XorpTimer stop_timer = eventloop.set_flag_after_ms(20 * KEEPALIVE_MS,
						       &stop);

    while (stop == false) {
	assert(s.alive());

	if (run_tests) {
	    test_hello(eventloop, s);
	    test_int32(eventloop, s);
	} else {
	    eventloop.run();
	}
    }
    test_xrlerror_note(eventloop, listener);
}

// ----------------------------------------------------------------------------
// Main

int main(int /* argc */, char *argv[])
{
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    // Set alarm
    alarm(60);

    run_test();

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
