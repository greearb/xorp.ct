// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2007-2011 XORP, Inc and Others
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



// Print information about OSPF neighbours

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "ospf/ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/service.hh"
#include "libxorp/status_codes.h"
#include "libxorp/eventloop.hh"
#include "libxorp/tlv.hh"




#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "libxipc/xrl_std_router.hh"

#include "xrl/interfaces/ospfv2_xif.hh"
#ifdef HAVE_IPV6
#include "xrl/interfaces/ospfv3_xif.hh"
#endif

#include "ospf/ospf.hh"
#include "ospf/test_common.hh"

class ClearDatabase {
public:
    ClearDatabase(XrlStdRouter& xrl_router, OspfTypes::Version version)
	: _xrl_router(xrl_router), _version(version),
	_done(false), _fail(false)
    {
    }

    void start() {
	switch(_version) {
	case OspfTypes::V2: {
	    XrlOspfv2V0p1Client ospfv2(&_xrl_router);
	    ospfv2.send_clear_database(xrl_target(_version),
				       callback(this,
						&ClearDatabase::response));
	}
	    break;
	case OspfTypes::V3: {
#ifdef HAVE_IPV6
	    XrlOspfv3V0p1Client ospfv3(&_xrl_router);
	    ospfv3.send_clear_database(xrl_target(_version),
				       callback(this,
						&ClearDatabase::response));
#endif
	}
	    break;
	}
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
    OspfTypes::Version _version;
    bool _done;
    bool _fail;
};

int
usage(const char *myname)
{
    fprintf(stderr, "usage: %s [-2|-3]\n", myname);

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

    OspfTypes::Version version = OspfTypes::V2;

    int c;
    while ((c = getopt(argc, argv, "23")) != -1) {
	switch (c) {
	case '2':
	    version = OspfTypes::V2;
	    break;
	case '3':
	    version = OspfTypes::V3;
	    break;
	default:
	    return usage(argv[0]);
	}
    }

    try {
	EventLoop eventloop;
	XrlStdRouter xrl_router(eventloop, "clear_database");

	debug_msg("Waiting for router");
	xrl_router.finalize();
	wait_until_xrl_router_is_ready(eventloop, xrl_router);
	debug_msg("\n");

	ClearDatabase clear_database(xrl_router, version);
	clear_database.start();
	while(clear_database.busy())
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
}
