// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/ospf/xorp_ospfv3.cc,v 1.15 2007/02/16 22:46:44 pavlin Exp $"

#include "ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/service.hh"
#include "libxorp/status_codes.h"
#include "libxorp/eventloop.hh"

#include <set>

#include "libxipc/xrl_std_router.hh"

#include "ospf.hh"
#include "io.hh"
#include "xrl_io.hh"
#include "xrl_target.hh"


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
	EventLoop eventloop;

	string feaname = "fea";
	string ribname = "rib";

	XrlStdRouter xrl_router(eventloop, TARGET_OSPFv3);

// 	XrlIO<IPv4> io_ipv4(eventloop, xrl_router, feaname, ribname);
	XrlIO<IPv6> io_ipv6(eventloop, xrl_router, feaname, ribname);
//  	Ospf<IPv4> ospf_ipv4(OspfTypes::V3, eventloop, &io_ipv4);
	Ospf<IPv6> ospf_ipv6(OspfTypes::V3, eventloop, &io_ipv6);

	XrlOspfV3Target v3target(&xrl_router, /* ospf_ipv4,*/ ospf_ipv6,
				 /* io_ipv4, */ io_ipv6);
	wait_until_xrl_router_is_ready(eventloop, xrl_router);
// 	io_ipv4.startup();
	io_ipv6.startup();

	while (/*ospf_ipv4.running() && */ospf_ipv6.running())
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
