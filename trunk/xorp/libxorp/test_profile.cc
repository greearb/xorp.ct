// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP$"

#define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "libxorp_module.h"
#include "xorp.h"
#include "debug.h"
#include "xlog.h"
#include "test_main.hh"
#include "exceptions.hh"
#include "profile.hh"

bool
test1(TestInfo& info)
{
    Profile p;

    string ar = "add_route";
    p.create(ar);

    // It shouldn't be possible to create this variable again.
    try {
	p.create(ar);
	DOUT(info) << "Create variable twice!!! " << ar << endl;
	return false;
    } catch(PVariableExists& p) {
	DOUT(info) << "Exception " << p.str() << endl;
    } catch(...) {
	DOUT(info) << "Unknown Exception\n";
	return false;
    }

    // Enable this variable.
    // XXX - This should enable global profiling.
    p.enable(ar);

    // Make sure that this variable is not enabled.
    string bogus = "bogus";
    // Test for an unknown variable.
    try {
	if (p.enabled(bogus)) {
	    DOUT(info) << "Testing for a bogus variable succeeded " << bogus
		       << endl;
	    return false;
	}
	return false;
    } catch(PVariableUnknown& p) {
	DOUT(info) << "Exception " << p.str() << endl;
    } catch(...) {
	DOUT(info) << "Unknown Exception\n";
	return false;
    }

    // Disable this variable.
    // XXX - This should disable global profiling.
    p.disable(ar);

    // Testing for a bogus variable will return false, but no
    // exception will be thrown as global profiling is now disabled.
    if (p.enabled(bogus)) {
	DOUT(info) << "Testing for a bogus variable succeeded " << bogus
		   << endl;
	return false;
    }

    // Try and log to a bogus variable.
    try {
	p.log(bogus, c_format("wow"));
	DOUT(info) << "Testing for a bogus variable succeeded " << bogus
		   << endl;
	return false;
    } catch(PVariableUnknown& p) {
	DOUT(info) << "Exception " << p.str() << endl;
    } catch(...) {
	DOUT(info) << "Unknown Exception\n";
	return false;
    }

    // Try and log to a valid variable that is not enabled.
    try {
	p.log(ar, c_format("wow"));
	DOUT(info) << "Logging to a valid disabled variable worked!!! "
		   << ar
		   << endl;
	return false;
    } catch(PVariableNotEnabled& p) {
	DOUT(info) << "Exception " << p.str() << endl;
    } catch(...) {
	DOUT(info) << "Unknown Exception\n";
	return false;
    }

    // Enable the variable for logging.
    p.enable(ar);

    string message = "wow";
    // Logging should succeed now.
    try {
	p.log(ar, message);
	DOUT(info) << "Logging succeeded " << ar << endl;
    } catch(PVariableNotEnabled& p) {
	DOUT(info) << "Exception " << p.str() << endl;
    } catch(...) {
	DOUT(info) << "Unknown Exception\n";
	return false;
    }

    // Disable the logging.
    p.disable(ar);

    // Lock the log.
    p.lock_log(ar);
    ProfileLogEntry ple;
    if (!p.read_log(ar, ple)) {
	DOUT(info) << "There should be one entry\n";
	return false;
    }

    if (ple.loginfo() != message) {
	    DOUT(info) << "Expected " << message  << " got " <<
		ple.loginfo() <<endl;
	return false;
    }

    if (p.read_log(ar, ple)) {
	DOUT(info) << "Too many entries\n";
	return false;
    }

    p.release_log(ar);
    p.clear(ar);

    return true;
}

bool
test2(TestInfo& info)
{
    Profile p;

    string ar = "add_route";
    p.create(ar);

    p.enable(ar);

    for(int i = 0; i < 100; i++)
	p.log(ar, c_format("%d", i));
    
    p.disable(ar);

    p.lock_log(ar);
    ProfileLogEntry ple;
    for(int i = 0; p.read_log(ar, ple); i++) {
	DOUT(info) << ple.time().str() << " " << ple.loginfo() << endl;
	if (ple.loginfo() != c_format("%d", i)) {
	    DOUT(info) << "Expected " << i  << " got " <<
		ple.loginfo() <<endl;
	    return false;
	}
    }

    p.release_log(ar);
    p.clear(ar);

    return true;
}

bool
test3(TestInfo& info)
{
    Profile p;
    string ar = "add_route";
    p.create(ar);

    // Try reading an entry while the entry is not locked.
    try {
	ProfileLogEntry ple;
	p.read_log(ar, ple);
	return false;
    } catch(PVariableNotLocked& pe) {
	DOUT(info) << "Exception " << pe.str() << endl;
    } catch(...) {
	DOUT(info) << "Unknown Exception\n";
	return false;
    }

    return true;
}

bool
test4(TestInfo& info)
{
    Profile p;
    string ar = "add_route_ribin";
    string lr = "add_route_ribout";
    p.create(ar, "Routes entering the RIB-IN");
    p.create(lr, "Routes being sent to the RIB");

    DOUT(info) << p.list();

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
	{"test2", callback(test2)},
	{"test3", callback(test3)},
	{"test4", callback(test4)},
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
