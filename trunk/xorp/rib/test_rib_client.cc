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

#ident "$XORP: xorp/rib/test_rib_client.cc,v 1.3 2003/05/29 17:59:10 pavlin Exp $"

#include "rib_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"
#include "libxipc/xrl_std_router.hh"

#include "rib_client.hh"

static bool
send_rib_client_commands(RibClient *rc, int *pcount)
{
    int& count = *pcount;
    
    count++;
    rc->delete_route(IPv4Net("128.16.8.8/16"));
    rc->add_route(IPv4Net("128.16.8.8/16"), IPv4("128.16.8.1"), "if0", "vif0",
		  10, 20, "static");
    cout << "Sending RibClient commands" << endl;
    return true;
}

void
rib_client_test()
{
    EventLoop e;
    XrlStdRouter rtr(e, "test_fea");
    RibClient rc(rtr, "fea");
    int n = 0;

    XorpTimer t = e.new_periodic(1000, callback(&send_rib_client_commands,
						&rc, &n));
    while (n < 10) 
	e.run();
    t.unschedule();
    
    while (rc.tasks_pending()) {
	e.run();
    }
}

int main(int, char *argv[])
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
	rib_client_test();
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
