// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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



#include "exceptions.hh"
#include <stdarg.h>
#include <stdio.h>

#ifndef XORP_USE_USTL
#include <typeinfo>
#endif


XorpException::XorpException(const char* init_what,
			     const char* file,
			     size_t line)
    : _what(init_what), _file(file), _line(line)
{
}

XorpException::~XorpException()
{
}

const string
XorpException::where() const
{
    return c_format("line %u of %s", XORP_UINT_CAST(_line), _file);
}

const string
XorpException::why() const
{
    return "Not specified";
}

string
XorpException::str() const
{
    return what() + " from " + where() + ": " + why();
}

XorpReasonedException::XorpReasonedException(const char* init_what,
					     const char* file,
					     size_t line,
					     const string& init_why)
    : XorpException(init_what, file, line), _why(init_why)
{
}

const string
XorpReasonedException::why() const
{
    return ( _why.size() != 0 ) ? _why : string("Not specified");
}

InvalidString::InvalidString(const char* file,
			     size_t line,
			     const string& init_why)
    : XorpReasonedException("InvalidString", file, line, init_why)
{
}

InvalidAddress::InvalidAddress(const char* file,
			       size_t line,
			       const string& init_why)
    : XorpReasonedException("InvalidAddress", file, line, init_why)
{
}

InvalidPort::InvalidPort(const char* file,
			 size_t line,
			 const string& init_why)
    : XorpReasonedException("InvalidPort", file, line, init_why)
{
}

InvalidCast::InvalidCast(const char* file,
			 size_t line,
			 const string& init_why)
    : XorpReasonedException("XorpCast", file, line, init_why)
{
}

InvalidBufferOffset::InvalidBufferOffset(const char* file,
					 size_t line,
					 const string& init_why)
    : XorpReasonedException("XorpInvalidBufferOffset", file, line, init_why)
{
}

InvalidFamily::InvalidFamily(const char* file,
			     size_t line,
			     int af)
    : XorpException("XorpInvalidFamily", file, line), _af(af)
{
}

const string
InvalidFamily::why() const
{
    return c_format("Unknown IP family - %d", _af);
}

InvalidPacket::InvalidPacket(const char* file,
			     size_t line,
			     const string& init_why)
    : XorpReasonedException("XorpInvalidPacket", file, line, init_why)
{
}


InvalidNetmaskLength::InvalidNetmaskLength(const char* file,
					   size_t line,
					   int netmask_length)
    : XorpException("XorpInvalidNetmaskLength", file, line),
      _netmask_length (netmask_length)
{
    // There was a case where fea was crashing due to un-caught exception.
    // Somehow, no useful info was being printed other than the exception
    // name.  So, add some logging here just in case it happens again.
    // (On reboot, couldn't cause the problem to happen again, so not sure
    // I actually fixed the root cause in fea yet.)
    cerr << "Creating InvalidNetmaskLength exception, file: "
     << file << ":" << line << " netmask_length: " << netmask_length
     << endl;
}

const string
InvalidNetmaskLength::why() const
{
    return c_format("Invalid netmask length - %d", _netmask_length);
}


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
