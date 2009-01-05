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

#ident "$XORP: xorp/libxorp/test_test_main.cc,v 1.17 2008/10/02 21:57:35 bms Exp $"

#include "libxorp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"

#include "test_main.hh"


bool
test1(TestInfo& info)
{
    DOUT(info) << "verbose on\n";

    return true;
}

bool
test2(TestInfo& info)
{
    DOUT(info) << "verbose on level = " <<  info.verbose_level() << endl;

    int level = 100;
    DOUT_LEVEL(info, level) << "debugging level >= " << level << endl;

    return true;
}

bool
test3(TestInfo& info, bool fail)
{
    DOUT(info) << info.test_name() << " Test will " <<
	    (fail ? "fail" : "succeed") << endl;

    if (fail)
	return false;

    return true;
}

bool
test4(TestInfo& info, const char *mess)
{
    DOUT(info) << "verbose on level = " << info.verbose_level() << 
	    " message = " <<  mess << endl;

    return true;
}

bool
test5(TestInfo& info, bool exception)
{
    DOUT(info) << info.test_name() << " Test will " <<
	    (exception ? "throw exception" : "succeed") << endl;

    if (exception)
	xorp_throw(InvalidString, "Hello");

    return true;
}

bool
test6(TestInfo& info, bool exception)
{
    DOUT(info) << info.test_name() << " Test will " <<
	    (exception ? "throw exception" : "succeed") << endl;

    if (exception)
	throw("Unexpected exception");

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
    bool fail =
	t.get_optional_flag("-f", "--fail", "fail test3");
    string mess = 
	t.get_optional_args("-m", "--message", "pass message to test4");
    bool exception = 
	t.get_optional_flag("-e", "--exception",
			    "throw xorp exception test5");
    bool unexpected = 
	t.get_optional_flag("-u", "--unexpected",
			    "throw unexpected exception test6");

    t.complete_args_parsing();

    struct test {
	string test_name;
	XorpCallback1<bool, TestInfo&>::RefPtr cb;
    } tests[] = {
	{"test1", callback(test1)},
	{"test2", callback(test2)},
	{"test3", callback(test3, fail)},
	{"test4", callback(test4, mess.c_str())},
	{"test5", callback(test5, exception)},
	{"test6", callback(test6, unexpected)},
    };

    try {
	if ("" == test) {
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
