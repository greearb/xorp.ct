// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/rib/test_rib_direct.cc,v 1.11 2004/05/06 23:05:24 hodson Exp $"

#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/exceptions.hh"
#include "libxorp/xlog.h"
#include "libxorp/eventloop.hh"

#include "parser.hh"
#include "parser_direct_cmds.hh"
#include "dummy_rib_manager.hh"
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
    add_command(new DirectEtherVifCommand(_rib));
    add_command(new DirectRouteAddCommand(_rib));
    add_command(new DirectRouteDeleteCommand(_rib));

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
    RibManager rib_manager;
    RIB<IPv4> rib(UNICAST, rib_manager, eventloop);
    DummyRegisterServer regserv;

    rib.initialize_register(&regserv);

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
