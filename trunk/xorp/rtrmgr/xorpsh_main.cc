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

#ident "$XORP: xorp/rtrmgr/xorpsh_main.cc,v 1.7 2003/04/23 21:09:32 mjh Exp $"

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

//
// Defaults
//
static const char* default_config_template_dir = "../etc/templates";
static const char* default_xrl_dir 	       = "../xrl/targets";

static bool running;

static void signalhandler(int) {
    running = false;
}

void
usage(char *name)
{
    fprintf(stderr,
	"usage: %s [-t cfg_dir] [-x xrl_dir]\n",
	    name);
    fprintf(stderr, "options:\n");

    fprintf(stderr, 
	    "\t-t cfg_dir	specify config directory	[ %s ]\n",
	    default_config_template_dir);

    fprintf(stderr, 
	    "\t-x xrl_dir	specify xrl directory		[ %s ]\n",
	    default_xrl_dir);

    exit(-1);
}

XorpShell::XorpShell(const string& IPCname, 
		     const string& config_template_dir, 
		     const string& xrl_dir)
    : _eventloop(), 
    _xrlrouter(_eventloop, IPCname.c_str()),
    _xclient(_eventloop, _xrlrouter),
    _rtrmgr_client(&_xrlrouter),
    _xorpsh_interface(&_xrlrouter, *this),
    _cli_node(AF_INET, XORP_MODULE_CLI, _eventloop),
    _mode(MODE_INITIALIZING)
{
    _ipc_name = IPCname;

    //read the router config template files
    try {
	_tt = new TemplateTree(config_template_dir.c_str(), xrl_dir.c_str());
    } catch (const XorpException&) {
#ifdef DEBUG_STARTUP
	printf("caught exception\n");
#endif
	xorp_unexpected_handler();
    }
#if DEBUG_STARTUP
    _tt->display_tree();
#endif


    //read the router operational template files
    try {
	_ocl = new OpCommandList(config_template_dir.c_str(), _tt);
    } catch (const XorpException&) {
#ifdef DEBUG_STARTUP
	printf("caught exception\n");
#endif
	xorp_unexpected_handler();
    } 
    _ocl->display_list();
}

void 
XorpShell::run() {
    //signal handlers so we can clean up when we're killed
    running = true;
    signal(SIGTERM, signalhandler);
    signal(SIGINT, signalhandler);


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

    FILE *file = fopen(_authfile.c_str(), "r");
    if (file==NULL) {
	XLOG_FATAL("Failed to open authfile %s",
		   _authfile.c_str());
    }
    char buf[256];
    memset(buf, 0, 256);
    if (fgets(buf, 255, file)==0) {
	fclose(file);
	XLOG_FATAL("Failed to read authfile %s",
		   _authfile.c_str());
    }
    fclose(file);
    if (unlink(_authfile.c_str())!=0) {
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

    while (_configuration.empty()) {
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
    
    _ct = new SlaveConfigTree(_configuration, _tt, _xclient);
    _ocl->set_config_tree(_ct);

    //start up the CLI
    _cli_node.enable();
    _router_cli = new RouterCLI(*this, _cli_node);


    _mode = MODE_IDLE;

    while (running) {
	_eventloop.run();
    }

    _done = false;
    _rtrmgr_client.send_unregister_client("rtrmgr",_authtoken, 
					  callback(this, 
						   &XorpShell::generic_done));
    _mode = MODE_SHUTDOWN;
    //we need to return to the event loop to cause the unregister to be sent
    while (!_done) {
	_eventloop.run();
    }
}

XorpShell::~XorpShell() {
    delete _ct;
    delete _tt;
    delete _ocl;
    delete _router_cli;
}

void 
XorpShell::register_done(const XrlError& e, const string* file, 
			 const uint32_t* pid) {
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
XorpShell::generic_done(const XrlError& e) {
    if (e == XrlError::OKAY()) {
	_done = true;
	return;
    } else if ((e == XrlError::COMMAND_FAILED()) 
	       && (e.note()=="AUTH_FAIL")) {
	fprintf(stderr, "Authentication Failure\n");
	exit(1);
    } else {
	fprintf(stderr, "XRL failed\n");
	fprintf(stderr, "%s\n", e.str().c_str());
	exit(1);
    }
}

void 
XorpShell::receive_config(const XrlError& e, const string* config) {
    if (e == XrlError::OKAY()) {
	_configuration = *config;
	return;
    } else if ((e == XrlError::COMMAND_FAILED()) 
	       && (e.note()=="AUTH_FAIL")) {
	fprintf(stderr, "Authentication Failure\n");
	exit(1);
    } else {
	fprintf(stderr, "Failed to register with router manager process\n");
	fprintf(stderr, "%s\n", e.str().c_str());
	exit(1);
    }
}


void
XorpShell::lock_config(LOCK_CALLBACK cb) {
    //lock for 30 seconds - this should be overkill
    _rtrmgr_client.send_lock_config("rtrmgr", _authtoken, 30000, cb);
}

void
XorpShell::commit_changes(const string& deltas, const string& deletions,
			  GENERIC_CALLBACK cb,
			  CallBack final_cb) {
    _commit_callback = final_cb;
    _rtrmgr_client.send_apply_config_change("rtrmgr", _authtoken, 
					    _ipc_name,
					    deltas, deletions, cb);
}

void 
XorpShell::commit_done(bool success, const string& errmsg) {
    //Call unlock_config.  The callback from unlock will finally clear
    //things up.
    _ct->commit_phase4(success, errmsg, _commit_callback, this);
}

void
XorpShell::unlock_config(GENERIC_CALLBACK cb) {
    _rtrmgr_client.send_unlock_config("rtrmgr", _authtoken, cb);
}

void
XorpShell::enter_config_mode(bool exclusive, GENERIC_CALLBACK cb) {
    _rtrmgr_client.send_enter_config_mode("rtrmgr", _authtoken, exclusive, cb);
}

void 
XorpShell::leave_config_mode(GENERIC_CALLBACK cb) {
    _rtrmgr_client.send_leave_config_mode("rtrmgr", _authtoken, cb);
}

void
XorpShell::get_config_users(GET_USERS_CALLBACK cb) {
    _rtrmgr_client.send_get_config_users("rtrmgr", _authtoken, cb);
}

void 
XorpShell::new_config_user(uid_t user_id) {
    _router_cli->new_config_user(user_id);
}

void 
XorpShell::save_to_file(const string& filename, GENERIC_CALLBACK cb) {
    _rtrmgr_client.send_save_config("rtrmgr", _authtoken, filename, cb);
}

void 
XorpShell::load_from_file(const string& filename, GENERIC_CALLBACK cb,
			  CallBack final_cb) {
    _commit_callback = final_cb;
    _rtrmgr_client.send_load_config("rtrmgr", _authtoken, _ipc_name,
				    filename, cb);
}

void 
XorpShell::config_changed(uid_t user_id, const string& deltas, 
			  const string& deletions) {
    if (_mode == MODE_COMMITTING) {
	/*this is the response back to our own request*/
	return;
    }
    string response;
    if (!_ct->apply_deltas(user_id, deltas, 
			   /*this is not a provisional change*/false, 
			   response)) {
	_router_cli->
	    notify_user("WARNING: Failed to merge deltas from rtrmgr\n", 
			/*urgent*/true);
	_router_cli->notify_user(response, /*urgent*/true);
	//XXX it's not clear we can continue if this happens
    }
    response == "";
    if (!_ct->apply_deletions(user_id, deletions, 
			      /*this is not a provisional change*/false, 
			      response)) {
	_router_cli->
	    notify_user("WARNING: Failed to merge deletions from rtrmgr\n", 
			/*urgent*/true);
	_router_cli->notify_user(response, /*urgent*/true);
	//XXX it's not clear we can continue if this happens
    }
    
    //notfiy the user that the config changed
    struct passwd *pwent = getpwuid(user_id);
    string username;
    if (pwent == NULL)
	username = c_format("UID:%d", user_id);
    else
	username = pwent->pw_name;

    if (_mode == MODE_LOADING) {
	//no need to notify, as the change was caused by us.
	return;
    }
    string alert = "The configuration had been changed by user " +
			    username + "\n";
    _router_cli->notify_user(alert, true);
}

int main(int argc, char *argv[]) {

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

    int c;
    string config_template_dir = default_config_template_dir;
    string xrl_dir = default_xrl_dir;
    string configuration;

    while ((c = getopt (argc, argv, "t:x")) != EOF) {
	switch(c) {  
	case 't':
	    config_template_dir = optarg;
	    break;
	case 'x':
	    xrl_dir = optarg;
	    break;
	case '?':
	    usage(argv[0]);
	}
    }

    //Initialize the IPC mechanism.
    //As there can be multiple xorpsh instances, we need to generate a
    //unique name for our xrlrouter.
    char hostname[MAXHOSTNAMELEN];
    if (gethostname(hostname, MAXHOSTNAMELEN) < 0) {
	XLOG_FATAL("gethostname failed: %s", strerror(errno));
    }
    hostname[MAXHOSTNAMELEN - 1] = '\0';

    string IPCname = "xorpsh" + c_format("-%d-%s", getpid(), hostname);


    XorpShell xorpsh(IPCname, config_template_dir, xrl_dir);

    //this doesn't return until we're done
    xorpsh.run();

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();
    
    printf("connection closed\n");
    return 0;
}

void
XorpShell::get_rtrmgr_pid(PID_CALLBACK cb) {
    _rtrmgr_client.send_get_pid("rtrmgr", cb);
}



