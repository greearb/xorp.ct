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

#ident "$XORP: xorp/rib/test_deletion.cc,v 1.15 2008/10/02 21:58:13 bms Exp $"

#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include "rib.hh"
#include "rt_tab_origin.hh"
#include "rt_tab_expect.hh"


int
main(int /* argc */, char* argv[])
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

    Vif tmp_vif1("vif1");
    Vif tmp_vif2("vif2");
    RibVif vif1((RIB<IPv4>*)NULL, tmp_vif1);
    RibVif vif2((RIB<IPv4>*)NULL, tmp_vif2);
    IPPeerNextHop<IPv4> nh1(IPv4("1.0.0.1"));
    IPPeerNextHop<IPv4> nh2(IPv4("1.0.0.2"));
    Protocol protocol("test", IGP, 0);
    IPv4Net net1("10.0.1.0/24");
    IPv4Net net2("10.0.2.0/24");

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
    while (dt.parent()->type() != ORIGIN_TABLE) {
	XLOG_ASSERT(dt.parent()->type() == DELETION_TABLE);
	eventloop.run();
    }
    XLOG_ASSERT(dt.expected_route_changes().empty());

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
    while (dt.parent()->type() != ORIGIN_TABLE) {
	XLOG_ASSERT(dt.parent()->type() == DELETION_TABLE);
	eventloop.run();
    }
    XLOG_ASSERT(dt.expected_route_changes().empty());

    XLOG_ASSERT(dt.parent()->type() == ORIGIN_TABLE);

    dt.expect_delete(route3);
    ot.delete_route(net1);

    return 0;
}
