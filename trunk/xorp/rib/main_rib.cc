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

#ident "$XORP: xorp/rib/main_rib.cc,v 1.14 2003/09/15 18:56:16 atanu Exp $"

#include <sysexits.h>

#include "rib_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "rib_manager.hh"

int 
main (int /* argc */, char *argv[]) 
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

    XorpUnexpectedHandler x(xorp_unexpected_handler);
    try {
	//
	// Init stuff
	//
	EventLoop eventloop;
	XrlStdRouter xrl_std_router_rib(eventloop, "rib");

	//
	// The RIB manager
	//
	RibManager rib_manager(eventloop, xrl_std_router_rib);
	rib_manager.enable();

	{
	    bool timed_out = false;
	    XorpTimer t = eventloop.set_flag_after_ms(10000, &timed_out);
	    while (xrl_std_router_rib.ready() == false && timed_out == false) {
		eventloop.run();
	    }

	    if (xrl_std_router_rib.ready() == false && timed_out) {
		XLOG_FATAL("XrlRouter did not become ready. No Finder?");
	    }
	}

	// Add the FEA as a RIB client
	rib_manager.add_rib_client("fea", AF_INET, true, false);
	rib_manager.add_rib_client("fea", AF_INET6, true, false);
	rib_manager.start();
	
	//
	// Main loop
	//
	string reason;
	while (rib_manager.status(reason) != PROC_SHUTDOWN) {
	    eventloop.run();
	}
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
