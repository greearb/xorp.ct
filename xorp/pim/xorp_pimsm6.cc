// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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




//
// XORP PIM-SM module implementation.
//


#include "pim_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/random.h"
#include "libxorp/callback.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"

#include "xrl_pim_node.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

//
// Local functions prototypes
//
static	void usage(const char *argv0, int exit_value);

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
    fprintf(output, "           -h                                    : usage (this message)\n");
    fprintf(output, "\n");
    fprintf(output, "Program name:   %s\n", progname);
    fprintf(output, "Module name:    %s\n", XORP_MODULE_NAME);
    fprintf(output, "Module version: %s\n", XORP_MODULE_VERSION);

    exit (exit_value);

    // NOTREACHED
}

static void
pim_main(const string& finder_hostname, uint16_t finder_port) {
#ifdef HAVE_IPV6

    setup_dflt_sighandlers();

    //
    // Init stuff
    //
    EventLoop eventloop;

    //
    // Initialize the random generator
    //
    {
	TimeVal now;
	TimerList::system_gettimeofday(&now);
	xorp_srandom(now.sec());
    }

    //
    // PIMSM node
    //
    XrlPimNode xrl_pimsm_node6(AF_INET6,
			       XORP_MODULE_PIMSM,
			       eventloop,
			       xorp_module_name(AF_INET6, XORP_MODULE_PIMSM),
			       finder_hostname,
			       finder_port,
			       "finder",
			       xorp_module_name(AF_INET6, XORP_MODULE_FEA),
			       xorp_module_name(AF_INET6, XORP_MODULE_MFEA),
			       xorp_module_name(AF_INET6, XORP_MODULE_RIB),
			       xorp_module_name(AF_INET6, XORP_MODULE_MLD6IGMP));
    wait_until_xrl_router_is_ready(eventloop, xrl_pimsm_node6.xrl_router());

    //
    // Startup
    //
#ifdef HAVE_IPV6_MULTICAST
    xrl_pimsm_node6.enable_pim();
    // xrl_pimsm_node6.startup();
    xrl_pimsm_node6.enable_cli();
    xrl_pimsm_node6.start_cli();
#endif

    //
    // Main loop
    //
#ifdef HAVE_IPV6_MULTICAST
    while (xorp_do_run && !xrl_pimsm_node6.is_done()) {
	eventloop.run();
    }

    while (xrl_pimsm_node6.xrl_router().pending()) {
	eventloop.run();
    }
#endif // HAVE_IPV6_MULTICAST

    // Manually clean up xrl_pim_node.  Without this logic, the destructor
    // will attempt the same, but that will cause some recursive cleanup
    // that ends up calling a pure virtual on the xlr_pim_node logic,
    // which throws an exception from deep in glibc and crashes us.
    xrl_pimsm_node6.delete_all_vifs();
    xrl_pimsm_node6.shutdown();

#endif // HAVE_IPV6

    UNUSED(finder_hostname);
    UNUSED(finder_port);
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
	pim_main(finder_hostname, finder_port);
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
