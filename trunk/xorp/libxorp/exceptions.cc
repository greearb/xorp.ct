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

#ident "$XORP: xorp/libxorp/exceptions.cc,v 1.10 2008/10/02 21:57:30 bms Exp $"

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
