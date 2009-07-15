// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

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



#include "olsr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"
#include "libxorp/service.hh"
#include "libxorp/status_codes.h"
#include "libxorp/eventloop.hh"

#include "libxipc/xrl_std_router.hh"

#include "olsr.hh"
#include "xrl_io.hh"
#include "xrl_target.hh"

static const char TARGET_OLSR[] = "olsr4";   // XRL target name.

int
main(int /*argc*/, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporarily increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_level_set_verbose(XLOG_LEVEL_WARNING, XLOG_VERBOSE_HIGH);
    xlog_level_set_verbose(XLOG_LEVEL_INFO, XLOG_VERBOSE_HIGH);
//     xlog_level_set_verbose(XLOG_LEVEL_TRACE, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    // XXX TODO: implement a command line option which periodically
    // prints OLSR protocol status in a manner near identical to olsrd.

    try {
	EventLoop eventloop;

	string feaname = "fea";
	string ribname = "rib";

	XrlStdRouter xrl_router(eventloop, TARGET_OLSR);

	XrlIO io(eventloop, xrl_router, feaname, ribname);
	Olsr olsr(eventloop, &io);

	XrlOlsr4Target target(&xrl_router, olsr, io);
	wait_until_xrl_router_is_ready(eventloop, xrl_router);
	io.startup();

	while (olsr.running())
	    eventloop.run();
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
