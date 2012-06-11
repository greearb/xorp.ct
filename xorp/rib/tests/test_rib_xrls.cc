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



#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include "libxipc/finder_server.hh"
#include "libxipc/xrl_std_router.hh"

#include "rib_manager.hh"
#include "parser.hh"
#include "parser_direct_cmds.hh"
#include "parser_xrl_cmds.hh"
#include "dummy_register_server.hh"
#include "xrl_target.hh"


bool verbose = false;

class XrlRibParser : public Parser {
public:
    XrlRibParser(EventLoop&	   e,
		 XrlRibV0p1Client& xrl_client,
		 RIB<IPv4>&	   rib,
		 XrlCompletion&    cv) {
	add_command(new XrlRouteAddCommand(e, xrl_client, cv));
	add_command(new XrlRouteDeleteCommand(e, xrl_client, cv));
	add_command(new XrlAddIGPTableCommand(e, xrl_client, cv));
	add_command(new XrlDeleteIGPTableCommand(e, xrl_client, cv));
	add_command(new XrlAddEGPTableCommand(e, xrl_client, cv));
	add_command(new XrlDeleteEGPTableCommand(e, xrl_client, cv));

	// The following do not exist in XRL interface so use direct methods
	add_command(new DirectRouteVifAddCommand(rib));
	add_command(new DirectRouteVerifyCommand(rib));
	add_command(new DirectTableOriginCommand(rib));

	// XXX The following should probably use XRL's but punting for
	// time being.
	add_command(new DirectDiscardVifCommand(rib));
	add_command(new DirectUnreachableVifCommand(rib));
	add_command(new DirectManagementVifCommand(rib));
	add_command(new DirectEtherVifCommand(rib));
	add_command(new DirectLoopbackVifCommand(rib));
    }
};

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

    wait_until_xrl_router_is_ready(eventloop, xrl_std_router_rib);

    rib_manager.start();

    XrlRibV0p1Client xrl_client(&xrl_std_router_rib);

    // Variable used to signal completion of Xrl parse completion
    XrlCompletion cv;
    XrlRibParser parser(eventloop, xrl_client, rib_manager.urib4(), cv);

    string cmd;
    int line = 0;

    while (feof(stdin) == 0) {
	getline(cin, cmd);
	if (feof(stdin) != 0)
	    break;
	line++;
	printf("%d: %s\n", line, cmd.c_str());

	cv = SUCCESS;
	parser.parse(cmd);	// Xrl based commands set cv to XRL_PENDING
				// and return immediately.  Ugly, but hack
				// to make compatible test interface with
				// exist synchronous code.
	while (cv == XRL_PENDING)
	    eventloop.run();
	XLOG_ASSERT(cv == SUCCESS);
    }
}

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

    exit (0);
}
