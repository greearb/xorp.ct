// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP$"

// Access the profiling support in XORP processes.

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include <stdio.h>
#include <sysexits.h>

#include "config.h"
#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/status_codes.h"
#include "libxorp/callback.hh"
#include "libxipc/xrl_std_router.hh"

#include "xrl/interfaces/profile_xif.hh"
#include "xrl/targets/profiler_base.hh"

class XrlProfilerTarget :  XrlProfilerTargetBase {
 public:
    XrlProfilerTarget(XrlRouter *r)
	:  XrlProfilerTargetBase(r), _xrl_router(r), _done(false)
    {}

    XrlCmdError
    common_0_1_get_target_name(string& name)
    {
	name = "profiler";
	return XrlCmdError::OKAY();
    }

    XrlCmdError
    common_0_1_get_version(string& version)
    {
	version = "0.1";
	return XrlCmdError::OKAY();
    }

    XrlCmdError
    common_0_1_get_status(uint32_t& status, string& /*reason*/)
    {
	status =  PROC_READY;
	return XrlCmdError::OKAY();
    }

    XrlCmdError
    common_0_1_shutdown()
    {
	_done = true;
	return XrlCmdError::OKAY();
    }

    XrlCmdError
    profile_client_0_1_log(const string& pname,	const uint32_t&	sec,
			   const uint32_t& usec, const string&	comment)
    {
	printf("%s %d %d %s\n", pname.c_str(), sec, usec, comment.c_str());

	return XrlCmdError::OKAY();
    }

    XrlCmdError 
    profile_client_0_1_finished(const string& /*pname*/)
    {
	_done = true;
	return XrlCmdError::OKAY();
    }

    void
    list(const string& target)
    {
	_done = false;

	XrlProfileV0p1Client profile(_xrl_router);
	profile.send_list(target.c_str(),
			  callback(this, &XrlProfilerTarget::list_cb));
    }

    void
    list_cb(const XrlError& error, const string* list)
    {
	_done = true;
	if(XrlError::OKAY() != error) {
	    XLOG_WARNING("callback: %s", error.str().c_str());
	    return;
	}
	printf("%s", list->c_str());
    }

    void
    enable(const string& target, const string& pname)
    {
	_done = false;

	XrlProfileV0p1Client profile(_xrl_router);
	profile.send_enable(target.c_str(),
			    pname,
			    callback(this, &XrlProfilerTarget::cb));
    }

    void
    disable(const string& target, const string& pname)
    {
	_done = false;

	XrlProfileV0p1Client profile(_xrl_router);
	profile.send_disable(target.c_str(),
			     pname,
			     callback(this, &XrlProfilerTarget::cb));
    }

    void
    clear(const string& target, const string& pname)
    {
	_done = false;

	XrlProfileV0p1Client profile(_xrl_router);
	profile.send_clear(target.c_str(),
			   pname,
			   callback(this, &XrlProfilerTarget::cb));
    }

    void
    get(const string& target, const string& pname)
    {
	_done = false;

	XrlProfileV0p1Client profile(_xrl_router);
	profile.send_get_entries(target.c_str(),
				 pname,
				 _xrl_router->instance_name(),
				 callback(this, &XrlProfilerTarget::get_cb));
    }

    void
    get_cb(const XrlError& error)
    {
	// If there is an error then set done to be true.
	if(XrlError::OKAY() != error) {
	    _done = true;
	    XLOG_WARNING("callback: %s", error.str().c_str());
	}
    }

    void
    cb(const XrlError& error)
    {
	_done = true;
	if(XrlError::OKAY() != error) {
	    XLOG_WARNING("callback: %s", error.str().c_str());
	}
    }

    bool done() const { return _done; }

 private:
    XrlRouter *_xrl_router;
    bool _done;
};

int
usage(const char *myname)
{
    fprintf(stderr,
	    "usage: %s -t target {-l | -v profile variable [-e|-d|-c|-g]}\n",
	    myname);
    return EX_USAGE;
}

int
main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    string target;	// Target name
    string pname;	// Profile variable
    string command;	// Command
    bool requires_pname = false;

    int c;
    while ((c = getopt(argc, argv, "t:lv:edcg")) != -1) {
	switch (c) {
	case 't':
	    target = optarg;
	    break;
	case 'l':
	    command = "list";
	    break;
	case 'v':
	    pname = optarg;
	    break;
	case 'e':
	    command = "enable";
	    requires_pname = true;
	    break;
	case 'd':
	    command = "disable";
	    requires_pname = true;
	    break;
	case 'c':
	    command = "clear";
	    requires_pname = true;
	    break;
	case 'g':
	    command = "get";
	    requires_pname = true;
	    break;
	default:
	    return usage(argv[0]);
	}
    }
    
    if (target.empty())
	return usage(argv[0]);

    if (command.empty())
	return usage(argv[0]);
    
    if (requires_pname && pname.empty())
	return usage(argv[0]);

    try {
	EventLoop eventloop;
	XrlStdRouter xrl_router(eventloop, "profiler");
	XrlProfilerTarget profiler(&xrl_router);

	xrl_router.finalize();
	wait_until_xrl_router_is_ready(eventloop, xrl_router);

	if (command == "list")
	    profiler.list(target);
	else if (command == "enable")
	    profiler.enable(target, pname);
	else if (command == "disable")
	    profiler.disable(target, pname);
	else if (command == "clear")
	    profiler.clear(target, pname);
	else if (command == "get")
	    profiler.get(target, pname);
	else
	    XLOG_FATAL("Unknown command");

	while (!profiler.done())
	    eventloop.run();

	while (xrl_router.pending())
 	    eventloop.run();

        } catch (...) {
	xorp_catch_standard_exceptions();
    }

    // 
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
