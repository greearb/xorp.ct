// -*- c-basic-offset: 4 -*-

// Copyright (c) 2001,2002 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the Xorp LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the Xorp LICENSE file; the license in that file is
// legally binding.

// test_xrlsarglist: String Serialization Tests

#ident "$Id$"

#include "xrl_module.h"
#include "libxorp/xlog.h"
#include "xrlargs.hh"

static void
run_test()
{
    XrlArgs al;
    // Add some atoms.  NB most of these classes have constructors
    // that take string arguments.
    al.add_ipv4("a_named_ipv4", "128.16.6.31");
    al.add_ipv4net("a_named_ipv4net", "128.16.6.31/24");
    al.add_ipv6("a_named_ipv6", "fe80::2c0:f0ff:fe74:9f39");
    al.add_ipv6net("a_named_ipv6net", "fe80::2c0:f0ff:fe74:9f39/96");
    al.add_int32("first_int", 1).add_int32("second_int", 0xffffffff);
    al.add_uint32("only_uint", 0xffffffff);
    al.add_string("a_named_string", "the cat sat on the mat.");
    al.add_mac("a_named_mac", Mac("01:02:03:04:05:06"));
    al.add_string(0, "potentially hazardous null pointer named string object");
    al.add_string("bad_karma", "");

    XrlAtomList xal;
    xal.append(XrlAtom("first",string("fooo")));
    xal.append(XrlAtom("second",string("baar")));
    al.add(XrlAtom("a_list", xal));

    XrlArgs b(al.str().c_str());
    assert(b == al);

    b = al;
    assert(b == al);

    // Iterate through the list
    for (XrlArgs::const_iterator i = al.begin(); i != al.end(); i++) {
    	const XrlAtom& atom = *i;
	atom.has_data(); // No-op
    }

    try {
	al.get_uint32("only_uint");
    } catch (const XrlArgs::XrlAtomNotFound &e) {
	fprintf(stderr, "Atom not Found\n");
	exit(-1);
    } catch (...) {
	exit(-1);
    }
    
    try {
	al.get_ipv4("a_named_ipv4");
	al.get_ipv4net("a_named_ipv4net");
	al.get_ipv6("a_named_ipv6");
	al.get_ipv6net("a_named_ipv6net");
	al.get_int32("first_int");
	al.get_int32("second_int");
	al.get_uint32("only_uint");
	al.get_string("a_named_string");
	al.get_mac("a_named_mac");
	al.get_string(0);
	al.get_string("");
	al.get_string("bad_karma");
    } catch (XrlArgs::XrlAtomNotFound& e) {
	fprintf(stderr, "Failed to find atom.\n");
	exit(-1);
    }
}

int main(int /* argc */, char *argv[]) {
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    run_test();
    
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
