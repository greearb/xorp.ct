// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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



#include "ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/test_main.hh"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include "ospf.hh"
#include "test_args.hh"
#include "test_build_lsa.hh"

int
main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    TestMain t(argc, argv);

    string lsa_description = 
	t.get_optional_args("-l", "--lsa", "lsa description");
    bool v2 = t.get_optional_flag("-2", "--OSPFv2", "OSPFv2");
    bool v3 = t.get_optional_flag("-3", "--OSPFv3", "OSPFv3");
    t.complete_args_parsing();

    if (0 != t.exit())
	return t.exit();

    OspfTypes::Version version;
    if (v2 == v3 || v2) {
	version = OspfTypes::V2;
    } else {
	version = OspfTypes::V3;
    }

    Args args(lsa_description);

    BuildLsa blsa(version);

    try {
	Lsa *lsa = blsa.generate(args);
	if (0 == lsa)
	    return -1;
	printf("%s\n", cstring(*lsa));
    } catch(...) {
	xorp_print_standard_exceptions();
	return -1;
    }

    xlog_stop();
    xlog_exit();

    return 0;
}

