// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/policy/common/operator_base.hh,v 1.7 2008/07/23 05:11:26 pavlin Exp $

#ifndef __POLICY_COMMON_OPERATOR_BASE_HH__
#define __POLICY_COMMON_OPERATOR_BASE_HH__

#include <string>

#include "policy/policy_module.h"
#include "libxorp/xlog.h"

enum {
    HASH_OP_AND = 1,
    HASH_OP_OR,
    HASH_OP_XOR,
    HASH_OP_NOT,
    HASH_OP_EQ,
    HASH_OP_NE,
    HASH_OP_LT,
    HASH_OP_GT,
    HASH_OP_LE,
    HASH_OP_GE,
    HASH_OP_ADD,
    HASH_OP_SUB,
    HASH_OP_MUL,
    HASH_OP_REGEX,
    HASH_OP_CTR,
    HASH_OP_NEINT,
    HASH_OP_HEAD,
    HASH_OP_MAX = 32 // last
};

/**
 * @short Base class for operations.
 *
 * An operation is simply an operation that may be done upon elements, such as
 * addition and comparison.
 */
class Oper {
public:
    typedef unsigned char Hash;

    Oper(Hash hash, unsigned arity) : _hash(hash), _arity(arity)
    {
	XLOG_ASSERT(_hash < HASH_OP_MAX);
    }

    virtual ~Oper() {};

    /**
     * @return number of arguments operation takes
     */
    unsigned arity() const { return _arity; }

    /**
     * Must be unique.
     *
     * @return string representation of operation.
     */
    virtual string str() const = 0;

    Hash hash() const { return _hash; }

private:
    Hash	_hash;
    unsigned	_arity;
};

/**
 * @short Base class for unary operations.
 */
class UnOper : public Oper {
public:
    UnOper(Hash hash) : Oper(hash, 1) {}
    virtual ~UnOper() {};

    virtual string str() const = 0;
};

/**
 * @short Base class for binary operations.
 */
class BinOper : public Oper {
public:
    BinOper(Hash hash) : Oper(hash, 2) {}
    virtual ~BinOper() {};

    virtual string str() const = 0;
};

#endif // __POLICY_COMMON_OPERATOR_BASE_HH__
