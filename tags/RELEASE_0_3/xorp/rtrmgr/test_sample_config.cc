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

#ident "$XORP: xorp/rtrmgr/test_sample_config.cc,v 1.4 2003/05/22 20:37:54 hodson Exp $"

#include <signal.h>

#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include "rtrmgr_module.h"
#include "libxorp/xlog.h"

#include "libxipc/finder_server.hh"
#include "libxipc/xrl_std_router.hh"

#include "template_tree_node.hh"
#include "template_commands.hh"
#include "template_tree.hh"
#include "master_conf_tree.hh"
#include "module_manager.hh"
#include "task.hh"
#include "main_rtrmgr.hh"

//
// Defaults
//

static const char* c_srcdir = getenv("srcdir");
static const string srcdir = c_srcdir ? c_srcdir : ".";
static const string default_config_template_dir = srcdir + "/../etc/templates";
static const string default_xrl_dir = srcdir + "/../xrl/targets";
static const string default_config_boot = srcdir + "/config.boot.sample";
/**
 * This test loads the template tree, then loads the sample config,
 * but doesn't call any XRLs or start any processes.
 */

int
main(int argc, char* const argv[])
{
    UNUSED(argc);
    int errcode = 0;

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    const string& config_template_dir = default_config_template_dir;
    const string& xrl_dir = default_xrl_dir;
    const string& config_boot = default_config_boot;

    //read the router config template files
    TemplateTree *tt;
    try {
	tt = new TemplateTree(config_template_dir, xrl_dir);
    } catch (const XorpException&) {
	xorp_unexpected_handler();
	fprintf(stderr, "test_sample_config: failed to load template file\n");
	fprintf(stderr, "test_sample_config: TEST FAILED\n");
	return -1;
    }

    //initialize the event loop
    EventLoop eventloop; 

    /* Finder Server */
    FinderServer fs(eventloop);

    //start the module manager
    ModuleManager mmgr(eventloop, /*verbose = */false);
    try {
	//initialize the IPC mechanism
	XrlStdRouter xrlrouter(eventloop, "rtrmgr-test", fs.addr(), fs.port());
	XorpClient xclient(eventloop, xrlrouter);

	TaskManager taskmgr(mmgr, xclient, false);

	//read the router startup configuration file,
	//start the processes required, and initialize them
	MasterConfigTree ct(config_boot, tt, taskmgr);
    } catch (InitError& e) {
	xorp_print_standard_exceptions();
	fprintf(stderr, "test_sample_config: failed to load config file\n");
	fprintf(stderr, "test_sample_config: TEST FAILED\n");
	return -1;
    }

    mmgr.shutdown();
    while (eventloop.timers_pending() && (!mmgr.shutdown_complete())) {
	eventloop.run();
    }

    delete tt;
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return (errcode);
}








