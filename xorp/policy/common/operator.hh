// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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


#ifndef __POLICY_COMMON_OPERATOR_HH__
#define __POLICY_COMMON_OPERATOR_HH__

#include "operator_base.hh"

#define DEFINE_OPER(name,human,parent) \
class name : public parent { \
public: \
    static Hash _hash; \
    name() : parent(_hash) {} \
    ~name() {} \
    string str() const { return #human; } \
    Hash hash() const { return _hash; } \
    void set_hash(const Hash& x) const { _hash = x; } \
}; 

#define DEFINE_UNOPER(name,human) \
DEFINE_OPER(name,human,UnOper)

#define DEFINE_BINOPER(name,human) \
DEFINE_OPER(name,human,BinOper)

// Logical operators
DEFINE_BINOPER(OpAnd,AND)
DEFINE_BINOPER(OpOr,OR)
DEFINE_BINOPER(OpXor,XOR)

// Relational operators
DEFINE_BINOPER(OpEq,==)
DEFINE_BINOPER(OpNe,!=)
DEFINE_BINOPER(OpLt,<)
DEFINE_BINOPER(OpGt,>)
DEFINE_BINOPER(OpLe,<=)
DEFINE_BINOPER(OpGe,>=)

// Math operators
DEFINE_BINOPER(OpAdd,+)
DEFINE_BINOPER(OpSub,-)
DEFINE_BINOPER(OpMul,*)
DEFINE_BINOPER(OpDiv,/)
DEFINE_BINOPER(OpLShift,<<)
DEFINE_BINOPER(OpRShift,>>)
DEFINE_BINOPER(OpBitAnd,&)
DEFINE_BINOPER(OpBitOr,|)
DEFINE_BINOPER(OpBitXor,^)


// Regular expression operator
DEFINE_BINOPER(OpRegex,REGEX)

DEFINE_BINOPER(OpCtr,CTR)

DEFINE_BINOPER(OpNEInt,NON_EMPTY_INTERSECTION)

// Unary operators
DEFINE_UNOPER(OpNot,NOT)
DEFINE_UNOPER(OpHead,HEAD)

#endif // __POLICY_COMMON_OPERATOR_HH__
