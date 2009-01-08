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

#ident "$XORP: xorp/libxipc/test_xrl.cc,v 1.18 2008/10/02 21:57:22 bms Exp $"

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

    //
    // XXX: This doesn't belong here, but gives the XRL packing and unpacking
    // a pretty good running through (i.e., it found some bugs in the first cut
    // of the packing code :-)
    //
    vector<uint8_t> buffer;
    buffer.resize(x.packed_bytes());

    x.pack(&buffer[0], buffer.size());
    Xrl z;
    if (z.unpack(&buffer[0], buffer.size()) != x.packed_bytes()) {
 	cout << "Unpacking failed " << x.str() << " != " << z.str() << endl;
 	return 1;
    }
    if (z != x) {
 	cout << "Unpacking produced bad result " << x.str() << " != " << z.str() << endl;
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
	    XrlAtom("foo-mac", Mac("aa:bb:cc:dd:ee:ff"))
	},
	{
	    "Xrl named string",
	    XrlAtom("foo-string",
		    string("ABCabc DEFdef 1234 !@#$%^&*(){}[]:;'\"<>"))
	},
	{
	    "Xrl named int64",
	    XrlAtom("foo-i64", int64_t(876543210123456789LL))
	},
	{
	    "Xrl named uint32",
	    XrlAtom("foo-u64", uint64_t(0xf00fdeadbabecafeULL))
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
