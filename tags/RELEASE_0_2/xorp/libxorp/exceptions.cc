// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/exceptions.cc,v 1.2 2003/01/16 19:09:28 hodson Exp $"

#include <stdarg.h>
#include <stdio.h>

#include <exception>
#include <typeinfo>
#include <iostream>

#include "exceptions.hh"

// ----------------------------------------------------------------------------
// Handlers

void
xorp_catch_standard_exceptions() {
    xorp_print_standard_exceptions();
    terminate();
}

void
xorp_print_standard_exceptions() {
    try {
	throw;	// Re-throw so we can inspect exception type
    } catch (const XorpException& xe) {
	cerr << xe.what() << " from " << xe.where() << " -> " 
	     << xe.why()  << "\n";
    } catch (const exception& e) {
	cerr << "Standard exception: " 
	     << e.what() << " (name = \"" <<  typeid(e).name() << "\")\n";
    }
}

void
xorp_unexpected_handler(void) {
    cerr << "Unexpected exception: "
	 << "\tthrown did not correspond to specification - fix code.\n";
    xorp_catch_standard_exceptions();
}

// ----------------------------------------------------------------------------
// EXAMPLE

//#define XORP_EXAMPLE_USAGE
#ifdef XORP_EXAMPLE_USAGE

#include <bitset>

void foo() {
    // do some stuff that happens to throw the non-descript exception
    // let's say invalid characters are "la-la"
    xorp_throw(XorpInvalidString, 
	       xorp_format_string("invalid characters occurred \"%s\"", 
				  "la-la"));
}

int main() {
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    try {
	foo();		// will throw a XorpInvalidString
	bitset<8> bs;
	bs.set(1000);	// will throw out_of_range("bitset");
	foo();
    } catch (...) {
	xorp_catch_standard_exceptions();
    }
    
    return 0;
}

#endif
