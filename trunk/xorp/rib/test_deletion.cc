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

#ident "$XORP: xorp/rib/test_deletion.cc,v 1.4 2004/02/06 22:44:12 pavlin Exp $"

#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"

#include "rt_tab_origin.hh"
#include "rt_tab_expect.hh"


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
    EventLoop eventloop;
    OriginTable<IPv4> ot("origin", 1, IGP, eventloop);
    ExpectTable<IPv4> dt("expect", &ot);

    Vif vif1("vif1");
    Vif vif2("vif2");
    IPPeerNextHop<IPv4> nh1(IPv4("1.0.0.1"));
    IPPeerNextHop<IPv4> nh2(IPv4("1.0.0.2"));
    Protocol protocol("test", IGP, 0);
    IPv4Net net1("10.0.1.0/24");
    IPv4Net net2("10.0.2.0/24");
    UNUSED(net2);

    IPRouteEntry<IPv4> route1(net1, &vif1, &nh1, protocol, 100);
    IPRouteEntry<IPv4> route2(net2, &vif1, &nh1, protocol, 100);
    dt.expect_add(route1);
    dt.expect_add(route2);

    ot.add_route(route1);
    ot.add_route(route2);

    dt.expect_delete(route1);
    dt.expect_delete(route2);

    ot.delete_route(net1);
    ot.delete_route(net2);

    printf("-------------------------------------------------------\n");

    // Validate that deletion table does remove the routes and delete itself

    dt.expect_add(route1);
    dt.expect_add(route2);

    ot.add_route(route1);
    ot.add_route(route2);

    dt.expect_delete(route1);
    dt.expect_delete(route2);

    XLOG_ASSERT(dt.parent()->type() == ORIGIN_TABLE);
    ot.routing_protocol_shutdown();

    // Validate that a deletion table got added
    XLOG_ASSERT(dt.parent()->type() == DELETION_TABLE);
    eventloop.run();
    eventloop.run();
    eventloop.run();

    XLOG_ASSERT(dt.parent()->type() == ORIGIN_TABLE);

    printf("-------------------------------------------------------\n");

    //
    // Validate that a routing protocol that comes back up and starts
    // sending routes doesn't cause a problem.
    //

    dt.expect_add(route1);
    dt.expect_add(route2);

    ot.add_route(route1);
    ot.add_route(route2);

    XLOG_ASSERT(dt.parent()->type() == ORIGIN_TABLE);
    ot.routing_protocol_shutdown();

    XLOG_ASSERT(dt.parent()->type() == DELETION_TABLE);
    IPRouteEntry<IPv4> route3(net1, &vif2, &nh2, protocol, 101);
    dt.expect_delete(route1);
    dt.expect_add(route3);

    ot.add_route(route3);

    dt.expect_delete(route2);
    XLOG_ASSERT(dt.parent()->type() == DELETION_TABLE);
    eventloop.run();
    XLOG_ASSERT(dt.parent()->type() == DELETION_TABLE);
    eventloop.run();

    XLOG_ASSERT(dt.parent()->type() == ORIGIN_TABLE);

    dt.expect_delete(route3);
    ot.delete_route(net1);

    return 0;
}
