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

#ident "$XORP: xorp/rtrmgr/test_sample_config.cc,v 1.16 2004/06/10 22:41:55 hodson Exp $"


#include <signal.h>

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include "libxipc/finder_server.hh"
#include "libxipc/xrl_std_router.hh"

#include "master_conf_tree.hh"
#include "module_manager.hh"
#include "rtrmgr_error.hh"
#include "task.hh"
#include "template_commands.hh"
#include "template_tree.hh"
#include "template_tree_node.hh"


//
// Defaults
//

static const char* c_srcdir = getenv("srcdir");
static const string srcdir = c_srcdir ? c_srcdir : ".";
static const string default_xorp_root_dir = "..";
static const string default_config_template_dir = srcdir + "/../etc/templates";
static const string default_xrl_targets_dir = srcdir + "/../xrl/targets";
static const string default_config_boot = srcdir + "/config.boot.sample";

//
// This test loads the template tree, then loads the sample config,
// but doesn't call any XRLs or start any processes.
//

int
main(int argc, char* const argv[])
{
    UNUSED(argc);

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
    const string& xrl_targets_dir = default_xrl_targets_dir;
    const string& config_boot = default_config_boot;

    // Read the router config template files
    TemplateTree *tt = NULL;
    try {
	tt = new TemplateTree(default_xorp_root_dir, config_template_dir,
			      xrl_targets_dir, true /* verbose */);
    } catch (const InitError& e) {
	fprintf(stderr, "test_sample_config: template tree init error: %s\n",
		e.why().c_str());
	fprintf(stderr, "test_sample_config: TEST FAILED\n");
	exit(1);
    } catch (...) {
	xorp_unexpected_handler();
	fprintf(stderr, "test_sample_config: unexpected error\n");
	fprintf(stderr, "test_sample_config: TEST FAILED\n");
	exit(1);
    }

    // Initialize the event loop
    EventLoop eventloop; 

    // Initialize the finder server
    FinderServer fs(eventloop);

    // Start the module manager
    ModuleManager mmgr(eventloop,
		       false,	/* do_restart */
		       false,	/* verbose */
		       default_xorp_root_dir);

    try {
	// Initialize the IPC mechanism
	XrlStdRouter xrl_router(eventloop, "rtrmgr-test", fs.addr(),
				fs.port());
	XorpClient xclient(eventloop, xrl_router);

	//
	// Read the router startup configuration file, start the processes
	// required, and initialize them.
	//
	MasterConfigTree ct(config_boot, tt, mmgr, xclient,
			    false /* do_exec */,
			    true /* verbose */);
    } catch (const InitError& e) {
	fprintf(stderr, "test_sample_config: config tree init error: %s\n",
		e.why().c_str());
	fprintf(stderr, "test_sample_config: TEST FAILED\n");
	exit(1);
    } catch (...) {
	xorp_unexpected_handler();
	fprintf(stderr, "test_sample_config: unexpected error\n");
	fprintf(stderr, "test_sample_config: TEST FAILED\n");
	exit(1);
    }

    mmgr.shutdown();
    while ((! mmgr.shutdown_complete()) && eventloop.timers_pending()) {
	eventloop.run();
    }

    delete tt;

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    exit(0);
}
