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

#ident "$XORP: xorp/rib/main_rib.cc,v 1.1.1.1 2002/12/11 23:56:13 hodson Exp $"

#include <sysexits.h>

#include "config.h"

#include "urib_module.h"
#include "rib_manager.hh"

static void
usage(const char* argv0)
{
    const char *progname = strrchr(argv0, '/');
    if (progname) {
	progname++;
    } else {
	progname = argv0;
    }
    fprintf(stderr, "Usage: %s [-F]\n", progname);
    fprintf(stderr, "\t-F\t\t: assume no FEA present\n");
    fprintf(stderr, "\n");
    exit(EX_USAGE);
}

int 
main (int argc, char *argv[]) 
{
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    bool fea_enabled = true;
    int c;
    while ((c = getopt(argc, argv, "F")) != -1) {
	switch (c) {
	case 'F':
	    fea_enabled = false;
	    break;
	default:
	    usage(argv[0]);
	    return -1;
	}
    }
    argc -= optind;
    argv += optind;
    
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    try {
	RibManager rib_manager;
	rib_manager.set_fea_enabled(fea_enabled);
	rib_manager.run_event_loop();
    } catch (...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();
    
    return 0;
}
