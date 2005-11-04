// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/ospf/test_checksum.cc,v 1.2 2005/01/14 21:36:32 atanu Exp $"

#include "config.h"

#include "ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/test_main.hh"
#include "libxorp/exceptions.hh"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "fletcher_checksum.hh"

#ifndef	DEBUG_LOGGING
#define DEBUG_LOGGING
#endif /* DEBUG_LOGGING */
#ifndef	DEBUG_PRINT_FUNCTION_NAME
#define DEBUG_PRINT_FUNCTION_NAME
#endif /* DEBUG_PRINT_FUNCTION_NAME */

uint8_t data[] = {
#include "iso512.data"
};

bool
test1(TestInfo& info)
{
    DOUT(info) << "Starting test" << endl;

    int32_t x, y;

    fletcher_checksum(&data[0], sizeof(data), 0 /* offset */, x, y);

    DOUT(info) << "x: " << x << " y: " << y << endl;

    if (!(0 == x && 0 == y)) {
	DOUT(info) << "Both values must be zero\n";
	return false;
    }

    return true;
}

int
main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    TestMain t(argc, argv);

    string test =
	t.get_optional_args("-t", "--test", "run only the specified test");
    t.complete_args_parsing();

    struct test {
	string test_name;
	XorpCallback1<bool, TestInfo&>::RefPtr cb;
    } tests[] = {
	{"test1", callback(test1)},
    };

    try {
	if (test.empty()) {
	    for (size_t i = 0; i < sizeof(tests) / sizeof(struct test); i++)
		t.run(tests[i].test_name, tests[i].cb);
	} else {
	    for (size_t i = 0; i < sizeof(tests) / sizeof(struct test); i++)
		if (test == tests[i].test_name) {
		    t.run(tests[i].test_name, tests[i].cb);
		    return t.exit();
		}
	    t.failed("No test with name " + test + " found\n");
	}
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    return t.exit();
}
