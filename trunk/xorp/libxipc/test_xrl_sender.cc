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

#ident "$XORP: xorp/libxipc/test_xrl_sender.cc,v 1.7 2004/10/13 05:41:48 pavlin Exp $"

//
// Test XRLs sender.
//

#include "xrl_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/callback.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"
#include "libxorp/status_codes.h"

#include "libxipc/xrl_std_router.hh"

#include "xrl/interfaces/test_xrls_xif.hh"
#include "xrl/targets/test_xrls_base.hh"

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
#define SEND_METHOD_PIPELINE		0
#define SEND_METHOD_NO_PIPELINE		1
#define SEND_METHOD_START_PIPELINE	2
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
static uint32_t MAX_XRL_ID	= 10000;	// Number of Xrls in a test run
static uint32_t XRL_PIPE_SIZE	= 40;		// Maximum in flight
static uint32_t g_send_method   = SEND_METHOD_PIPELINE;
static uint32_t g_send_style    = SEND_CMDLINE_VAR_XRL;
static uint32_t g_atoms_per_xrl = 1;
static bool     g_run_receiver	= false;

static const char* send_methods[3] = {
    "pipeline",
    "no pipeline",
    "start pipeline"
};

//
// Local functions prototypes
//
static	void usage(const char *argv0, int exit_value);

class TestSender {
public:
    TestSender(EventLoop& eventloop, XrlRouter* xrl_router,
	       size_t max_xrl_id)
	: _eventloop(eventloop),
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
    ~TestSender() {}

    inline bool done() const { return _done; }

    inline void print_xrl_sent() const {
#if PRINT_DEBUG
	if (! (_next_xrl_send_id % 10000))
	    printf("Sending %u\n", (uint32_t)_next_xrl_send_id);
#endif // PRINT_DEBUG
    }

    inline void print_xrl_received() const {
#if PRINT_DEBUG
	printf(".");
	if (! (_next_xrl_recv_id % 10000))
	    printf("Receiving %u\n", (uint32_t)_next_xrl_recv_id);
#endif // PRINT_DEBUG
    }

    void clear() {
	_next_xrl_send_id = 0;
	_next_xrl_recv_id = 0;
	_sent_end_transmission = false;
	_done = false;
    }

    void
    start_transmission() {
	_start_transmission_timer = _eventloop.new_oneoff_after(
		TimeVal(1, 0),
		callback(this, &TestSender::start_transmission_process));
    }

    void start_transmission_process() {
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

private:

    bool transmit_xrl_next()
    {
	switch (g_send_style) {
	case SEND_SHORT_XRL:
	    return _test_xrls_client.send_add_xrl0(
			_receiver_target.c_str(),
			callback(this, &TestSender::send_next_cb));

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
			callback(this, &TestSender::send_next_cb));

	case SEND_CMDLINE_VAR_XRL:
	    XrlAtomList xal;
	    for (uint32_t i = 1; i < g_atoms_per_xrl; i++) {
		xal.append(XrlAtom(_my_int));
	    }
	    return _test_xrls_client.send_add_xrlx(
	    		_receiver_target.c_str(), xal,
			callback(this, &TestSender::send_next_cb)
			);
	}
	return false;
    }

    bool transmit_xrl_next_pipeline()
    {
	switch (g_send_style) {
	case SEND_SHORT_XRL:
	    return _test_xrls_client.send_add_xrl0(
			_receiver_target.c_str(),
			callback(this, &TestSender::send_next_pipeline_cb));

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
			callback(this, &TestSender::send_next_pipeline_cb));

	case SEND_CMDLINE_VAR_XRL:
	    XrlAtomList xal;
	    for (uint32_t i = 1; i < g_atoms_per_xrl; i++) {
		xal.append(XrlAtom(_my_int));
	    }
	    return _test_xrls_client.send_add_xrlx(
	    		_receiver_target.c_str(), xal,
			callback(this, &TestSender::send_next_pipeline_cb)
			);
	}
	return false;
    }

    void start_transmission_cb(const XrlError& xrl_error) {
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
	}
    }

    void send_next() {
	bool success;

	if (_next_xrl_send_id >= _max_xrl_id) {
	    end_transmission();
	    return;
	}

	success = transmit_xrl_next();
	if (success) {
	    print_xrl_sent();
	    _next_xrl_send_id++;
	    return;
	} else {
	    _try_again_timer =
		_eventloop.new_oneoff_after(
		    TimeVal(0, 1), callback(this, &TestSender::send_next)
		    );
	}
#if PRINT_DEBUG
	printf("send_next() failure\n");
#endif
    }

    void send_next_cb(const XrlError& xrl_error) {
	if (xrl_error != XrlError::OKAY()) {
	    XLOG_FATAL("Cannot end transmission: %s",
		       xrl_error.str().c_str());
	}
	print_xrl_received();
	_next_xrl_recv_id++;
	send_next();
    }

    void send_next_start_pipeline() {
	bool success;

	if (_next_xrl_send_id >= _max_xrl_id) {
	    end_transmission();
	    return;
	}

	success = transmit_xrl_next_pipeline();
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
			callback(this, &TestSender::send_next_start_pipeline)
			);
	    return;
	}
#if PRINT_DEBUG
	printf("send_next_start_pipeline() failure\n");
#endif
    }

    void send_next_pipeline() {
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

	    success = transmit_xrl_next_pipeline();
	    if (success) {
		print_xrl_sent();
		_next_xrl_send_id++;
		continue;
	    } else {
		_try_again_timer =
		    _eventloop.new_oneoff_after(
			TimeVal(0, 1),
			callback(this, &TestSender::send_next_pipeline)
			);
		return;
	    }
	} while (true);
    }

    void send_next_pipeline_cb(const XrlError& xrl_error) {
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

    void end_transmission() {
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

    void end_transmission_cb(const XrlError& xrl_error) {
	if (xrl_error != XrlError::OKAY()) {
	    _end_transmission_timer = _eventloop.new_oneoff_after(
		TimeVal(0, 0),
		callback(this, &TestSender::end_transmission));
	    return;
	    //	    XLOG_FATAL("Cannot end transmission: %s",
	    //		       xrl_error.str().c_str());
	}
	start_transmission();
#if SEND_DO_EXIT
	if (_exit_timer.scheduled() == false)
	    _exit_timer = _eventloop.set_flag_after_ms(TIMEOUT_EXIT_MS,
						       &_done);
#endif
	return;
    }

    EventLoop&			_eventloop;
    XrlTestXrlsV0p1Client	_test_xrls_client;
    string			_receiver_target;

    XorpTimer			_start_transmission_timer;
    XorpTimer			_end_transmission_timer;
    XorpTimer			_try_again_timer;
    XorpTimer			_exit_timer;
    size_t			_max_xrl_id;
    size_t			_next_xrl_send_id;
    size_t			_next_xrl_recv_id;
    bool			_sent_end_transmission;
    bool			_done;

    // Data to send
    bool		_my_bool;
    int32_t		_my_int;
    IPv4		_my_ipv4;
    IPv4Net		_my_ipv4net;
    IPv6		_my_ipv6;
    IPv6Net		_my_ipv6net;
    Mac			_my_mac;
    string		_my_string;
    vector<uint8_t>	_my_vector;
};

class TestReceiver : public XrlTestXrlsTargetBase {
public:
    TestReceiver(EventLoop& eventloop, XrlRouter* xrl_router)
	: XrlTestXrlsTargetBase(xrl_router),
	  _eventloop(eventloop),
	  _received_xrls(0),
	  _done(false)
    {}
    ~TestReceiver() {}

    inline bool done() const { return _done; }

    inline void print_xrl_received() const {
#if PRINT_DEBUG
    	printf(".");
	if (! (_received_xrls % 10000))
	    printf("Received %u\n", (uint32_t)_received_xrls);
#endif // PRINT_DEBUG
    }

private:
    XrlCmdError common_0_1_get_target_name(
	// Output values,
	string&	name) {

	name = "TestReceiver";
	return XrlCmdError::OKAY();
    }

    XrlCmdError common_0_1_get_version(
	// Output values,
	string&	version) {

	version = "0.1";
	return XrlCmdError::OKAY();
    }

    XrlCmdError common_0_1_get_status(
	// Output values,
	uint32_t&	status,
	string&	reason) {
	return XrlCmdError::OKAY();

	reason = "Ready";
	status = PROC_READY;
	return XrlCmdError::OKAY();
    }

    XrlCmdError common_0_1_shutdown() {
	// TODO: XXX: PAVPAVPAV: implement it!
	return XrlCmdError::OKAY();
    }


    XrlCmdError test_xrls_0_1_start_transmission() {
	_received_xrls = 0;
	TimerList::system_gettimeofday(&_start_time);
	return XrlCmdError::OKAY();
    }

    XrlCmdError test_xrls_0_1_end_transmission() {
	TimerList::system_gettimeofday(&_end_time);
	print_statistics();
	return XrlCmdError::OKAY();
    }

    XrlCmdError test_xrls_0_1_add_xrl0() {
	print_xrl_received();
	_received_xrls++;
	return XrlCmdError::OKAY();
    }

    XrlCmdError test_xrls_0_1_add_xrl1(
	// Input values,
	const string&	data1) {
	print_xrl_received();
	_received_xrls++;
	return XrlCmdError::OKAY();
	UNUSED(data1);
    }

    XrlCmdError test_xrls_0_1_add_xrl2(
	// Input values,
	const string&	data1,
	const string&	data2) {
	print_xrl_received();
	_received_xrls++;
	return XrlCmdError::OKAY();
	UNUSED(data1);
	UNUSED(data2);
    }

    XrlCmdError test_xrls_0_1_add_xrl9(
	// Input values,
	const bool&	data1,
	const int32_t&	data2,
	const IPv4&	data3,
	const IPv4Net&	data4,
	const IPv6&	data5,
	const IPv6Net&	data6,
	const Mac&	data7,
	const string&	data8,
	const vector<uint8_t>& data9) {
	print_xrl_received();
	_received_xrls++;
	return XrlCmdError::OKAY();
	UNUSED(data1);
	UNUSED(data2);
	UNUSED(data3);
	UNUSED(data4);
	UNUSED(data5);
	UNUSED(data6);
	UNUSED(data7);
	UNUSED(data8);
	UNUSED(data9);
    }

    XrlCmdError
    test_xrls_0_1_add_xrlx(const XrlAtomList &)
    {
	print_xrl_received();
	_received_xrls++;
	return XrlCmdError::OKAY();
    }

    void print_statistics() {
	TimeVal delta_time = _end_time - _start_time;

	if (_received_xrls == 0) {
	    printf("No XRLs received\n");
	    return;
	}
	if (delta_time == TimeVal::ZERO()) {
	    printf("Received %u XRLs; delta-time = %s secs\n",
		   (uint32_t)_received_xrls, delta_time.str().c_str());
	    return;
	}

	double double_time = delta_time.get_double();
	double speed = _received_xrls;
	speed /= double_time;

	printf("Received %u XRLs; delta_time = %s secs; speed = %f XRLs/s\n",
	       (uint32_t)_received_xrls, delta_time.str().c_str(), speed);

#if RECEIVE_DO_EXIT
	// XXX: if enabled, then exit after all XRLs have been received.
	if (_exit_timer.scheduled() == false)
	    _exit_timer = _eventloop.set_flag_after_ms(TIMEOUT_EXIT_MS,
						       &_done);
#endif

	return;
    }

    EventLoop&	_eventloop;
    TimeVal	_start_time;
    TimeVal	_end_time;
    size_t	_received_xrls;
    XorpTimer	_exit_timer;
    bool	_done;
};

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
    fprintf(output, "           -r                                    : run receiver in same process");

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

    //
    // Sender
    //
    XrlStdRouter xrl_std_router_test_sender(eventloop, "test_xrl_sender",
					    finder_hostname, finder_port);
    TestSender test_sender(eventloop, &xrl_std_router_test_sender,
			   MAX_XRL_ID);
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
					 xrl_std_router_test_receiver);
	wait_until_xrl_router_is_ready(eventloop,
				       *xrl_std_router_test_receiver);
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
    string finder_hostname = FINDER_DEFAULT_HOST.str();
    uint16_t finder_port = FINDER_DEFAULT_PORT;	// XXX: default (in host order)

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
    while ((ch = getopt(argc, argv, "F:hm:n:r")) != -1) {
	switch (ch) {
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
	case 'r':
	    g_run_receiver = true;
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
