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

#ident "$XORP: xorp/static_routes/xorp_static_routes.cc,v 1.4 2004/04/22 01:14:29 pavlin Exp $"


//
// XORP MLD/IGMP module implementation.
//


#include "mld6igmp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/callback.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"

#include "libxipc/xrl_std_router.hh"

#include "xrl_mld6igmp_node.hh"


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

//
// Wait until the XrlRouter becomes ready
//
static void
wait_until_xrl_router_is_ready(EventLoop& eventloop, XrlRouter& xrl_router)
{
    bool timed_out = false;

    XorpTimer t = eventloop.set_flag_after_ms(10000, &timed_out);
    while (xrl_router.ready() == false && timed_out == false) {
	eventloop.run();
    }

    if (xrl_router.ready() == false) {
	XLOG_FATAL("XrlRouter did not become ready.  No Finder?");
    }
}

static void
mld6igmp_main(const char* finder_hostname, uint16_t finder_port)
{
    //
    // Init stuff
    //
    EventLoop eventloop;

    //
    // MLD6IGMP node
    //
    XrlStdRouter xrl_std_router_mld6igmp4(eventloop,
					  xorp_module_name(AF_INET,
							   XORP_MODULE_MLD6IGMP),
					  finder_hostname, finder_port);
    XrlMld6igmpNode xrl_mld6igmp_node4(AF_INET,
				       XORP_MODULE_MLD6IGMP,
				       eventloop,
				       &xrl_std_router_mld6igmp4,
				       xorp_module_name(AF_INET,
							XORP_MODULE_MFEA));
    wait_until_xrl_router_is_ready(eventloop, xrl_std_router_mld6igmp4);

#ifdef HAVE_IPV6
    XrlStdRouter xrl_std_router_mld6igmp6(eventloop,
					  xorp_module_name(AF_INET6,
							   XORP_MODULE_MLD6IGMP),
					  finder_hostname, finder_port);
    XrlMld6igmpNode xrl_mld6igmp_node6(AF_INET6,
				       XORP_MODULE_MLD6IGMP,
				       eventloop,
				       &xrl_std_router_mld6igmp6,
				       xorp_module_name(AF_INET6,
							XORP_MODULE_MFEA));
    wait_until_xrl_router_is_ready(eventloop, xrl_std_router_mld6igmp6);
#endif // HAVE_IPV6

    // Startup
    xrl_mld6igmp_node4.startup();
    // xrl_mld6igmp_node6.startup();

    //
    // Main loop
    //
    while (! xrl_mld6igmp_node4.is_done()) {
	eventloop.run();
    }

    while (xrl_std_router_mld6igmp4.pending()) {
	eventloop.run();
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
	mld6igmp_main(finder_hostname.c_str(), finder_port);
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
