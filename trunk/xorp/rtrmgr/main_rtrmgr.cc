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

#ident "$XORP: xorp/rtrmgr/main_rtrmgr.cc,v 1.45 2004/05/22 06:09:06 atanu Exp $"

#include <signal.h>

#include "rtrmgr_module.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <glob.h>

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
#include "rtrmgr_error.hh"
#include "util.hh"
#include "main_rtrmgr.hh"

//
// Default values
//
static bool default_do_exec = true;

//
// Local state
//
static bool	running = false;
static string	template_dir;
static string	xrl_dir;
static string	boot_file;
static string	save_hook;
static bool	do_exec = default_do_exec;
list<IPv4>	bind_addrs;
uint16_t	bind_port = FINDER_DEFAULT_PORT;
int32_t		quit_time = -1;

void cleanup_and_exit(int errcode);

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
    fprintf(stderr, "  -s <app>  Specify save config file hook\n");
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

static void
wait_until_xrl_router_is_ready(EventLoop& eventloop, XrlRouter& xrl_router)
{
    while (xrl_router.ready() == false) {
	eventloop.run();
	if (xrl_router.failed()) {
	    XLOG_FATAL("XrlRouter failed.  No Finder?");
	}
    }
}

Rtrmgr::Rtrmgr(const string& template_dir, 
	       const string& xrl_dir,
	       const string& boot_file,
	       const list<IPv4>& bind_addrs,
	       uint16_t bind_port,
	       const string& save_hook,
	       bool	do_exec,
	       int32_t quit_time)
    : _template_dir(template_dir),
      _xrl_dir(xrl_dir),
      _boot_file(boot_file),
      _bind_addrs(bind_addrs),
      _bind_port(bind_port),
      _save_hook(save_hook),
      _do_exec(do_exec),
      _quit_time(quit_time),
      _ready(false),
      _mct(NULL)
{
}

int
Rtrmgr::run()
{
    int errcode = 0;

    running = true;

    //
    // Install signal handlers so we can clean up when we're killed
    //
    signal(SIGTERM, signalhandler);
    signal(SIGINT, signalhandler);
    // XXX signal(SIGBUS, signalhandler);
    // XXX signal(SIGSEGV, signalhandler);

    //
    // Initialize the event loop
    //
    EventLoop eventloop;

    // 
    // Validate the save hook is sane
    //
    if (validate_save_hook() != XORP_OK) {
	XLOG_ERROR("xorp_rtrmgr save hook %s is not executable", 
		   _save_hook.c_str());
	return (1);
    }

    //
    // Read the router config template files
    //
    TemplateTree* tt = NULL;
    try {
	tt = new TemplateTree(xorp_config_root_dir(), _template_dir, _xrl_dir);
    } catch (const InitError& e) {
	XLOG_ERROR("Shutting down due to an init error: %s", e.why().c_str());
	return (1);
    }
#if 1
    tt->display_tree();
#endif

    //
    // Start the finder and the rest of the rtrmgr components.
    // These are dynamically created so we have control over the
    // deletion order.
    //
    FinderServer* fs = NULL;
    try {
	fs = new FinderServer(eventloop, _bind_port);
	while (_bind_addrs.empty() == false) {
	    if (fs->add_binding(_bind_addrs.front(), _bind_port) == false) {
		XLOG_WARNING("Finder failed to bind interface %s port %d",
			     _bind_addrs.front().str().c_str(), _bind_port);
	    }
	    _bind_addrs.pop_front();
	}
    } catch (const InvalidPort& i) {
	XLOG_ERROR("%s: a finder may already be running.", i.why().c_str());
	delete tt;
	return (1);
    } catch (...) {
	xorp_catch_standard_exceptions();
	delete tt;
	return (1);
    }

    //
    // Initialize the IPC mechanism
    //
    XrlStdRouter xrl_router(eventloop, "rtrmgr", fs->addr(), fs->port());
    XorpClient xclient(eventloop, xrl_router);

    //
    // Start the module manager
    //
    ModuleManager mmgr(eventloop, /*verbose = */false, xorp_binary_root_dir());

    try {
	//
	// Read the router startup configuration file,
	// start the processes required, and initialize them.
	//
	RandomGen randgen;
	UserDB userdb;
	userdb.load_password_file();
	XrlRtrmgrInterface xrt(xrl_router, userdb, eventloop, randgen, *this);

	wait_until_xrl_router_is_ready(eventloop, xrl_router);

	_mct = new MasterConfigTree(boot_file, tt, mmgr, xclient, _do_exec);
	//
	// XXX: note that theoretically we may receive an XRL before
	// we call XrlRtrmgrInterface::set_conf_tree().
	// For now we ignore that possibility...
	//
	xrt.set_conf_tree(_mct);

	// For testing purposes, rtrmgr can terminate itself after some time.
	XorpTimer quit_timer;
	if (_quit_time > 0) {
	    quit_timer =
		eventloop.new_oneoff_after_ms(_quit_time * 1000,
					      callback(signalhandler, 0));
	}

	_ready = true;
	//
	// Loop while handling configuration events and signals
	//
	while (running) {
	    fflush(stdout);
	    eventloop.run();
	    if (_mct->config_failed())
		running = false;
	}
	fflush(stdout);
	_ready = false;

	//
	// Shutdown everything
	//

	// Delete the configuration
	_mct->delete_entire_config();

	// Wait until changes due to deleting config have finished
	// being applied.
	while (eventloop.timers_pending() && (_mct->commit_in_progress())) {
	    eventloop.run();
	}
	delete _mct;
    } catch (const InitError& e) {
	XLOG_ERROR("rtrmgr shutting down due to an init error: %s",
		   e.why().c_str());
	errcode = 1;
    }

    // Shut down child processes that haven't already been shutdown
    mmgr.shutdown();

    // Wait until child processes have terminated
    while (mmgr.shutdown_complete() == false && eventloop.timers_pending()) {
	eventloop.run();
    }

    // Delete the template tree
    delete tt;

    // Shutdown the finder
    delete fs;

    return (errcode);
}

bool 
Rtrmgr::ready() const {
    if (!_ready)
	return false;
    if (_mct->commit_in_progress())
	return false;
    return true;
}

int
Rtrmgr::validate_save_hook() {
    if (_save_hook.empty())
	return XORP_OK;
    string expanded_path;
    if (_save_hook[0] != '/') {
	// we're going to call glob, but don't want to allow wildcard expansion
	for (size_t i = 0; i < _save_hook.length(); i++) {
	    char c = _save_hook[i];
	    if ((c == '*') || (c == '?') || (c == '[')) {
		string err = _save_hook + ": bad filename";
		XLOG_ERROR(err.c_str());
		return XORP_ERROR;
	    }
	}
	glob_t pglob;
	glob(_save_hook.c_str(), GLOB_TILDE, NULL, &pglob);
	if (pglob.gl_pathc != 1) {
	    string err(_save_hook + ": File does not exist.");
	    XLOG_ERROR(err.c_str());
	    return XORP_ERROR;
	}
	expanded_path = pglob.gl_pathv[0];
	globfree(&pglob);
    } else {
	expanded_path = _save_hook;
    }

    struct stat sb;
    if (stat(expanded_path.c_str(), &sb) < 0) {
	string err = expanded_path + ": ";
	switch (errno) {
	case ENOTDIR:
	    err += "A component of the path prefix is not a directory.";
	    break;
	case ENOENT:
	    err += "File does not exist.";
	    break;
	case EACCES:
	    err += "Permission denied.";
	    break;
	case ELOOP:
	    err += "Too many symbolic links.";
	    break;
	default:
	    err += "Unknown error accessing file.";
	}
	XLOG_ERROR(err.c_str());
	return XORP_ERROR;
    }
    _save_hook = expanded_path;
    return XORP_OK;
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

    //
    // Install the handler for unexpected exceptions
    //
    XorpUnexpectedHandler ex(xorp_unexpected_handler);

    //
    // Expand the default variables to include the XORP root path
    //
    xorp_path_init(argv[0]);
    template_dir	= xorp_template_dir();
    xrl_dir		= xorp_xrl_targets_dir();
    boot_file		= xorp_boot_file();
    save_hook           = "";

    int c;
    while ((c = getopt(argc, argv, "t:b:s:x:i:p:q:ndh")) != EOF) {
	switch(c) {
	case 't':
	    template_dir = optarg;
	    break;
	case 'b':
	    boot_file = optarg;
	    break;
	case 's':
	    save_hook = optarg;
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
		cleanup_and_exit(1);
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
		    cleanup_and_exit(1);
		}
		bind_addrs.push_back(bind_addr);
	    } catch (const InvalidString&) {
		fprintf(stderr, "%s is not a valid interface address.\n",
			optarg);
		cleanup_and_exit(1);
	    }
	    break;
	case 'h':
	case '?':
	default:
	    usage(argv[0]);
	    display_defaults();
	    cleanup_and_exit(1);
	}
    }

    //
    // The main procedure
    //
    Rtrmgr rtrmgr(template_dir, xrl_dir, boot_file, bind_addrs,
		  bind_port, save_hook, do_exec, quit_time);
    errcode = rtrmgr.run();

    cleanup_and_exit(errcode);
}

void cleanup_and_exit(int errcode) 
{
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    exit(errcode);
}
