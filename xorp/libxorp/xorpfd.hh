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

#ifndef __LIBXORP_XORPFD_HH__
#define __LIBXORP_XORPFD_HH__

#include "xorp.h"

#include "c_format.hh"

/**
 * @short XorpFd definition
 *
 * XorpFd is a wrapper class used to encapsulate a file descriptor.
 *
 * It exists because of fundamental differences between UNIX and Windows
 * in terms of how the two families of operating systems deal with file
 * descriptors; in most flavours of UNIX, all file descriptors are
 * created equal, and may be represented using an 'int' type which is
 * usually 32 bits wide. In Windows, sockets are of type SOCKET, which
 * is a typedef alias of u_int; whereas all other system objects are
 * of type HANDLE, which in turn is a typedef alias of 'void *'.
 *
 * The situation is made even more confusing by the fact that under
 * Windows, SOCKETs and HANDLEs may both be passed to various Windows
 * API functions.
 *
 * In order to prevent a situation where the developer has to explicitly
 * cast all arguments passed to such functions (in order to keep the XORP
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
 *
 * XXX: Because Windows defines HANDLE in terms of a pointer, but also
 * defines SOCKET in terms of a 32-bit-wide unsigned integer, beware of
 * mixing 32-bit and 64-bit comparisons under Win64 when working with
 * socket APIs (or indeed any C/C++ library which will potentially do
 * work with sockets under Win64 such as libcomm).
 */

#ifdef HOST_OS_WINDOWS
#define	BAD_XORPFD	INVALID_HANDLE_VALUE
#else
#define	BAD_XORPFD	(-1)
#endif

#ifndef	HOST_OS_WINDOWS
// Non-Windows code.
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

#else // HOST_OS_WINDOWS
// Windows code.
class XorpFd {
public:
    enum WinFdType {
	FDTYPE_ERROR,		// Invalid handle or method failure
	FDTYPE_FILE,		// Disk file
	FDTYPE_CONSOLE,		// Console or character device
	FDTYPE_PIPE,		// Named or anonymous pipe
	FDTYPE_SOCKET,		// Socket
	FDTYPE_PROCESS,		// Process handle
	FDTYPE_OTHER		// Unknown handle type
    };

private:
    //
    // Helper function to return what kind of object the encapsulated
    // Windows object handle points to. Optimized for sockets.
    //
    WinFdType get_type() const {
	if (!this->is_valid())
	    return (FDTYPE_ERROR);

	// Try to find invalid handles quickly at the cost of 1 syscall.
	DWORD dwflags;
	if (GetHandleInformation(*this, &dwflags) == 0)
	    return (FDTYPE_ERROR);

	struct sockaddr_storage ss;
	socklen_t len = sizeof(ss);
	int ret = getsockname(*this, (struct sockaddr *)&ss, &len);
	if (ret != -1)
	    return (FDTYPE_SOCKET);
	else if (GetLastError() == WSAEINVAL)
	    return (FDTYPE_ERROR);

	DWORD ntype = GetFileType(*this);
	switch (ntype) {
	case FILE_TYPE_CHAR:
	    return (FDTYPE_CONSOLE);
	    break;
	case FILE_TYPE_DISK:
	    return (FDTYPE_FILE);
	    break;
	case FILE_TYPE_PIPE:
	    return (FDTYPE_PIPE);
	    break;
	default:
	    if (GetLastError() != NO_ERROR) {
		if (0 != GetProcessId(*this)) {
		    return (FDTYPE_PROCESS);
		}
		return (FDTYPE_ERROR);
	    }
	    break;
	}

	return (FDTYPE_OTHER);
    }

public:
    XorpFd() : _filedesc(BAD_XORPFD), _type(FDTYPE_ERROR) {}

    XorpFd(HANDLE h) : _filedesc(h), _type(get_type()) {}

    // _get_osfhandle() returns a long. We need to force a call
    // to get_type() to discover the underlying handle type.
    XorpFd(long l)
     : _filedesc(reinterpret_cast<HANDLE>(l)), _type(get_type())
    {}

    XorpFd(SOCKET s)
     : _filedesc(reinterpret_cast<HANDLE>(s)), _type(FDTYPE_SOCKET)
    {}

    XorpFd(const XorpFd& rhand)
     : _filedesc(rhand._filedesc), _type(rhand._type)
    {}

    operator HANDLE() const { return _filedesc; }

    operator SOCKET() const { return reinterpret_cast<SOCKET>(_filedesc); }

    void clear() { _filedesc = BAD_XORPFD; _type = FDTYPE_ERROR; }

    string str() const { return c_format("%p", _filedesc); }

    bool is_valid() const { return (_filedesc != BAD_XORPFD); }

    WinFdType type() const { return _type; }

    bool is_console() const { return (_type == FDTYPE_CONSOLE); }

    bool is_process() const { return (_type == FDTYPE_PROCESS); }

    bool is_pipe() const { return (_type == FDTYPE_PIPE); }

    bool is_socket() const { return (_type == FDTYPE_SOCKET); }

    // On Windows, HANDLE is a void *.
    // Because there are several cast operators, and any may be
    // invoked implicitly in the context of an expression containing
    // an instance of XorpFd, we must disambiguate by providing
    // comparison operators here.

    bool operator ==(const XorpFd& rhand) const {
	return (_filedesc == rhand._filedesc);
    }

    bool operator !=(const XorpFd& rhand) const {
	return (_filedesc != rhand._filedesc);
    }

    bool operator >(const XorpFd& rhand) const {
	return (_filedesc > rhand._filedesc);
    }

    bool operator <(const XorpFd& rhand) const {
	return (_filedesc < rhand._filedesc);
    }

private:
    HANDLE	_filedesc;
    WinFdType	_type;
};
#endif // HOST_OS_WINDOWS

#endif // __LIBXORP_XORPFD_HH__
