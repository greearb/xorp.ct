// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/ospf/test_checksum.cc,v 1.9 2007/02/16 22:46:42 pavlin Exp $"

#define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/test_main.hh"
#include "libxorp/exceptions.hh"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "fletcher_checksum.hh"


uint8_t data[] = {
#include "iso512.data"
};

uint8_t lsa1[] = {
#include "lsa1.data"
};

uint8_t lsa2[] = {
#include "lsa2.data"
};

bool
test_checksum(TestInfo& info, const char *name, uint8_t* data, size_t len)
{
    DOUT(info) << "Testing: " << name << endl;

    int32_t x, y;

    fletcher_checksum(data, len, 0 /* offset */, x, y);

    DOUT(info) << "x: " << x << " y: " << y << endl;

    if (!(255 == x && 255 == y)) {
	DOUT(info) << "Both values must be 255\n";
	return false;
    }

    return true;
}

int
main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    TestMain t(argc, argv);

    string test =
	t.get_optional_args("-t", "--test", "run only the specified test");
    t.complete_args_parsing();

    struct test {
	string test_name;
	XorpCallback1<bool, TestInfo&>::RefPtr cb;
    } tests[] = {
	{"test1",
	 callback(test_checksum, "iso512.data", &data[0], sizeof(data))},
	{"test2",
	 callback(test_checksum, "lsa1.data", &lsa1[0], sizeof(lsa1))},
	{"test3",
	 callback(test_checksum, "lsa2.data", &lsa2[0], sizeof(lsa2))},
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

    xlog_stop();
    xlog_exit();

    return t.exit();
}
