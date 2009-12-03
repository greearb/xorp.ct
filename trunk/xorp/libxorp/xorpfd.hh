// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/libxorp/xorpfd.hh,v 1.11 2008/10/02 21:57:37 bms Exp $

#ifndef __LIBXORP_XORPFD_HH__
#define __LIBXORP_XORPFD_HH__

#include "xorp.h"

#include "c_format.hh"

/**
 * @short XorpFd definition
 *
 * XorpFd is a wrapper class used to encapsulate a file descriptor.
 *
 * In order to prevent a situation where the developer has to explicitly
 * cast all arguments passed to I/O API calls (in order to keep the XORP
 * code base compatible across all the operating systems we support), we
 * define a wrapper class with casting operators for the underlying types.
 *
 * When constructed, we always initialize the encapsulated file descriptor
 * to an invalid value appropriate to the OS under which we are running.
 *
 * The non-Windows case is very simple. We do not define both sets of
 * functions at once so that the compiler will flag as an error those
 * situations where file descriptors are being used in a UNIX-like way,
 * i.e. where developers try to exploit the fact that UNIX file descriptors
 * are monotonically increasing integers.
 */

#define	BAD_XORPFD	(-1)

class XorpFd {
public:
    XorpFd() : _filedesc(BAD_XORPFD) {}

    XorpFd(int fd) : _filedesc(fd) {}

    operator int() const	{ return _filedesc; }

    string str() const		{ return c_format("%d", _filedesc); }

    void clear()		{ _filedesc = BAD_XORPFD; }

    bool is_valid() const	{ return (_filedesc != BAD_XORPFD); }

private:
    int	    _filedesc;
};

#endif // __LIBXORP_XORPFD_HH__
