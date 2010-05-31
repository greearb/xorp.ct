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
