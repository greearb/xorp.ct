// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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




#include <signal.h>

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include "libxipc/finder_server.hh"
#include "libxipc/xrl_std_router.hh"

#include "test_sample_config.hh"
#include "master_conf_tree.hh"
#include "module_manager.hh"
#include "rtrmgr_error.hh"
#include "task.hh"
#include "template_commands.hh"
#include "template_tree.hh"
#include "template_tree_node.hh"
#include "master_template_tree.hh"
#include "master_template_tree_node.hh"
#include "xrldb.hh"
//
// Defaults
//

static const char* c_srcdir = getenv("srcdir");
static const string srcdir = c_srcdir ? c_srcdir : ".";
static const string default_xorp_root_dir = "..";
static const string default_config_template_dir = srcdir + "/../etc/templates";
static const string default_xrl_targets_dir = srcdir + "/../xrl/targets";
static const string default_config_boot_set[] = {
    "bgp.boot",
    "click.boot",
    "interfaces.boot",
    "multicast4.boot",
    "multicast6.boot",
    "ospfv2.boot",
    "ospfv3.boot",
    "rip.boot",
    "ripng.boot",
    "snmp.boot",
    "static.boot"
};

// the following two functions are an ugly hack to cause the C code in
// the parser to call methods on the right version of the TemplateTree

void
add_cmd_adaptor(char *cmd, TemplateTree* tt) throw (ParseError)
{
    ((MasterTemplateTree*)tt)->add_cmd(cmd);
}


void
add_cmd_action_adaptor(const string& cmd, const list<string>& action,
		       TemplateTree* tt) throw (ParseError)
{
    ((MasterTemplateTree*)tt)->add_cmd_action(cmd, action);
}


//
// This test loads the template tree, then loads the sample config,
// but doesn't call any XRLs or start any processes.
//


Rtrmgr::Rtrmgr()
{
}


int 
Rtrmgr::run()
{
    const string& config_template_dir = default_config_template_dir;
    const string& xrl_targets_dir = default_xrl_targets_dir;

    XRLdb* xrldb = NULL;
    try {
	xrldb = new XRLdb(xrl_targets_dir, /*verbose*/ true);
    } catch (const InitError& e) {
	fprintf(stderr, "Init error in XrlDB: %s\n", e.why().c_str());
	fprintf(stderr, "test_sample_config: TEST FAILED\n");
	return (1);
    }

    // Read the router config template files
    MasterTemplateTree *tt = NULL;
    try {
	tt = new MasterTemplateTree(default_xorp_root_dir, 
				    xrldb, true /* verbose */);
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

    string errmsg;
    if (tt->load_template_tree(config_template_dir, errmsg) == false) {
	fprintf(stderr, "%s\n", errmsg.c_str());
	fprintf(stderr, "test_sample_config: TEST FAILED\n");
	exit(1);
    }

    // Initialize the event loop
    EventLoop eventloop; 

    // Initialize the finder server
    FinderServer fs(eventloop, FinderConstants::FINDER_DEFAULT_HOST(),
		    FinderConstants::FINDER_DEFAULT_PORT());

    // Start the module manager
    ModuleManager mmgr(eventloop, *this,
		       false,	/* do_restart */
		       false,	/* verbose */
		       default_xorp_root_dir,
               default_xorp_root_dir);

    // Try each configuration file in a loop
    for (size_t i = 0;
	 i < sizeof(default_config_boot_set)/sizeof(default_config_boot_set[0]);
	 i++) {
	string config_boot = srcdir + "/config/" + default_config_boot_set[i];
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
    }

    mmgr.shutdown();
    while ((mmgr.is_shutdown_completed() != true)
	   && eventloop.events_pending()) {
	eventloop.run();
    }

    delete tt;
    delete xrldb;

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

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    Rtrmgr rtrmgr;
    rtrmgr.run();

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    exit(0);
}
