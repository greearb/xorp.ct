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

#ident "$XORP: xorp/rib/main_rib.cc,v 1.20 2004/05/24 01:22:34 hodson Exp $"

#include <sysexits.h>

#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "rib_manager.hh"

int
main (int /* argc */, char* argv[])
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
	RibManager rib_manager(eventloop, xrl_std_router_rib, "fea");
	rib_manager.enable();

	wait_until_xrl_router_is_ready(eventloop, xrl_std_router_rib);

	// Add the FEA as a RIB client
#if 0	// TODO: change to "#if 1" to switch-back to the old interface
	rib_manager.add_rib_client("fea", AF_INET, true, false);
	rib_manager.add_rib_client("fea", AF_INET6, true, false);
#else
	rib_manager.add_redist_xrl_output4("fea",	/* target_name */
					   "all",	/* from_protocol */
					   true,	/* unicast */
					   false,	/* multicast */
					   "all",	/* cookie */
					   true /* is_xrl_transaction_output */
	    );
	rib_manager.add_redist_xrl_output6("fea",	/* target_name */
					   "all",	/* from_protocol */
					   true,	/* unicast */
					   false,	/* multicast */
					   "all",	/* cookie */
					   true /* is_xrl_transaction_output */
	    );
#endif // 0/1

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
