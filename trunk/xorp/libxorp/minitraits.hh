// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/libxorp/minitraits.hh,v 1.3 2003/12/18 17:56:51 hodson Exp $

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
