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

#ident "$XORP: xorp/mrt/test_mtimer.cc,v 1.3 2003/03/27 01:51:59 hodson Exp $"


//
// Multicast timers test program.
//


#include "mrt_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"
#include "mrt/timer.hh"


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//


//
// Local variables
//

//
// Local functions prototypes
//
static	bool wakeup_hook();


static Timer my_mtimer;
static void
my_mtimer_timeout(void *)
{
    fprintf(stdout, "%s My mtimer expired!\n", xlog_localtime2string());
    fflush(stdout);
    
    my_mtimer.start(1, 0, my_mtimer_timeout, NULL);
}

int
main(int /* argc */, char *argv[])
{
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();
    
    try {
	EventLoop e;
	XorpTimer wakeywakey = e.new_periodic(5000,
					      callback(wakeup_hook));
	
	timers_init();
	
	my_mtimer.start(1, 0, my_mtimer_timeout, NULL);
	
	// Main loop
	for (;;) {
	    xorp_schedule_mtimer(e);
	    e.run();
	}
	
    } catch(...) {
	xorp_catch_standard_exceptions();
    }
    
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();
    
    exit(0);
}

static bool
wakeup_hook()
{
    fprintf(stdout, "%s XorpTimer expired!\n", xlog_localtime2string());
    fflush(stdout);
    
    return (true);
}

