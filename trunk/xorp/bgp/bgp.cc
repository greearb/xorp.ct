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

#ident "$XORP: xorp/bgp/bgp.cc,v 1.8 2003/06/20 21:44:26 atanu Exp $"

#include "config.h"
#include "bgp_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libxorp/eventloop.hh"
#include "libxipc/xrl_std_router.hh"
#include "xrl_target.hh"

BGPMain *bgpmain;

void
terminate_main_loop(int sig)
{
    debug_msg("Signal %d\n", sig);
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
    xlog_level_set_verbose(XLOG_LEVEL_INFO, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    try {
// 	signal(SIGINT, terminate_main_loop);

	BGPMain bgp;
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
	    bgp.eventloop().run();
	}

	/*
	** Check we shouldn't be exiting.
	*/
 	if (!bgp.get_xrl_target()->done())
 	   bgp.main_loop();

	/* XXX
	** Wait for one second before starting to exit.
	** This is a hack to allow any callers of our shutdown XRL to
	** get a response. It would be better if we could poll to
	** discover when our response had been sent.
	*/
	bool wait_one = false;
	XorpTimer t = bgp.eventloop().set_flag_after_ms(1000, &wait_one);
	while ( wait_one == false)
	    bgp.eventloop().run();

    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();
    debug_msg("Bye!\n");
    return 0;
}
