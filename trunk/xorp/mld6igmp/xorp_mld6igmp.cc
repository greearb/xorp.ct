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

#ident "$XORP: xorp/mld6igmp/xorp_mld6igmp.cc,v 1.1 2003/09/30 16:12:06 pavlin Exp $"


//
// MLD and IGMP program.
//


#include "mld6igmp_module.h"
#include "libxorp/xorp.h"

#include <netdb.h>

#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/callback.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"
#include "libxipc/finder_server.hh"
#include "libxipc/xrl_std_router.hh"
#include "cli/xrl_cli_node.hh"
#include "mld6igmp/xrl_mld6igmp_node.hh"


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//

//
// Local variables
//

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
mld6igmp_main(const char* finder_hostname, uint16_t finder_port)
{
    EventLoop eventloop;

    //
    // CLI (for debug purpose)
    //
    CliNode cli_node4(AF_INET, XORP_MODULE_CLI, eventloop);
    cli_node4.set_cli_port(12000);
#if 0	// XXX: for now we use only one CLI, and it requires IPv4 access to it
    CliNode cli_node6(AF_INET6, XORP_MODULE_CLI, eventloop);
    cli_node6.set_cli_port(12000);
#endif // 0
    //
    // CLI access
    //
    // IPvXNet enable_ipvxnet1("127.0.0.1/32");
    // IPvXNet enable_ipvxnet2("192.150.187.0/25");
    // IPvXNet disable_ipvxnet1("0.0.0.0/0");	// Disable everything else
    //
    // cli_node4.add_enable_cli_access_from_subnet(enable_ipvxnet1);
    // cli_node4.add_enable_cli_access_from_subnet(enable_ipvxnet2);
    // cli_node4.add_disable_cli_access_from_subnet(disable_ipvxnet1);
    //
    // Create and configure the CLI XRL interface
    //
    // IPv4
    XrlStdRouter xrl_std_router_cli4(eventloop, cli_node4.module_name(),
				     finder_hostname, finder_port);
    XrlCliNode xrl_cli_node(&xrl_std_router_cli4, cli_node4);
    {
	// Wait until the XrlRouter becomes ready
	bool timed_out = false;

	XorpTimer t = eventloop.set_flag_after_ms(10000, &timed_out);
	while (xrl_std_router_cli4.ready() == false && timed_out == false) {
	    eventloop.run();
	}

	if (xrl_std_router_cli4.ready() == false) {
	    XLOG_FATAL("XrlRouter did not become ready.  No Finder?");
	}
    }
#if 0	// XXX: for now we use only one CLI, and it requires IPv4 access to it
    XrlStdRouter xrl_std_router_cli6(eventloop, cli_node6.module_name(),
				     finder_hostname, finder_port);
    XrlCliNode xrl_cli_node(&xrl_std_router_cli6, cli_node6);
    {
	// Wait until the XrlRouter becomes ready
	bool timed_out = false;

	XorpTimer t = eventloop.set_flag_after_ms(10000, &timed_out);
	while (xrl_std_router_cli6.ready() == false && timed_out == false) {
	    eventloop.run();
	}

	if (xrl_std_router_cli6.ready() == false) {
	    XLOG_FATAL("XrlRouter did not become ready.  No Finder?");
	}
    }
#endif // 0

    //
    // MLD6IGMP node
    //
    // IPv4 node (IGMP)
    XrlStdRouter xrl_std_router_mld6igmp4(eventloop,
					  xorp_module_name(AF_INET,
							   XORP_MODULE_MLD6IGMP),
					  finder_hostname, finder_port);
    XrlMld6igmpNode xrl_mld6igmp_node4(AF_INET, XORP_MODULE_MLD6IGMP,
				       eventloop, &xrl_std_router_mld6igmp4);
    {
	// Wait until the XrlRouter becomes ready
	bool timed_out = false;

	XorpTimer t = eventloop.set_flag_after_ms(10000, &timed_out);
	while (xrl_std_router_mld6igmp4.ready() == false
	       && timed_out == false) {
	    eventloop.run();
	}

	if (xrl_std_router_mld6igmp4.ready() == false) {
	    XLOG_FATAL("XrlRouter did not become ready.  No Finder?");
	}
    }
    // IPv6 node (MLD)
    XrlStdRouter xrl_std_router_mld6igmp6(eventloop,
					  xorp_module_name(AF_INET6,
							   XORP_MODULE_MLD6IGMP),
					  finder_hostname, finder_port);
    XrlMld6igmpNode xrl_mld6igmp_node6(AF_INET6, XORP_MODULE_MLD6IGMP,
				       eventloop, &xrl_std_router_mld6igmp6);
    {
	// Wait until the XrlRouter becomes ready
	bool timed_out = false;

	XorpTimer t = eventloop.set_flag_after_ms(10000, &timed_out);
	while (xrl_std_router_mld6igmp6.ready() == false
	       && timed_out == false) {
	    eventloop.run();
	}

	if (xrl_std_router_mld6igmp6.ready() == false) {
	    XLOG_FATAL("XrlRouter did not become ready.  No Finder?");
	}
    }

    //
    // Start the nodes
    //
    // cli_node4.enable();
    // cli_node4.start();
    //
    // cli_node6.enable();
    // cli_node6.start();

    // xrl_mld6igmp_node4.enable_cli();
    // xrl_mld6igmp_node4.start_cli();
    // xrl_mld6igmp_node4.enable_mld6igmp();
    // xrl_mld6igmp_node4.start_mld6igmp();
    // xrl_mld6igmp_node4.enable_all_vifs();
    // xrl_mld6igmp_node4.start_all_vifs();
    //
    // xrl_mld6igmp_node6.enable_cli();
    // xrl_mld6igmp_node6.start_cli();
    // xrl_mld6igmp_node6.enable_mld6igmp();
    // xrl_mld6igmp_node6.start_mld6igmp();
    // xrl_mld6igmp_node6.enable_all_vifs();
    // xrl_mld6igmp_node6.start_all_vifs();


    //
    // Main loop
    //
    string reason;
    while (xrl_mld6igmp_node4.Mld6igmpNode::node_status(reason)
	   != PROC_DONE) {
	eventloop.run();
    }

    while (xrl_std_router_mld6igmp4.pending()) {
	eventloop.run();
    }

    //
    // Stop the nodes
    //
    xrl_mld6igmp_node4.stop_mld6igmp();
    xrl_mld6igmp_node4.stop_cli();
    //
    xrl_mld6igmp_node6.stop_mld6igmp();
    xrl_mld6igmp_node6.stop_cli();

    cli_node4.stop();
    //
    // cli_node6.stop();
}

int
main(int argc, char *argv[])
{
    int ch;
    const char *argv0 = argv[0];
    char finder_hostname[MAXHOSTNAMELEN + 1];
    uint16_t finder_port = FINDER_DEFAULT_PORT;	// XXX: default (in host order)

    // Default finder hostname
    strncpy(finder_hostname, FINDER_DEFAULT_HOST.str().c_str(),
	    sizeof(finder_hostname) - 1);
    finder_hostname[sizeof(finder_hostname) - 1] = '\0';

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_level_set_verbose(XLOG_LEVEL_WARNING, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    //
    // Get the program options
    //
    while ((ch = getopt(argc, argv, "F:h")) != -1) {
	switch (ch) {
	case 'F':
	    // Finder hostname and port
	    char *p;
	    strncpy(finder_hostname, optarg, sizeof(finder_hostname) - 1);
	    finder_hostname[sizeof(finder_hostname) - 1] = '\0';
	    p = strrchr(finder_hostname, ':');
	    if (p != NULL)
		*p = '\0';
	    p = strrchr(optarg, ':');
	    if (p != NULL) {
		p++;
		if (*p == '\0') {
		    usage(argv0, 1);
		    // NOTREACHED
		    break;
		}
		finder_port = static_cast<uint16_t>(atoi(p));
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

    try {
	mld6igmp_main(finder_hostname, finder_port);
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
