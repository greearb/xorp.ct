// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/bgp/exceptions.hh,v 1.16 2008/11/08 06:14:36 mjh Exp $

#ifndef __BGP_EXCEPTIONS_HH__
#define __BGP_EXCEPTIONS_HH__

#include "libxorp/xorp.h"
#include "libxorp/exceptions.hh"

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif


/**
 * This exception is thrown when a bad input message is received.
 */
class CorruptMessage : public XorpReasonedException {
public:
    static const size_t MAXPACKETSIZE = 4096;
    CorruptMessage(const char* file, size_t line, const string init_why = "")
 	: XorpReasonedException("CorruptMessage", file, line, init_why),
	  _error(0), _subcode(0), _len(0)
    {}

    CorruptMessage(const char* file, size_t line,
		   const string init_why,
		   const int error, const int subcode)
 	: XorpReasonedException("CorruptMessage", file, line, init_why),
	  _error(error), _subcode(subcode), _len(0)
    {}

    CorruptMessage(const char* file, size_t line,
		   const string init_why,
		   const int error, const int subcode,
		   const uint8_t *data, const size_t len)
 	: XorpReasonedException("CorruptMessage", file, line, init_why),
	  _error(error), _subcode(subcode), _len(len)
    {assert(_len < MAXPACKETSIZE); memcpy(_data, data, _len);}

    int error() const				{ return _error; }
    int subcode() const				{ return _subcode; }
    const uint8_t *data() const			{ return _data; }
    size_t len() const				{ return _len; }

private:
    const int		_error;
    const int		_subcode;
    // this can't reference external data, as that may go out of
    // scope, so have a large enough buffer here to store anything we
    // need to return
    uint8_t	        _data[MAXPACKETSIZE];
    const size_t	_len;
};

/**
 * This exception is thrown when an update message is received, and we
 * find it unusable, but not so bad that we want to send a
 * Notification and terminate the connection.
 */
class UnusableMessage : public XorpReasonedException {
public:
    UnusableMessage(const char* file, size_t line,
		    const string init_why)
	: XorpReasonedException("CorruptMessage", file, line, init_why)
    {}
};

class NoFinder : public XorpReasonedException {
public:
    NoFinder(const char* file, size_t line, const string init_why = "")
 	: XorpReasonedException("NoFinder", file, line, init_why)
    {}
};

#endif // __BGP_EXCEPTIONS_HH__
