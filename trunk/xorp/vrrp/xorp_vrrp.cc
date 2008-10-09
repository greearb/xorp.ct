// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008 XORP, Inc.
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

#ident "$XORP$"

#include "vrrp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/eventloop.hh"
#include "libxipc/xrl_std_router.hh"
#include "vrrp_target.hh"
#include "vrrp_exception.hh"

static void start()
{
    EventLoop e;

    XrlStdRouter rtr(e, VRRPTarget::vrrp_target_name.c_str(),
		     FinderConstants::FINDER_DEFAULT_HOST().str().c_str());

    VRRPTarget vrrp(rtr);

    wait_until_xrl_router_is_ready(e, rtr);

    while (vrrp.running())
	e.run();
}

int main(int argc, char* argv[])
{
    UNUSED(argc);

    xlog_init(argv[0], 0);
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    try {
	start();
    } catch(const VRRPException& e) {
	XLOG_FATAL("VRRPException: %s", e.str().c_str());
    }

    xlog_stop();
    xlog_exit();

    exit(0);
}
