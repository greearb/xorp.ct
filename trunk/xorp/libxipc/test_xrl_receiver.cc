// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/libxipc/test_xrl_receiver.cc,v 1.19 2008/09/23 08:05:17 abittau Exp $"

//
// Test XRLs receiver.
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

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "libxipc/xrl_std_router.hh"
#include "xrl/targets/test_xrls_base.hh"
#include "test_receiver.hh"

//
// This is a receiver program for testing XRL performance. It is used
// as a pair with the "test_xrl_sender" program.
//
// Usage:
// 1. Start ./xorp_finder
// 2. Start ./test_xrl_receiver
// 3. Start ./test_xrl_sender
//
// Use the definitions below to change the receiver's behavior.
//

//
// Define to 1 to enable printing of debug output
//
#define PRINT_DEBUG			0

//
// Define to 1 to exit after end of transmission
//
#define RECEIVE_DO_EXIT			0

// Parameters
static int  g_aggressiveness = -1;

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

    fprintf(output, "Usage: %s [-F <finder_hostname>[:<finder_port>]]\n",
	    progname);
    fprintf(output, "           -F <finder_hostname>[:<finder_port>]  : finder hostname and port\n");
    fprintf(output, "           -a                                    : eventloop aggressiveness\n");
    fprintf(output, "           -h                                    : usage (this message)\n");
    fprintf(output, "\n");
    fprintf(output, "Program name:   %s\n", progname);
    fprintf(output, "Module name:    %s\n", XORP_MODULE_NAME);
    fprintf(output, "Module version: %s\n", XORP_MODULE_VERSION);

    exit (exit_value);

    // NOTREACHED
}

static void
test_xrls_receiver_main(const char* finder_hostname, uint16_t finder_port)
{
    //
    // Init stuff
    //
    EventLoop eventloop;

    if (g_aggressiveness != -1)
	eventloop.set_aggressiveness(g_aggressiveness);

    //
    // Receiver
    //
    XrlStdRouter xrl_std_router_test_receiver(eventloop, "test_xrl_receiver",
					      finder_hostname, finder_port);
    TestReceiver test_receiver(eventloop, &xrl_std_router_test_receiver);
    wait_until_xrl_router_is_ready(eventloop, xrl_std_router_test_receiver);

    //
    // Run everything
    //
    while (! test_receiver.done()) {
	eventloop.run();
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
    while ((ch = getopt(argc, argv, "F:ha:")) != -1) {
	switch (ch) {
	case 'a':
	    g_aggressiveness = atoi(optarg);
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

    //
    // Run everything
    //
    try {
	test_xrls_receiver_main(finder_hostname.c_str(), finder_port);
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
