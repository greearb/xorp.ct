// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/test_types.cc,v 1.4 2002/12/09 18:29:14 hodson Exp $"

#include <iostream>

#include "libxorp_module.h"
#include "xorp.h"
#include "xlog.h"
#include "ipv4net.hh"
#include "ipv6net.hh"

template <class C> static bool
test_serialization(const string& name, const C& c)
{
    string serialized = c.str();
    if (c.str() == C(serialized.c_str()).str()) 
	return false;
    cerr << "Failed serialization test for " << name << endl;
    return true;
}

static int
test_serializations()
{
    if (test_serialization("IPv4", IPv4("10.1.2.3"))) {
	return -1;
    } else if (test_serialization("IPv4Net", IPv4Net("10.1.2.3/24"))) {
	return -1;
    } else if (test_serialization("IPv6", IPv6("fe80::2c0:4fff:fe68:8c58"))) {
	return -1;
    } else if (test_serialization("IPv6", 
				  IPv6Net("fe80::2c0:4fff:fe68:8c58/23"))) {
	return -1;
    }
    return 0;
}

static int
test_ipv4_operators()
{
    IPv4 ip4("192.150.187.250");
    if (IPv4("0.0.0.0") == ip4) {
	cerr << "First IPv4 equality test failed." << endl; 
	return -1;
    } else if (IPv4("192.150.187.250") != ip4) {
	cerr << "Second IPv4 equality test failed." << endl; 
	return -1;
    }else if (IPv4("192.150.187.251") != ++ip4) {
	cerr << "Failed IPv4 increment test" << endl;
	return -1;
    } else if (IPv4("192.150.187.250") != --ip4) {
	cerr << "Failed IPv4 decrement test" << endl;
	return -1;
    } else if ((ip4 << 1) != IPv4("129.45.119.244")) {
	cerr << "Failed IPv4 left roll test" << endl;
	return -1;
    } else if ((ip4 >> 1) != IPv4("96.75.93.253")) {
	cerr << "Failed IPv4 right roll test" << endl;
	return -1;
    }

    return 0;
}

int
main (int /* argc */, char *argv[]) 
{
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);         // Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    XorpUnexpectedHandler x(xorp_unexpected_handler);
    int r = -1;
    try {
	r  = test_serializations();
	r |= test_ipv4_operators();
    } catch (...) {
        xorp_catch_standard_exceptions();
    }
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return r;
}

