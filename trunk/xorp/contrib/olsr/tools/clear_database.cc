// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2007-2008 XORP, Inc.
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

#ident "$XORP: xorp/contrib/olsr/tools/clear_database.cc,v 1.1 2008/04/24 15:20:02 bms Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "olsr/olsr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"
#include "libxorp/service.hh"
#include "libxorp/status_codes.h"
#include "libxorp/eventloop.hh"
#include "libxorp/tlv.hh"

#include <set>
#include <list>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "libxipc/xrl_std_router.hh"

#include "xrl/interfaces/olsr4_xif.hh"

#include "olsr/olsr.hh"
#include "olsr/test_common.hh"

class ClearDatabase {
public:
    ClearDatabase(XrlStdRouter& xrl_router)
	: _xrl_router(xrl_router),
	_done(false), _fail(false)
    {
    }

    void start() {
	XrlOlsr4V0p1Client olsr4(&_xrl_router);
	olsr4.send_clear_database("olsr4",
				  callback(this, &ClearDatabase::response));
    }

    bool busy() {
	return !_done;
    }

    bool fail() {
	return _fail;
    }

private:
    void response(const XrlError& error) {
	_done = true;
	if (XrlError::OKAY() != error) {
		XLOG_WARNING("Attempt to clear database");
		_fail = true;
		return;
	}
    }
private:
    XrlStdRouter &_xrl_router;
    bool _done;
    bool _fail;
};

int
usage(const char *myname)
{
    fprintf(stderr, "usage: %s\n", myname);

    return -1;
}

int 
main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    try {
	EventLoop eventloop;
	XrlStdRouter xrl_router(eventloop, "clear_database");

	debug_msg("Waiting for router\n");
	xrl_router.finalize();
	wait_until_xrl_router_is_ready(eventloop, xrl_router);
	debug_msg("\n");

	ClearDatabase clear_database(xrl_router);
	clear_database.start();
	while (clear_database.busy())
	    eventloop.run();

	if (clear_database.fail()) {
	    XLOG_ERROR("Failed to clear database");
	    return -1;
	}

    } catch (...) {
	xorp_catch_standard_exceptions();
    }

    xlog_stop();
    xlog_exit();

    return 0;
    UNUSED(argc);
}
