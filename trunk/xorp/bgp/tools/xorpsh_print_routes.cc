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

#ident "$XORP: xorp/bgp/tools/xorpsh_print_routes.cc,v 1.6 2004/05/18 06:16:40 atanu Exp $"

#include "print_routes.hh"
#include "bgp/aspath.hh"
#include "bgp/path_attribute.hh"

void usage()
{
    fprintf(stderr,
	    "Usage: [ -l <lines> ] xorpsh_print_routes show bgp routes "
	    "[ipv4|ipv6] [unicast|multicast] [summary|detail]\n"
	    "where summary enables brief output.\n");
}


int main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    bool ipv4, ipv6, unicast, multicast;
    ipv4 = ipv6 = unicast = multicast = false;

    PrintRoutes<IPv4>::detail_t verbose_ipv4 = PrintRoutes<IPv4>::NORMAL;
    PrintRoutes<IPv6>::detail_t verbose_ipv6 = PrintRoutes<IPv6>::NORMAL;
    int interval = -1;
    int lines = -1;

    int c;
    while ((c = getopt(argc, argv, "46uml:")) != -1) {
	switch (c) {
	case '4':
	    ipv4 = true;
	    break;
	case '6':
	    ipv6 = true;
	    break;
	case 'u':
	    unicast = true;
	    break;
	case 'm':
	    multicast = true;
	    break;
	case 'l':
	    lines = atoi(optarg);
	    break;
	default:
	    printf("hello\n");
	    usage();
	    return -1;
	}
    }

    argc -= optind;
    argv += optind;

    if ((argc < 3) || (argc > 6)) {
	usage();
	return -1;
    }

    if (strcmp(argv[0], "show") != 0) {
	usage();
	return -1;
    }
    if (strcmp(argv[1], "bgp") != 0) {
	usage();
	return -1;
    }
    if (strcmp(argv[2], "routes") != 0) {
	usage();
	return -1;
    }
    int counter = 4;
    if (argc > 3) {
	if (strcmp(argv[3], "ipv4") == 0)
	    ipv4 = true;
	if (strcmp(argv[3], "ipv6") == 0)
	    ipv6 = true;
	counter++;
    }
    if (argc > 4) {
	if (strcmp(argv[4], "unicast") == 0)
	    unicast = true;
	if (strcmp(argv[4], "multicast") == 0)
	    multicast = true;
	counter++;
    }
    if (argc == counter) {
	if (strcmp(argv[counter - 1], "summary")==0) {
	    verbose_ipv4 = PrintRoutes<IPv4>::SUMMARY;
	    verbose_ipv6 = PrintRoutes<IPv6>::SUMMARY;
	} else if (strcmp(argv[counter - 1], "detail")==0) {
	    verbose_ipv4 = PrintRoutes<IPv4>::DETAIL;
	    verbose_ipv6 = PrintRoutes<IPv6>::DETAIL;
	} else {
	    usage();
	    return -1;
	}
    }

    if ((ipv4 == false) && (ipv6 == false))
	ipv4 = true;

    if ((unicast == false) && (multicast == false))
	unicast = true;

    try {
	if (ipv4)
	    PrintRoutes<IPv4> route_printer(verbose_ipv4, interval,
					    unicast, multicast, lines);
	if (ipv6)
	    PrintRoutes<IPv6> route_printer(verbose_ipv6, interval,
					    unicast, multicast, lines);
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

