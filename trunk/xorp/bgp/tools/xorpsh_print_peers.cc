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

#ident "$XORP: xorp/bgp/tools/print_peer.cc,v 1.7 2003/01/25 02:06:53 atanu Exp $"

#include "print_peer.hh"

void usage()
{
    fprintf(stderr,
	    "Usage: xorpsh_print_peers show bgp peers [detail]\n"
	    "where detail enables verbose output.\n");
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

    bool verbose = false;
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
    if (strcmp(argv[3], "peers") != 0) {
	usage();
	return -1;
    }
    if (argc == 5) {
	if (strcmp(argv[4], "detail")==0) {
	    verbose = true;
	} else {
	    usage();
	    return -1;
	}
    }
    try {
	PrintPeers peer_printer(verbose, interval);
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

