// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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



#include "libxipc/ipc_module.h"

#include "libxorp/xorp.h"
#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"



#include "libxipc/finder_server.hh"
#include "libxipc/xrl_std_router.hh"
#include "libxipc/xrl_pf_sudp.hh"

#include "test_xifs.hh"
#include "test_tgt.hh"


static const char* g_tgt_name = "test_tgt";
static const char* g_clnt_name = "test_clnt";

static void
run_test()
{
    EventLoop e;
    FinderServer finder(e, FinderConstants::FINDER_DEFAULT_HOST(),
			FinderConstants::FINDER_DEFAULT_PORT());

    // Configure target for test
    XrlStdRouter tgt_router(e, g_tgt_name, finder.addr(), finder.port());
    XrlTestTarget tgt_target(&tgt_router);

    // Configure client for test
    XrlRouter clnt_router(e, g_clnt_name, finder.addr(), finder.port());

    bool timeout = false;
    XorpTimer t = e.set_flag_after_ms(2000, &timeout);
    while (tgt_router.ready() == false && clnt_router.ready() == false)
	e.run();

    if (timeout) {
	fprintf(stderr, "Timed out connecting to finder\n");
	exit(-1);
    }

    while (timeout == false)
	e.run();

    // Run test
    fprintf(stderr, "common xif methods\n");
    try_common_xif_methods(&e, &clnt_router, tgt_router.name().c_str());
    fprintf(stderr, "test xif methods\n");
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
