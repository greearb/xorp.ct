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

#ident "$XORP: xorp/libxipc/finder.cc,v 1.18 2002/12/09 18:29:04 hodson Exp $"

#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include <unistd.h>

#include "finder_module.h"
#include "config.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "finder-server.hh"

// ----------------------------------------------------------------------------
// The Finder Program...

static jmp_buf tidy_exit;

static void
trigger_exit(int signo) {
    fprintf(stderr, "Finder caught signal: %d\n", signo);
    longjmp(tidy_exit, -1);
}

static void
install_signal_traps(sig_t func) {
    signal(SIGINT, func);
    signal(SIGTERM, func);
}

static bool
print_twirl()
{
    static const char t[] = { '\\', '|', '/', '-' };
    static const size_t nt = sizeof(t) / sizeof(t[0]);
    static size_t n = 0;
    static char erase = '\0';
   
    printf("%c%c", erase, t[n % nt]); fflush(stdout);
    n++;
    erase = '\b';
    return true;
}

static void
finder_main(int argc, char* const argv[])
{
    bool run_verbose = false;
    int ch;

    while((ch = getopt(argc, argv, "hv")) != -1) {
	switch(ch) {
	case 'v':
	    run_verbose = true;
	    break;
	case 'h':
	default:
	    fprintf(stderr, "usage: finder [-v]\n");
	    exit(-1);
	}
    }

    install_signal_traps(trigger_exit);
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    try {
	EventLoop e;
        FinderServer fs(e);

	XorpTimer twirl;	
	if (run_verbose) 
	    twirl = e.new_periodic(250, callback(print_twirl));

	// Using setjmp ensures that the Eventloop and FinderServer
	// destructor's get called if a signal is received whilst in
	// the code on the eventloop.  In particular, we clean up all
	// socket state associated with the FinderServer so we can 
	// restart the Finder immediately.
	if (setjmp(tidy_exit) == 0) {
	    for(;;) {
		e.run();
	    }
	}

	// XXX Reached when one of the SIG's at the start of main is
	// received.  Install new signal handlers so if we get a second
	// signal application bails.
    } catch (const FinderTCPServerIPCFactory::FactoryError& fe) {
	fprintf(stderr, "Could not instantiate finder: %s\n", 
		fe.why().c_str());
	fprintf(stderr, 
		"Check whether another instance is already running.\n");
    } catch (...) {
	xorp_catch_standard_exceptions();
    }
    install_signal_traps(SIG_DFL);
}

int 
main(int argc, char * const argv[]) {
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    finder_main(argc, argv);

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();
    
    return 0;
}
