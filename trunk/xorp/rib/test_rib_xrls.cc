// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the Xorp LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the Xorp LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/rib/test_rib_xrls.cc,v 1.9 2003/03/20 00:57:53 pavlin Exp $"

#include "rib_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include "libxipc/finder_server.hh"
#include "libxipc/xrl_std_router.hh"

#include "parser.hh"
#include "parser_direct_cmds.hh"
#include "parser_xrl_cmds.hh"
#include "xrl_target.hh"
#include "rib_client.hh"
#include "dummy_register_server.hh"

#ifdef ORIGINAL_FINDER
typedef FinderServer TestFinderServer;
#else
typedef FinderNGServer TestFinderServer;
#endif

bool verbose = false;

class XrlRibParser : public Parser {
public:
    XrlRibParser(EventLoop&	   e, 
		 XrlRibV0p1Client& xrl_client, 
		 RIB<IPv4>&	   rib,
		 XrlCompletion&    cv) {
	add_command(new XrlRouteAddCommand(e, xrl_client, cv));
	add_command(new XrlRouteDeleteCommand(e, xrl_client, cv));
	add_command(new XrlRedistEnableCommand(e, xrl_client, cv));
	add_command(new XrlRedistDisableCommand(e, xrl_client, cv));
	add_command(new XrlAddIGPTableCommand(e, xrl_client, cv));
	add_command(new XrlDeleteIGPTableCommand(e, xrl_client, cv));
	add_command(new XrlAddEGPTableCommand(e, xrl_client, cv));
	add_command(new XrlDeleteEGPTableCommand(e, xrl_client, cv));

	// The following do not exist in XRL interface so use direct methods
	add_command(new DirectRouteVerifyCommand(rib));
	add_command(new DirectTableOriginCommand(rib));
	add_command(new DirectTableMergedCommand(rib));
	add_command(new DirectTableExtIntCommand(rib));

	// XXX The following should probably use XRL's but punting for
	// time being.
	add_command(new DirectEtherVifCommand(rib));
    }
};

static void
parser_main()
{
    EventLoop event_loop;

    // Finder Server
    TestFinderServer fs(event_loop);

    // Rib Server component
    XrlStdRouter xrl_router(event_loop, "rib");
    RibClient rib_client(xrl_router, "fea");

    // RIB Instantiations for XrlRibTarget
    RIB<IPv4> urib4(UNICAST);
    DummyRegisterServer regserv;
    urib4.initialize_register(&regserv);

    // Instantiated but not used
    RIB<IPv4> mrib4(MULTICAST);
    RIB<IPv6> urib6(UNICAST);
    RIB<IPv6> mrib6(MULTICAST);

    VifManager vifmanager(xrl_router, event_loop, NULL);
    vifmanager.enable();
    vifmanager.start();
    XrlRibTarget xrt(&xrl_router, urib4, mrib4, urib6, mrib6, vifmanager, NULL);

    XrlRibV0p1Client xrl_client(&xrl_router);

    // Variable used to signal completion of Xrl parse completion
    XrlCompletion cv;
    XrlRibParser parser(event_loop, xrl_client, urib4, cv);

    string cmd;
    int line = 0;

    while (feof(stdin) == 0) {
	getline(cin, cmd);
	if (feof(stdin)!=0)
	    break;
	line++;
	cout << line << ": " << cmd << endl;

	cv = SUCCESS;
	parser.parse(cmd);	// Xrl based commands set cv to XRL_PENDING
				// and return immediately.  Ugly, but hack
				// to make compatible test interface with
				// exist synchronous code.
	while (cv == XRL_PENDING)
	    event_loop.run();
	assert (cv == SUCCESS);
    }
}

int
main(int /* argc */, char *argv[])
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
