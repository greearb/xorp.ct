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

// $XORP: xorp/libxorp/exceptions.hh,v 1.2 2003/01/26 04:06:21 pavlin Exp $


#ifndef __LIBXORP_EXCEPTIONS_HH__
#define __LIBXORP_EXCEPTIONS_HH__

#include <exception>
#include <stdarg.h>
#include <string>
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
    XorpException(const char* init_what, const char* file, size_t line) 
	: _what(init_what), _file(file), _line(line) {}

    /**
     * Destructor
     */
    virtual ~XorpException() {}

    /**
     * Get the type of this exception.
     * 
     * @return the string with the type of this exception.
     */
    const string what() const { return _what; }

    /**
     * Get the location for throwing an exception.
     * 
     * @return the string with the location (file name and file line number)
     * for throwing an exception.
     */
    const string where() const { 
	return c_format("line %u of %s", (uint32_t)_line, _file); 
    }

    /**
     * Get the reason for throwing an exception.
     * 
     * @return the string with the reason for throwing an exception.
     */
    virtual const string why() const { return "Not specified"; }

    /**
     * Convert this exception from binary form to presentation format.
     * 
     * @return C++ string with the human-readable ASCII representation
     * of the exception.
     */
    string str() const {
	return what() + " from " + where() + ": " + why();
    }

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
			  size_t line, const string& init_why) 
	: XorpException(init_what, file, line), _why(init_why) {}

    /**
     * Get the reason for throwing an exception.
     * 
     * @return the string with the reason for throwing an exception.
     */
    const string why() const { 
	return ( _why.size() != 0 ) ? _why : string("Not specified"); 
    }

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
    InvalidString(const char* file, size_t line, const string& init_why = "") 
	: XorpReasonedException("InvalidString", file, line, init_why) {}
};

/**
 * @short A standard XORP exception that is thrown if an address is invalid.
 */
class InvalidAddress : public XorpReasonedException {
public:
    InvalidAddress(const char* file, size_t line, const string& init_why = "")
	: XorpReasonedException("InvalidAddress", file, line, init_why) {}
};

/**
 * @short A standard XORP exception that is thrown if a port is invalid.
 */
class InvalidPort : public XorpReasonedException {
public:
    InvalidPort(const char* file, size_t line, const string& init_why = "")
	: XorpReasonedException("InvalidPort", file, line, init_why) {}
};

/**
 * @short A standard XORP exception that is thrown if a cast is invalid.
 */
class InvalidCast : public XorpReasonedException {
public:
    InvalidCast(const char* file, size_t line, const string& init_why = "") 
	: XorpReasonedException("XorpCast", file, line, init_why) {}
};

/**
 * @short A standard XORP exception that is thrown if a buffer ofset is
 * invalid.
 */
class InvalidBufferOffset : public XorpReasonedException {
public:
    InvalidBufferOffset(const char* file, size_t line,
			const string& init_why = "")
	: XorpReasonedException("XorpInvalidBufferOffset", file, line,
				init_why) {}
};

/**
 * @short A standard XORP exception that is thrown if an address family is
 * invalid.
 */
class InvalidFamily : public XorpException {
public:
    InvalidFamily(const char* file, size_t line, int af) : 
	XorpException("XorpInvalidFamily", file, line), _af(af) {} 

    const string why() const { 
	    return c_format("Unknown IP family - %d", _af);
    }

protected:
    int _af;
};

/**
 * @short A standard XORP exception that is thrown if the packet is invalid.
 */
class InvalidPacket : public XorpReasonedException {
public:
    InvalidPacket(const char* file, size_t line, const string& init_why = "")
	: XorpReasonedException("XorpInvalidPacket", file, line, init_why) {}
};

/**
 * @short A standard XORP exception that is thrown if an IP netmask length is
 * invalid.
 */
class InvalidNetmaskLength : public XorpException {
public:
    InvalidNetmaskLength(const char* file, size_t line, int netmask_length) : 
	XorpException("XorpInvalidNetmaskLength", file, line), _netmask_length
(netmask_length) {} 

    const string why() const { 
	    return c_format("Invalid netmask length - %d", _netmask_length);
    }

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
