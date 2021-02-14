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

// $XORP: xorp/libxorp/c_format.hh,v 1.14 2008/10/03 17:41:31 atanu Exp $

#ifndef __LIBXORP_C_FORMAT_HH__
#define __LIBXORP_C_FORMAT_HH__

#include "libxorp/xorp.h"


//
// c_format is a macro that creates a string from a c-style format
// string.  It takes the same arguments as printf, but %n is illegal and
// will cause abort to be called.
//
// Pseudo prototype:
// 	string c_format(const char* format, ...);
//
// In practice c_format is a nasty macro, but by doing this we can check
// the compile time arguments are sane and the run time arguments.
//

#define c_format(format, args...) 					      \
    (c_format_validate(format, arg_count(args)), do_c_format(format, ## args))

//
// Template magic to allow us to count the number of varargs passed to
// the macro c_format.  We could also count the size of the var args data
// for extra protection if we were doing the formatting ourselves...
//

// Comment this out to find unnecessary calls to c_format()
template <class...Args> int arg_count(Args ... args) { return sizeof...(args);}

void c_format_validate(const char* fmt, int n);

#if defined(__printflike)
string do_c_format(const char* fmt, ...) __printflike(1,2);
#elif (defined(__GNUC__))
string do_c_format(const char* fmt, ...)
    __attribute__((__format__(printf, 1, 2)));
#else
string do_c_format(const char* fmt, ...);
#endif

#endif // __LIBXORP_C_FORMAT_HH__
