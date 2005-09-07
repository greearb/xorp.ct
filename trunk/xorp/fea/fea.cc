// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/fea/fea.cc,v 1.50 2005/08/18 15:45:43 bms Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "libxorp/profile.hh"

#include "libcomm/comm_api.h"

#include "libxipc/xrl_std_router.hh"

#include "cli/xrl_cli_node.hh"

#include "fticonfig.hh"
#include "ifmanager.hh"
#include "ifconfig.hh"
#include "ifconfig_addr_table.hh"
#include "libfeaclient_bridge.hh"
#include "nexthop_port_mapper.hh"
#include "xrl_ifupdate.hh"
#include "xrl_mfea_node.hh"
#include "xrl_packet_acl.hh"
#include "xrl_rawsock4.hh"
#include "xrl_rawsock6.hh"
#include "xrl_socket_server.hh"
#include "xrl_target.hh"
#include "profile_vars.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

static const char* xrl_entity = "fea";
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
fea_main(const string& finder_hostname, uint16_t finder_port)
{
    string error_msg;
    EventLoop eventloop;

    comm_init();

    XrlStdRouter xrl_std_router_fea(eventloop,
				    xrl_entity,
				    finder_hostname.c_str(),
				    finder_port);

    //
    // Initialize Components
    //
    //

    //
    // Profile entity
    //
    Profile profile;
    initialize_profiling_variables(profile);

    //
    // Next-hop port mapper
    //
    NexthopPortMapper nexthop_port_mapper;

    //
    // Interface Configurator and reporters
    //
    XrlIfConfigUpdateReporter xrl_ifc_reporter(xrl_std_router_fea);
    LibFeaClientBridge        lfc_bridge(xrl_std_router_fea);

    IfConfigUpdateReplicator ifc_repl;
    ifc_repl.add_reporter(&xrl_ifc_reporter);
    ifc_repl.add_reporter(&lfc_bridge);

    IfConfigErrorReporter if_err;

    IfConfig ifconfig(eventloop, ifc_repl, if_err, nexthop_port_mapper);
    if (is_dummy)
	ifconfig.set_dummy();
    if (ifconfig.start(error_msg) != XORP_OK) {
	XLOG_FATAL("Cannot start IfConfig: %s", error_msg.c_str());
    }

    //
    // Interface manager
    //
    InterfaceManager ifm(ifconfig);

    //
    // Hook IfTree of interface manager into libfeaclient
    // so it can read config to determine deltas
    //
    const IfTree& iftree = ifm.iftree();
    lfc_bridge.set_iftree(&iftree);

    //
    // FtiConfig
    //
    FtiConfig fticonfig(eventloop, profile, ifm.iftree(), nexthop_port_mapper);
    if (is_dummy)
	fticonfig.set_dummy();
    if (fticonfig.start(error_msg) != XORP_OK) {
	XLOG_FATAL("Cannot start FtiConfig: %s", error_msg.c_str());
    }

    //
    // Raw Sockets
    //
    XrlRawSocket4Manager xrsm4(eventloop, iftree, xrl_std_router_fea);
    XrlRawSocket6Manager xrsm6(eventloop, iftree, xrl_std_router_fea);

    //
    // Xrl Socket Server and related components
    //
    IfConfigAddressTable ifc_addr_table(iftree);
    ifc_repl.add_reporter(&ifc_addr_table);
    XrlSocketServer xss(eventloop,
			ifc_addr_table,
			xrl_std_router_fea.finder_address(),
			xrl_std_router_fea.finder_port());
    xss.startup();

    //
    // XRL Target
    //
    XrlFeaTarget xrl_fea_target(eventloop, xrl_std_router_fea,
				fticonfig, ifm, xrl_ifc_reporter,
				profile, &xrsm4, &xrsm6, &lfc_bridge, &xss);
    wait_until_xrl_router_is_ready(eventloop, xrl_std_router_fea);

   //
   // Packet ACL transaction manager and XRL target.
   //
   PaTableManager ptm;
   PaTransactionManager patm(eventloop, ptm);
   XrlStdRouter xrl_std_router_packet_acl(eventloop,
					  "packet_acl",
					  finder_hostname.c_str(),
					  finder_port);
   XrlPacketAclTarget xrl_packet_acl_target(&xrl_std_router_packet_acl,
					    eventloop, patm);
   wait_until_xrl_router_is_ready(eventloop, xrl_std_router_packet_acl);

#ifndef FEA_DUMMY
    //
    // CLI (for debug purpose)
    //
    // XXX: currently, the access to the CLI is either IPv4 or IPv6
    CliNode cli_node4(AF_INET, XORP_MODULE_CLI, eventloop);
    cli_node4.set_cli_port(12000);
    XrlCliNode xrl_cli_node(eventloop,
			    cli_node4.module_name(),
			    finder_hostname,
			    finder_port,
			    "finder",
			    cli_node4);
    wait_until_xrl_router_is_ready(eventloop, xrl_cli_node.xrl_router());

    //
    //  MFEA node
    //
    // XXX: for now we don't have dummy MFEA
    XrlMfeaNode xrl_mfea_node4(AF_INET,
			       XORP_MODULE_MFEA,
			       eventloop,
			       xorp_module_name(AF_INET, XORP_MODULE_MFEA),
			       finder_hostname,
			       finder_port,
			       "finder",
			       xorp_module_name(AF_INET, XORP_MODULE_FEA));
    wait_until_xrl_router_is_ready(eventloop, xrl_mfea_node4.xrl_router());

#ifdef HAVE_IPV6_MULTICAST
    XrlMfeaNode xrl_mfea_node6(AF_INET6,
			       XORP_MODULE_MFEA,
			       eventloop,
			       xorp_module_name(AF_INET6, XORP_MODULE_MFEA),
			       finder_hostname,
			       finder_port,
			       "finder",
			       xorp_module_name(AF_INET6, XORP_MODULE_FEA));
    wait_until_xrl_router_is_ready(eventloop, xrl_mfea_node6.xrl_router());
#endif // HAVE_IPV6_MULTICAST

    //
    // Startup
    //
    // XXX: temporary enable the built-in CLI access
    xrl_cli_node.enable_cli();
    xrl_cli_node.start_cli();
    xrl_mfea_node4.enable_mfea();
    // xrl_mfea_node4.startup();
    xrl_mfea_node4.enable_cli();
    xrl_mfea_node4.start_cli();
#ifdef HAVE_IPV6_MULTICAST
    xrl_mfea_node6.enable_mfea();
    // xrl_mfea_node6.startup();
    xrl_mfea_node6.enable_cli();
    xrl_mfea_node6.start_cli();
#endif
#endif // ! FEA_DUMMY

    //
    // Main loop
    //
    while (true) {
	while (! xrl_fea_target.done()) {
	    eventloop.run();
	}
	//
	// Gracefully stop the FEA
	//
	// TODO: this may not work if we depend on reading asynchronously
	// data from sockets. To fix this, we need to run the eventloop
	// until we get all the data we need. Tricky...
	//
	if (fticonfig.stop(error_msg) != XORP_OK) {
	    XLOG_ERROR("Cannot stop FtiConfig: %s", error_msg.c_str());
	}
	if (ifconfig.stop(error_msg) != XORP_OK) {
	    XLOG_ERROR("Cannot stop IfConfig: %s", error_msg.c_str());
	}

#ifndef FEA_DUMMY
	if (xrl_mfea_node4.MfeaNode::node_status() != PROC_STARTUP) {
	    while (! xrl_mfea_node4.MfeaNode::is_down()) {
		eventloop.run();
	    }
	}
#ifdef HAVE_IPV6_MULTICAST
	if (xrl_mfea_node6.MfeaNode::node_status() != PROC_STARTUP) {
	    while (! xrl_mfea_node6.MfeaNode::is_down()) {
		eventloop.run();
	    }
	}
#endif
#endif // ! FEA_DUMMY

	if (xrl_fea_target.done())
	    break;
    }
    xss.shutdown();

    while (xrl_std_router_fea.pending()
#ifndef FEA_DUMMY
	   || xrl_cli_node.xrl_router().pending()
	   || xrl_mfea_node4.xrl_router().pending()
#ifdef HAVE_IPV6_MULTICAST
	   || xrl_mfea_node6.xrl_router().pending()
#endif
#endif // ! FEA_DUMMY
	) {
	eventloop.run();
    }
}

int
main(int argc, char *argv[])
{
    int ch;
    const char *argv0 = argv[0];
    char finder_hostname[MAXHOSTNAMELEN + 1];
    uint16_t finder_port = FinderConstants::FINDER_DEFAULT_PORT();

    // Default finder hostname
    strncpy(finder_hostname, FinderConstants::FINDER_DEFAULT_HOST().str().c_str(),
	    sizeof(finder_hostname) - 1);
    finder_hostname[sizeof(finder_hostname) - 1] = '\0';

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

    //
    // XXX
    // Do all that daemon stuff take off into the background disable signals
    // and all that other good stuff.
    //

    try {
	fea_main(finder_hostname, finder_port);
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    comm_exit();

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
