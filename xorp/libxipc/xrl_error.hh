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


#ifndef	__LIBXIPC_XRL_ERROR_HH__
#define __LIBXIPC_XRL_ERROR_HH__

#include "libxorp/xorp.h"
#include "libxorp/c_format.hh"


class XrlErrlet;

enum XrlErrorCode {
    OKAY		  = 100,
    BAD_ARGS		  = 101,
    COMMAND_FAILED	  = 102,

    NO_FINDER		  = 200,
    RESOLVE_FAILED	  = 201,
    NO_SUCH_METHOD	  = 202,

    SEND_FAILED		  = 210,
    REPLY_TIMED_OUT	  = 211,
    SEND_FAILED_TRANSIENT = 212,

    INTERNAL_ERROR	  = 220
};

/**
 * All known error codes arising from XRL dispatches.  These include
 * underlying transport, transport location, and invocation failures.
 *
 * This class can be sub-classed to provide a sub-set of the known
 * errors, and also to append domain specific errors.
 */
class XrlError {
public:
    /**
     * The value that should be returned by functions whose execution
     * completed normally.
     */
    static const XrlError& OKAY();

    /**
     * The value that should be returned when the arguments in an XRL
     * do not match what the receiver expected.
     */
    static const XrlError& BAD_ARGS();

    /**
     * The value that should be returned when the command cannot be
     * executed by Xrl Target.
     */
    static const XrlError& COMMAND_FAILED();

    /**
     * The Xrl Finder process is not running or not ready to resolve
     * Xrl target names
     */
    static const XrlError& NO_FINDER();

    /**
     * Returned when an XRL cannot be dispatched because the target name
     * is not registered in the system.
     */
    static const XrlError& RESOLVE_FAILED();

    /**
     * Returned when the method within the XRL is not recognized by
     * the receiver.
     */
    static const XrlError& NO_SUCH_METHOD();

    /**
     * Returned when the underlying XRL transport mechanism fails.
     */
    static const XrlError& SEND_FAILED();

    /**
     * Returned when the reply is not returned within the timeout
     * period of the underlying transport mechanism.
     */
    static const XrlError& REPLY_TIMED_OUT();

    /**
     * Returned when the underlying XRL transport mechanism fails.
     */
    static const XrlError& SEND_FAILED_TRANSIENT();

    /**
     * An error has occurred within the XRL system.  This is usually a sign
     * of an implementation issue.  This error replaces no longer
     * existent errors of CORRUPT_XRL, CORRUPT_XRL_RESPONSE, and
     * BAD_PROTOCOL_VERSION.  The note associated with the error should
     * contain more information.
     */
    static const XrlError& INTERNAL_ERROR();

    /**
     * @return the unique identifer number associated with error.
     */
    XrlErrorCode error_code() const;

    bool isOK() const { return error_code() == ::OKAY; }

    /**
     * @return string containing textual description of error.
     */
    const char* error_msg() const;

    /**
     * @return string containing user annotation about source of error
     * (if set).
     */
    const string& note() const { return _note; }

    /**
     * @return string containing error_code(), error_msg(), and note().
     */
    string str() const {
	string r = c_format("%d ", error_code()) + error_msg();
	return note().size() ? (r + " " + note()) : r;
    }

    /**
     * @return true if error_code corresponds to known error.
     */
    static bool known_code(uint32_t code);

    XrlError();
    XrlError(XrlErrorCode error_code, const string& note = "");
    XrlError(const XrlError& xe) : _errlet(xe._errlet), _note(xe._note) {}

    XrlError& operator=(const XrlError& xe) {
	_errlet = xe._errlet;
	_note = xe._note;
	return *this;
    }

    /* Strictly for classes that have access to XrlErrlet to construct
       XrlError's */
    XrlError(const XrlErrlet& x, const string& note = "") :
	_errlet(&x), _note(note) {}

    XrlError(const XrlErrlet*);

protected:
    const XrlErrlet* _errlet;
    string	     _note;
};


/**
 * Error codes for user callbacks.
 * These are a subset of @ref XrlError
 * TODO:  Passing these on the stack wastes resources, and comparing
 *   them with generated values != XrlCmdError::OKAY(), for instance,
 *   is not good for performance or code size either.
 */
struct XrlCmdError {
public:
    /**
     * The default return value.  Indicates that the arguments to the
     * XRL method were correct.  Inability to perform operation should
     * still return OKAY(), but the return list should indicate the
     * error.
     */
    static const XrlCmdError& OKAY() { return _xce_ok; }

    /**
     * Return value when the method arguments are incorrect.
     */
    static const XrlCmdError BAD_ARGS(const string& reason = "") {
	return XrlError(XrlError::BAD_ARGS().error_code(), reason);
    }

    /**
     * Return value when the method could not be execute.
     */
    static const XrlCmdError COMMAND_FAILED(const string& reason = "") {
	return XrlError(XrlError::COMMAND_FAILED().error_code(), reason);
    }

    /**
     * Convert to XrlError (needed for XRL protocol families).
     */
    operator XrlError() const { return _xrl_error; }

    /**
     * @return string containing representation of command error.
     */
    string str() const { return string("XrlCmdError ") + _xrl_error.str(); }

    /**
     * @return the unique identifer number associated with error.
     */
    XrlErrorCode error_code() const { return _xrl_error.error_code(); }

    bool isOK() const { return _xrl_error.isOK(); }

    /**
     * @return note associated with origin of error (i.e., the reason).
     */
    const string& note() const { return _xrl_error.note(); }

private:
    XrlCmdError(const XrlError& xe) : _xrl_error(xe) {}
    XrlError _xrl_error;
    static XrlCmdError _xce_ok;
};


/**
 * Test for equality between a pair of XrlError instances.  The test
 * only examines the error codes associated with each instance.
 */
inline bool operator==(const XrlError& e1, const XrlError& e2)
{
    return e1.error_code() == e2.error_code();
}

/**
 * Test for inequality between a pair of XrlError instances.  The test
 * only examines the error codes associated with each instance.
 */
inline bool operator!=(const XrlError& e1, const XrlError& e2)
{
    return e1.error_code() != e2.error_code();
}

#endif // __LIBXIPC_XRL_ERROR_HH__
