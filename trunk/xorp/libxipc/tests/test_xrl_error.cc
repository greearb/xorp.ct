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
	XrlErrorCode ec = XrlErrorCode(i);
	XrlError e(ec);
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
