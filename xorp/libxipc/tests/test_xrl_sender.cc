// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2010 XORP, Inc and Others
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
// Test XRLs sender.
//

#include "xrl_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/xorpfd.hh"
#include "libxorp/callback.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"
#include "libxorp/status_codes.h"
#include "libxorp/profile.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "libxipc/xrl_std_router.hh"
#include "xrl/interfaces/test_xrls_xif.hh"
#include "xrl/targets/test_xrls_base.hh"
#include "test_receiver.hh"
#include "test_xrl_sender.hh"

using namespace SP;

//
// This is a sender program for testing XRL performance. It is used
// as a pair with the "test_xrl_receiver" program.
//
// Usage:
// 1. Start ./xorp_finder
// 2. Start ./test_xrl_receiver
// 3. Start ./test_xrl_sender
//
// Use the definitions below to change the sender's behavior.
//
// Note that it's possible to run both the sender and receiver are in same
// binary, by supply -r as a command-line argument.  If so then there is
// no need to start "./test_xrl_receiver".
//

//
// Definitions for send style.
//
#define SEND_SHORT_XRL                  0
#define SEND_CMDLINE_VAR_XRL            1
#define SEND_LARGE_XRL                  2

//
// Define the size of the string and vector arguments in the long XRLs.
// Note that those are used only if we are sending long XRLs.
//
#define MY_STRING_SIZE			256		// 1024
#define MY_VECTOR_SIZE			1024		// 4096

//
// Define to 1 to enable printing of debug output
//
#define PRINT_DEBUG			0

//
// Define the sending method
//
enum {
    SEND_METHOD_PIPELINE = 0,
    SEND_METHOD_NO_PIPELINE,
    SEND_METHOD_START_PIPELINE,
    SEND_METHOD_SINGLE,
    SEND_METHOD_BATCH
};

#define SEND_METHOD			SEND_METHOD_PIPELINE

//
// Define to 1 to exit after end of transmission
//
#define SEND_DO_EXIT			1
#define RECEIVE_DO_EXIT			1
#define TIMEOUT_EXIT_MS			20000

//
// Global variables
//
static uint32_t    XRL_PIPE_SIZE    = 40;	// Maximum in flight
static uint32_t    g_max_xrls_per_run	    = 10000;
static uint32_t    g_send_method    = SEND_METHOD_PIPELINE;
static uint32_t    g_send_style     = SEND_CMDLINE_VAR_XRL;
static uint32_t    g_atoms_per_xrl  = 1;
static bool        g_run_receiver   = false;
static TestSender* g_test_sender    = NULL;
static int         g_aggressiveness = -1;
static char*	   g_param1         = NULL;
static char*	   g_param2	    = NULL;

static const char* send_methods[] = {
    "pipeline",
    "no pipeline",
    "start pipeline",
    "single",
    "batch"
};

//
// Local functions prototypes
//
static	void usage(const char *argv0, int exit_value);

TestSender::TestSender(EventLoop& eventloop, XrlRouter* xrl_router,
	               size_t max_xrl_id)
	: _eventloop(eventloop),
	  _xrl_router(*xrl_router),
	  _test_xrls_client(xrl_router),
	  _receiver_target("test_xrl_receiver"),
	  _max_xrl_id(max_xrl_id),
	  _next_xrl_send_id(0),
	  _next_xrl_recv_id(0),
	  _sent_end_transmission(false),
	  _done(false)
{
    _my_bool = false;
    _my_int = -100000000;
    _my_ipv4 = IPv4("123.123.123.123");
    _my_ipv4net = IPv4Net("123.123.123.123/32");
    _my_ipv6 = IPv6("1234:5678:90ab:cdef:1234:5678:90ab:cdef");
    _my_ipv6net = IPv6Net("1234:5678:90ab:cdef:1234:5678:90ab:cdef/128");
    _my_mac = Mac("12:34:56:78:90:ab");
    _my_string = string(MY_STRING_SIZE, 'a');
    _my_vector.resize(MY_VECTOR_SIZE);
#if 0
    for (size_t i = 0; i < _my_vector.size(); i++)
	_my_vector[i] = i % 0xff;
#endif
}

TestSender::~TestSender()
{
}

bool
TestSender::done() const
{
    return _done;
}

void
TestSender::print_xrl_sent() const
{
#if PRINT_DEBUG
    if (! (_next_xrl_send_id % 10000))
	printf("Sending %u\n", XORP_UINT_CAST(_next_xrl_send_id));
#endif // PRINT_DEBUG
}

void
TestSender::print_xrl_received() const
{
#if PRINT_DEBUG
    printf(".");
    if (! (_next_xrl_recv_id % 10000))
        printf("Receiving %u\n", XORP_UINT_CAST(_next_xrl_recv_id));
#endif // PRINT_DEBUG
}

void
TestSender::clear()
{
    _next_xrl_send_id = 0;
    _next_xrl_recv_id = 0;
    _sent_end_transmission = false;
    _done = false;
}

void
TestSender::start_transmission()
{
    _start_transmission_timer = _eventloop.new_oneoff_after(
		TimeVal(1, 0),
		callback(this, &TestSender::start_transmission_process));
}

void
TestSender::start_transmission_process()
{
    bool success;
    clear();

    success = _test_xrls_client.send_start_transmission(
	    _receiver_target.c_str(),
	    callback(this, &TestSender::start_transmission_cb));
    if (success != true) {
        printf("start_transmission() failure\n");
        start_transmission();
        return;
    }
}

bool
TestSender::transmit_xrl_next(CB cb)
{
    switch (g_send_style) {
    case SEND_SHORT_XRL:
        return _test_xrls_client.send_add_xrl0(
		    _receiver_target.c_str(),
		    callback(this, cb));

    case SEND_LARGE_XRL:
        return _test_xrls_client.send_add_xrl9(
		    _receiver_target.c_str(),
		    _my_bool,
		    _my_int,
		    _my_ipv4,
		    _my_ipv4net,
		    _my_ipv6,
		    _my_ipv6net,
		    _my_mac,
		    _my_string,
		    _my_vector,
		    callback(this, cb));

    case SEND_CMDLINE_VAR_XRL:
        XrlAtomList xal;
        for (uint32_t i = 1; i < g_atoms_per_xrl; i++)
	   xal.append(XrlAtom(_my_int));

	return _test_xrls_client.send_add_xrlx(
		    _receiver_target.c_str(), xal,
		    callback(this, cb));
    }
    return false;
}

void
TestSender::send_single()
{
    add_sample("start");

    XrlAtomList xal;
    add_sample("XRL Atom List Creation");

    for (uint32_t i = 0; i < g_atoms_per_xrl; i++) {
	XrlAtom atom(_my_int);
	add_sample("XRL Atom creation");

	xal.append(atom);
	add_sample("XRL Atom addition");

    }

    bool rc = _test_xrls_client.send_add_xrlx(_receiver_target.c_str(), xal,
		    callback(this, &TestSender::send_single_cb));

    add_sample("XRL send");
    XLOG_ASSERT(rc);
}

void
TestSender::send_single_cb(const XrlError& xrl_error)
{
    add_sample("XRL send cb");

    XLOG_ASSERT(xrl_error == XrlError::OKAY());

    print_samples();
    end_transmission();
}

void
TestSender::start_transmission_cb(const XrlError& xrl_error)
{
    printf("start_transmission_cb %s\n", xrl_error.str().c_str());
    if (xrl_error != XrlError::OKAY()) {
        XLOG_FATAL("Cannot start transmission: %s",
		   xrl_error.str().c_str());
    }
    // XXX: select pipeline method
    switch (g_send_method) {
    case SEND_METHOD_PIPELINE:
	send_next_pipeline();
	break;

    case SEND_METHOD_START_PIPELINE:
	send_next_start_pipeline();
	break;

    case SEND_METHOD_NO_PIPELINE:
	send_next();
	break;

    case SEND_METHOD_SINGLE:
	send_single();
	break;

    case SEND_METHOD_BATCH:
	abort();
	break;

    default:
	cout << "Unknown send method " << g_send_method << endl;
	abort();
	break;
    }
}

void
TestSender::send_batch()
{
    unsigned int xrls = 100000;

    // XXX get target into finder cache
    bool rc = transmit_xrl_next(&TestSender::send_batch_cb);
    XLOG_ASSERT(rc);
    for (int i = 0; i < 5; i++)
	_eventloop.run();

    // parameters
    _batch_per_run   = 1;
    _batch_size	     = 1;

    if (g_param1)
	_batch_per_run = atoi(g_param1);

    if (g_param2)
	_batch_size = atoi(g_param2);

    printf("Sending %d XRLs per eventloop run, batching each %d XRLs\n",
	   _batch_per_run, _batch_size);

    // own it
    while (1) {
	TimeVal a, b;

	TimerList* t = TimerList::instance();
	t->advance_time();
	t->current_time(a);

	send_batch_do(xrls);

	t->advance_time();
	t->current_time(b);

	TimeVal diff = b - a;

	double speed = xrls;
	speed /= (double) diff.to_ms();
	speed *= 1000.0;

	printf("Sent %u XRLs speed %.0f XRLs/sec\n", xrls, speed);
	if (_batch_errors)
	    printf("Errors: %d\n", _batch_errors);
    }
}

void
TestSender::send_batch_do(unsigned xrls)
{
    _batch_remaining = xrls;
    _batch_sent	     = 0;
    _batch_got	     = 0;
    _batch_errors    = 0;

    _batch_task = _eventloop.new_task(
		    callback(this, &TestSender::send_batch_task),
		    XorpTask::PRIORITY_BACKGROUND, XorpTask::WEIGHT_DEFAULT);

    while (_batch_task.scheduled())
	_eventloop.run();

    while (_batch_got != xrls)
	_eventloop.run();
}

bool
TestSender::send_batch_task()
{
    if (_batch_remaining == 0)
	return false;

    // send only so many per eventloop run
    for (int i = 0; i < _batch_per_run; i++) {
	if (_batch_sent == 0)
	    _xrl_router.batch_start(_receiver_target);

	bool rc = transmit_xrl_next(&TestSender::send_batch_cb);
	if (!rc) {
	    _batch_errors++;
	    break;
	}

	XLOG_ASSERT(rc);
	_batch_remaining--;
	_batch_sent++;

	// batch X amount of XRLs
	if (_batch_sent == _batch_size) {
	    _xrl_router.batch_stop(_receiver_target);
	    _batch_sent = 0;
	}

	if (!_batch_remaining) {
	    if (_batch_sent)
		_xrl_router.batch_stop(_receiver_target);

	    break;
	}
    }

    return true;
}

void
TestSender::send_batch_cb(const XrlError& xrl_error)
{
    XLOG_ASSERT(xrl_error == XrlError::OKAY());

    _batch_got++;
}

void
TestSender::send_next()
{
    bool success;

    if (_next_xrl_send_id >= _max_xrl_id) {
        end_transmission();
        return;
    }

    success = transmit_xrl_next(&TestSender::send_next_cb);
    if (success) {
        print_xrl_sent();
        _next_xrl_send_id++;
        return;
    } else {
        _try_again_timer =
	    _eventloop.new_oneoff_after(
	        TimeVal(0, 1), callback(this, &TestSender::send_next));
    }
#if PRINT_DEBUG
    printf("send_next() failure\n");
#endif
}

void
TestSender::send_next_cb(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	XLOG_FATAL("Cannot end transmission: %s",
	       xrl_error.str().c_str());
    }
    print_xrl_received();
    _next_xrl_recv_id++;
    send_next();
}

void
TestSender::send_next_start_pipeline()
{
    bool success;

    if (_next_xrl_send_id >= _max_xrl_id) {
        end_transmission();
        return;
    }

    success = transmit_xrl_next(&TestSender::send_next_pipeline_cb);
#if PRINT_DEBUG
    printf("send_next_start_pipeline() %s\n", success ? "pass" : "fail");
#endif
    if (success) {
        print_xrl_sent();
        _next_xrl_send_id++;
        return;
    } else {
        _try_again_timer =
		_eventloop.new_oneoff_after(
		    TimeVal(0, 1),
		    callback(this, &TestSender::send_next_start_pipeline));
        return;
    }
#if PRINT_DEBUG
    printf("send_next_start_pipeline() failure\n");
#endif
}

void
TestSender::send_next_pipeline()
{
    bool success;

    size_t i = 0;
    do {
        if (i >= XRL_PIPE_SIZE)
	   break;
	i++;
	if (_next_xrl_send_id >= _max_xrl_id) {
	    end_transmission();
	    return;
	}

	success = transmit_xrl_next(&TestSender::send_next_pipeline_cb);
	if (success) {
	    print_xrl_sent();
	    _next_xrl_send_id++;
	    continue;
	} else {
	    _try_again_timer =
		_eventloop.new_oneoff_after(
		    TimeVal(0, 1),
		    callback(this, &TestSender::send_next_pipeline));
	    return;
	}
    } while (true);
}

void
TestSender::send_next_pipeline_cb(const XrlError& xrl_error)
{
    if (xrl_error != XrlError::OKAY()) {
	//XLOG_FATAL("Cannot end transmission: %s",
	//xrl_error.str().c_str());
    }
    print_xrl_received();
    _next_xrl_recv_id++;

    if (g_send_method != SEND_METHOD_START_PIPELINE) {
	// XXX: don't blast anymore
	send_next();
    } else {
	// XXX: a hack to send the rest of the XRLs after the first one
	send_next_pipeline();
    }
}

void
TestSender::end_transmission()
{
    bool success;

    _try_again_timer.unschedule();

    if (_sent_end_transmission || _done)
	return;

    success = _test_xrls_client.send_end_transmission(
	_receiver_target.c_str(),
	callback(this, &TestSender::end_transmission_cb));
    
    if (success != true) {
	printf("end_transmission() failure\n");
	       _end_transmission_timer = _eventloop.new_oneoff_after(
		    TimeVal(0, 0),
		    callback(this, &TestSender::end_transmission));
	return;
    }
    _sent_end_transmission = true;
}

void
TestSender::end_transmission_cb(const XrlError& xrl_error) {
    if (xrl_error != XrlError::OKAY()) {
	_end_transmission_timer = _eventloop.new_oneoff_after(
	    TimeVal(0, 0),
	    callback(this, &TestSender::end_transmission));
	return;
    //  XLOG_FATAL("Cannot end transmission: %s",
    //             xrl_error.str().c_str());
    }
    start_transmission();
#if SEND_DO_EXIT
    if (_exit_timer.scheduled() == false)
	_exit_timer = _eventloop.set_flag_after_ms(TIMEOUT_EXIT_MS, &_done);
#endif
    return;
}

/**
 * usage:
 * @argv0: Argument 0 when the program was called (the program name itself).
 * @exit_value: The exit value of the program.
 *
 * Print the program usage.
 * If @exit_value is 0, the usage will be printed to the standart output,
 * otherwise to the standart error.
 **/
static void
usage(const char *argv0, int exit_value)
{
    FILE *output;
    const char *progname = strrchr(argv0, '/');

    if (progname != NULL)
	progname++;		// Skip the last '/'
    if (progname == NULL)
	progname = argv0;

    //
    // If the usage is printed because of error, output to stderr, otherwise
    // output to stdout.
    //
    if (exit_value == 0)
	output = stdout;
    else
	output = stderr;

    // Low we persist with this totally unconvential usage message :-D
    fprintf(output, "Usage: %s [-F <finder_hostname>[:<finder_port>]] [options]\n",
	    progname);
    fprintf(output, "           -F <finder_hostname>[:<finder_port>]  : finder hostname and port\n");
    fprintf(output, "           -h                                    : usage (this message)\n");
    fprintf(output, "           -m <M>                                : send method (see below)\n");
    fprintf(output, "           -n <count>                            : number of XrlAtoms in each Xrl call\n");
    fprintf(output, "           -N <count>                            : number of Xrls to send in a test run\n");
    fprintf(output, "           -r                                    : run receiver in same process\n");
    fprintf(output, "           -s <style>  0: short-xrl, 1: var-xrl (default), 2: long-xrl\n");
    fprintf(output, "           -a                                    : eventloop aggressiveness\n");
    fprintf(output, "           -1                                    : parameter 1\n");
    fprintf(output, "           -2                                    : parameter 2\n");

    fprintf(output, "Send methods <M> :\n");
    for (uint32_t i = 0; i < sizeof(send_methods) / sizeof(send_methods[0]); i++)
	fprintf(output, "\t%u - %s\n", i, send_methods[i]);

    fprintf(output, "\n\n");
    fprintf(output, "Program name:   %s\n", progname);
    fprintf(output, "Module name:    %s\n", XORP_MODULE_NAME);
    fprintf(output, "Module version: %s\n", XORP_MODULE_VERSION);

    exit (exit_value);

    // NOTREACHED
}

static void
test_xrls_sender_main(const char* finder_hostname, uint16_t finder_port)
{
    //
    // Init stuff
    //
    EventLoop eventloop;

    if (g_aggressiveness != -1)
	eventloop.set_aggressiveness(g_aggressiveness);

    //
    // Sender
    //
    XrlStdRouter xrl_std_router_test_sender(eventloop, "test_xrl_sender",
					    finder_hostname, finder_port);
    TestSender test_sender(eventloop, &xrl_std_router_test_sender,
			   g_max_xrls_per_run);
    g_test_sender = &test_sender;

    xrl_std_router_test_sender.finalize();
    wait_until_xrl_router_is_ready(eventloop, xrl_std_router_test_sender);

    //
    // Receiver
    //
    XrlStdRouter* xrl_std_router_test_receiver = NULL;
    TestReceiver* test_receiver = NULL;
    if (g_run_receiver == true) {
	xrl_std_router_test_receiver = new XrlStdRouter(eventloop,
							"test_xrl_receiver",
							finder_hostname,
							finder_port);
	test_receiver = new TestReceiver(eventloop,
					 xrl_std_router_test_receiver,
					 stdout);

	if (g_send_method == SEND_METHOD_SINGLE)
	    test_receiver->enable_sampler();

	wait_until_xrl_router_is_ready(eventloop,
				       *xrl_std_router_test_receiver);
    }

    if (g_send_method == SEND_METHOD_BATCH) {
	test_sender.send_batch();
	exit(0);
    }

    //
    // Start transmission
    //
    test_sender.start_transmission();

    //
    // Run everything
    //
    if (g_run_receiver == true) {
	while (! (test_sender.done() && test_receiver->done())) {
	    eventloop.run();
	}
	delete test_receiver;
	delete xrl_std_router_test_receiver;
    } else {
	while (! test_sender.done()) {
	    eventloop.run();
	}
    }
}

int
main(int argc, char *argv[])
{
    int ch;
    string::size_type idx;
    const char *argv0 = argv[0];
    string finder_hostname = FinderConstants::FINDER_DEFAULT_HOST().str();
    uint16_t finder_port = FinderConstants::FINDER_DEFAULT_PORT();

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    //
    // Get the program options
    //
    while ((ch = getopt(argc, argv, "F:hm:n:N:rs:a:1:2:")) != -1) {
	switch (ch) {
	case 'a':
	    g_aggressiveness = atoi(optarg);
	    break;

	case '1':
	    g_param1 = optarg;
	    break;

	case '2':
	    g_param2 = optarg;
	    break;

	case 'F':
	    // Finder hostname and port
	    finder_hostname = optarg;
	    idx = finder_hostname.find(':');
	    if (idx != string::npos) {
		if (idx + 1 >= finder_hostname.length()) {
		    // No port number
		    usage(argv0, 1);
		    // NOTREACHED
		}
		char* p = &finder_hostname[idx + 1];
		finder_port = static_cast<uint16_t>(atoi(p));
		finder_hostname = finder_hostname.substr(0, idx);
	    }
	    break;
	case 'm':
	    g_send_method = atoi(optarg);
	    if (g_send_method >= sizeof(send_methods)/sizeof(send_methods[0]))
		usage(argv0, 1);
		// NOT REACHED
	    break;
	case 'n':
	    g_atoms_per_xrl = atoi(optarg);
	    break;
	case 'N':
	    g_max_xrls_per_run = atoi(optarg);
	    break;
	case 'r':
	    g_run_receiver = true;
	    break;
	case 's':
	    g_send_style = atoi(optarg);
	    break;
	case 'h':
	case '?':
	    // Help
	    usage(argv0, 0);
	    // NOTREACHED
	    break;
	default:
	    usage(argv0, 1);
	    // NOTREACHED
	    break;
	}
    }
    argc -= optind;
    argv += optind;
    if (argc != 0) {
	usage(argv0, 1);
	// NOTREACHED
    }

    fprintf(stdout, "XrlAtoms per call = %u\n", g_atoms_per_xrl);
    fprintf(stdout, "Send method = %s\n", send_methods[g_send_method]);
    fprintf(stdout, "Send style = %i\n", g_send_style);

    //
    // Run everything
    //
    try {
	test_xrls_sender_main(finder_hostname.c_str(), finder_port);
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    exit (0);
}
