// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/libxipc/call_xrl.cc,v 1.6 2003/02/26 19:07:56 hodson Exp $"

#include "xrl_module.h"
#include "config.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "xrl_std_router.hh"
#include "xrl_args.hh"
#include "xrl_parser_input.hh"

static const char* ROUTER_NAME = "call_xrl";

static int retry_count = 0;	// Number of times to resend xrl on error.
static bool stdin_forever = false;

static void
response_handler(const XrlError& e,
		 XrlArgs*	 response,
		 bool*		 done_flag,
		 bool* 		 resolve_failed,
		 Xrl*		 xrl)
{
    if (e == XrlError::RESOLVE_FAILED()) {
	XLOG_ERROR("Failed.  Reason: %s (\"%s\")",
		   e.str().c_str(), xrl->str().c_str());
	*resolve_failed = true;
	return;
    }
    if (e !=  XrlError::OKAY()) {
	XLOG_ERROR("Failed.  Reason: %s (\"%s\")",
		   e.str().c_str(), xrl->str().c_str());
	exit(-1);
    }
    printf("%s\n", response->str().c_str());
    fflush(stdout);
    *done_flag = true;
}

void usage()
{
    fprintf(stderr,
    "Usage: call_xrl [-r retry] <[-E] -f file1 ... fileN | xrl1 ... xrl>\n"
    "where -f reads XRLs from a file rather than the command line\n"
    "and   -E only passes XRLs through the preprocessor\n"
    "-r number of times to retry connecting to the finder and sending xrl");
}

static int
call_xrl(EventLoop& e, XrlRouter& router, const char* request)
{
    try {
	Xrl x(request);

	// Wait for the finder
	int i;
	bool done_flag = false;
	bool resolve_failed = true;
	i = 0;
	while (done_flag == false && router.connected() == true) {
	    if (resolve_failed) {
		if (i++ > retry_count)
		    break;
		resolve_failed = false;
		router.send(x,
			    callback(&response_handler,
				     &done_flag,
				     &resolve_failed,
				     &x));
		// Retry the request sleep for 1 second between requests.
		if (1 != i)
		    sleep(1);
	    }
	    e.run();
	}
	if (router.connected() == false) {
	    XLOG_FATAL("Lost connection to finder\n");
	}
	return 0;
    } catch(const InvalidString& s) {
	cerr << s.str() << endl;
	return -1;
    }
}

static void
preprocess_file(XrlParserFileInput& xfp)
{
    try {
	while (!xfp.eof()) {
	    string l;
	    if (xfp.getline(l) == false) continue;
	    /* if preprocessing only print line and continue. */
	    cout << l << endl;
	}
    } catch (...) {
	xorp_catch_standard_exceptions();
    }
}

static void
input_file(EventLoop& event_loop,
	   XrlRouter& router,
	   XrlParserFileInput& xfp)
{

    while (!xfp.eof()) {
	string l;
	if (xfp.getline(l) == true) continue;
	/* if line length is zero or line looks like a preprocessor directive
	 * continue. */
	if (l.length() == 0 || l[0] == '#') continue;
	if (call_xrl(event_loop, router, l.c_str()) < 0) {
	    cerr << xfp.stack_trace() << endl;
	    cerr << "Xrl failed: " << l;
	    return;
	}
    }
}

static void
input_files(EventLoop& e,
	    XrlRouter& router,
	    int argc,  char* const argv[], bool pponly)
{
    do {
	if (argc == 0 || argv[0][0] == '-') {
	    do {
		XrlParserFileInput xfp(&cin);
		if (pponly) {
		    preprocess_file(xfp);
		} else {
		    input_file(e, router, xfp);
		}
		usleep(250000);
	    } while (stdin_forever);
	} else {
	    XrlParserFileInput xfp(argv[0]);
	    if (pponly) {
		preprocess_file(xfp);
	    } else {
		input_file(e, router, xfp);
	    }
	}
	argc--;
	argv++;
    } while (argc > 0);
}

static void
input_cmds(EventLoop& e, XrlRouter& router,
	   int argc, char* const argv[])
{
    for (int i = 0; i < argc; i++) {
	if (call_xrl(e, router, argv[i]) < 0) {
	    XLOG_ERROR("Bad XRL syntax: %s\nStopping.", argv[i]);
	    return;
	}
    }
}

int
main(int argc, char* const argv[])
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

    bool pponly = false;	// Pre-process files only
    bool fileinput = false;
    int c;
    while ((c = getopt(argc, argv, "Efir:")) != -1) {
	switch (c) {
	case 'E':
	    pponly = true;
	    fileinput = true;
	    break;
	case 'f':
	    fileinput = true;
	    break;
	case 'i':
	    stdin_forever = true;
	    break;
	case 'r':
	    retry_count = atoi(optarg);
	    break;
	default:
	    usage();
	    return -1;
	}
    }
    argc -= optind;
    argv += optind;

    EventLoop	 e;
    XrlStdRouter router(e, ROUTER_NAME);
    bool	 timeout = false;
    XorpTimer	 to = e.set_flag_after_ms(1000, &timeout);

    while (false == timeout && false == router.connected()) {
	e.run();
    }

    if (false == router.connected()) {
	XLOG_ERROR("Could not connect to finder\n");
	exit(-1);
    }
    
    try {
	if (fileinput) {
	    input_files(e, router, argc, argv, pponly);
	} else if (argc != 0) {
	    input_cmds(e, router, argc, argv);
	} else {
	    usage();
	}
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
