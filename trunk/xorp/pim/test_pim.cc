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

#ident "$XORP: xorp/pim/test_pim.cc,v 1.18 2003/05/23 00:24:44 pavlin Exp $"


//
// PIM test program.
//


#include "pim_module.h"
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
#include "mfea/xrl_mfea_node.hh"
#include "rib/rib_manager.hh"
#include "mld6igmp/xrl_mld6igmp_node.hh"
#include "pim/xrl_pim_node.hh"


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//

// XXX: set to 1 for IPv4, or set to 0 for IPv6
#define DO_IPV4 1

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
    fprintf(output, "           -f <finder_hostname>[:<finder_port>]  : finder hostname and port\n");
    fprintf(output, "           -h                                    : usage (this message)\n");
    fprintf(output, "\n");
    fprintf(output, "Program name:   %s\n", progname);
    fprintf(output, "Module name:    %s\n", XORP_MODULE_NAME);
    fprintf(output, "Module version: %s\n", XORP_MODULE_VERSION);
    
    exit (exit_value);
    
    // NOTREACHED
}

int
main(int argc, char *argv[])
{
    int ch;
    const char *argv0 = argv[0];
    char *finder_hostname_port = NULL;
    IPv4 finder_addr = IPv4::ZERO();
    uint16_t finder_port = 0;		// XXX: host order
    
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
	    finder_hostname_port = optarg;
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
    // Get the finder hostname and port
    //
    if (finder_hostname_port != NULL) {
	char buf[1024];
	char *p;
	struct hostent *h;
	
	// Get the finder address
	strcpy(buf, finder_hostname_port);
	p = strrchr(buf, ':');
	if (p != NULL)
	    *p = '\0';
	h = gethostbyname(buf);
	if (h == NULL) {
	    fprintf(stderr, "Can't resolve IP address for %s: %s\n",
		    buf, hstrerror(h_errno));
	    usage(argv0, 1);
	}
	
	try {
	    IPvX addr(h->h_addrtype, (uint8_t *)h->h_addr_list[0]);
	    finder_addr = addr.get_ipv4();
	} catch (const InvalidFamily&) {
	    fprintf(stderr, "Invalid finder address family: %d\n",
		    h->h_addrtype);
	    usage(argv0, 1);
	} catch (const InvalidCast&) {
	    fprintf(stderr, "Invalid finder address family: %d (expected IPv4)\n",
		    h->h_addrtype);
	    usage(argv0, 1);
	}
	
	// Get the finder port
	strcpy(buf, finder_hostname_port);
	p = strrchr(buf, ':');
	if (p != NULL) {
	    p++;
	    finder_port = static_cast<uint16_t>(atoi(p));
	    if (finder_port == 0) {
		fprintf(stderr, "Invalid finder port: %d\n", finder_port);
		usage(argv0, 1);
	    }
	}
    }
    
    //
    // The main body
    //
    try {
	//
	// Init stuff
	//
	EventLoop eventloop;
	
	//
	// Finder
	//
	FinderServer *finder = NULL;
	if (finder_hostname_port == NULL) {
	    // Start our own finder
	    try {
		finder = new FinderServer(eventloop);
	    } catch (const InvalidPort&) {
		XLOG_FATAL("Could not start in-process Finder");
	    }
	    finder_addr = finder->addr();
	    finder_port = finder->port();
	}
	
	//
	// CLI
	//
#if DO_IPV4
	CliNode cli_node4(AF_INET, XORP_MODULE_CLI, eventloop);
	cli_node4.set_cli_port(12000);
#else
	CliNode cli_node6(AF_INET6, XORP_MODULE_CLI, eventloop);
	cli_node6.set_cli_port(12000);
#endif // ! DO_IPV4
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
#if DO_IPV4
	XrlStdRouter xrl_std_router_cli4(eventloop, cli_node4.module_name(),
					 finder_addr, finder_port);
	XrlCliNode xrl_cli_node(&xrl_std_router_cli4, cli_node4);
#else
	XrlStdRouter xrl_std_router_cli6(eventloop, cli_node6.module_name(),
					 finder_addr, finder_port);
	XrlCliNode xrl_cli_node(&xrl_std_router_cli6, cli_node6);
#endif // ! DO_IPV4
	
	//
	// MFEA node
	//
#if DO_IPV4
	XrlStdRouter xrl_std_router_mfea4(eventloop,
					  xorp_module_name(AF_INET,
							   XORP_MODULE_MFEA),
					  finder_addr, finder_port);
	XrlMfeaNode xrl_mfea_node4(AF_INET, XORP_MODULE_MFEA,
				   eventloop,
				   &xrl_std_router_mfea4);
#else
	XrlStdRouter xrl_std_router_mfea6(eventloop,
					  xorp_module_name(AF_INET6,
							   XORP_MODULE_MFEA),
					  finder_addr, finder_port);
	XrlMfeaNode xrl_mfea_node6(AF_INET6, XORP_MODULE_MFEA,
				   eventloop,
				   &xrl_std_router_mfea6);
#endif // ! DO_IPV4
	
	//
	// MLD6IGMP node
	//
#if DO_IPV4
	XrlStdRouter xrl_std_router_mld6igmp4(eventloop,
					      xorp_module_name(AF_INET,
							       XORP_MODULE_MLD6IGMP),
					      finder_addr, finder_port);
	XrlMld6igmpNode xrl_mld6igmp_node4(AF_INET, XORP_MODULE_MLD6IGMP,
					   eventloop,
					   &xrl_std_router_mld6igmp4);
#else
	XrlStdRouter xrl_std_router_mld6igmp6(eventloop,
					      xorp_module_name(AF_INET6,
							       XORP_MODULE_MLD6IGMP),
					      finder_addr, finder_port);
	XrlMld6igmpNode xrl_mld6igmp_node6(AF_INET6, XORP_MODULE_MLD6IGMP,
					   eventloop,
					   &xrl_std_router_mld6igmp6);
#endif // ! DO_IPV4
	
	//
	// The RIB manager
	//
#if DO_IPV4
	XrlStdRouter xrl_std_router_rib4(eventloop,
					 xorp_module_name(AF_INET,
							  XORP_MODULE_RIB),
					 finder_addr, finder_port);
	RibManager rib_manager4(eventloop, xrl_std_router_rib4);
	rib_manager4.no_fea();
#else
	XrlStdRouter xrl_std_router_rib6(eventloop,
					 xorp_module_name(AF_INET6,
							  XORP_MODULE_RIB),
					 finder_addr, finder_port);
	RibManager rib_manager6(eventloop, xrl_std_router_rib6);
	rib_manager6.no_fea();
#endif // ! DO_IPV4
	
	//
	// PIMSM node
	//
#if DO_IPV4
	XrlStdRouter xrl_std_router_pimsm4(eventloop,
					   xorp_module_name(AF_INET,
							    XORP_MODULE_PIMSM),
					   finder_addr, finder_port);
	XrlPimNode xrl_pimsm_node4(AF_INET, XORP_MODULE_PIMSM,
				   eventloop,
				   &xrl_std_router_pimsm4);
	// XXX: print the PimMre state dependency (for debug purpose)
	// xrl_pimsm_node4.pim_mrt().track_state_print_actions_name();
	// xrl_pimsm_node4.pim_mrt().track_state_print_actions_num();
	// exit (0);
#else
	XrlStdRouter xrl_std_router_pimsm6(eventloop,
					   xorp_module_name(AF_INET6,
							    XORP_MODULE_PIMSM),
					   finder_addr, finder_port);
	XrlPimNode xrl_pimsm_node6(AF_INET6, XORP_MODULE_PIMSM,
				   eventloop,
				   &xrl_std_router_pimsm6);
#endif // ! DO_IPV4
	
	//
	// Start the nodes
	//
	
	// cli_node4.enable();
	// cli_node4.start();
	//
	// cli_node6.enable();
	// cli_node6.start();
	
	// xrl_mfea_node4.enable_cli();
	// xrl_mfea_node4.start_cli();
	// xrl_mfea_node4.enable_mfea();
	// xrl_mfea_node4.start_mfea();
	// xrl_mfea_node4.enable_all_vifs();
	// xrl_mfea_node4.start_all_vifs();
	//
	// xrl_mfea_node6.enable_cli();
	// xrl_mfea_node6.start_cli();
	// xrl_mfea_node6.enable_mfea();
	// xrl_mfea_node6.start_mfea();
	// xrl_mfea_node6.enable_all_vifs();
	// xrl_mfea_node6.start_all_vifs();
	
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
	
	// xrl_pim_node4.enable_cli();
	// xrl_pim_node4.start_cli();
	// xrl_pim_node4.enable_pim();
	// xrl_pim_node4.start_pim();
	// xrl_pim_node4.enable_all_vifs();
	// xrl_pim_node4.start_all_vifs();
	//
	// xrl_pim_node6.enable_cli();
	// xrl_pim_node6.start_cli();
	// xrl_pim_node6.enable_pim();
	// xrl_pim_node6.start_pim();
	// xrl_pim_node6.enable_all_vifs();
	// xrl_pim_node6.start_all_vifs();
	
	
	//
	// Main loop
	//
	for (;;) {
	    eventloop.run();
	}

	//
	// Stop the nodes
	//
	
	// xrl_pim_node4.stop_all_vifs();
	// xrl_pim_node4.stop_pim();
	// xrl_pim_node4.stop_cli();
	//
	// xrl_pim_node6.stop_all_vifs();
	// xrl_pim_node6.stop_pim();
	// xrl_pim_node6.stop_cli();
	
	// xrl_mld6igmp_node4.stop_all_vifs();
	// xrl_mld6igmp_node4.stop_mld6igmp();
	// xrl_mld6igmp_node4.stop_cli();
	//
	// xrl_mld6igmp_node6.stop_all_vifs();
	// xrl_mld6igmp_node6.stop_mld6igmp();
	// xrl_mld6igmp_node6.stop_cli();
	
	// xrl_mfea_node4.stop_all_vifs();
	// xrl_mfea_node4.stop_mfea();
	// xrl_mfea_node4.stop_cli();
	//
	// xrl_mfea_node6.stop_all_vifs();
	// xrl_mfea_node6.stop_mfea();
	// xrl_mfea_node6.stop_cli();
	
	// cli_node4.stop();
	//
	// cli_node6.stop();
	
	
	if (finder != NULL)
	    delete finder;
	
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
