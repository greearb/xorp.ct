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

#ident "$XORP: xorp/rtrmgr/main_rtrmgr.cc,v 1.26 2003/09/11 22:23:54 hodson Exp $"

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

//
// Defaults
//
#define DEFAULT_XORP_ROOT_DIR		XORP_ROOT
#define DEFAULT_CONFIG_BOOT		"config.boot"
#define DEFAULT_CONFIG_TEMPLATE_DIR	"etc/templates"
#define DEFAULT_XRL_DIR			"xrl/targets"
#define DEFAULT_DO_EXEC			true
string	default_xorp_root_dir		= DEFAULT_XORP_ROOT_DIR;
string	default_config_boot		= DEFAULT_CONFIG_BOOT;
string	default_config_template_dir	= DEFAULT_CONFIG_TEMPLATE_DIR;
string	default_xrl_dir			= DEFAULT_XRL_DIR;
bool	default_do_exec			= DEFAULT_DO_EXEC;

static bool running;

static void signalhandler(int)
{
    running = false;
}

void
usage()
{
    fprintf(stderr,
	"usage: rtrmgr [-n] [-b config.boot] [-t cfg_dir] [-x xrl_dir]\n");
    fprintf(stderr, "options:\n");

    fprintf(stderr,
	    "\t-n		do not execute XRLs		[ %s ]\n",
	    default_do_exec ? "false" : "true");

    fprintf(stderr,
	    "\t-b config.boot	specify boot file 		[ %s ]\n",
	    default_config_boot.c_str());

    fprintf(stderr,
	    "\t-t cfg_dir	specify config directory	[ %s ]\n",
	    default_config_template_dir.c_str());

    fprintf(stderr,
	    "\t-x xrl_dir	specify xrl directory		[ %s ]\n",
	    default_xrl_dir.c_str());

    exit(-1);
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

//
// Return the directory path name (without the trailing '/') to the
// executable program "progname".
// If the path wasn't found, the return string is empty.
//
string
find_exec_path_name(const char *progname)
{
    char *b, *p;

    // Check if we have already specified a path to the program
    do {
	char max_path[MAXPATHLEN + 1];

	strncpy(max_path, progname,  sizeof(max_path) - 1);
	max_path[sizeof(max_path) - 1] = '\0';

	p = strrchr(max_path, '/');
	if ( p != NULL) {
	    // We have specified a path to the program; return that path
	    *p = '\0';
	    while (max_path[strlen(max_path) - 1] == '/')
		max_path[strlen(max_path) - 1] = '\0';
	    return (string(max_path));
	}
    } while (false);

    //
    // Go through the PATH environment variable and find the program location
    //
    char* exec_path = getenv("PATH");
    if (exec_path == NULL)
	return (string(""));

    char buff[strlen(exec_path)];
    strncpy(buff, exec_path, sizeof(buff) - 1);
    buff[sizeof(buff) - 1] = '\0';

    b = buff;
    do {
	if ((b == NULL) || (*b == '\0'))
	    break;

	// Cut-off the next directory name
	p = strchr(b, ':');
	if (p != NULL) {
	    *p = '\0';
	    p++;
	}
	while (b[strlen(b) - 1] == '/')
	    b[strlen(b) - 1] = '\0';
	string prefix_name = string(b);
	string abs_progname = prefix_name + string("/") + string(progname);

	if (access(abs_progname.c_str(), X_OK) == 0) {
	    return (prefix_name);	// Found
	}
	b = p;
    } while (true);

    return (string(""));
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

    //
    // Get the root of the tree
    // The ordering is:
    // 1. The shell environment XORP_ROOT
    // 2. The parent directory (only if it contains the template and the
    //    xrl directories)
    // 3. The XORP_ROOT value as defined in config.h
    //
    do {
	// Try the shell environment XORP_ROOT
	char *p = getenv("XORP_ROOT");
	if (p != NULL) {
	    default_xorp_root_dir = p;
	    break;
	}

	// Try the parent directory
	string s = find_exec_path_name(argv[0]);
	if (strlen(s.c_str()) > 0) {
	    s += "/..";		// XXX: add the parent directory
	    string t_dir = s + "/" + DEFAULT_CONFIG_TEMPLATE_DIR;
	    string x_dir = s + "/" + DEFAULT_XRL_DIR;
	    struct stat t_stat, x_stat;
	    if ((stat(t_dir.c_str(), &t_stat) == 0)
		&& (stat(x_dir.c_str(), &x_stat) == 0)
		&& (S_ISDIR(t_stat.st_mode))
		&& (S_ISDIR(x_stat.st_mode))) {
		default_xorp_root_dir = s;
		break;
	    }
	}

	// The XORP_ROOT value
	default_xorp_root_dir = DEFAULT_XORP_ROOT_DIR;
	break;
    } while (false);

    //
    // Expand the default variables to include the XORP root path
    //
    default_config_boot = default_xorp_root_dir + "/" + default_config_boot;
    default_config_template_dir = default_xorp_root_dir + "/"
	+ default_config_template_dir;
    default_xrl_dir = default_xorp_root_dir + "/" + default_xrl_dir;

    string	config_template_dir = default_config_template_dir;
    string	xrl_dir 	    = default_xrl_dir;
    string	config_boot         = default_config_boot;

    bool do_exec = default_do_exec;

    list<IPv4>	bind_addrs;
    uint16_t	bind_port = FINDER_DEFAULT_PORT;
    int32_t     quit_time = -1;

    int c;

    while ((c = getopt (argc, argv, "t:b:x:i:p:q:n")) != EOF) {
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
	case 'q':
	    quit_time = atoi(optarg);
	    break;
	case 'n':
	    do_exec = false;
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
		usage();
		exit(-1);
	    }
	    break;
	case '?':
	default:
	    usage();
	}
    }

    // read the router config template files
    TemplateTree *tt;
    try {
	tt = new TemplateTree(default_xorp_root_dir,
			      config_template_dir,
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
    ModuleManager mmgr(eventloop, /*verbose = */true, default_xorp_root_dir);

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
	    
	    if (xrl_router.ready() == false && timed_out) {
		XLOG_FATAL("XrlRouter did not become ready.  No Finder?");
		exit (1);
	    }
	}
	MasterConfigTree* ct = new MasterConfigTree(config_boot, tt, taskmgr);
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

