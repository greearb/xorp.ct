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

#ident "$XORP: xorp/rtrmgr/xorpsh_main.cc,v 1.20 2003/12/03 20:27:57 pavlin Exp $"

#include <sys/types.h>
#include <pwd.h>
#include <signal.h>

#include "libxorp/debug.h"

#include "rtrmgr_module.h"
#include "libxorp/xlog.h"

#include "template_tree_node.hh"
#include "template_commands.hh"
#include "template_tree.hh"
#include "op_commands.hh"
#include "slave_conf_tree.hh"
#include "cli.hh"
#include "xorpsh_main.hh"
#include "util.hh"

//
// Defaults
//
static bool running = false;

static void signal_handler(int signal_value);
static void exit_handler(CliClient*);

static void
announce_waiting()
{
    fprintf(stderr, "Waiting for xorp_rtrmgr...\n");
}

static bool
wait_for_xrlrouter_ready(EventLoop& e, XrlRouter& rtr)
{
    bool bad_router = false;
    XorpTimer timeout = e.set_flag_after_ms(10 * 1000, &bad_router);
    XorpTimer announcer = e.new_oneoff_after_ms(3 * 1000,
						callback(&announce_waiting));
    while (rtr.ready() == false) {
	e.run();
	if (bad_router) {
	    return false;
	}
    }
    return true;
}

// ----------------------------------------------------------------------------
// XorpShell implementation

XorpShell::XorpShell(const string& IPCname,
		     const string& xorp_root_dir,
		     const string& config_template_dir,
		     const string& xrl_dir) throw (InitError)
    : _eventloop(),
      _xrlrouter(_eventloop, IPCname.c_str()),
      _xclient(_eventloop, _xrlrouter),
      _rtrmgr_client(&_xrlrouter),
      _xorpsh_interface(&_xrlrouter, *this),
      _tt(NULL),
      _ct(NULL),
      _ocl(NULL),
      _cli_node(AF_INET, XORP_MODULE_CLI, _eventloop),
      _router_cli(NULL),
      _xorp_root_dir(xorp_root_dir),
      _got_config(false),
      _mode(MODE_INITIALIZING)
{
    _ipc_name = IPCname;

    // Read the router config template files
    try {
	_tt = new TemplateTree(xorp_root_dir, config_template_dir, xrl_dir);
    } catch (const InitError& e) {
	xorp_throw(InitError, e.why());
    }
#if DEBUG_STARTUP
    _tt->display_tree();
#endif

    // Read the router operational template files
    try {
	_ocl = new OpCommandList(config_template_dir.c_str(), _tt);
    } catch (const InitError& e) {
	xorp_throw(InitError, e.why());
    }
    _ocl->display_list();
}

XorpShell::~XorpShell()
{
    if (_ct != NULL)
	delete _ct;
    if (_tt != NULL)
	delete _tt;
    if (_ocl != NULL)
	delete _ocl;
    if (_router_cli != NULL)
	delete _router_cli;
}

void
XorpShell::run()
{
    // Signal handlers so we can clean up when we're killed
    running = true;
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    // Set the callback when the CLI exits (e.g., after Ctrl-D)
    _cli_node.set_cli_client_delete_callback(callback(exit_handler));

    if (wait_for_xrlrouter_ready(_eventloop, _xrlrouter) == false) {
	// RtrMgr contains finder
	fprintf(stderr, "Failed to connect to xorp_rtrmgr.\n");
	return;
    }

    const uint32_t uid = getuid();
    _rtrmgr_client.send_register_client("rtrmgr", uid, _ipc_name,
					callback(this,
						 &XorpShell::register_done));

    _mode = MODE_AUTHENTICATING;
    while (_authfile.empty()) {
	_eventloop.run();
    }

#ifdef DEBUG_STARTUP
    printf("Registry with rtrmgr successful: token is %s\n",
	   _authfile.c_str());
#endif

    FILE* file = fopen(_authfile.c_str(), "r");
    if (file == NULL) {
	XLOG_FATAL("Failed to open authfile %s",
		   _authfile.c_str());
    }
    char buf[256];
    memset(buf, 0, sizeof(buf));
    if (fgets(buf, sizeof(buf) - 1, file) == 0) {
	fclose(file);
	XLOG_FATAL("Failed to read authfile %s", _authfile.c_str());
    }
    fclose(file);
    if (unlink(_authfile.c_str()) != 0) {
	XLOG_WARNING("xorpsh is unable to unlink temporary file %s",
		     _authfile.c_str());
    }
    _authtoken = buf;

#ifdef DEBUG_STARTUP
    printf("authtoken = >%s<\n", _authtoken.c_str());
#endif

    _done = false;
    _rtrmgr_client.send_authenticate_client("rtrmgr", uid, _ipc_name,
					    _authtoken,
					    callback(this,
						     &XorpShell::generic_done));
    while (!_done) {
	_eventloop.run();
    }

    _mode = MODE_INITIALIZING;

    _rtrmgr_client.send_get_running_config("rtrmgr", _authtoken,
				callback(this, &XorpShell::receive_config));

    while (_got_config == false) {
#ifdef DEBUG_STARTUP
	printf("+");
	fflush(stdout);
#endif
	_eventloop.run();
    }

#ifdef DEBUG_STARTUP
    printf("==============================================================\n");
    printf("%s", _configuration.c_str());
    printf("==============================================================\n");
#endif

    try {
	_ct = new SlaveConfigTree(_configuration, _tt, _xclient);

	_ocl->set_config_tree(_ct);

	// Start up the CLI
	_cli_node.enable();
	_router_cli = new RouterCLI(*this, _cli_node);
    } catch (const InitError& e) {
	XLOG_ERROR("shutting down due to a parse error: %s", e.why().c_str());
	running = false;
    }

    _mode = MODE_IDLE;
    while (running) {
	_eventloop.run();
    }

    _done = false;
    _rtrmgr_client.send_unregister_client("rtrmgr",_authtoken,
					  callback(this,
						   &XorpShell::generic_done));
    _mode = MODE_SHUTDOWN;
    // We need to return to the event loop to cause the unregister to be sent
    while (!_done) {
	_eventloop.run();
    }
}

void
XorpShell::register_done(const XrlError& e, const string* file,
			 const uint32_t* pid)
{
    if (e == XrlError::OKAY()) {
	_authfile = *file;
	_rtrmgr_pid = *pid;
	printf("rtrmgr PID=%d\n", _rtrmgr_pid);
	return;
    } else {
	fprintf(stderr, "Failed to register with router manager process\n");
	fprintf(stderr, "%s\n", e.str().c_str());
	exit(1);
    }
}

void
XorpShell::generic_done(const XrlError& e)
{
    if (e == XrlError::OKAY()) {
	_done = true;
	return;
    } else if ((e == XrlError::COMMAND_FAILED())
	       && (e.note() == "AUTH_FAIL")) {
	fprintf(stderr, "Authentication Failure\n");
	exit(1);
    } else {
	fprintf(stderr, "XRL failed\n");
	fprintf(stderr, "%s\n", e.str().c_str());
	exit(1);
    }
}

void
XorpShell::receive_config(const XrlError& e, const string* config)
{
    if (e == XrlError::OKAY()) {
	_configuration = *config;
	_got_config = true;
	return;
    } else if ((e == XrlError::COMMAND_FAILED())
	       && (e.note() == "AUTH_FAIL")) {
	fprintf(stderr, "Authentication Failure\n");
	exit(1);
    } else {
	fprintf(stderr, "Failed to register with router manager process\n");
	fprintf(stderr, "%s\n", e.str().c_str());
	exit(1);
    }
}

void
XorpShell::lock_config(LOCK_CALLBACK cb)
{
    // Lock for 30 seconds - this should be overkill
    _rtrmgr_client.send_lock_config("rtrmgr", _authtoken, 30000, cb);
}

void
XorpShell::commit_changes(const string& deltas, const string& deletions,
			  GENERIC_CALLBACK cb, CallBack final_cb)
{
    _commit_callback = final_cb;
    _rtrmgr_client.send_apply_config_change("rtrmgr", _authtoken, _ipc_name,
					    deltas, deletions, cb);
}

void
XorpShell::commit_done(bool success, const string& errmsg)
{
    // Call unlock_config.  The callback from unlock will finally clear
    // things up.
    _ct->commit_phase4(success, errmsg, _commit_callback, this);
}

void
XorpShell::unlock_config(GENERIC_CALLBACK cb)
{
    _rtrmgr_client.send_unlock_config("rtrmgr", _authtoken, cb);
}

void
XorpShell::enter_config_mode(bool exclusive, GENERIC_CALLBACK cb)
{
    _rtrmgr_client.send_enter_config_mode("rtrmgr", _authtoken, exclusive, cb);
}

void
XorpShell::leave_config_mode(GENERIC_CALLBACK cb)
{
    _rtrmgr_client.send_leave_config_mode("rtrmgr", _authtoken, cb);
}

void
XorpShell::get_config_users(GET_USERS_CALLBACK cb)
{
    _rtrmgr_client.send_get_config_users("rtrmgr", _authtoken, cb);
}

void
XorpShell::new_config_user(uid_t user_id)
{
    _router_cli->new_config_user(user_id);
}

void
XorpShell::save_to_file(const string& filename, GENERIC_CALLBACK cb)
{
    _rtrmgr_client.send_save_config("rtrmgr", _authtoken, filename, cb);
}

void
XorpShell::load_from_file(const string& filename, GENERIC_CALLBACK cb,
			  CallBack final_cb)
{
    _commit_callback = final_cb;
    _rtrmgr_client.send_load_config("rtrmgr", _authtoken, _ipc_name,
				    filename, cb);
}

void
XorpShell::config_changed(uid_t user_id, const string& deltas,
			  const string& deletions)
{
    if (_mode == MODE_COMMITTING) {
	// This is the response back to our own request
	return;
    }
    string response;
    if (!_ct->apply_deltas(user_id, deltas,
			   /* this is not a provisional change */ false,
			   response)) {
	_router_cli->notify_user("WARNING: Failed to merge deltas "
				 "from rtrmgr\n",
				 /* urgent */ true);
	_router_cli->notify_user(response, /* urgent */ true);
	// XXX it's not clear we can continue if this happens
    }
    response == "";
    if (!_ct->apply_deletions(user_id, deletions,
			      /* this is not a provisional change */ false,
			      response)) {
	_router_cli->notify_user("WARNING: Failed to merge deletions "
				 "from rtrmgr\n",
				 /* urgent */ true);
	_router_cli->notify_user(response, /* urgent */ true);
	// XXX: it's not clear we can continue if this happens
    }

    // Notify the user that the config changed
    struct passwd *pwent = getpwuid(user_id);
    string username;
    if (pwent == NULL)
	username = c_format("UID:%d", user_id);
    else
	username = pwent->pw_name;

    if (_mode == MODE_LOADING) {
	// No need to notify, as the change was caused by us.
	return;
    }
    string alert = "The configuration had been changed by user " +
	username + "\n";
    _router_cli->notify_user(alert, true);
}

void
XorpShell::get_rtrmgr_pid(PID_CALLBACK cb)
{
    _rtrmgr_client.send_get_pid("rtrmgr", cb);
}

// ----------------------------------------------------------------------------
// main() and it's helpers

static void
signal_handler(int signal_value)
{
    switch (signal_value) {
    case SIGINT:
	// Ignore Ctrl-C: it is used by the CLI to interrupt a command.
	break;
    default:
	// XXX: anything else we have intercepted will terminate us.
	running = false;
	break;
    }
}

static void
exit_handler(CliClient*)
{
    running = false;
}

static void
usage(const char *argv0)
{
    fprintf(stderr, "Usage: %s [options]\n", xorp_basename(argv0));
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h        Display this information\n");
    fprintf(stderr, "  -d        Display defaults\n");
    fprintf(stderr, "  -t <dir>  Specify templates directory\n");
    fprintf(stderr, "  -x <dir>  Specify Xrl targets directory\n");
}

static void
display_defaults()
{
    fprintf(stderr, "Defaults:\n");
    fprintf(stderr, "  Templates directory   := %s\n",
	    xorp_template_dir().c_str());
    fprintf(stderr, "  Xrl targets directory := %s\n",
	    xorp_xrl_targets_dir().c_str());
}

int
main(int argc, char *argv[])
{
    //
    // Initialize and start xlog
    //
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    //
    // Install handler for unexpected exceptions
    //
    XorpUnexpectedHandler eh(xorp_unexpected_handler);

    //
    // Expand the default variables to include the XORP root path
    //
    xorp_path_init(argv[0]);

    string template_dir = xorp_template_dir();
    string xrl_dir	= xorp_xrl_targets_dir();

    int c;
    while ((c = getopt (argc, argv, "t:x:dh")) != EOF) {
	switch(c) {
	case 't':
	    template_dir = optarg;
	    break;
	case 'x':
	    xrl_dir = optarg;
	    break;
	case 'd':
	    display_defaults();
	    break;
	case '?':
	case 'h':
	    usage(argv[0]);
	    display_defaults();
	    xlog_stop();
	    xlog_exit();
	    exit(-1);
	}
    }

    // Initialize the IPC mechanism.
    // As there can be multiple xorpsh instances, we need to generate a
    // unique name for our xrlrouter.
    char hostname[MAXHOSTNAMELEN];
    if (gethostname(hostname, MAXHOSTNAMELEN) < 0) {
	XLOG_FATAL("gethostname failed: %s", strerror(errno));
    }
    hostname[MAXHOSTNAMELEN - 1] = '\0';

    string xname = "xorpsh" + c_format("-%d-%s", getpid(), hostname);

    try {
	XorpShell xorpsh(xname, xorp_binary_root_dir(), template_dir, xrl_dir);
	xorpsh.run();
    } catch (const InitError& e) {
	XLOG_ERROR("xorpsh exiting due to an init error: %s", e.why().c_str());
	exit(1);
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
