// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/tools/print_routes.cc,v 1.3 2003/02/09 03:40:19 mjh Exp $"

#include "print_routes.hh"
#include "bgp/aspath.hh"
#include "bgp/path_attribute.hh"

void usage()
{
    fprintf(stderr,
	    "Usage: print_routes [-v] [-i <repeat_interval>]\n"
	    "where -v enables verbose output.\n");
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

    PrintRoutes::detail_t verbose = PrintRoutes::NORMAL;
    int c;
    int interval = -1;
    while ((c = getopt(argc, argv, "i:v")) != -1) {
	switch (c) {
	case 'v':
	    verbose = PrintRoutes::DETAIL;
	    break;
	case 'i':
	    interval = atoi(optarg);
	    break;
	default:
	    usage();
	    return -1;
	}
    }
    try {
	PrintRoutes route_printer(verbose, interval);
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

