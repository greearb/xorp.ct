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

#ident "$XORP: xorp/bgp/tools/xorpsh_print_routes.cc,v 1.2 2003/03/10 23:20:10 hodson Exp $"

#include "print_routes.hh"
#include "bgp/aspath.hh"
#include "bgp/path_attribute.hh"

void usage()
{
    fprintf(stderr,
	    "Usage: xorpsh_print_routes show bgp routes [summary]\n"
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

    PrintRoutes::detail_t verbose = PrintRoutes::NORMAL;
    int interval = -1;
        if (argc > 5) {
	usage();
	return -1;
    }
    if (argc < 4) {
	usage();
	return -1;
    }
    if (strcmp(argv[1], "show") != 0) {
	usage();
	return -1;
    }
    if (strcmp(argv[2], "bgp") != 0) {
	usage();
	return -1;
    }
    if (strcmp(argv[3], "routes") != 0) {
	usage();
	return -1;
    }
    if (argc == 5) {
	if (strcmp(argv[4], "summary")==0) {
	    verbose = PrintRoutes::SUMMARY;
	} else if (strcmp(argv[4], "detail")==0) {
	    verbose = PrintRoutes::DETAIL;
	} else {
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

