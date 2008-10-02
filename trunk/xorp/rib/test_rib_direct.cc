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

#ident "$XORP: xorp/rib/test_rib_direct.cc,v 1.25 2008/07/23 05:11:32 pavlin Exp $"

#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/exceptions.hh"
#include "libxorp/eventloop.hh"

#include "libxipc/finder_server.hh"

#include "parser.hh"
#include "parser_direct_cmds.hh"
#include "rib_manager.hh"
#include "rib.hh"
#include "dummy_register_server.hh"


bool verbose = false;

class RibParser : public Parser {
public:
    RibParser(RIB<IPv4>& rib);

private:
    RIB<IPv4>& _rib;
};

RibParser::RibParser(RIB<IPv4>& rib)
    : _rib(rib)
{
    add_command(new DirectDiscardVifCommand(_rib));
    add_command(new DirectUnreachableVifCommand(_rib));
    add_command(new DirectManagementVifCommand(_rib));
    add_command(new DirectEtherVifCommand(_rib));
    add_command(new DirectLoopbackVifCommand(_rib));

    add_command(new DirectRouteAddCommand(_rib));
    add_command(new DirectRouteDeleteCommand(_rib));
    add_command(new DirectRouteVifAddCommand(_rib));

    add_command(new DirectRouteVerifyCommand(_rib));
    add_command(new DirectTableOriginCommand(_rib));

    add_command(new DirectAddIGPTableCommand(_rib));
    add_command(new DirectDeleteIGPTableCommand(_rib));

    add_command(new DirectAddEGPTableCommand(_rib));
    add_command(new DirectDeleteEGPTableCommand(_rib));
}

static void
parser_main()
{
    EventLoop eventloop;

    // Finder Server
    FinderServer fs(eventloop, FinderConstants::FINDER_DEFAULT_HOST(),
		    FinderConstants::FINDER_DEFAULT_PORT());

    // Rib Server component
    XrlStdRouter xrl_std_router_rib(eventloop, "rib", fs.addr(), fs.port());

    //
    // The RIB manager
    //
    RibManager rib_manager(eventloop, xrl_std_router_rib, "fea");
    rib_manager.enable();

    RIB<IPv4> rib(UNICAST, rib_manager, eventloop);
    DummyRegisterServer register_server;

    rib.initialize_register(register_server);

    wait_until_xrl_router_is_ready(eventloop, xrl_std_router_rib);

    RibParser parser(rib);

    string cmd;
    int line = 0;
    while (feof(stdin) == 0) {
	getline(cin, cmd);
	if (feof(stdin) != 0)
	    break;
	line++;
	printf("%d: %s\n", line, cmd.c_str());
	parser.parse(cmd);
    }
}

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
	parser_main();
    } catch (...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    exit(0);
}
