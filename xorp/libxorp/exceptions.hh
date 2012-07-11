// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/libxorp/exceptions.hh,v 1.12 2009/01/05 18:30:57 jtc Exp $


#ifndef __LIBXORP_EXCEPTIONS_HH__
#define __LIBXORP_EXCEPTIONS_HH__

#include "xorp.h"

#ifdef XORP_USE_USTL
#include <ustl/uexception.h>
using namespace std;
#else
#include <exception>
#endif

#include <stdarg.h>

#include "libxorp/c_format.hh"

/**
 * Macro to known insert values into exception arguments.
 */
#define xorp_throw(_class, args...) \
	throw _class(__FILE__, __LINE__, args);

#define xorp_throw0(_class) \
	throw _class(__FILE__, __LINE__);

/**
 * @short A base class for XORP exceptions.
 */
class XorpException {
public:
    /**
     * Constructor for a given type for exception, file name,
     * and file line number.
     *
     * @param init_what the type of exception.
     * @param file the file name where the exception was thrown.
     * @param line the line in @ref file where the exception was thrown.
     */
    XorpException(const char* init_what, const char* file, size_t line);

    /**
     * Destructor
     */
    virtual ~XorpException();

    /**
     * Get the type of this exception.
     *
     * @return the string with the type of this exception.
     */
    const string& what() const { return _what; }

    /**
     * Get the location for throwing an exception.
     *
     * @return the string with the location (file name and file line number)
     * for throwing an exception.
     */
    const string where() const;

    /**
     * Get the reason for throwing an exception.
     *
     * @return the string with the reason for throwing an exception.
     */
    virtual const string why() const;

    /**
     * Convert this exception from binary form to presentation format.
     *
     * @return C++ string with the human-readable ASCII representation
     * of the exception.
     */
    string str() const;

protected:
    string	_what;		// The type of exception
    const char*	_file;		// The file name where exception occured
    size_t	_line;		// The line number where exception occured
};

/**
 * @short A base class for XORP exceptions that keeps the reason for exception.
 */
class XorpReasonedException : public XorpException {
public:
    /**
     * Constructor for a given type for exception, file name,
     * file line number, and a reason.
     *
     * @param init_what the type of exception.
     * @param file the file name where the exception was thrown.
     * @param line the line in @ref file where the exception was thrown.
     * @param init_why the reason for the exception that was thrown.
     */
    XorpReasonedException(const char* init_what, const char* file,
			  size_t line, const string& init_why);

    /**
     * Get the reason for throwing an exception.
     *
     * @return the string with the reason for throwing an exception.
     */
    const string why() const;

protected:
    string _why;		// The reason for the exception
};

// ----------------------------------------------------------------------------
// Standard XORP exceptions

/**
 * @short A standard XORP exception that is thrown if a string is invalid.
 */
class InvalidString : public XorpReasonedException {
public:
    InvalidString(const char* file, size_t line, const string& init_why = "");
};

/**
 * @short A standard XORP exception that is thrown if an address is invalid.
 */
class InvalidAddress : public XorpReasonedException {
public:
    InvalidAddress(const char* file, size_t line, const string& init_why = "");
};

/**
 * @short A standard XORP exception that is thrown if a port is invalid.
 */
class InvalidPort : public XorpReasonedException {
public:
    InvalidPort(const char* file, size_t line, const string& init_why = "");
};

/**
 * @short A standard XORP exception that is thrown if a cast is invalid.
 */
class InvalidCast : public XorpReasonedException {
public:
    InvalidCast(const char* file, size_t line, const string& init_why = "");
};

/**
 * @short A standard XORP exception that is thrown if a buffer ofset is
 * invalid.
 */
class InvalidBufferOffset : public XorpReasonedException {
public:
    InvalidBufferOffset(const char* file, size_t line,
			const string& init_why = "");
};

/**
 * @short A standard XORP exception that is thrown if an address family is
 * invalid.
 */
class InvalidFamily : public XorpException {
public:
    InvalidFamily(const char* file, size_t line, int af);
    const string why() const;

protected:
    int _af;
};

/**
 * @short A standard XORP exception that is thrown if the packet is invalid.
 */
class InvalidPacket : public XorpReasonedException {
public:
    InvalidPacket(const char* file, size_t line, const string& init_why = "");
};

/**
 * @short A standard XORP exception that is thrown if an IP netmask length is
 * invalid.
 */
class InvalidNetmaskLength : public XorpException {
public:
    InvalidNetmaskLength(const char* file, size_t line, int netmask_length);
    const string why() const;

protected:
    int _netmask_length;
};


// ----------------------------------------------------------------------------

/**
 * Print diagnostic message if exception is derived from @ref XorpException or
 * from standard exceptions, and terminate the program.
 */
void xorp_catch_standard_exceptions();

/**
 * Print diagnostic message if exception is derived from @ref XorpException or
 * from standard exceptions.
 *
 * Note that unlike @ref xorp_catch_standard_exceptions(), the program
 * does NOT terminate.
 */
void xorp_print_standard_exceptions();

/**
 * Unexpected exceptions are programming errors and this handler can
 * be installed with XorpUnexpectedHandler to print out as much
 * information as possible.  The default behaviour is just to dump
 * core, but it very hard to detect what happened.
 */
void xorp_unexpected_handler();

/**
 * XorpUnexpectedHandler installs the xorp_unexpected_handler when
 * instantiated and re-installed the previous handler when
 * uninstantiated.
 */
class XorpUnexpectedHandler {
public:
    XorpUnexpectedHandler(unexpected_handler h = xorp_unexpected_handler) {
	_oh = set_unexpected(h);
    }
    ~XorpUnexpectedHandler() { set_unexpected(_oh); }
private:
    unexpected_handler _oh;
};

#endif // __LIBXORP_EXCEPTIONS_HH__
