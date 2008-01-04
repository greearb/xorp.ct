// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/policy/common/operator_base.hh,v 1.5 2007/02/16 22:47:03 pavlin Exp $

#ifndef __POLICY_COMMON_OPERATOR_BASE_HH__
#define __POLICY_COMMON_OPERATOR_BASE_HH__

#include <string>

/**
 * @short Base class for operations.
 *
 * An operation is simply an operation that may be done upon elements, such as
 * addition and comparison.
 */
class Oper {
public:
    typedef unsigned char Hash;
    virtual ~Oper() {};

    /**
     * @return number of arguments operation takes
     */
    virtual unsigned arity() const = 0;

    /**
     * Must be unique.
     *
     * @return string representation of operation.
     */
    virtual string str() const = 0;

    virtual Hash hash() const = 0;
    virtual void set_hash(const Hash&) const = 0;
};


/**
 * @short Base class for unary operations.
 */
class UnOper : public Oper {
public:
    virtual ~UnOper() {};

    virtual string str() const = 0;

    /**
     * @return 1 will always be returned since it is an unary operation.
     */
    unsigned arity() const { return 1; }

    virtual Hash hash() const = 0;
    virtual void set_hash(const Hash&) const = 0;
};

/**
 * @short Base class for binary operations.
 */
class BinOper : public Oper {
public:
    virtual ~BinOper() {};

    virtual string str() const = 0;

    /**
     * @return since it is a binary operation, a constant of 2 is returned.
     */
    unsigned arity() const { return 2; }

    virtual Hash hash() const = 0;
    virtual void set_hash(const Hash&) const = 0;
};

#endif // __POLICY_COMMON_OPERATOR_BASE_HH__
