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

#ident "$XORP: xorp/libxipc/test_stcp.cc,v 1.18 2002/12/09 18:29:05 hodson Exp $"

/*
#define DEBUG_LOGGING
*/

#include <stdio.h>
#include "xrl_module.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "xrlpf-stcp.hh"

static bool g_trace = true;
#define tracef(args...) \
do { \
    if (g_trace) { printf(args) ; fflush(stdout); } \
} while (0)

// ----------------------------------------------------------------------------
// Hello message handlers (zero arguments to Xrl)

static bool hello_done = false;

static const XrlCmdError
hello_recv_handler(const Xrl&	request, 
		   XrlArgs*	response, 
		   void*	cookie) {
    tracef("hello_recv_handler: request %s response %p cookie %p\n", 
	   request.str().c_str(), response, cookie);
    return XrlCmdError::OKAY();
}

static void
hello_reply_handler(const XrlError&	e,
		    const Xrl&		request, 
		    XrlArgs*		response, 
		    void*		cookie) {
    if (e != XrlError::OKAY()) {
	fprintf(stderr, "hello failed: %s\n", e.str().c_str());
	exit(-1);
    }
    tracef("hello_reply_handler: request %s response %p cookie %p\n", 
	   request.str().c_str(), response, cookie);
    hello_done = true;
}

static void
test_hello(EventLoop& e, XrlPFSTCPSender &s) {
    Xrl x("anywhere", "hello");

    debug_msg("test_hello\n");
    s.send(x, hello_reply_handler, 0);
    while (hello_done == false) {
	e.run();
    }
    hello_done = false;
}

// ----------------------------------------------------------------------------
// Hello message handlers (zero arguments to Xrl)

static bool int32_done = false;

static const XrlCmdError
int32_recv_handler(const Xrl&	request, 
		   XrlArgs*	response, 
		   void*	cookie) {
    tracef("int32_recv_handler: request %s response %p cookie %p\n", 
	   request.str().c_str(), response, cookie);
    if (response) {
	response->add_int32("an_int32", 123456);
    }
    return XrlCmdError::OKAY();
}

static void
int32_reply_handler(const XrlError&	e, 
		    const Xrl&		request, 
		    XrlArgs*		response, 
		    void*		cookie) {
    if (e != XrlError::OKAY()) {
	fprintf(stderr, "get_int32 failed: %s\n", e.str().c_str());
	exit(-1);
    }
    tracef("int32_reply_handler: request %s response %p cookie %p\n", 
	   request.str().c_str(), response, cookie);
    tracef("int32 -> %s\n", response->str().c_str());
    int32_done = true;
}

static void
test_int32(EventLoop& e, XrlPFSTCPSender& s) {
    Xrl x("anywhere", "get_int32");

    debug_msg("test_int32\n");
    s.send(x, int32_reply_handler, 0);

    while (int32_done == 0) {
	printf(".");
	e.run();
    }
    int32_done = false;
}

static const char* NOISE = "Random arbitrary noise";

static const XrlCmdError
no_execute_recv_handler(const Xrl&  /* request*/,
			XrlArgs*    /* args */,
			const char* noise)
{
    return XrlCmdError::COMMAND_FAILED(noise);
}

static void
no_execute_reply_handler(const XrlError& e,
			 const Xrl&	 /* request */,
			 XrlArgs*	 /* response */,
			 void*		 thunked_done)
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
    bool* done = static_cast<bool*>(thunked_done);
    *done = true;
}

static void
test_xrlerror_note(EventLoop&e, XrlPFSTCPListener& l) 
{
    Xrl x("anywhere", "no_execute");

    XrlPFSTCPSender s(e, l.address());

    bool done = false;
    s.send(x, no_execute_reply_handler, &done);

    while (done == false) {
	e.run();
    }
}

static bool
print_dot()
{
    printf("."); fflush(stdout);
    return true;
}

static void
run_test()
{
    EventLoop event_loop;

    XrlCmdMap cmd_map;
    cmd_map.add_handler("hello", hello_recv_handler, 0);
    cmd_map.add_handler("get_int32", int32_recv_handler, 0);
    cmd_map.add_handler("no_execute", 
			callback(no_execute_recv_handler, NOISE));

    XrlPFSTCPListener listener(event_loop, &cmd_map);
    XrlPFSTCPSender s(event_loop, listener.address());
    s.set_keepalive_ms(2500);

    tracef("listener address: %s\n", listener.address());

    XorpTimer dp = event_loop.new_periodic(500, callback(&print_dot));

    tracef("Testing XrlPFSTCP\n");
    for(int i = 1; i < 50; i++) {
	test_hello(event_loop, s);
	assert(s.alive());

	if ((i % 10) == 0) {
	    // keepalive test
	    bool flag = false;
	    XorpTimer t = event_loop.set_flag_after_ms(4000, &flag);
	    while (flag == false) {
		event_loop.run();
	    }
	}

	test_int32(event_loop, s);
	assert(s.alive());
    }
    test_xrlerror_note(event_loop, listener);
}

// ----------------------------------------------------------------------------
// Main

int main(int /* argc */, char *argv[]) {
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
