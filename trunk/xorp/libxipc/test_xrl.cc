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

#ident "$XORP: xorp/libxipc/test_xrl.cc,v 1.8 2003/06/19 00:44:42 hodson Exp $"

// test_xrl: String Serialization Tests

#include <iostream>

#include "xrl_module.h"
#include "libxorp/xlog.h"
#include "xrl.hh"
#include "xrl_args.hh"

static int
xrl_test(const char* testname, const Xrl& x, const Xrl& y)
{
    if (x != y) {
	cout << "Test failed: " << testname << endl;
        cout << "\t" << x.str() << "!=" << y.str().c_str() << endl;
	cout << "\t\t" << x.protocol() << "\t" << y.protocol() << endl;
	cout << "\t\t" << x.target() << "\t" << y.target() << endl;
	cout << "\t\t" << x.command() << "\t" << y.command() << endl;
	cout << "\t\t" << x.args().str() << "\t" << y.args().str()
	     << endl;
        return 1;
    }
    return 0;
}

static int
run_test()
{
    int  failures = 0;
    bool failure = false;

    struct test_args {
	const char*	testname;
	XrlAtom		arg;
    } tests [] = {
	{
	    "Xrl named int32",
	    XrlAtom("foo-i32", int32_t(987654321))
	},
	{
	    "Xrl named uint32",
	    XrlAtom("foo-u32", uint32_t(0xcafebabe))
	},
	{
	    "Xrl named ipv4",
	    XrlAtom("foo-ipv4", IPv4("128.16.64.84"))
	},
	{
	    "Xrl named ipv6",
	    XrlAtom("foo-ipv6", IPv6("fe80::2c0:4fff:fea1:1a71"))
	},
	{
	    "Xrl named mac",
	    XrlAtom("foo-ether", Mac("aa:bb:cc:dd:ee:ff"))
	},
	{
	    "Xrl named string",
	    XrlAtom("foo-string",
		    string("ABCabc DEFdef 1234 !@#$%^&*(){}[]:;'\"<>"))
	},
    };

    uint32_t ntests = sizeof(tests) / sizeof(tests[0]);
    for (uint32_t i = 0; i < ntests; i++) {
	failure = false;
	Xrl x("some target", "take an argument");
	x.args().add(tests[i].arg);
        Xrl sx(x.str().c_str());
        failure = xrl_test(tests[i].testname, x, sx);
	failures += (failure) ? 1 : 0;
    }

    for (uint32_t i = 0; i < ntests; i++) {
	for (uint32_t j = 0; j < ntests; j++) {
	    // Skip if two arguments are unnamed
	    if ((strstr(tests[i].testname, "unnamed") != 0) &&
		(strstr(tests[j].testname, "unnamed") != 0)) {
		continue;
	    }

	    // Skip if two arguments would have the same name
	    if (i == j) {
		continue;
	    }


	    try {
		failure = false;
		Xrl x("some target", "take an argument");
		x.args().add(tests[i].arg);
		x.args().add(tests[j].arg);

		Xrl sx(x.str().c_str());
		string nom = string(tests[i].testname) + string(" + ") +
		    string(tests[j].testname);
		failure = xrl_test(nom.c_str(), x, sx);
		failures += (failure) ? 1 : 0;
	    } catch (const InvalidString&) {
		cout << "invalid string (" << i << ", " << j << ")" << endl;
		failures++;
		break;
	    } catch (const XrlArgs::XrlAtomFound&) {
		cout << "Adding same argument twice ("
		     << tests[i].testname << ", " << tests[j].testname << ")"
		     << endl;
		failures++;
	    }
	}
    }
    return failures;
}

int main(int /* argc */, char *argv[])
{
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    int r = run_test();
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return r;
}
