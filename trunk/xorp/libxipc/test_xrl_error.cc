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

#ident "$XORP: xorp/devnotes/template.cc,v 1.2 2003/01/16 19:08:48 mjh Exp $"

#include <assert.h>
#include <stdio.h>

#include "xrl_module.h"
#include "libxorp/xlog.h"
#include "xrl_error.hh"

static bool g_trace = false;
#define tracef(args...) if (g_trace) printf(args)

static const XrlError foo()
{
    return XrlError::REPLY_TIMED_OUT();
}

static const XrlCmdError bar()
{
    return XrlCmdError::BAD_ARGS();
}

static const XrlError baz()
{
    return foo();
}

static const XrlError baz2()
{
    return baz();
}

static void
run_test()
{
    for (uint32_t i = 0; i < 1000; i++) {
	    XrlError e(i);
	    tracef("%s\n", e.str().c_str());
    }
    const XrlCmdError& xce = XrlCmdError::OKAY();
    const XrlError& xe = XrlError::OKAY();

    assert(xce == xe);

    const XrlError& e1 = foo();
    tracef("%s\n", e1.str().c_str());

    const XrlCmdError& e2 = bar();
    tracef("%s\n", e2.str().c_str());

    const XrlError& e3 = e2;
    tracef("%s\n", e3.str().c_str());

    tracef("%s\n", baz().str().c_str());
    tracef("%s\n", baz2().str().c_str());
}

int main(int /* argc */, char *argv[])
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

    run_test();

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
