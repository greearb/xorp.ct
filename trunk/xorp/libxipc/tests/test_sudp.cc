// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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
hello_recv_handler(const XrlArgs& inputs, XrlArgs* outputs)
{
    trace("hello_recv_handler: inputs %s outputs %p\n",
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
    trace("hello_reply_handler: request %s response %p\n",
	  request.str().c_str(), response);
    hello_done = true;
}

static void
test_hello(EventLoop& e, XrlPFSUDPListener &l)
{
    Xrl x("anywhere", "hello");

    XrlPFSUDPSender s(e, l.address());
    s.send(x, false, callback(hello_reply_handler, x));

    while (hello_done == 0) {
	e.run();
    }
}

// ----------------------------------------------------------------------------
// Hello message handlers (zero arguments to Xrl)

static bool int32_done = false;

static const XrlCmdError
int32_recv_handler(const XrlArgs& inputs, XrlArgs* outputs)
{
    trace("int32_recv_handler: inputs %s outputs %p\n",
	   inputs.str().c_str(), outputs);
    if (outputs) {
	outputs->add_int32("an_int32", 123456);
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
    s.send(x, false, callback(int32_reply_handler, x));

    while (int32_done == 0) {
	e.run();
    }
}

static const char* NOISE = "Random arbitrary noise";

static const XrlCmdError
no_execute_recv_handler(const XrlArgs&  /* inputs */,
			XrlArgs*	/* outputs */,
			const char*	noise)
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
    s.send(x, false, callback(no_execute_reply_handler, &done));

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
