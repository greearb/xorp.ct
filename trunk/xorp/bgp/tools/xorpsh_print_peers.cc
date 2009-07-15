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

