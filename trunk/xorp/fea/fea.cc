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

#ident "$XORP: xorp/fea/fea_rtsock.cc,v 1.3 2003/03/10 23:20:13 hodson Exp $"

#include "config.h"
#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"
#include "libxipc/xrl_std_router.hh"
#include "fticonfig.hh"
#include "ifmanager.hh"
#include "ifconfig.hh"
#include "xrl_ifupdate.hh"
#include "xrl_target.hh"

static const char* xrl_entity = "fea";

void
usage(char *name)
{
    fprintf(stderr,
"usage: %s [-h finder_host]\n",
	    name);
    exit(-1);
}

static void
fea_main(const char* finder_host)
{
    EventLoop e;

    XrlStdRouter rtr(e, xrl_entity, finder_host);

    //
    // Initialize Components
    //
    // 1. Routing Socket
    //
#ifndef HOST_OS_LINUX
    // RoutingSocket rs(e);

    //
    // 2. Routing Socket based Fti
    //
    // RoutingSocketFti fti(rs);
    
    FtiConfig fticonfig(e);
#endif

    //
    // 3. Interface Configurator and reporters
    //
    XrlIfConfigUpdateReporter ifreporter(rtr);
    IfConfigErrorReporter iferr;

    IfConfig ifconfig(e, ifreporter, iferr);
    ifconfig.start();
    
    //
    // 4. Interface manager
    //
    InterfaceManager ifm(ifconfig);

    //
    // 5. Raw Socket TODO
    //

#ifndef HOST_OS_LINUX
    XrlFeaTarget xrl_tgt(e, rtr, fticonfig, ifm, ifreporter, 0);
#endif

    for (;;)
	e.run();
}


int
main(int argc, char *argv[])
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    int c;
    const char *finder_host = "localhost";

    /*
    ** Initialize and start xlog
    */
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    while((c = getopt (argc, argv, "h:")) != EOF) {
	switch (c) {
	case 'h':
	    finder_host = optarg;
	    break;
	case '?':
	    usage(argv[0]);
	}
    }

    /* XXX
     * Do all that daemon stuff take off into the background disable signals
     * and all that other good stuff.
     */

    try {
	fea_main(finder_host);
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    /*
     * Gracefully stop and exit xlog
     */
    xlog_stop();
    xlog_exit();
    return 0;
}
