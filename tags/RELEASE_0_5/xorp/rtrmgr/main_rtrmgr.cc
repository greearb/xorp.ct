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

#ident "$XORP: xorp/rtrmgr/main_rtrmgr.cc,v 1.31 2003/09/25 00:54:10 hodson Exp $"

#include <signal.h>

#include "rtrmgr_module.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>

#include "libxipc/sockutil.hh"
#include "libxipc/finder_server.hh"
#include "libxipc/finder_constants.hh"
#include "libxipc/permits.hh"
#include "libxipc/xrl_std_router.hh"

#include "template_tree_node.hh"
#include "template_commands.hh"
#include "template_tree.hh"
#include "master_conf_tree.hh"
#include "module_manager.hh"
#include "task.hh"
#include "userdb.hh"
#include "xrl_rtrmgr_interface.hh"
#include "randomness.hh"
#include "main_rtrmgr.hh"
#include "util.hh"

//
// Defaults
//
static bool default_do_exec = true;
static bool running = false;

static void signalhandler(int)
{
    running = false;
}

static void
usage(const char* argv0)
{
    fprintf(stderr, "Usage: %s [options]\n", xorp_basename(argv0));
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h        Display this information\n");
    fprintf(stderr, "  -d        Display defaults\n");
    fprintf(stderr, "  -b <file> Specify boot file\n");
    fprintf(stderr, "  -n        Do not execute XRLs\n");
    fprintf(stderr, "  -i <addr> Set or add an interface run Finder on\n");
    fprintf(stderr, "  -p <port> Set port to run Finder on\n");
    fprintf(stderr, "  -q <secs> Set forced quit period\n");
    fprintf(stderr, "  -t <dir>  Specify templates directory\n");
    fprintf(stderr, "  -x <dir>  Specify Xrl targets directory\n");
}

static void
display_defaults()
{
    fprintf(stderr, "Defaults:\n");
    fprintf(stderr, "  Execute Xrls          := %s\n",
	    default_do_exec ? "true" : "false");
    fprintf(stderr, "  Boot file             := %s\n",
	    xorp_boot_file().c_str());
    fprintf(stderr, "  Templates directory   := %s\n",
	    xorp_template_dir().c_str());
    fprintf(stderr, "  Xrl targets directory := %s\n",
	    xorp_xrl_targets_dir().c_str());
}

static bool
valid_interface(const IPv4& addr)
{
    uint32_t naddr = if_count();
    uint16_t any_up = 0;

    for (uint32_t n = 1; n <= naddr; n++) {
        string name;
        in_addr if_addr;
        uint16_t flags;

        if (if_probe(n, name, if_addr, flags) == false)
            continue;

        any_up |= (flags & IFF_UP);

        if (IPv4(if_addr) == addr && flags & IFF_UP) {
            return true;
        }
    }

    if (IPv4::ANY() == addr && any_up)
        return true;

    return false;
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
    xlog_level_set_verbose(XLOG_LEVEL_WARNING, XLOG_VERBOSE_HIGH);
    xlog_level_set_verbose(XLOG_LEVEL_INFO, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    running = true;

    RandomGen randgen;

    //
    // Expand the default variables to include the XORP root path
    //
    xorp_path_init(argv[0]);
    string template_dir	= xorp_template_dir();
    string xrl_dir	= xorp_xrl_targets_dir();
    string boot_file	= xorp_boot_file();

    bool do_exec = default_do_exec;

    list<IPv4>	bind_addrs;
    uint16_t	bind_port = FINDER_DEFAULT_PORT;
    int32_t     quit_time = -1;

    int c;

    while ((c = getopt (argc, argv, "t:b:x:i:p:q:ndh")) != EOF) {
	switch(c) {
	case 't':
	    template_dir = optarg;
	    break;
	case 'b':
	    boot_file = optarg;
	    break;
	case 'x':
	    xrl_dir = optarg;
	    break;
	case 'q':
	    quit_time = atoi(optarg);
	    break;
	case 'n':
	    do_exec = false;
	    break;
	case 'd':
	    display_defaults();
	    break;
	case 'p':
	    bind_port = static_cast<uint16_t>(atoi(optarg));
	    if (bind_port == 0) {
		fprintf(stderr, "0 is not a valid port.\n");
		exit(-1);
	    }
	    break;
	case 'i':
	    //
	    // User is specifying which interface to bind finder to
	    //
	    try {
		IPv4 bind_addr = IPv4(optarg);
		if (valid_interface(bind_addr) == false) {
		    fprintf(stderr,
			    "%s is not the address of an active interface.\n",
			    optarg);
		    exit(-1);
		}
		bind_addrs.push_back(bind_addr);
	    } catch (const InvalidString&) {
		fprintf(stderr, "%s is not a valid interface address.\n",
			optarg);
		usage(argv[0]);
		xlog_stop();
		xlog_exit();
		exit(-1);
	    }
	    break;
	case 'h':
	case '?':
	default:
	    usage(argv[0]);
	    xlog_stop();
	    xlog_exit();
	    exit(-1);
	}
    }

    // read the router config template files
    TemplateTree *tt;
    try {
	tt = new TemplateTree(xorp_config_root_dir(),
			      template_dir,
			      xrl_dir);
    } catch (const XorpException&) {
	printf("caught exception\n");
	xorp_unexpected_handler();
    }
#if 0
    tt->display_tree();
#endif

    // signal handlers so we can clean up when we're killed
    signal(SIGTERM, signalhandler);
    signal(SIGINT, signalhandler);
    // XXX signal(SIGBUS, signalhandler);
    // XXX signal(SIGSEGV, signalhandler);

    // initialize the event loop
    EventLoop eventloop;
    randgen.add_eventloop(&eventloop);

    // Start the finder.
    // These are dynamically created so we have control over the
    // deletion order.
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    FinderServer* fs;
    try {
	fs = new FinderServer(eventloop, bind_port);
	while (bind_addrs.empty() == false) {
	    if (fs->add_binding(bind_addrs.front(), bind_port) == false) {
		XLOG_WARNING("Finder failed to bind interface %s port %d\n",
			     bind_addrs.front().str().c_str(), bind_port);
	    }
	    bind_addrs.pop_front();
	}
    } catch (const InvalidPort& i) {
	fprintf(stderr, "%s: a finder may already be running.\n",
		i.why().c_str());
	delete tt;
	exit(-1);
    } catch (...) {
	xorp_catch_standard_exceptions();
	delete tt;
	exit(-1);
    }

    // start the module manager
    ModuleManager mmgr(eventloop, /*verbose = */true, xorp_binary_root_dir());

    UserDB userdb;
    userdb.load_password_file();

    // initialize the IPC mechanism
    XrlStdRouter xrl_router(eventloop, "rtrmgr", fs->addr(), fs->port());
    XorpClient xclient(eventloop, xrl_router);

    // initialize the Task Manager
    TaskManager taskmgr(mmgr, xclient, do_exec);

    try {
	// read the router startup configuration file,
	// start the processes required, and initialize them
	XrlRtrmgrInterface xrt(xrl_router, userdb, eventloop, randgen);
	{
	    // Wait until the XrlRouter becomes ready
	    bool timed_out = false;

	    XorpTimer t = eventloop.set_flag_after_ms(10000, &timed_out);
	    while (xrl_router.ready() == false && timed_out == false) {
		eventloop.run();
	    }

	    if (xrl_router.ready() == false) {
		XLOG_FATAL("XrlRouter did not become ready.  No Finder?");
	    }
	}
	MasterConfigTree* ct = new MasterConfigTree(boot_file, tt, taskmgr);
	//
	// XXX: note that theoretically we may receive an XRL before
	// we call XrlRtrmgrInterface::set_conf_tree().
	// For now we ignore that possibility...
	//
	xrt.set_conf_tree(ct);

	// For testing purposes, rtrmgr can terminate itself after some time.
	XorpTimer quit_timer;
	if (quit_time > 0) {
	    quit_timer =
		eventloop.new_oneoff_after_ms(quit_time*1000,
					      callback(signalhandler, 0));
	}

	// loop while handling configuration events and signals
	while (running) {
	    fflush(stdout);
	    eventloop.run();
	}

	// Shutdown everything

	// Delete the configuration.
	ct->delete_entire_config();

	// Wait until changes due to deleting config have finished
	// being applied.
	while (eventloop.timers_pending() && (ct->commit_in_progress())) {
	    eventloop.run();
	}
	delete ct;
    } catch (InitError& e) {
	XLOG_ERROR("rtrmgr shutting down due to error\n");
	fprintf(stderr, "rtrmgr shutting down due to error\n");
	errcode = 1;
	running = false;
    }

    //Shut down child processes that haven't already been shutdown.
    mmgr.shutdown();

    //Wait until child processes have terminated
    while (mmgr.shutdown_complete() == false && eventloop.timers_pending()) {
	eventloop.run();
    }

    //delete the template tree
    delete tt;

    //shutdown the finder
    delete fs;

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return (errcode);
}

