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

// $XORP: xorp/libxorp/minitraits.hh,v 1.10 2008/10/02 21:57:32 bms Exp $

#ifndef __LIBXORP_MINITRAITS_HH__
#define __LIBXORP_MINITRAITS_HH__

/**
 * @short Class to determine subset of type traits.
 *
 * This class can be use to determine the non-const form of a type.
 * It is a temporary fix for g++ 2.96 (Redhat) which has problems
 * tracking const pointer types in templates.
 */
template <typename T>
class MiniTraits {
    template <class U>
    struct UnConst {
	typedef U Result;
    };
    template <class U>
    struct UnConst <const U> {
        typedef U Result;
    };
public:
    typedef typename UnConst<T>::Result NonConst;
};

/**
 * @short Class to determine if two types are base and derived.
 *
 * This class tests whether a pointer for type B is useable as pointer
 * to type D.  Typically, this implies that B is a base for D.  It may also
 * imply that B is void, or B and D are the same type.
 *
 * How this works? Overloaded definition of function X::f().  The
 * first of which takes a const B* pointer as an argument and returns
 * are char.  The second of which takes a ... and returns a double.
 * sizeof is used to determine the size of the return type that would
 * used if the code were executed.  Thus, if B and D are compatible
 * pointer types then sizeof(X::f()) for both of them is sizeof(char).
 */
template <typename B, typename D>
class BaseAndDerived {
    struct X {
	static char f(const B*);
	static double f(...);
    };

public:
    static const bool True = ( sizeof(X::f((D*)0)) == sizeof(X::f((B*)0)) );
};

#endif // __LIBXORP_MINITRAITS_HH__
