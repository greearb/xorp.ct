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

#ident "$XORP: xorp/cli/tools/send_cli_processor_xrl.cc,v 1.1 2004/05/05 05:10:24 pavlin Exp $"

#include "pim/pim_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/callback.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"

#include "libxipc/xrl_std_router.hh"

#include "xrl/interfaces/cli_processor_xif.hh"

//
// A program for sending a CLI request to a module that has built-in CLI
// support.
//

static const char* STD_ROUTER_NAME = "send_cli_processor_xrl";

//
// Local functions prototypes
//
static	void usage(const char* argv0, int exit_value);
static void
recv_process_command_output(const XrlError& xrl_error,
			    const string* processor_name,
			    const string* cli_term_name,
			    const uint32_t* cli_session_id,
			    const string* command_output,
			    bool* done_flag);

/**
 * usage:
 * @argv0: Argument 0 when the program was called (the program name itself).
 * @exit_value: The exit value of the program.
 *
 * Print the program usage.
 * If @exit_value is 0, the usage will be printed to the standart output,
 * otherwise to the standart error.
 **/
static void
usage(const char* argv0, int exit_value)
{
    FILE *output;
    const char* progname = strrchr(argv0, '/');

    if (progname != NULL)
	progname++;		// Skip the last '/'
    if (progname == NULL)
	progname = argv0;

    //
    // If the usage is printed because of error, output to stderr, otherwise
    // output to stdout.
    //
    if (exit_value == 0)
	output = stdout;
    else
	output = stderr;

    fprintf(output, "Usage: %s [-F <finder_hostname>[:<finder_port>]] -t <target> <command>\n",
	    progname);
    fprintf(output, "           -F <finder_hostname>[:<finder_port>]  : finder hostname and port\n");
    fprintf(output, "           -t <target>                           : target name\n");
    fprintf(output, "           -h                                    : usage (this message)\n");
    fprintf(output, "              <command>                          : CLI command\n");
    fprintf(output, "\n");
    fprintf(output, "Program name:   %s\n", progname);
    fprintf(output, "Module name:    %s\n", XORP_MODULE_NAME);
    fprintf(output, "Module version: %s\n", XORP_MODULE_VERSION);

    exit (exit_value);

    // NOTREACHED
}

//
// Wait until the XrlRouter becomes ready
//
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

//
// Send a request to a CLI processor
//
static void
send_process_command(XrlRouter* xrl_router,
		     const string& target,
		     const string& processor_name,
		     const string& cli_term_name,
		     uint32_t cli_session_id,
		     const string& command_name,
		     const string& command_args,
		     bool* done_flag)
{
    XrlCliProcessorV0p1Client _xrl_cli_processor_client(xrl_router);

    //
    // Send the request
    //
    _xrl_cli_processor_client.send_process_command(
	target.c_str(),
	processor_name,
	cli_term_name,
	cli_session_id,
	command_name,
	command_args,
	callback(&recv_process_command_output, done_flag));
}

//
// Process the response of a command processed by a remote CLI processor
//
static void
recv_process_command_output(const XrlError& xrl_error,
			    const string* processor_name,
			    const string* cli_term_name,
			    const uint32_t* cli_session_id,
			    const string* command_output,
			    bool* done_flag)
{
    *done_flag = true;

    if (xrl_error != XrlError::OKAY()) {
	fprintf(stderr, "Error: %s\n", xrl_error.str().c_str());
        return;
    }

    fprintf(stdout, "%s", command_output->c_str());

    UNUSED(processor_name);
    UNUSED(cli_term_name);
    UNUSED(cli_session_id);
}

static void
send_cli_processor_xrl_main(const char* finder_hostname, uint16_t finder_port,
			    const string& target,
			    const string& command_name,
			    const string& command_args)
{
    bool done_flag = false;

    EventLoop eventloop;

    XrlStdRouter xrl_std_router(eventloop, STD_ROUTER_NAME,
				finder_hostname, finder_port);
    xrl_std_router.finalize();
    wait_until_xrl_router_is_ready(eventloop, xrl_std_router);


    send_process_command(&xrl_std_router,
			 target,
			 string(""),		// processor_name,
			 string(""),		// cli_term_name,
			 0,			// cli_session_id,
			 command_name,
			 command_args,
			 &done_flag);

    while (xrl_std_router.pending() || !done_flag) {
	eventloop.run();
    }
}

int
main(int argc, char* const argv[])
{
    int ch;
    string::size_type idx;
    const char* argv0 = argv[0];
    string finder_hostname = FINDER_DEFAULT_HOST.str();
    uint16_t finder_port = FINDER_DEFAULT_PORT;	// XXX: default (in host order)
    string arguments, command, target;

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    //
    // Get the program options
    //
    while ((ch = getopt(argc, argv, "F:t:h")) != -1) {
	switch (ch) {
	case 'F':
	    // Finder hostname and port
	    finder_hostname = optarg;
	    idx = finder_hostname.find(':');
	    if (idx != string::npos) {
		if (idx + 1 >= finder_hostname.length()) {
		    // No port number
		    usage(argv0, 1);
		    // NOTREACHED
		}
		char* p = &finder_hostname[idx + 1];
		finder_port = static_cast<uint16_t>(atoi(p));
		finder_hostname = finder_hostname.substr(0, idx);
	    }
	    break;
	case 't':
	    target = string(optarg);
	    break;
	case 'h':
	case '?':
	    // Help
	    usage(argv0, 0);
	    // NOTREACHED
	    break;
        default:
	    usage(argv0, 1);
	    // NOTREACHED
	    break;
	}
    }
    argc -= optind;
    argv += optind;

    // Get the command itself
    if (argc == 0) {
	usage(argv0, 1);
	// NOTREACHED
    }
    while (argc != 0) {
	if (! command.empty())
	    command += " ";
	command += string(argv[0]);
	argc--;
	argv++;
    }

    if (command.empty() || target.empty()) {
	usage(argv0, 1);
	// NOTREACHED
    }

    try {
	send_cli_processor_xrl_main(finder_hostname.c_str(), finder_port,
				    target, command, arguments);
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    exit (0);
}
