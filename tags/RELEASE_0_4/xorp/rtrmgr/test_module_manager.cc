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

#ident "$XORP: xorp/rtrmgr/test_module_manager.cc,v 1.6 2003/05/03 21:26:47 mjh Exp $"

#include "rtrmgr_module.h"
#include "libxorp/xlog.h"
#include "template_tree.hh"
#include "module_manager.hh"
#include "split.hh"

static bool waiting = false;
static bool run_success = false;

void
module_run_done(bool success)
{
    run_success = success;
    waiting = false;
}

int
main(int argc, char* const argv[])
{
    UNUSED(argc);

    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);	
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    // initialize the event loop
    EventLoop eventloop; 

    // start the module manager
    ModuleManager mmgr(eventloop, /*verbose = */true, ".");
    mmgr.new_module("finder", "../libxipc/xorp_finder");

    if (mmgr.module_has_started("finder") == true) {
	fprintf(stderr, "Incorrect initialization state for new module\n");
	mmgr.shutdown();
	return -1;
    }

    XorpCallback1<void, bool>::RefPtr cb;
    cb = callback(&module_run_done);
    waiting = true;
    mmgr.start_module("finder", true, cb);

    printf("Verifying finder starting\n");
    if (mmgr.module_has_started("finder") != true) {
	mmgr.shutdown();
	return -1;
    }

    while (eventloop.timers_pending() || waiting==true) {
	printf(".");
	eventloop.run();
    }
    
    printf("finder should now be running\n");
    //assert(mmgr.module_running("finder") == true);
    sleep(2);
    
    printf("shutting down\n");
    mmgr.shutdown();
    while (eventloop.timers_pending() && (!mmgr.shutdown_complete())) {
	printf(".");
	eventloop.run();
    }
    printf("module manager has shut down\n");
    
    printf("bye\n");
    return 0;
}
