// ===========================================================================
//  Copyright (C) 2012 Jiangxin Hu <jiangxin.hu@crc.gc.ca>
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
// ===========================================================================

#include "wrapper_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"
#include "libxorp/service.hh"
#include "libxorp/status_codes.h"
#include "libxorp/eventloop.hh"

#include "libxipc/xrl_std_router.hh"

#include "wrapper.hh"
#include "xorp_io.hh"
#include "xorp_wrapper4.hh"

static const char TARGET_WRAPPER[] = "wrapper4";   // XRL target name.

static bool wakeup_hook(int n)
{
    UNUSED(n);
    return true;
}

int
main(int /*argc*/, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);         // Least verbose messages
    // XXX: verbosity of the error messages temporarily increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_level_set_verbose(XLOG_LEVEL_WARNING, XLOG_VERBOSE_HIGH);
    xlog_level_set_verbose(XLOG_LEVEL_INFO, XLOG_VERBOSE_HIGH);
    xlog_level_set_verbose(XLOG_LEVEL_TRACE, XLOG_VERBOSE_HIGH);
    //xlog_add_default_output();
    //xlog_add_output(stdout);
    xlog_add_output(stderr);
    xlog_start();

    try {
        EventLoop eventloop;

        string feaname = "fea";
        string ribname = "rib";
        string protocol(TARGET_WRAPPER);

        XrlStdRouter xrl_router(eventloop, TARGET_WRAPPER);

        XrlIO io(eventloop, xrl_router, feaname, ribname,protocol);
        Wrapper wrapper(eventloop, &io);

        XrlWrapper4Target target(&xrl_router, wrapper, io);
        wait_until_xrl_router_is_ready(eventloop, xrl_router);
        io.wstartup(&wrapper);
        while (!io.doReg()) eventloop.run();

        XorpTimer wakeywakey = eventloop.new_periodic_ms(50, callback(wakeup_hook, 1));

        while (wrapper.running())
            eventloop.run();
    } catch(...) {
        xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    fprintf(stderr,"Exiting wrapper4 ...\n");
    xlog_stop();
    xlog_exit();
    return 0;
}

