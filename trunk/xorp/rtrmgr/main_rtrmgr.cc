// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/rtrmgr/main_rtrmgr.cc,v 1.47 2002/12/09 18:29:37 hodson Exp $"

#include <signal.h>

#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include "rtrmgr_module.h"
#include "libxorp/xlog.h"

#include "libxipc/finder-server.hh"
#include "libxipc/xrlstdrouter.hh"

#include "template_tree_node.hh"
#include "template_commands.hh"
#include "template_tree.hh"
#include "master_conf_tree.hh"
#include "module_manager.hh"
#include "userdb.hh"
#include "xrl_rtrmgr_interface.hh"
#include "randomness.hh"
#include "main_rtrmgr.hh"

//
// Defaults
//
static bool 	   default_no_execute 	       = false;
static const char* default_config_boot	       = "config.boot";
static const char* default_config_template_dir = "../etc/templates";
static const char* default_xrl_dir 	       = "../xrl/targets";

static bool running;

static void signalhandler(int) {
    running = false;
}

void
usage(const char *name)
{
    fprintf(stderr,
	"usage: %s [-n] [-b config.boot] [-t cfg_dir] [-x xrl_dir]\n",
	    name);
    fprintf(stderr, "options:\n");

    fprintf(stderr, 
	    "\t-n		do not execute XRLs		[ %s ]\n",
	    default_no_execute ? "true" : "false");

    fprintf(stderr, 
	    "\t-b config.boot	specify boot file 		[ %s ]\n",
	    default_config_boot);
	    
    fprintf(stderr, 
	    "\t-t cfg_dir	specify config directory	[ %s ]\n",
	    default_config_template_dir);

    fprintf(stderr, 
	    "\t-x xrl_dir	specify xrl directory		[ %s ]\n",
	    default_xrl_dir);

    exit(-1);
}

int
main(int argc, char* const argv[])
{
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
    running = true;

    RandomGen randgen;    

    bool no_execute = default_no_execute;
    const char*	config_template_dir = default_config_template_dir;
    const char*	xrl_dir 	    = default_xrl_dir;
    const char*	config_boot         = default_config_boot;

    int c;

    while ((c = getopt (argc, argv, "t:b:x:n")) != EOF) {
	switch(c) {  
	case 't':
	    config_template_dir = optarg;
	    break;
	case 'b':
	    config_boot = optarg;
	    break;
	case 'x':
	    xrl_dir = optarg;
	    break;
	case 'n':
	    no_execute = true;
	    break;
	case '?':
	default:
	    usage(argv[0]);
	}
    }

    //read the router config template files
    TemplateTree *tt;
    try {
	tt = new TemplateTree(config_template_dir, xrl_dir);
    } catch (const XorpException&) {
	printf("caught exception\n");
	xorp_unexpected_handler();
    }
    tt->display_tree();


    //signal handlers so we can clean up when we're killed
    signal(SIGTERM, signalhandler);
    signal(SIGINT, signalhandler);
    //    signal(SIGBUS, signalhandler);
    //    signal(SIGSEGV, signalhandler);

    //initialize the event loop
    EventLoop event_loop; 
    randgen.add_event_loop(&event_loop);

    /* Finder Server */
    FinderServer fs(event_loop);

    //start the module manager
    ModuleManager mmgr(&event_loop);
    try {
	UserDB userdb;
	userdb.load_password_file();

	//initialize the IPC mechanism
	XrlStdRouter xrlrouter(event_loop, "rtrmgr");

	if (no_execute == false) {
	
	    XorpClient xclient(&event_loop, &xrlrouter);

	    //read the router startup configuration file,
	    //start the processes required, and initialize them
	    MasterConfigTree ct(config_boot, tt, &mmgr, &xclient, no_execute);
	    XrlRtrmgrInterface rtrmgr_target(&xrlrouter, &userdb,
					     &ct, &event_loop,
					     &randgen);

	    //loop while handling configuration events and signals
	    while (running) {
		printf("+");
		fflush(stdout);
		event_loop.run();
	    }
	} else {
	    //no-execute mode, for debugging purposes

	    XorpClient xclient(&event_loop, NULL);

	    printf("here0_______________________\n");
	    MasterConfigTree ct(config_boot, tt, &mmgr, &xclient, no_execute);
	    printf("here1_______________________\n");
	    XrlRtrmgrInterface rtrmgr_target(&xrlrouter, &userdb,
					     &ct, &event_loop,
					     &randgen);

	    while (running) {
		printf("+");
		fflush(stdout);
		event_loop.run();
	    }
	}
    } catch (InitError& e) {
	XLOG_ERROR("rtrmgr shutting down due to error\n");
	xorp_print_standard_exceptions();
	errcode = 1;
    }

    mmgr.shutdown();
    delete tt;
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return (errcode);
}








