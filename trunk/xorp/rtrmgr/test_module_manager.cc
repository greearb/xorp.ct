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

#ident "$XORP: xorp/devnotes/template.cc,v 1.2 2003/01/16 19:08:48 mjh Exp $"

#include "rtrmgr_module.h"
#include "template_tree.hh"
#include "template_commands.hh"
#include "module_manager.hh"
#include "split.hh"


static const char* default_config_template_dir = "../etc/templates";
static const char* default_xrl_dir 	       = "../xrl/targets";

static bool waiting = false;
static bool run_success = false;

void module_run_done(bool success) {
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

    //read the router config template files
    TemplateTree *tt;
    try {
	tt = new TemplateTree(default_config_template_dir, default_xrl_dir);
    } catch (const XorpException&) {
	xorp_unexpected_handler();
	fprintf(stderr, "test_sample_config: failed to load template file\n");
	fprintf(stderr, "test_sample_config: TEST FAILED\n");
	return -1;
    }

    //initialize the event loop
    EventLoop eventloop; 

    //start the module manager
    ModuleManager mmgr(eventloop, /*verbose = */true);

    ModuleCommand mod_cmd("%modinfo", *tt);
    mod_cmd.add_action(split("provides finder", ' '), tt->xrldb());
    mod_cmd.add_action(split("path ../libxipc/finder", ' '), tt->xrldb());

    mmgr.new_module(mod_cmd);

    if (mmgr.module_starting("finder") == true
	|| mmgr.module_running("finder") == true) {
	mmgr.shutdown();
	return -1;
    }

    XorpCallback1<void, bool>::RefPtr cb;
    cb = callback(&module_run_done);
    waiting = true;
    mmgr.run_module("finder", true, cb);

    printf("Verifying finder starting\n");
    if (mmgr.module_starting("finder") != true) {
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
    printf("bye\n");
    return 0;
}
