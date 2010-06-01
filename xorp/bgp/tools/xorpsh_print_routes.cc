// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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



#include "print_routes.hh"
#include "bgp/aspath.hh"
#include "bgp/path_attribute.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

void usage()
{
    fprintf(stderr,
	    "Usage: [ -l <lines> ] xorpsh_print_routes show bgp routes "
	    "[ipv4|ipv6] [unicast|multicast] [summary|detail] [x.x.x.x/n]\n"
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

    IPNet<IPv4> net_ipv4;
    IPNet<IPv6> net_ipv6;
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

    if ((argc < 3) || (argc > 7)) {
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
	if (strcmp(argv[3], "ipv4") == 0) {
	    ipv4 = true;
	    counter++;
	}
	if (strcmp(argv[3], "ipv6") == 0) {
	    ipv6 = true;
	    counter++;
	}
    }
    if (argc > 4) {
	if (strcmp(argv[4], "unicast") == 0) {
	    unicast = true;
	    counter++;
	}
	if (strcmp(argv[4], "multicast") == 0) {
	    multicast = true;
	    counter++;
	}
    }
    if (argc >= counter) {
	if (strcmp(argv[counter - 1], "summary")==0) {
	    verbose_ipv4 = PrintRoutes<IPv4>::SUMMARY;
	    verbose_ipv6 = PrintRoutes<IPv6>::SUMMARY;
	    counter++;
	} else if (strcmp(argv[counter - 1], "detail")==0) {
	    verbose_ipv4 = PrintRoutes<IPv4>::DETAIL;
	    verbose_ipv6 = PrintRoutes<IPv6>::DETAIL;
	    counter++;
	}
    }
    if (argc == counter) {
	try {
		IPNet<IPv4> subnet(argv[counter - 1]);
		net_ipv4 = subnet;
	} catch(...) {
	    try {
		IPNet<IPv6> subnet(argv[counter - 1]);
		net_ipv6 = subnet;
	    } catch(...) {
		// xorp_catch_standard_exceptions();
		usage();
		return -1;
	    }
	}
    }

    if ((ipv4 == false) && (ipv6 == false))
	ipv4 = true;

    if ((unicast == false) && (multicast == false))
	unicast = true;

    try {
	if (ipv4) {
	    PrintRoutes<IPv4> route_printer(verbose_ipv4, interval, net_ipv4,
					    unicast, multicast, lines);
	}
#ifdef HAVE_IPV6
	if (ipv6) {
	    PrintRoutes<IPv6> route_printer(verbose_ipv6, interval, net_ipv6,
					    unicast, multicast, lines);
	}
#endif
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

