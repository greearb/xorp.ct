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

#ident "$XORP: xorp/libxipc/call_xrl.cc,v 1.22 2003/09/16 21:54:41 hodson Exp $"

#include "xrl_module.h"
#include "config.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "xrl_std_router.hh"
#include "xrl_args.hh"
#include "xrl_parser_input.hh"

static const char* ROUTER_NAME = "call_xrl";

static int wait_time = 1000;	// Time to wait for the callback in ms.
static int retry_count = 0;	// Number of times to resend xrl on error.
static bool stdin_forever = false;

enum {
    // Return values from call_xrl
    OK = 0, BADXRL = -1, NOCALLBACK = -2
};

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
    if (e != XrlError::OKAY()) {
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
	    "Usage: call_xrl [options] "
	    "<[-E] -f file1 ... fileN | xrl1 ... xrl>\n"
	    "where -f reads XRLs from a file rather than the command line\n"
	    "and   -E only passes XRLs through the preprocessor\n"
	    "Options:\n"
	    "  -F <host>[:<port>]   Specify Finder host and port\n"
	    "  -r <retries>         Specify number of retry attempts\n"
	    "  -w <time ms>         Time to wait for a callback\n");
}

static int
call_xrl(EventLoop& e, XrlRouter& router, const char* request)
{
    try {
	Xrl x(request);

	int tries;
	bool done, resolve_failed;

	tries = 0;
	done = false;
	resolve_failed = true;

	while (done == false && tries <= retry_count) {
	    resolve_failed = false;
	    router.send(x, callback(&response_handler,
				    &done,
				    &resolve_failed,
				    &x));
	    
	    bool timed_out = false;
	    XorpTimer timeout = e.set_flag_after_ms(wait_time, &timed_out);
	    while (timed_out == false && done == false) {
		// NB we don't test for resolve failed here because if
		// resolved failed we want to wait before retrying.
		e.run();
	    }
	    tries++;

	    if (resolve_failed) {
		continue;
	    }
	    
	    if (timed_out) {
		XLOG_WARNING("request: %s no response waited %d ms", request,
			     wait_time);
		continue;
	    }

	    if (router.connected() == false) {
		XLOG_FATAL("Lost connection to finder\n");
	    }
	}

	if (resolve_failed) {
	    XLOG_WARNING("request: %s resolve failed", request);
	}
	
 	if (false == done && true == resolve_failed)
	    XLOG_WARNING("request: %s failed after %d retries",
			 request, retry_count);
	return done == true ? OK : NOCALLBACK;
    } catch(const InvalidString& s) {
	cerr << s.str() << endl;
	return BADXRL;
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

static int
input_file(EventLoop&	       eventloop,
	   XrlRouter&	       router,
	   XrlParserFileInput& xfp)
{

    while (!xfp.eof()) {
	string l;
	if (xfp.getline(l) == true) continue;
	/* if line length is zero or line looks like a preprocessor directive
	 * continue. */
	if (l.length() == 0 || l[0] == '#') continue;
	int err = call_xrl(eventloop, router, l.c_str());
	if (err) {
	    cerr << xfp.stack_trace() << endl;
	    cerr << "Xrl failed: " << l;
	    return err;
	}
    }
    return 0;
}

static int
input_files(EventLoop&	e,
	    XrlRouter&	router,
	    int		argc,
	    char* const argv[],
	    bool	pponly)
{
    do {
	if (argc == 0 || argv[0][0] == '-') {
	    do {
		XrlParserFileInput xfp(&cin);
		if (pponly) {
		    preprocess_file(xfp);
		} else {
		    int err = input_file(e, router, xfp);
		    if (err)
			return err;
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
    return 0;
}

static int
input_cmds(EventLoop&  e,
	   XrlRouter&  router,
	   int	       argc,
	   char* const argv[])
{
    for (int i = 0; i < argc; i++) {
	int err = call_xrl(e, router, argv[i]);
	switch (err) {
	case OK:
	    break;
	case BADXRL:
	    XLOG_ERROR("Bad XRL syntax: %s\nStopping.", argv[i]);
	    return err;
	    break;
	case NOCALLBACK:
	    XLOG_ERROR("No callback: %s\nStopping.", argv[i]);
	    return err;
	    break;
	}
    }
    return 0;
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
    const char *finder_host = "localhost";
    uint16_t port = FINDER_DEFAULT_PORT;
    int c;
    while ((c = getopt(argc, argv, "F:Efir:w:")) != -1) {
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
	case 'w':
	    wait_time = atoi(optarg) * 1000;
	    break;
	case 'F':
	    {
	    finder_host = strsep(&optarg, ":");
	    const char *tmpport = strsep(&optarg, ":");
	    if (0 != tmpport)
		port = atoi(tmpport);
	    }
	    break;
	default:
	    usage();
	    return -1;
	}
    }
    argc -= optind;
    argv += optind;

    try {
	EventLoop e;
	XrlStdRouter router(e, ROUTER_NAME, finder_host, port);

	router.finalize();

	bool timeout = false;
	XorpTimer to = e.set_flag_after_ms(5000, &timeout);

	while (false == timeout && false == router.ready()) {
	    e.run();
	}

	if (false == router.connected()) {
	    XLOG_ERROR("Could not connect to finder.\n");
	    exit(-1);
	}

	if (false == router.ready()) {
	    XLOG_ERROR("Connected to finder, but did not become ready.\n");
	    exit(-1);
	}

	if (fileinput) {
	    if (input_files(e, router, argc, argv, pponly)) {
		return -1;
	    }
	} else if (argc != 0) {
	    if (input_cmds(e, router, argc, argv)) {
		return -1;
	    }
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
