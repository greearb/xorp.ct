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

#ident "$XORP: xorp/xrl/tests/test_generated.cc,v 1.4 2002/12/09 18:29:41 hodson Exp $"

#include <iostream>

#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"

#include "libxipc/finder-server.hh"
#include "libxipc/xrlstdrouter.hh"
#include "libxipc/xrlpf-sudp.hh"

#include "test_xifs.hh"
#include "test_tgt.hh"

static const char* g_tgt_name = "test_tgt";
static const char* g_clnt_name = "test_clnt";

static void
run_test()
{
    EventLoop e;
    FinderServer finder(e);

    // Configure target for test    
    XrlStdRouter tgt_router(e, g_tgt_name); 
    XrlTestTarget tgt_target(&tgt_router);
    
    // Configure client for test
    XrlRouter clnt_router(e, g_clnt_name);

    // Run test
    try_common_xif_methods(&e, &clnt_router, tgt_router.name().c_str());
    try_test_xif_methods(&e, &clnt_router, tgt_router.name().c_str());
}

int main(int /* argc */, const char* argv[])
{
    xlog_init(argv[0], 0);
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    try {
	run_test();
    } catch (...) {
	xorp_catch_standard_exceptions();
    }

    xlog_stop();
    xlog_exit();

    return 0;
}
