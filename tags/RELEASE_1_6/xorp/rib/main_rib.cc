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

#ident "$XORP: xorp/rib/main_rib.cc,v 1.32 2008/10/02 21:58:09 bms Exp $"

#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "rib_manager.hh"

#ifdef HAVE_SYSEXITS_H
#include <sysexits.h>
#endif

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
	rib_manager.add_redist_xrl_output4("fea",	/* target_name */
					   "all",	/* from_protocol */
					   true,	/* unicast */
					   false,	/* multicast */
					   IPv4Net(IPv4::ZERO(), 0), /* network_prefix */
					   "all",	/* cookie */
					   true /* is_xrl_transaction_output */
	    );
	rib_manager.add_redist_xrl_output6("fea",	/* target_name */
					   "all",	/* from_protocol */
					   true,	/* unicast */
					   false,	/* multicast */
					   IPv6Net(IPv6::ZERO(), 0), /* network_prefix */
					   "all",	/* cookie */
					   true /* is_xrl_transaction_output */
	    );

	rib_manager.start();

	//
	// Main loop
	//
	string reason;
	while (rib_manager.status(reason) != PROC_DONE) {
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
