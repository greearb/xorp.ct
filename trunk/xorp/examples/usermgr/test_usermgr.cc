// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008-2009 XORP, Inc.
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

#ident "$XORP: xorp/examples/usermgr/test_usermgr.cc,v 1.3 2008/11/18 19:17:19 atanu Exp $"

/*
 * Test the UserDB implementation.
 */

#define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "usermgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/test_main.hh"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/exceptions.hh"

#include "usermgr.hh"

bool
test1(TestInfo& info)
{
    UserDB theone;

    /*
     * create some users and groups.
     */
    theone.add_user("george", 111);
    theone.add_user("hermoine", 106);
    theone.add_user("phillip", 126);
    theone.add_user("andrew", 136);
    theone.add_group("group44", 146);
    theone.add_group("fubarz", 156);
    theone.add_group("muumuus", 105);
    theone.add_group("tekeleks", 155);
    theone.add_user("geromino", 166);
    theone.add_user("vasili", 606);
    theone.add_group("vinieski", 666);
    theone.add_user("ocupine", 166);

    DOUT(info) << theone.str();

    /*
     * delete some of the users and groups.
     */
    theone.del_group("fubarz");
    theone.del_user("andrew");
    theone.del_user("george");
    theone.del_user("ocupine");

    DOUT(info) << theone.str();

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

    xlog_stop();
    xlog_exit();

    return t.exit();
}
