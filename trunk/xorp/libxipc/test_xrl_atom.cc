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

// test_xrlatom: String Serialization Tests

#ident "$Id$"

#include "config.h"
#include "libxorp/xorp.h"

#include "xrl_module.h"
#include "libxorp/xlog.h"

#include "xrl_atom.hh"
#include "xrl_atom_encoding.hh"

static bool g_trace = false;
#define tracef(args...) if (g_trace) printf(args)

static void
dump(const uint8_t* p, size_t p_bytes) {
    for (size_t i = 0; i < p_bytes; i++) {
	tracef("%02x ", p[i]);
    }
    tracef("\n");
}

static bool
assignment_test(const XrlAtom& a) {
    XrlAtom b(a);
    tracef("Assignment test... %s\n", (a == b) ? "yes" : "NO");
    return (a == b);
}

static bool
ascii_test(const XrlAtom& a) {
    // Test 1 string serialization
    XrlAtom b(a.str().c_str());
    if (a != b) {
	tracef("%s %d %d\n", a.str().c_str(), a.type(), a.has_data());
	tracef("%s %d %d\n", b.str().c_str(), b.type(), b.has_data());
    }
    tracef("\tASCII Serialization... %s\n", (a == b) ? "yes" : "NO");
    return (a == b);
}

static bool
binary_test(const XrlAtom& a) {
    XrlAtom b;
    vector<uint8_t> buffer(a.packed_bytes());
    if (a.pack(buffer.begin(), buffer.size()) == a.packed_bytes()) {
	dump(buffer.begin(), buffer.size());
	b.unpack(buffer.begin(), buffer.size());
    }
    tracef("\tBinary Serialization I...");
    tracef("%s\n", (a == b) ? "yes" : "NO");
    return (a == b);
}

static bool
binary_test2(const XrlAtom& a) {
    tracef("\tBinary Serialization II...");

    // buffer is too small shouldn't be able to pack
    XrlAtom b;
    vector<uint8_t> buffer(a.packed_bytes() - 1);
    if (a.pack(buffer.begin(), buffer.size()) != 0) {
	tracef("NO\n");
	return false;
    } else {
	tracef("yes\n");
	return true;
    }
}

static bool
binary_test3(const XrlAtom& a) {
    tracef("\tBinary Serialization III...");

    XrlAtom b;
    // Buffer is too big, shouldn't make any difference.
    vector<uint8_t> buffer(a.packed_bytes() + 1);
    if (a.pack(buffer.begin(), buffer.size()) == a.packed_bytes()) {
	b.unpack(buffer.begin(), buffer.size());
    }
    tracef("%s\n", (a == b) ? "yes" : "NO");
    return a == b;
}

static void
test_atom(const XrlAtom& a)
{
    
    // Print test name
    tracef("Testing %s %s\n", 
	   (a.name().size()) ? "Named" : "Unnamed",
	   a.type_name().c_str());

    const char* name = a.name().c_str();
    const char* tname = a.type_name().c_str();

    if (assignment_test(a) == false) {
	fprintf(stderr, "Failed assignment test: %s:%s", name, tname);
	exit(-1);
    } else if (ascii_test(a) == false) {
	fprintf(stderr, "Failed ascii test: %s:%s", name, tname);
	exit(-1);
    } else if (binary_test(a) == false) {
	fprintf(stderr, "Failed binary test: %s:%s", name, tname);
	exit(-1);
    } else if (binary_test2(a) == false) {
	fprintf(stderr, "Failed binary test 2: %s:%s", name, tname);
	exit(-1);
    } else if (binary_test3(a) == false) {
	fprintf(stderr, "Failed binary test 3: %s:%s", name, tname);
	exit(-1);
    }
}

static void
test()
{
    // This is a simple switch to prevent compilation of this test
    // when new XrlAtom types are added. Please add your type below
    // *and* write a test for it.
    for (XrlAtomType t = xrlatom_start /* xrlatom_list */; t != xrlatom_no_type; ++t) {
	switch(t) {
	case xrlatom_no_type:
	    // No test
	    break;
	case xrlatom_boolean:
	    test_atom(XrlAtom("test_boolean_value", true));
	    break;
	case xrlatom_int32:
	    //	    test_atom(XrlAtom(int32_t(0x12345678)));
	    test_atom(XrlAtom("test_int32_value", int32_t(0x12345678)));
	    break;
	case xrlatom_uint32:
	    //	    test_atom(XrlAtom(uint32_t(0xfedcba98)));
	    test_atom(XrlAtom("test_uint32_value", uint32_t(0xfedcba98)));
	    break;
	case xrlatom_ipv4:
	    //	    test_atom(XrlAtom(IPv4("128.16.64.84")));
	    test_atom(XrlAtom("test_ipv4_value", IPv4("128.16.64.72")));
	    break;
	case xrlatom_ipv4net:
	    //	    test_atom(XrlAtom(IPv4Net("128.16.64.84/24")));
	    test_atom(XrlAtom("test_net4_value", IPv4Net("128.16.64.72/24")));
	    break;
	case xrlatom_ipv6:
	    //	    test_atom(XrlAtom(IPv6("fe80::2c0:4fff:fea1:1a71")));
	    test_atom(XrlAtom("test_ip6_value", 
			      IPv6("fe80::2c0:4fff:fea1:1a71")));
	    break;
	case xrlatom_ipv6net:
	    //	    test_atom(XrlAtom(IPv6Net("fe80::2c0:4fff:fea1:1a71/96")));
	    test_atom(XrlAtom("A_net6_value", 
			      IPv6Net("fe80::2c0:4fff:fea1:1a71/96")));
	    break;
	case xrlatom_mac:
	    //	    test_atom(XrlAtom(Mac("11:22:33:44:55:66")));
	    test_atom(XrlAtom("Some_Ethernet_Mac_address_you_have_there_sir", 
			      Mac("11:22:33:44:55:66")));
	    break;
	case xrlatom_text:
	    {
		string t = "ABCabcDEFdef1234 !@#$%^&*(){}[]:;'<>";
#ifdef VERBOSE_STRING_TEST
		{
		    string encoded = xrlatom_encode_value(t);
		    cout << "Original: \"" << t << "\"" << endl;
		    cout << "Encoded:  \"" << encoded << "\"" << endl;
		    string decoded;
		    xrlatom_decode_value(encoded.c_str(), encoded.size(),
					 decoded);
		    cout << "Decoded:  \"" << decoded << "\"" << endl;
		}
#endif /* VERBOSE_STRING_TEST */
		test_atom(XrlAtom("A_string_object", t));
		test_atom(XrlAtom("Empty_string_of_mine", string("")));
	    }
	    break;
	case xrlatom_list:
	    {
		XrlAtomList xl;
		xl.append(XrlAtom ("string_1", string("abc")));
		xl.append(XrlAtom ("string_2", string("def")));
		xl.append(XrlAtom ("string_3", string("ghi")));
		tracef("XrlAtomList looks like: %s\n", 
		       XrlAtom("foo",xl).str().c_str());
		test_atom(XrlAtom("An-XrlAtomList", xl));
		tracef("---");
		XrlAtomList yl;
		test_atom(XrlAtom("An-XrlAtomList", yl));
	    }
	    break;
	case xrlatom_binary:
	    {
		for (int sz = 1; sz < 1000; sz++) {
		    vector<uint8_t> t(sz);
		    for (int i = 1; i < sz; i++) {
			t.push_back(random());
		    }
		    test_atom(XrlAtom("binary_data", t));
		}
	    }
	    break;
	}
    }
}

int 
main(int argc, const char *argv[]) {
    

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    if (argc == 2 && 	argv[1][0] == '-' && argv[1][1] == 'v')
	g_trace = true;

    XorpUnexpectedHandler x(xorp_unexpected_handler);
    try {
	test();
    } catch (...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
