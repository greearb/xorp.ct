// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#ident "$XORP: xorp/vrrp/xorp_vrrp.cc,v 1.1 2008/10/09 17:40:58 abittau Exp $"

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
