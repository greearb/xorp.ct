// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2010 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"
#include "libxorp/clock.hh"

#include "xrl_fea_node.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

// TODO: XXX: those XRL target names should be defined somewhere else
static const string xrl_fea_targetname = "fea";
static const string xrl_finder_targetname = "finder";

#ifndef FEA_DUMMY
static bool is_dummy = false;
#else
static bool is_dummy = true;
#endif


/**
 * Print the program usage.
 *
 * If @param exit_value is 0, the usage will be printed to the standart
 * output, otherwise to the standart error.
 *
 * @param argv0 argument 0 when the program is called (the program name
 * itself).
 * @param exit_value the exit value of the program.
 */
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
    fprintf(output, "           -h                                    : usage (this message)\n");
    fprintf(output, "\n");
    fprintf(output, "Program name:   %s\n", progname);
    fprintf(output, "Module name:    %s\n", XORP_MODULE_NAME);
    fprintf(output, "Module version: %s\n", XORP_MODULE_VERSION);

    exit (exit_value);

    // NOTREACHED
}

static void
fea_main(const string& finder_hostname, uint16_t finder_port) {

    setup_dflt_sighandlers();

    EventLoop eventloop;
    XrlFeaNode xrl_fea_node(eventloop, xrl_fea_targetname,
			    xrl_finder_targetname, finder_hostname,
			    finder_port, is_dummy);

    // Start operations
    xrl_fea_node.startup();

    //
    // Main loop
    //
    while (xorp_do_run && !xrl_fea_node.is_shutdown_received()) {
	eventloop.run();
    }

    //
    // Shutdown request received. Shutdown all operations and cleanup.
    //
    xrl_fea_node.shutdown();
    while (xrl_fea_node.is_running()) {
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

    // Initialize random number generator.
    SystemClock sc;
    TimeVal tv;
    sc.current_time(tv);
    xorp_srandom(tv.usec());

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    //xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    // XXX: verbosity of the error messages temporary increased
    //xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    //xlog_enable(XLOG_LEVEL_INFO);  Doesn't work?  --Ben
    xlog_add_default_output();
    xlog_start();

    //
    // Get the program options
    //
    while ((ch = getopt(argc, argv, "F:h")) != -1) {
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
	fea_main(finder_hostname, finder_port);
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
