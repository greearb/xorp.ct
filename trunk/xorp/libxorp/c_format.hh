// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/libxorp/c_format.hh,v 1.13 2008/10/02 21:57:28 bms Exp $

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
inline int arg_count() { return 0; }


template <class A>
inline int arg_count(A) { return 1; }

template <class A, class B>
inline int arg_count(A,B) { return 2; }

template <class A, class B, class C>
inline int arg_count(A,B,C) { return 3; }

template <class A, class B, class C, class D>
inline int arg_count(A,B,C,D) { return 4; }

template <class A, class B, class C, class D, class E>
inline int arg_count(A,B,C,D,E) { return 5; }

template <class A, class B, class C,class D, class E, class F>
inline int arg_count(A,B,C,D,E,F) { return 6; }

template <class A, class B, class C, class D, class E, class F, class G>
inline int arg_count(A,B,C,D,E,F,G) { return 7; }

template <class A, class B, class C, class D, class E, class F, class G,
	  class H>
inline int arg_count(A,B,C,D,E,F,G,H) { return 8; }

template <class A, class B, class C, class D, class E, class F, class G,
	  class H, class I>
inline int arg_count(A,B,C,D,E,F,G,H,I) { return 9; }

template <class A, class B, class C, class D, class E, class F, class G,
	  class H, class I, class J>
inline int arg_count(A,B,C,D,E,F,G,H,I,J) { return 10; }

template <class A, class B, class C, class D, class E, class F, class G,
	  class H, class I, class J, class K>
inline int arg_count(A,B,C,D,E,F,G,H,I,J,K) { return 11; }

template <class A, class B, class C, class D, class E, class F, class G,
	  class H, class I, class J, class K, class L>
inline int arg_count(A,B,C,D,E,F,G,H,I,J,K,L) { return 12; }

template <class A, class B, class C, class D, class E, class F, class G,
	  class H, class I, class J, class K, class L, class M>
inline int arg_count(A,B,C,D,E,F,G,H,I,J,K,L,M) { return 13; }

template <class A, class B, class C, class D, class E, class F, class G,
	  class H, class I, class J, class K, class L, class M, class N>
inline int arg_count(A,B,C,D,E,F,G,H,I,J,K,L,M,N) { return 14; }

template <class A, class B, class C, class D, class E, class F, class G,
	  class H, class I, class J, class K, class L, class M, class N,
	  class O>
inline int arg_count(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O) { return 15; }

template <class A, class B, class C, class D, class E, class F, class G,
	  class H, class I, class J, class K, class L, class M, class N,
	  class O, class P>
inline int arg_count(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P) { return 16; }

template <class A, class B, class C, class D, class E, class F, class G,
	  class H, class I, class J, class K, class L, class M, class N,
	  class O, class P, class Q>
inline int arg_count(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q) { return 17; }

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
