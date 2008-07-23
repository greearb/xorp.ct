// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/ospf/test_build_lsa_main.cc,v 1.8 2008/01/04 03:16:57 pavlin Exp $"

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

