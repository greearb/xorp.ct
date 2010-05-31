// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "finder_module.h"

#include "libxorp/xorp.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "finder_client.hh"
#include "finder_client_xrl_target.hh"
#include "finder_tcp_messenger.hh"
#include "finder_constants.hh"


//
// A client process to help test the finder keepalive and time out
// mechanism.  This process can be launched to either run without
// interruption (well behaving) or to run and then sleep for a finite
// period of time (misbehaving).
//

static const char *program_name         = "test_finder_to";
static const char *program_description  = "Test timeout behaviour of "
					  "Finder wrt Finder client";
static const char *program_version_id   = "0.1";
static const char *program_date         = "April, 2003";
static const char *program_copyright    = "See file LICENSE";
static const char *program_return_value = "0 on success, 1 if test error, 2 if internal error";

///////////////////////////////////////////////////////////////////////////////
//
// Verbosity level control
//

static bool s_verbose = false;
bool verbose()                  { return s_verbose; }
void set_verbose(bool v)        { s_verbose = v; }

#define verbose_log(x...) _verbose_log(__FILE__,__LINE__, x)

#define _verbose_log(file, line, x...)					\
do {									\
    if (verbose()) {							\
	printf("From %s:%d: ", file, line);				\
	printf(x);							\
    }									\
} while(0)

///////////////////////////////////////////////////////////////////////////////
//
// Test body
//

static int
test_main(uint32_t pre_block_secs,
	  uint32_t block_secs,
	  uint32_t post_block_secs,
	  uint16_t finder_port)
{
    EventLoop e;

    FinderClient fc;
    FinderClientXrlTarget fxt(&fc, &fc.commands());
    FinderTcpAutoConnector fac(e, fc, fc.commands(),
			       FinderConstants::FINDER_DEFAULT_HOST(),
			       finder_port);

    bool timeout = false;
    XorpTimer t = e.set_flag_after(TimeVal(1, 0), &timeout);
    while (fc.connected() == false && timeout == false) {
	e.run();
    }

    if (timeout) {
	verbose_log("Failed to connect to finder.\n");
	return 1;
    }

    string instance_foo("foo");
    fc.register_xrl_target(instance_foo, "bar", 0);
    fc.enable_xrls(instance_foo);

    timeout = false;
    t = e.set_flag_after(TimeVal(1, 0), &timeout);
    while (fc.ready() == false && timeout == false) {
	e.run();
    }

    if (timeout) {
	verbose_log("Client failed to become ready.\n");
	return 1;
    }

    verbose_log("Running for %u seconds\n", XORP_UINT_CAST(pre_block_secs));
    t = e.set_flag_after(TimeVal(pre_block_secs, 0), &timeout);
    while (t.scheduled()) {
	e.run();
    }

    verbose_log("Sleeping for %u seconds\n", XORP_UINT_CAST(block_secs));
    if (block_secs) {
	TimerList::system_sleep(TimeVal(block_secs, 0));
    }

    fprintf(stderr, "Connected %d\n", fc.connected());

    verbose_log("Running for %u seconds\n", XORP_UINT_CAST(block_secs));
    t = e.set_flag_after(TimeVal(post_block_secs, 0), &timeout);
    while (t.scheduled()) {
	e.run();
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// Standard test program rubble.
//

/**
 * Print program info to output stream.
 *
 * @param stream the output stream the print the program info to.
 */
static void
print_program_info(FILE *stream)
{
    fprintf(stream, "Name:          %s\n", program_name);
    fprintf(stream, "Description:   %s\n", program_description);
    fprintf(stream, "Version:       %s\n", program_version_id);
    fprintf(stream, "Date:          %s\n", program_date);
    fprintf(stream, "Copyright:     %s\n", program_copyright);
    fprintf(stream, "Return:        %s\n", program_return_value);
}

/**
 * Print program usage information to the stderr.
 *
 * @param progname the name of the program.
 */
static void
usage(const char* progname)
{
    print_program_info(stderr);
    fprintf(stderr,
	    "usage: %s [-b secs ] [-p <secs> ] [-x <secs>] [-v] [-h] [port]\n",
	    progname);
    fprintf(stderr, "       -b		: set process blocking period\n");
    fprintf(stderr, "       -p		: set pre-blocking period\n");
    fprintf(stderr, "       -x		: set execution time\n");
    fprintf(stderr, "       -h          : usage (this message)\n");
    fprintf(stderr, "       -v          : verbose output\n");
}

int
main(int argc, char * const argv[])
{
    const char* argv0;
    uint32_t pre_block_secs, block_secs, post_block_secs;
    uint32_t exec_secs;
    uint16_t port;

    argv0 	    = argv[0];
    pre_block_secs  = 0;
    block_secs 	    = 0;
    post_block_secs = 0;
    exec_secs 	    = 0;
    port 	    = FinderConstants::FINDER_DEFAULT_PORT();

    int ch;
    while ((ch = getopt(argc, argv, "b:hp:vx:")) != -1) {
	switch (ch) {
	case 'b':
	    block_secs = strtoul(optarg, 0, 10);
	    break;
	case 'p':
	    pre_block_secs = strtoul(optarg, 0, 10);
	    break;
	case 'x':
	    exec_secs = strtoul(optarg, 0, 10);
	    break;
	case 'v':
	    set_verbose(true);
	    break;
	case 'h':
	default:
	    usage(argv[0]);
	}
    }

    argc -= optind;
    argv += optind;

    if (argc > 1) {
	usage(argv0);
    } else if (argc == 1) {
	port = strtoul(argv[0], 0, 10);
    }

    if (exec_secs < pre_block_secs + block_secs) {
	fprintf(stderr,
		"Execution time must be greater or equal to the block and "
		"pre-block times.");
	exit(2);
    }

    post_block_secs = exec_secs - pre_block_secs - block_secs;

    //
    // Initialize and start xlog
    //
    xlog_init(argv0, NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);         // Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    XorpUnexpectedHandler x(xorp_unexpected_handler);
    int ret_value;
    try {
	ret_value = test_main(pre_block_secs, block_secs, post_block_secs,
			      port);
    } catch (...) {
        // Internal error
        xorp_print_standard_exceptions();
        ret_value = 2;
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return ret_value;
}


