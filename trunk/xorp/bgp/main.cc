// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/bgp/main.cc,v 1.50 2008/07/23 05:09:33 pavlin Exp $"

#include "bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/eventloop.hh"

#include "libcomm/comm_api.h"

#include "libxipc/xrl_std_router.hh"

#include "bgp.hh"
#include "xrl_target.hh"


BGPMain *bgpmain;

void
terminate_main_loop(int sig)
{
    debug_msg("Signal %d\n", sig);
    UNUSED(sig);
    bgpmain->terminate();
}

int
main(int /*argc*/, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_level_set_verbose(XLOG_LEVEL_WARNING, XLOG_VERBOSE_HIGH);
    // Enable verbose tracing via configuration to increase the tracing level
//     xlog_level_set_verbose(XLOG_LEVEL_INFO, XLOG_VERBOSE_HIGH);
//     xlog_level_set_verbose(XLOG_LEVEL_TRACE, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    comm_init();

    try {
	EventLoop eventloop;

// 	signal(SIGINT, terminate_main_loop);

	BGPMain bgp(eventloop);
	bgpmain = &bgp;		// An external reference for the
				// signal handler.

	/*
	** By default assume there is a rib running.
	*/
 	bgp.register_ribname("rib");

	/*
	** Wait for our local configuration information and for the
	** FEA and RIB to start.
	*/
	while (bgp.get_xrl_target()->waiting() && bgp.run()) {
	    eventloop.run();
	}

	/*
	** Check we shouldn't be exiting.
	*/
 	if (!bgp.get_xrl_target()->done())
 	   bgp.main_loop();
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();
    comm_exit();
    debug_msg("Bye!\n");
    return 0;
}
