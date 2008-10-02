// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/rtrmgr/main_rtrmgr.cc,v 1.77 2008/07/23 05:11:41 pavlin Exp $"

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/daemon.h"
#include "libxorp/eventloop.hh"
#include "libxorp/utils.hh"

#include <signal.h>

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include "libxipc/sockutil.hh"
#include "libxipc/finder_server.hh"
#include "libxipc/finder_constants.hh"
#include "libxipc/permits.hh"
#include "libxipc/xrl_std_router.hh"

#include "main_rtrmgr.hh"
#include "master_conf_tree.hh"
#include "module_manager.hh"
#include "randomness.hh"
#include "rtrmgr_error.hh"
#include "task.hh"
#include "template_commands.hh"
#include "master_template_tree.hh"
#include "master_template_tree_node.hh"
#include "userdb.hh"
#include "util.hh"
#include "xrl_rtrmgr_interface.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif


//
// Default values
//
static bool default_do_exec = true;
static bool default_do_restart = false;
static bool default_verbose = false;


//
// Local state
//
static volatile bool	running = false;
static string	template_dir;
static string	xrl_targets_dir;
static string	boot_file;
static bool     do_logfile = false;
static bool     do_pidfile = false;
static bool     do_syslog = false;
static bool	do_exec = default_do_exec;
static bool	do_restart = default_do_restart;
static bool	verbose = default_verbose;
list<IPv4>	bind_addrs;
uint16_t	bind_port = FinderConstants::FINDER_DEFAULT_PORT();
static string	syslogspec;
static string	logfilename;
static string	pidfilename;
int32_t		quit_time = -1;
static bool	daemon_mode = false;

static void cleanup_and_exit(int errcode);

static void
signalhandler(int)
{
    running = false;
}

static void
usage(const char* argv0)
{
    fprintf(stderr, "Usage: %s [options]\n", xorp_basename(argv0));
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -a <allowed host> Host allowed by the finder\n");
    fprintf(stderr, "  -d        Run as a UNIX daemon (detach from tty)\n");
    fprintf(stderr, "  -l <file> Log to file <file>\n");
    fprintf(stderr, "  -L <facility.priority> Log to syslog facility\n");
    fprintf(stderr, "  -n <allowed net>  Subnet allowed by the finder\n");
    fprintf(stderr, "  -P <pid>  Write process ID to file <pid>\n");
    fprintf(stderr, "  -h        Display this information\n");
    fprintf(stderr, "  -v        Print verbose information\n");
    fprintf(stderr, "  -b <file> Specify boot file\n");
    fprintf(stderr, "  -N        Do not execute XRLs and do not start processes\n");
    fprintf(stderr, "  -r        Restart failed processes (not implemented yet)\n");
    fprintf(stderr, "  -i <addr> Set or add an interface run Finder on\n");
    fprintf(stderr, "  -p <port> Set port to run Finder on\n");
    fprintf(stderr, "  -q <secs> Set forced quit period\n");
    fprintf(stderr, "  -t <dir>  Specify templates directory\n");
    fprintf(stderr, "  -x <dir>  Specify Xrl targets directory\n");
    fprintf(stderr, "  -d        Daemon mode, run in the background\n");
}

static void
display_defaults()
{
    fprintf(stderr, "Defaults:\n");
    fprintf(stderr, "  Boot file                  := %s\n",
	    xorp_boot_file().c_str());
    fprintf(stderr, "  Templates directory        := %s\n",
	    xorp_template_dir().c_str());
    fprintf(stderr, "  Xrl targets directory      := %s\n",
	    xorp_xrl_targets_dir().c_str());
    fprintf(stderr, "  Execute Xrls               := %s\n",
	    bool_c_str(default_do_exec));
    fprintf(stderr, "  Restart failed processes   := %s\n",
	    bool_c_str(default_do_restart));
    fprintf(stderr, "  Print verbose information  := %s\n",
	    bool_c_str(default_verbose));
}

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


Rtrmgr::Rtrmgr(const string& template_dir, 
	       const string& xrl_targets_dir,
	       const string& boot_file,
	       const list<IPv4>& bind_addrs,
	       uint16_t bind_port,
	       bool	do_exec,
	       bool	do_restart,
	       bool	verbose,
	       int32_t quit_time,
	       bool daemon_mode)
    : _template_dir(template_dir),
      _xrl_targets_dir(xrl_targets_dir),
      _boot_file(boot_file),
      _bind_addrs(bind_addrs),
      _bind_port(bind_port),
      _do_exec(do_exec),
      _do_restart(do_restart),
      _verbose(verbose),
      _quit_time(quit_time),
      _daemon_mode(daemon_mode),
      _ready(false),
      _mct(NULL)
{
}

int
Rtrmgr::run()
{
    int errcode = 0;
    string errmsg;

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
    // Print various information
    //
    XLOG_TRACE(_verbose, "Boot file                  := %s\n",
	       boot_file.c_str());
    XLOG_TRACE(_verbose, "Templates directory        := %s\n",
	       template_dir.c_str());
    XLOG_TRACE(_verbose, "Xrl targets directory      := %s\n",
	       xrl_targets_dir.c_str());
    XLOG_TRACE(_verbose, "Execute Xrls               := %s\n",
	       bool_c_str(do_exec));
    XLOG_TRACE(_verbose, "Restart failed processes   := %s\n",
	       bool_c_str(do_restart));
    XLOG_TRACE(_verbose, "Print verbose information  := %s\n",
	       bool_c_str(_verbose));


    XRLdb* xrldb = NULL;
    try {
	xrldb = new XRLdb(_xrl_targets_dir, _verbose);
    } catch (const InitError& e) {
	XLOG_ERROR("Shutting down due to an init error: %s", e.why().c_str());
	return (1);
    }

    //
    // Read the router config template files
    //
    MasterTemplateTree* tt = new MasterTemplateTree(xorp_config_root_dir(),
						    *xrldb, _verbose);
    if (!tt->load_template_tree(_template_dir, errmsg)) {
	XLOG_ERROR("Shutting down due to an init error: %s", errmsg.c_str());
	return (1);
    }
    debug_msg("%s", tt->tree_str().c_str());

    //
    // Start the finder and the rest of the rtrmgr components.
    // These are dynamically created so we have control over the
    // deletion order.
    //
    FinderServer* fs = NULL;
    try {
	fs = new FinderServer(eventloop,
			      FinderConstants::FINDER_DEFAULT_HOST(),
			      _bind_port);
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
    ModuleManager mmgr(eventloop, *this, _do_restart, _verbose,
		       xorp_binary_root_dir());

    try {
	//
	// Read the router startup configuration file,
	// start the processes required, and initialize them.
	//
	RandomGen randgen;
	UserDB userdb(_verbose);

	userdb.load_password_file();
	_xrt = new XrlRtrmgrInterface(xrl_router, userdb, eventloop, 
				      randgen, *this);

	wait_until_xrl_router_is_ready(eventloop, xrl_router);

#if 0
	// Let the module manager know how to send XRLs to xorpsh
	mmgr.set_xrl_interface(_xrt);
#endif

	_mct = new MasterConfigTree(boot_file, tt, mmgr, xclient, _do_exec,
				    _verbose);
	if (_daemon_mode) {
	    _mct->set_task_completed(callback(this, &Rtrmgr::daemonize));
	}
	//
	// XXX: note that theoretically we may receive an XRL before
	// we call XrlRtrmgrInterface::set_master_config_tree()
	// or ModuleManager::set_master_config_tree() below.
	// For now we ignore that possibility...
	//
	_xrt->set_master_config_tree(_mct);
	mmgr.set_master_config_tree(_mct);

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
	while (eventloop.events_pending() && (_mct->commit_in_progress())) {
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
    while ((mmgr.is_shutdown_completed() != true)
	   && eventloop.events_pending()) {
	eventloop.run();
    }

    // Delete the template tree
    delete tt;

    // Delete the XRLdb
    delete xrldb;

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

void
Rtrmgr::module_status_changed(const string& module_name,
			      GenericModule::ModuleStatus status)
{
    _xrt->module_status_changed(module_name, status);
}

void
open_logfile()
{
    if (logfilename.empty()) {
	fprintf(stderr, "Empty log filename specified\n");
	return;
    }
    FILE* logfile;
    if ((logfile = fopen(logfilename.c_str(), "a")) != NULL) {
       xlog_add_output(logfile);
    } else {
	fprintf(stderr, "Failed to open log file %s\n", logfilename.c_str());
    }
}

void
open_syslog()
{
    if (syslogspec.empty()) {
	fprintf(stderr, "Empty syslog spec\n");
	return;
    }
    int retval = xlog_add_syslog_output(syslogspec.c_str());
    if (retval == -1)
	fprintf(stderr, "Failed to open syslog spec %s\n", logfilename.c_str());
}

void
write_pidfile(pid_t thepid)
{
    if (pidfilename.empty()) {
	fprintf(stderr, "Empty PID filename specified\n");
	return;
    }
    FILE *pidfile;
    if ((pidfile = fopen(pidfilename.c_str(), "w")) != NULL) {
	fprintf(pidfile, "%u\n", thepid);
	fclose(pidfile);
    } else {
	fprintf(stderr, "Failed to write pid file\n");
    }
}

void
Rtrmgr::daemonize()
{
    // If not daemonizing, do nothing.
    if (! daemon_mode)
	return;

#ifndef HOST_OS_WINDOWS
    // Daemonize the XORP process. Close open stdio descriptors,
    // but don't chdir -- we need to stay where we're told to go.
    int newpid = xorp_daemonize(DAEMON_NOCHDIR, DAEMON_CLOSE);
    if (newpid == -1) {
	fprintf(stderr, "Failed to start as a daemon process\n");
	cleanup_and_exit(1);
    }

    // Make sure we open the pid file in the parent, and the
    // log file in the child. Close fds but don't chdir.
    if (newpid == 0) {
	// We are now in the child.
#if 0
	if (do_logfile)
	    open_logfile();
#endif
	return;
    }

    // We are in the parent. Write the PID file and exit.
    if (do_pidfile)
	write_pidfile(newpid);
    _exit(0);
#endif
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
    xrl_targets_dir	= xorp_xrl_targets_dir();
    boot_file		= xorp_boot_file();

    int c;
    while ((c = getopt(argc, argv, "da:l:L:n:t:b:x:i:P:p:q:Nrvh")) != EOF) {
	switch(c) {
	case 'd':
	    daemon_mode = true;
	    break;
	case 'a':
	    //
	    // User is specifying an IPv4 address to accept finder
	    // connections from.
	    //
	    try {
		add_permitted_host(IPv4(optarg));
	    } catch (const InvalidString&) {
		fprintf(stderr, "%s is not a valid IPv4 address.\n", optarg);
		usage(argv[0]);
		cleanup_and_exit(1);
	    }
	    break;
	case 'l':
	    do_logfile = true;
	    logfilename = optarg;
	    break;
	case 'L':
	    do_syslog = true;
	    syslogspec = optarg;
	    break;
	case 'n':
	    //
	    // User is specifying a network address to accept finder
	    // connections from.
	    //
	    try {
		add_permitted_net(IPv4Net(optarg));
	    } catch (const InvalidString&) {
		fprintf(stderr, "%s is not a valid IPv4 network.\n", optarg);
		usage(argv[0]);
		cleanup_and_exit(1);
	    }
	    break;
	case 't':
	    template_dir = optarg;
	    break;
	case 'b':
	    boot_file = optarg;
	    break;
	case 'x':
	    xrl_targets_dir = optarg;
	    break;
	case 'q':
	    quit_time = atoi(optarg);
	    break;
	case 'N':
	    do_exec = false;
	    break;
	case 'r':
	    //
	    // Enable failed processes to be restarted by the rtrmgr.
	    // Note: this option is not recommended, because it does
	    // not work properly. E.g., if a process fails during
	    // reconfiguration via xorpsh, then the rtrmgr itself may
	    // coredump.
	    //
	    do_restart = true;
	    break;
	case 'v':
	    verbose = true;
	    break;
	case 'P':
	    do_pidfile = true;
	    pidfilename = optarg;
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
		in_addr ina;
		bind_addr.copy_out(ina);
		if (is_ip_configured(ina) == false) {
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

    // If not daemonizing, open the pid file now.
    if (! daemon_mode) {
	if (do_pidfile)
	    write_pidfile(getpid());
    }

    // Open the new log facility now so that all output, up to when we
    // daemonize, will go to the specified log facility.
    if (do_logfile || do_syslog)
        xlog_remove_default_output();
    if (do_logfile)
	open_logfile();
    if (do_syslog)
	open_syslog();

    //
    // The main procedure
    //
    Rtrmgr rtrmgr(template_dir, xrl_targets_dir, boot_file, bind_addrs,
		  bind_port, do_exec, do_restart, verbose, quit_time,
		  daemon_mode);
    errcode = rtrmgr.run();

    cleanup_and_exit(errcode);
}

void
cleanup_and_exit(int errcode) 
{
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    exit(errcode);
}
