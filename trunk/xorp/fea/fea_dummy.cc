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

#ident "$XORP: xorp/fea/fea.cc,v 1.3 2003/05/05 21:56:54 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"
#include "libxipc/xrl_std_router.hh"

#if 0	// TODO: XXX: PAVPAVPAV: to be added later
#include "cli/xrl_cli_node.hh"
#endif

#include "fticonfig.hh"
#include "ifmanager.hh"
#include "ifconfig.hh"
#include "xrl_ifupdate.hh"
#if 0	// TODO: XXX: PAVPAVPAV: to be added later
#include "xrl_mfea_node.hh"
#endif
#include "xrl_target.hh"

static const char* xrl_entity = "fea";

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
    
    fprintf(output, "Usage: %s [-f <finder_hostname>]\n",
	    progname);
    fprintf(output, "           -f <finder_hostname>  : finder hostname\n");
    fprintf(output, "           -h                    : usage (this message)\n");
    fprintf(output, "\n");
    fprintf(output, "Program name:   %s\n", progname);
    fprintf(output, "Module name:    %s\n", XORP_MODULE_NAME);
    fprintf(output, "Module version: %s\n", XORP_MODULE_VERSION);
    
    exit (exit_value);
    
    // NOTREACHED
}

static void
fea_main(const char* finder_hostname)
{
    EventLoop eventloop;

    XrlStdRouter xrl_std_router_fea(eventloop, xrl_entity, finder_hostname);

    //
    // Initialize Components
    //

    //
    // 1. FtiConfig
    //
    FtiConfig fticonfig(eventloop);
    fticonfig.set_dummy();
    fticonfig.start();
    
    //
    // 2. Interface Configurator and reporters
    //
    XrlIfConfigUpdateReporter ifreporter(xrl_std_router_fea);
    IfConfigErrorReporter iferr;
    
    IfConfig ifconfig(eventloop, ifreporter, iferr);
    ifconfig.set_dummy();
    ifconfig.start();
    
    //
    // 3. Interface manager
    //
    InterfaceManager ifm(ifconfig);

    //
    // 4. Raw Socket TODO
    //

    //
    // 5. XRL Target
    //
    XrlFeaTarget xrl_tgt(eventloop, xrl_std_router_fea, fticonfig, ifm,
			 ifreporter, 0);
    
#if 0	// TODO: XXX: PAVPAVPAV: to be added later
    //
    // CLI (for debug purpose)
    //
    // XXX: currently, the access to the CLI is either IPv4 or IPv6
    CliNode cli_node4(AF_INET, XORP_MODULE_CLI, eventloop);
    cli_node4.set_cli_port(12000);
    XrlStdRouter xrl_std_router_cli4(eventloop, cli_node4.module_name());
    
    //
    //  MFEA node
    //
    XrlStdRouter xrl_std_router_mfea4(eventloop,
				      xorp_module_name(AF_INET,
						       XORP_MODULE_MFEA));
    XrlMfeaNode xrl_mfea_node4(AF_INET, XORP_MODULE_MFEA, eventloop,
			       &xrl_std_router_mfea4);
    XrlStdRouter xrl_std_router_mfea6(eventloop,
				      xorp_module_name(AF_INET6,
						       XORP_MODULE_MFEA));
    XrlMfeaNode xrl_mfea_node6(AF_INET6, XORP_MODULE_MFEA, eventloop,
			       &xrl_std_router_mfea6);
#endif // 0
    
    //
    // Main loop
    //
    for ( ; ; ) {
	eventloop.run();
    }
}


int
main(int argc, char *argv[])
{
    int c;
    const char *argv0 = argv[0];
    const char *finder_hostname = "localhost";		// XXX: default
    
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
    while ((c = getopt(argc, argv, "f:h")) != -1) {
	switch (c) {
	case 'f':
	    // Finder hostname
	    finder_hostname = optarg;
	    break;
	case 'h':
	case '?':
	    // Help
	    usage(argv0, 0);
	    break;
	default:
	    usage(argv0, 1);
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
    // XXX
    // Do all that daemon stuff take off into the background disable signals
    // and all that other good stuff.
    //

    try {
	fea_main(finder_hostname);
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
