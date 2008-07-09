// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/rtrmgr/xorpsh_main.cc,v 1.69 2007/10/13 01:35:03 pavlin Exp $"

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/utils.hh"
#ifdef HOST_OS_WINDOWS
#include "libxorp/win_io.h"
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#include <signal.h>

#include "libcomm/comm_api.h"

#include "cli.hh"
#include "op_commands.hh"
#include "slave_conf_tree.hh"
#include "template_commands.hh"
#include "template_tree.hh"
#include "template_tree_node.hh"
#include "util.hh"
#include "xorpsh_main.hh"


#ifdef HOST_OS_WINDOWS
#define FILENO(x) ((HANDLE)_get_osfhandle(_fileno(x)))
#else
#define FILENO(x) fileno(x)
#endif

//
// Default values
//
static bool default_verbose = false;

//
// Local state
//
static bool	is_interrupted = false;
static bool	is_user_exited = false;
static bool	verbose = default_verbose;

static void signal_handler(int signal_value);
static void exit_handler(CliClient*);


static void
announce_waiting()
{
    fprintf(stderr, "Waiting for xorp_rtrmgr...\n");
}

static bool
wait_for_xrl_router_ready(EventLoop& eventloop, XrlRouter& xrl_router,
			  bool exit_on_error)
{
    XorpTimer announcer = eventloop.new_oneoff_after_ms(
				3 * 1000, callback(&announce_waiting)
				);
    while (xrl_router.ready() == false) {
	eventloop.run();
	if (xrl_router.failed()) {
	    XLOG_ERROR("XrlRouter failed.  No Finder?");
	    return false;
	    break;
	}
	if (xrl_router.connect_failed() && exit_on_error)
	    return false;
    }
    return true;
}

// the following two functions are an ugly hack to cause the C code in
// the parser to call methods on the right version of the TemplateTree

void
add_cmd_adaptor(char *cmd, TemplateTree* tt) throw (ParseError)
{
    tt->add_cmd(cmd);
}


void
add_cmd_action_adaptor(const string& cmd, const list<string>& action,
		       TemplateTree* tt) throw (ParseError)
{
    tt->add_cmd_action(cmd, action);
}

// ----------------------------------------------------------------------------
// XorpShell implementation

XorpShell::XorpShell(EventLoop& eventloop,
		     const string& IPCname,
		     const string& xorp_root_dir,
		     const string& config_template_dir,
		     const string& xrl_targets_dir,
		     bool verbose) throw (InitError)
    : XrlStdRouter(eventloop, IPCname.c_str()),
      _eventloop(eventloop),
      _xrl_router(*this),
      _xclient(_eventloop, _xrl_router),
      _rtrmgr_client(&_xrl_router),
      _mmgr(_eventloop),
      _is_connected_to_finder(false),
      _tt(NULL),
      _ct(NULL),
      _ocl(NULL),
      _cli_node(AF_INET, XORP_MODULE_CLI, _eventloop),
      _router_cli(NULL),
      _xorp_root_dir(xorp_root_dir),
      _verbose(verbose),
      _ipc_name(IPCname),
      _got_config(false),
      _got_modules(false),
      _mode(MODE_INITIALIZING),
      _xorpsh_interface(&_xrl_router, *this)
{
    string error_msg;

    //
    // Print various information
    //
    XLOG_TRACE(_verbose, "XORP root directory        := %s\n",
	       xorp_root_dir.c_str());
    XLOG_TRACE(_verbose, "Templates directory        := %s\n",
	       config_template_dir.c_str());
    XLOG_TRACE(_verbose, "Xrl targets directory      := %s\n",
	       xrl_targets_dir.c_str());
    XLOG_TRACE(_verbose, "Print verbose information  := %s\n",
	       bool_c_str(_verbose));

    // Read the router config template files
    _tt = new TemplateTree(xorp_root_dir, _verbose);
    if (!_tt->load_template_tree(config_template_dir, error_msg)) {
	xorp_throw(InitError, error_msg);
    }

    debug_msg("%s", _tt->tree_str().c_str());

    // Read the router operational template files
    try {
	_ocl = new OpCommandList(config_template_dir.c_str(), _tt, _mmgr);
    } catch (const InitError& e) {
	xorp_throw(InitError, e.why());
    }
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

#ifndef HOST_OS_WINDOWS
    // Close the opened file descriptors
    size_t i;
    for (i = 0; i < sizeof(_fddesc) / sizeof(_fddesc[0]); i++) {
	if (_fddesc[i].is_valid()) {
	    close(_fddesc[i]);
	    _fddesc[i].clear();
	}
    }
#endif // ! HOST_OS_WINDOWS
}

void
XorpShell::run(const string& commands, bool exit_on_error)
{
    bool success;
    string error_msg;
    XorpFd xorpsh_input_fd;
    XorpFd xorpsh_output_fd;
    XorpFd xorpsh_write_commands_fd;

    if (commands.empty()) {
	// Accept commands from the stdin
	xorpsh_input_fd = FILENO(stdin);
	xorpsh_output_fd = FILENO(stdout);
    }
#ifndef HOST_OS_WINDOWS
    else {
	//
	// Create an internal pipe to pass the commands to the CLI
	//
	int pipedesc[2];

	if (pipe(pipedesc) != 0) {
	    error_msg = c_format("Cannot create an internal pipe: %s",
				 strerror(errno));
	    xorp_throw(InitError, error_msg);
	}
	// xorpsh_write_commands_fd = pipedesc[1];
	_fddesc[0] = xorpsh_input_fd = pipedesc[0];
	_fddesc[1] = xorpsh_write_commands_fd = pipedesc[1];
	xorpsh_output_fd = FILENO(stdout);
    }
#endif // ! HOST_OS_WINDOWS

    // Signal handlers so we can clean up when we're killed
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
#ifdef SIGPIPE
    signal(SIGPIPE, signal_handler);
#endif

    // Set the callback when the CLI exits (e.g., after Ctrl-D)
    _cli_node.set_cli_client_delete_callback(callback(exit_handler));

    _is_connected_to_finder = false;
    if (wait_for_xrl_router_ready(_eventloop, _xrl_router, exit_on_error)
	== false) {
	// RtrMgr contains finder
	error_msg = c_format("Failed to connect to the router manager");
	xorp_throw(InitError, error_msg);
    }
    _is_connected_to_finder = true;

#ifdef HOST_OS_WINDOWS
    const uint32_t uid = 0;
#else
    const uint32_t uid = getuid();
#endif
    success = _rtrmgr_client.send_register_client("rtrmgr", uid, _ipc_name,
						  callback(this,
							   &XorpShell::register_done));
    if (! success) {
	error_msg = c_format("Failed to send a registration request to the "
			     "router manager");
	xorp_throw(InitError, error_msg);
    }
    _mode = MODE_AUTHENTICATING;
    while (_authfile.empty()) {
	_eventloop.run();
    }

    XLOG_TRACE(_verbose, "Registry with rtrmgr successful: token is %s\n",
	       _authfile.c_str());


#ifdef NO_XORPSH_AUTHENTICATION
    // XXX: a hack to access the rtrmgr on a remote machine
    _authtoken = _authfile;
#else // ! NO_XORPSH_AUTHENTICATION
    FILE* file = fopen(_authfile.c_str(), "r");
    if (file == NULL) {
	XLOG_FATAL("Failed to open authfile %s", _authfile.c_str());
    }
    char buf[256];
    memset(buf, 0, sizeof(buf));
    if (fgets(buf, sizeof(buf) - 1, file) == 0) {
	fclose(file);
	XLOG_FATAL("Failed to read authfile %s", _authfile.c_str());
    }
    fclose(file);
#ifdef HOST_OS_WINDOWS
    (void)DeleteFileA(_authfile.c_str());	// XXX
#else
    if (unlink(_authfile.c_str()) != 0) {
	XLOG_WARNING("xorpsh is unable to unlink temporary file %s",
		     _authfile.c_str());
    }
#endif // ! HOST_OS_WINDOWS
    _authtoken = buf;
#endif // ! NO_XORPSH_AUTHENTICATION

    XLOG_TRACE(_verbose, "authtoken = >%s<\n", _authtoken.c_str());

    _xrl_generic_done = false;
    success = _rtrmgr_client.send_authenticate_client("rtrmgr", uid, _ipc_name,
						      _authtoken,
						      callback(this,
							       &XorpShell::generic_done));
    if (! success) {
	error_msg = c_format("Failed to send an authentication request to the "
			     "router manager");
	xorp_throw(InitError, error_msg);
    }

    while (!_xrl_generic_done) {
	_eventloop.run();
    }

    _mode = MODE_INITIALIZING;
    _configuration = "";
    _got_config = false;

    //
    // We wait now to receive the configuration and list of running
    // modules from the rtrmgr.  We don't have to ask for these - we
    // just get them automatically as a result of authenticating.
    //

    XLOG_TRACE(_verbose, "Waiting to receive configuration from rtrmgr...");
    while (_got_config == false) {
	_eventloop.run();
    }

    XLOG_TRACE(_verbose,
	       "==========================================================\n");
    XLOG_TRACE(_verbose, "\n%s", _configuration.c_str());
    XLOG_TRACE(_verbose,
	       "==========================================================\n");

    try {
	_ct = new SlaveConfigTree(_configuration, _tt, _xclient, _clientid,
				  _verbose);

	_ocl->set_slave_config_tree(_ct);

	// Start up the CLI
	_cli_node.enable();
	_router_cli = new RouterCLI(*this, _cli_node, xorpsh_input_fd,
				    xorpsh_output_fd, _verbose);
    } catch (const InitError& e) {
	error_msg = c_format("Shutting down due to a parse error: %s",
			     e.why().c_str());

	_xrl_generic_done = false;
	success = _rtrmgr_client.send_unregister_client(
	    "rtrmgr",
	    _authtoken,
	    callback(this, &XorpShell::generic_done));
	_mode = MODE_SHUTDOWN;
	// Run the event loop to cause the unregister to be sent
	while (success && ! _xrl_generic_done) {
	    _eventloop.run();
	}
	xorp_throw(InitError, error_msg);
    }

    //
    // Write the commands to one end of the pipe
    //
    if (xorpsh_write_commands_fd.is_valid()) {
	string modified_commands = commands;
	if (! modified_commands.empty()) {
	    if (modified_commands[modified_commands.size() - 1] != '\n')
		modified_commands += "\n";
#ifdef HOST_OS_WINDOWS
	    DWORD written;
	    WriteFile(xorpsh_write_commands_fd, modified_commands.c_str(),
		  modified_commands.size(), &written, NULL);
	    CloseHandle(xorpsh_write_commands_fd);
#else // ! HOST_OS_WINDOWS
	    write(xorpsh_write_commands_fd, modified_commands.c_str(),
		  modified_commands.size());
	    close(xorpsh_write_commands_fd);
#endif // ! HOST_OS_WINDOWS
	    xorpsh_write_commands_fd.clear();
	}
    }

    _mode = MODE_IDLE;
    while ((! is_interrupted) && (! is_user_exited)) {
	_eventloop.run();
    }
    while (! done()) {
	_eventloop.run();
    }

    _xrl_generic_done = false;
    success = _rtrmgr_client.send_unregister_client(
	"rtrmgr",
	_authtoken,
	callback(this, &XorpShell::generic_done));
    _mode = MODE_SHUTDOWN;
    // Run the event loop to cause the unregister to be sent
    while (success && ! _xrl_generic_done) {
	_eventloop.run();
    }
}

bool
XorpShell::done() const
{
    return (_xrl_generic_done && _ocl->done() && _router_cli->done());
}

void
XorpShell::register_done(const XrlError& e, const string* file,
			 const uint32_t* pid, const uint32_t* clientid)
{
    if (e == XrlError::OKAY()) {
	_authfile = *file;
	_rtrmgr_pid = *pid;
	_clientid = *clientid;
	XLOG_TRACE(_verbose, "rtrmgr PID=%u\n", XORP_UINT_CAST(_rtrmgr_pid));
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
	_xrl_generic_done = true;
	return;
    }

    if ((e == XrlError::COMMAND_FAILED()) && (e.note() == "AUTH_FAIL")) {
	fprintf(stderr, "Authentication Failure\n");
    } else {
	fprintf(stderr, "XRL failed\n");
	fprintf(stderr, "%s\n", e.str().c_str());
    }
    exit(1);
}

#if 0
bool
XorpShell::request_config()
{
    return (_rtrmgr_client.send_get_running_config("rtrmgr", _authtoken,
						   callback(this, &XorpShell::receive_config))
	    == true);
}

void
XorpShell::receive_config(const XrlError& e, const bool* ready,
			  const string* config)
{
    if (e == XrlError::OKAY()) {
	if (*ready) {
	    _configuration = *config;
	    _got_config = true;
	    return;
	} else {
	    /* the rtrmgr is not ready to pass us a config - typically
               this is because it is in the process of reconfiguring */
	    _repeat_request_timer = 
		eventloop().new_oneoff_after_ms(1000,
                              callback(this, &XorpShell::request_config));
	    return;
	}
    }

    if ((e == XrlError::COMMAND_FAILED()) && (e.note() == "AUTH_FAIL")) {
	fprintf(stderr, "Authentication Failure\n");
    } else {
	fprintf(stderr, "Failed to register with router manager process\n");
	fprintf(stderr, "%s\n", e.str().c_str());
    }
    exit(1);
}
#endif // 0

bool
XorpShell::lock_config(LOCK_CALLBACK cb)
{
    // Lock for 60 seconds - this should be sufficient
    return (_rtrmgr_client.send_lock_config("rtrmgr", _authtoken, 60000, cb)
	    == true);
}

bool
XorpShell::commit_changes(const string& deltas, const string& deletions,
			  GENERIC_CALLBACK cb, CallBack final_cb)
{
    _commit_callback = final_cb;
    if (_rtrmgr_client.send_apply_config_change("rtrmgr", _authtoken,
						_ipc_name, deltas, deletions,
						cb)
	!= true) {
	return (false);
    }
    return (true);
}

void
XorpShell::config_saved_done(bool success, const string& error_msg)
{
    // Call unlock_config. The callback from unlock will finally clear
    // things up.
    _ct->save_phase4(success, error_msg, _config_save_callback, this);
}

void
XorpShell::commit_done(bool success, const string& error_msg)
{
    // Call unlock_config. The callback from unlock will finally clear
    // things up.
    _ct->commit_phase4(success, error_msg, _commit_callback, this);
}

bool
XorpShell::unlock_config(GENERIC_CALLBACK cb)
{
    return (_rtrmgr_client.send_unlock_config("rtrmgr", _authtoken, cb)
	    == true);
}

bool
XorpShell::enter_config_mode(bool exclusive, GENERIC_CALLBACK cb)
{
    return (_rtrmgr_client.send_enter_config_mode("rtrmgr", _authtoken,
						  exclusive, cb)
	    == true);
}

bool
XorpShell::leave_config_mode(GENERIC_CALLBACK cb)
{
    return (_rtrmgr_client.send_leave_config_mode("rtrmgr", _authtoken, cb)
	    == true);
}

bool
XorpShell::get_config_users(GET_USERS_CALLBACK cb)
{
    return (_rtrmgr_client.send_get_config_users("rtrmgr", _authtoken, cb)
	    == true);
}

void
XorpShell::new_config_user(uid_t user_id)
{
    _router_cli->new_config_user(user_id);
}

bool
XorpShell::save_to_file(const string& filename, GENERIC_CALLBACK cb,
			CallBack final_cb)
{
    _config_save_callback = final_cb;
    LOCK_CALLBACK locked_cb = callback(this,
				       &XorpShell::save_lock_achieved,
				       filename,
				       cb);
    if (_rtrmgr_client.send_lock_config("rtrmgr", _authtoken, 60000, locked_cb)
	!= true) {
	return (false);
    }

    return (true);
}

void
XorpShell::save_lock_achieved(const XrlError& e, const bool* locked,
			      const uint32_t* /* lock_holder */,
			      const string filename,
			      GENERIC_CALLBACK cb)
{
    string error_msg;

    if (!locked || (e != XrlError::OKAY())) {
	error_msg = c_format("Failed to get configuration lock");
	_config_save_callback->dispatch(false, error_msg);
	return;
    }

    if (_rtrmgr_client.send_save_config("rtrmgr", _authtoken, _ipc_name,
					filename, cb)
	!= true) {
	error_msg = c_format("Failed to send configuration. No Finder?");
	_config_save_callback->dispatch(false, error_msg);
	return;
    }
}

bool
XorpShell::load_from_file(const string& filename, GENERIC_CALLBACK cb,
			  CallBack final_cb)
{
    _commit_callback = final_cb;
    LOCK_CALLBACK locked_cb = callback(this,
				       &XorpShell::load_lock_achieved,
				       filename,
				       cb);
    if (_rtrmgr_client.send_lock_config("rtrmgr", _authtoken, 60000, locked_cb)
	!= true) {
	return (false);
    }

    return (true);
}

void
XorpShell::load_lock_achieved(const XrlError& e, const bool* locked,
			      const uint32_t* /* lock_holder */,
			      const string filename,
			      GENERIC_CALLBACK cb)
{
    string error_msg;

    if (!locked || (e != XrlError::OKAY())) {
	error_msg = c_format("Failed to get configuration lock");
	_commit_callback->dispatch(false, error_msg);
	return;
    }

    if (_rtrmgr_client.send_load_config("rtrmgr", _authtoken, _ipc_name,
					filename, cb)
	!= true) {
	error_msg = c_format("Failed to load the configuration. No Finder?");
	_commit_callback->dispatch(false, error_msg);
	return;
    }
}

void
XorpShell::config_changed(uid_t user_id, const string& deltas,
			  const string& deletions)
{
#if 0
    if (_mode == MODE_COMMITTING) {
	// This is the response back to our own request
	return;
    }
#endif
    if (_mode == MODE_INITIALIZING) {
	// We were just starting up
	XLOG_ASSERT(deletions == "");
	_configuration = deltas;
	_got_config = true;
	return;
    }

    string response;
    if (!_ct->apply_deltas(user_id, deltas,
			   false /* this is not a provisional change */,
			   true /* preserve_node_id */,
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

    if (_mode == MODE_LOADING) {
	// No need to notify, as the change was caused by us.
	return;
    }

#ifdef HOST_OS_WINDOWS
    string username("root");
#else // ! HOST_OS_WINDOWS
    // Notify the user that the config changed
    struct passwd *pwent = getpwuid(user_id);
    string username;
    if (pwent == NULL)
	username = c_format("UID:%u", XORP_UINT_CAST(user_id));
    else
	username = pwent->pw_name;
#endif // ! HOST_OS_WINDOWS


    if (_mode == MODE_COMMITTING) {
	// This is the response back to our own request
	return;
    }

    //
    // Print the configuration change alert
    //
    string alert = "The configuration has been changed by user " +
	username + "\n";
    if (! deltas.empty()) {
	alert += "DELTAS:\n";
	SlaveConfigTree sct(deltas, _tt, _xclient, _clientid, false);
	alert += sct.show_tree(false);
    }
    if (! deletions.empty()) {
	alert += "DELETIONS:\n";
	SlaveConfigTree sct(deletions, _tt, _xclient, _clientid, false);
	alert += sct.show_tree(false);
    }
    _router_cli->notify_user(alert, true);
    _router_cli->config_changed_by_other_user();
}

void 
XorpShell::module_status_change(const string& module_name, 
				GenericModule::ModuleStatus status)
{
    string error_msg;

    debug_msg("Module status change: %s status %d\n",
	      module_name.c_str(), XORP_INT_CAST(status));
    GenericModule *module = _mmgr.find_module(module_name);
    if (module == NULL) {
	module = _mmgr.new_module(module_name, error_msg);
    }
    XLOG_ASSERT(module != NULL);
    module->new_status(status);
}

bool
XorpShell::get_rtrmgr_pid(PID_CALLBACK cb)
{
    return (_rtrmgr_client.send_get_pid("rtrmgr", cb) == true);
}

/**
 * Called when Finder connection is established.
 *
 * Note that this method overwrites an XrlRouter virtual method.
 */
void
XorpShell::finder_connect_event()
{
    //
    // XXX: nothing to do, because on startup we wait until the finder
    // is ready.
    //
}

/**
 * Called when Finder disconnect occurs.
 *
 * Note that this method overwrites an XrlRouter virtual method.
 */
void
XorpShell::finder_disconnect_event()
{
    if (_is_connected_to_finder)
	fprintf(stderr, "Finder disconnected. No Finder?\n");
    _is_connected_to_finder = false;
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
#ifdef SIGPIPE
    case SIGPIPE:
	// Ignore SIGPIPE: it may be generated when executing commands
	// specified on the command line.
	break;
#endif
    default:
	// XXX: anything else we have intercepted will terminate us.
	is_interrupted = true;
	break;
    }
}

static void
exit_handler(CliClient*)
{
    is_user_exited = true;
}

static void
usage(const char *argv0)
{
    fprintf(stderr, "Usage: %s [options]\n", xorp_basename(argv0));
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -c        Specify command(s) to execute\n");
    fprintf(stderr, "  -e        Exit immediately if cannot connect to the rtrmgr\n");
    fprintf(stderr, "  -h        Display this information\n");
    fprintf(stderr, "  -v        Print verbose information\n");
    fprintf(stderr, "  -t <dir>  Specify templates directory\n");
    fprintf(stderr, "  -x <dir>  Specify Xrl targets directory\n");
}

static void
display_defaults()
{
    fprintf(stderr, "Defaults:\n");
    fprintf(stderr, "  Templates directory        := %s\n",
	    xorp_template_dir().c_str());
    fprintf(stderr, "  Xrl targets directory      := %s\n",
	    xorp_xrl_targets_dir().c_str());
    fprintf(stderr, "  Print verbose information  := %s\n",
	    bool_c_str(default_verbose));
}

int
main(int argc, char *argv[])
{
    int errcode = 0;
    string commands;
    bool exit_on_error = false;

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

    comm_init();

    //
    // Install the handler for unexpected exceptions
    //
    XorpUnexpectedHandler eh(xorp_unexpected_handler);

    //
    // Expand the default variables to include the XORP root path
    //
    xorp_path_init(argv[0]);
    string template_dir		= xorp_template_dir();
    string xrl_targets_dir	= xorp_xrl_targets_dir();

    int c;
    while ((c = getopt(argc, argv, "c:et:x:vh")) != EOF) {
	switch(c) {
	case 'c':
	    // XXX: Append the arguments to allow multiple "-c cmd" commands
	    commands += optarg;
	    commands += "\n";
	    break;
	case 'e':
	    exit_on_error = true;
	    break;
	case 't':
	    template_dir = optarg;
	    break;
	case 'x':
	    xrl_targets_dir = optarg;
	    break;
	case 'v':
	    verbose = true;
	    break;
	case '?':
	case 'h':
	    usage(argv[0]);
	    display_defaults();
	    errcode = 1;
	    goto cleanup;
	}
    }

    //
    // Initialize the IPC mechanism.
    // As there can be multiple xorpsh instances, we need to generate a
    // unique name for our xrl_router.
    //
    char hostname[MAXHOSTNAMELEN];
    if (gethostname(hostname, sizeof(hostname)) < 0) {
#ifdef HOST_OS_WINDOWS
	XLOG_FATAL("gethostname() failed: %s", win_strerror(WSAGetLastError()));
#else
	XLOG_FATAL("gethostname() failed: %s", strerror(errno));
#endif
    }
    hostname[sizeof(hostname) - 1] = '\0';

    try {
	EventLoop eventloop;
	string xname = "xorpsh" + c_format("-%d-%s", XORP_INT_CAST(getpid()),
					   hostname);
	XorpShell xorpsh(eventloop, xname, xorp_binary_root_dir(),
			 template_dir, xrl_targets_dir, verbose);
	xorpsh.run(commands, exit_on_error);
    } catch (const InitError& e) {
	XLOG_ERROR("xorpsh exiting due to an init error: %s", e.why().c_str());
	errcode = 1;
	goto cleanup;
    }

 cleanup:

    comm_exit();

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    exit(errcode);
}
