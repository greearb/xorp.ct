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

#ident "$XORP: xorp/libxorp/test_mac.cc,v 1.5 2002/12/09 18:29:14 hodson Exp $"

#include "config.h"
#include "libxorp/xorp.h"
#include "libxorp/mac.hh"

bool
test1()
{
    /*
    ** Try and create a Mac with a bad format.
    */
    try {
	Mac m1("hello");
	return false;
    } catch(const XorpException& xe) {
	cout << xe.what() << " from " << xe.where() << " -> " 
	     << xe.why()  << "\n";
    }

    return true;
}

bool
test2()
{
    /*
    ** Create an EtherMac go to Mac and back to EtherMac.
    */
    try {
 	EtherMac m1("aa:aa:aa:aa:aa:aa");
	cout << "EtherMac: m1 " << m1.str() << "\n";
	Mac m2 = m1;
	cout << "Mac: m2 " << m2.str() << "\n";
	EtherMac m3 = m2;
	cout << "EtherMac: m3 " << m3.str() << "\n";
    } catch(exception& e) {
	cerr << "Exception: " << e.what() << "\n";
	return false;
    } catch(const XorpException& xe) {
	cout << xe.what() << " from " << xe.where() << " -> " 
	     << xe.why()  << "\n";
	return false;
    }

    return true;
}

bool
test3()
{
    /*
    ** Serialize and deserialize Mac
    */
    try {
 	Mac m1("bb:aa:aa:aa:aa:aa");
	string ser = m1.str();
	cout << "Serialized: " << ser << "\n";
	Mac m2(ser);
	cout << "Deserialized: " <<   m2.str() << "\n";
    } catch(exception& e) {
	cerr << "Exception: " << e.what() << "\n";
	return false;
    } catch(const XorpException& xe) {
	cout << xe.what() << " from " << xe.where() << " -> " 
	     << xe.why()  << "\n";
	return false;
    }

    return true;
}

bool
test4()
{
    /*
    ** Serialize and deserialize EtherMac
    */
    try {
 	EtherMac m1("bb:aa:aa:aa:aa:aa");
	string ser = m1.str();
	cout << "Serialized: " << ser << "\n";
	Mac m2(ser);
	cout << "Deserialized: " <<   m2.str() << "\n";
	EtherMac m3 = m2;
	cout << "Back to an EtherMac: " << m3.str() << "\n";
    } catch(exception& e) {
	cerr << "Exception: " << e.what() << "\n";
	return false;
    } catch(const XorpException& xe) {
	cout << xe.what() << " from " << xe.where() << " -> " 
	     << xe.why()  << "\n";
	return false;
    }

    return true;
}

int
main()
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    bool passed = true;

    struct test {
	const char *name;
	bool (*func)();
    } tests[] = {
	{"test1 Bad Mac test", test1},
	{"test2 Mac -> EtherMac -> Mac", test2},
	{"test3 Serialize/Deserialize Mac", test3},
	{"test4 Serialize/Deserialize EtherMac", test4},
    };
	
    int ntest = sizeof(tests) / sizeof(test);
    int i;

    for(i = 0; i < ntest; i++) {
	cout << "Running: " << tests[i].name << "\n";
	if(tests[i].func()) {
	    cout << "TEST\t\t\tOK\n";
	} else {
	    cout << "TEST\t\t\tFAILED\n";
	    passed = false;
	}
    }

    return true == passed ? 0 : -1;
}
