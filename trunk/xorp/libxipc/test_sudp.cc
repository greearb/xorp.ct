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

#ident "$XORP: xorp/libxipc/test_sudp.cc,v 1.9 2003/05/09 21:00:52 hodson Exp $"

#include <map>

#include "xrl_module.h"
#include "libxorp/xlog.h"

#include "xrl_error.hh"
#include "xrl_pf_sudp.hh"
#include "xrl_router.hh"

static bool g_trace = false;
#define trace(args...) if (g_trace) printf(args)

// ----------------------------------------------------------------------------
// Hello message handlers (zero arguments to Xrl)

static bool hello_done = false;

static const XrlCmdError
hello_recv_handler(const Xrl&	request,
		   XrlArgs*	response)
{
    trace("hello_recv_handler: request %s response %p\n",
	  request.str().c_str(), response);
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
    trace("hello_reply_handler: request %s response %p\n",
	  request.str().c_str(), response);
    hello_done = true;
}

static void
test_hello(EventLoop& e, XrlPFSUDPListener &l)
{
    Xrl x("anywhere", "hello");

    XrlPFSUDPSender s(e, l.address());
    s.send(x, callback(hello_reply_handler, x));

    while (hello_done == 0) {
	e.run();
    }
}

// ----------------------------------------------------------------------------
// Hello message handlers (zero arguments to Xrl)

static bool int32_done = false;

static const XrlCmdError
int32_recv_handler(const Xrl&	request,
		   XrlArgs*	response)
{
    trace("int32_recv_handler: request %s response %p\n",
	   request.str().c_str(), response);
    if (response) {
	response->add_int32("an_int32", 123456);
    }
    return XrlCmdError::OKAY();
}

static void
int32_reply_handler(const XrlError&	e,
		    XrlArgs*		response,
		    Xrl			request)
{
    if (e != XrlError::OKAY()) {
	fprintf(stderr, "get_int32 failed: %s\n", e.str().c_str());
	exit(-1);
    }
    trace("int32_reply_handler: request %s response %p\n",
	  request.str().c_str(), response);
    trace("int32 -> %s\n", response->str().c_str());
    int32_done = true;
}

static void
test_int32(EventLoop& e, XrlPFSUDPListener& l)
{
    Xrl x("anywhere", "get_int32");

    XrlPFSUDPSender s(e, l.address());
    s.send(x, callback(int32_reply_handler, x));

    while (int32_done == 0) {
	e.run();
    }
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
test_xrlerror_note(EventLoop&e, XrlPFSUDPListener& l)
{
    Xrl x("anywhere", "no_execute");

    XrlPFSUDPSender s(e, l.address());

    bool done = false;
    s.send(x, callback(no_execute_reply_handler, &done));

    while (done == false) {
	e.run();
    }
}

static void
run_test()
{
    EventLoop eventloop;

    XrlDispatcher cmd_dispatcher("tester");
    cmd_dispatcher.add_handler("hello", callback(hello_recv_handler));
    cmd_dispatcher.add_handler("get_int32", callback(int32_recv_handler));
    cmd_dispatcher.add_handler("no_execute",
			callback(no_execute_recv_handler, NOISE));

    XrlPFSUDPListener listener(eventloop, &cmd_dispatcher);

    trace("listener address: %s\n", listener.address());

    test_hello(eventloop, listener);
    test_int32(eventloop, listener);
    test_xrlerror_note(eventloop, listener);
}

// ----------------------------------------------------------------------------
// Main

int main(int /* argc */, char* argv[])
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

    run_test();

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
