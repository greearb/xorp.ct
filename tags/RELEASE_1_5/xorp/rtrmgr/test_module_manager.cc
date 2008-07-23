// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/rtrmgr/test_module_manager.cc,v 1.23 2008/01/04 03:17:44 pavlin Exp $"


#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "module_manager.hh"
#include "master_template_tree.hh"
#include "util.hh"
#include "test_module_manager.hh"


static bool waiting = false;
static bool run_success = false;

void
module_run_done(bool success)
{
    run_success = success;
    waiting = false;
}

// the following two functions are an ugly hack to cause the C code in
// the parser to call methods on the right version of the TemplateTree

void
add_cmd_adaptor(char *cmd, TemplateTree* tt) throw (ParseError)
{
    // ((MasterTemplateTree*)tt)->add_cmd(cmd);
    UNUSED(cmd);
    UNUSED(tt);
}

void
add_cmd_action_adaptor(const string& cmd, const list<string>& action,
		       TemplateTree* tt) throw (ParseError)
{
    // ((MasterTemplateTree*)tt)->add_cmd_action(cmd, action);
    UNUSED(cmd);
    UNUSED(action);
    UNUSED(tt);
}


Rtrmgr::Rtrmgr()
{
}

int
Rtrmgr::run()
{
    string finder_str = "finder";

    // Initialize the event loop
    EventLoop eventloop; 

    // Start the module manager
    ModuleManager mmgr(eventloop, *this,
		       false,	/* do_restart */
		       true,	/* verbose = */ 
		       ".");
    string error_msg;
    if (mmgr.new_module(finder_str, "../libxipc/xorp_finder", error_msg)
	!= true) {
	fprintf(stderr, "Cannot add module %s: %s\n", finder_str.c_str(),
		error_msg.c_str());
	return -1;
    }

    if (mmgr.module_has_started(finder_str) == true) {
	fprintf(stderr, "Incorrect initialization state for new module\n");
	mmgr.shutdown();
	return -1;
    }

    XorpCallback1<void, bool>::RefPtr cb;
    cb = callback(&module_run_done);
    waiting = true;
    mmgr.start_module(finder_str, true, false, cb);

    printf("Verifying finder starting\n");
    if (mmgr.module_has_started(finder_str) != true) {
	mmgr.shutdown();
	return -1;
    }

    while (eventloop.events_pending() || waiting == true) {
	printf(".");
	eventloop.run();
    }
    
    printf("finder should now be running\n");
    // XLOG_ASSERT(mmgr.module_running(finder_str) == true);
    TimerList::system_sleep(TimeVal(2, 0));

    printf("shutting down\n");
    mmgr.shutdown();
    while ((mmgr.is_shutdown_completed() != true)
	   && eventloop.events_pending()) {
	printf(".");
	eventloop.run();
    }
    printf("module manager has shut down\n");
    
    printf("bye\n");
    return 0;
}

void 
Rtrmgr::module_status_changed(const string& module_name,
			      GenericModule::ModuleStatus status)
{
    UNUSED(module_name);
    UNUSED(status);
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

    Rtrmgr rtrmgr;
    rtrmgr.run();
    
    printf("bye\n");
    return 0;
}
