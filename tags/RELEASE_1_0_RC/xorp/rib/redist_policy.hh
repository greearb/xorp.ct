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

// $XORP: xorp/devnotes/template.hh,v 1.1.1.1 2002/12/11 23:55:54 hodson Exp $

#ifndef __RIB_REDIST_POLICY_HH__
#define __RIB_REDIST_POLICY_HH__

/**
 * @short Base class for Redistribution Policy objects.
 *
 * Redistribution Policy objects are intended to be composable.
 * Logical Operators as well as route attibute operators are defined.
 */
template <typename A>
class RedistPolicy {
public:
    /**
     * Determine whether route should be accepted for redistribution.
     *
     * @param ipr route to be examined.
     *
     * @return true if route should be accepted for redistribution, false
     *         otherwise.
     */
    virtual bool accept(const IPRouteEntry<A>& ipr) const = 0;

    virtual ~RedistPolicy() {};
};

/**
 * @short Base class for Unary Redistribution Policy objects.
 */
template <typename A>
class RedistUnaryOp {
public:
    /**
     * Constructor.
     *
     * @param policy policy object allocated with new.
     */
    RedistUnaryOp(const RedistPolicy<A>* policy) : _p1(policy) {}
    ~RedistUnaryOp() { delete _p1; }

private:
    // The following are not implemented
    RedistUnaryOp();
    RedistUnaryOp(const RedistUnaryOp<A>&);
    RedistUnaryOp<A>& operator=(const RedistUnaryOp<A>&);

protected:
    const RedistPolicy<A>* _p1;
};

/**
 * @short Base class for Binary Redistribution Policy objects.
 */
template <typename A>
class RedistBinaryOp : public RedistPolicy<A> {
public:
    /**
     * Constructor.
     *
     * @param one policy object allocated with new.
     * @param two policy object allocated with new.
     *
     * Note: destructor deletes supplied policy objects.
     */
    RedistBinaryOp(RedistPolicy<A>* one, RedistPolicy<A>* two)
	: _p1(one), _p2(two) {}
    ~RedistBinaryOp() { delete _p1; delete _p2; }

private:
    // The following are not implemented
    RedistBinaryOp();
    RedistBinaryOp(const RedistBinaryOp<A>&);
    RedistBinaryOp<A>& operator=(const RedistBinaryOp<A>&);

protected:
    const RedistPolicy<A>* _p1;
    const RedistPolicy<A>* _p2;
};


/**
 * @short Logical-Not for Redistribution Policy objects.
 */
template <typename A>
class RedistLogicalNot : public RedistUnaryOp<A> {
public:
    RedistLogicalNot(const RedistPolicy<A>* p) : RedistUnaryOp<A>(p) {}
    bool accept() const { return ! this->_p1->accept(); }
};

/**
 * @short Logical-And for Redistribution Policy objects.
 */
template <typename A>
class RedistLogicalAnd : public RedistBinaryOp<A> {
public:
    RedistLogicalAnd(const RedistPolicy<A>* p1, const RedistPolicy<A>* p2)
	: RedistBinaryOp<A>(p1, p2)
    {}
    bool accept(const IPRouteEntry<A>& ipr)
    {
	return this->_p1->accept(ipr) && this->_p2->accept(ipr);
    }
};

/**
 * @short Logical-And for Redistribution Policy objects.
 */
template <typename A>
class RedistLogicalOr : public RedistBinaryOp<A> {
public:
    RedistLogicalOr(const RedistPolicy<A>* one, const RedistPolicy<A>* two)
	: RedistBinaryOp<A>(one, two)
    {}
    bool accept(const IPRouteEntry<A>& ipr)
    {
	return this->_p1->accept(ipr) || this->_p2->accept(ipr);
    }
};


/**
 * @short Protocol Policy class.
 *
 * Accepts route update from a specific routing protocol.
 */
template <typename A>
class IsOfProtocol : public RedistPolicy<A>
{
public:
    IsOfProtocol(const Protocol& p) : _protocol(p) {}
    bool accept(const IPRouteEntry<A>& ipr) const {
	return ipr.protocol() == _protocol;
    }
private:
    Protocol _protocol;
};

/**
 * @short IGP Protocol Policy.
 *
 * Accepts route updates from Interior Gateway Protocols.
 */
template <typename A>
class IsIGP : public RedistPolicy<A>
{
public:
    IsIGP() {}
    bool accept(const IPRouteEntry<A>& ipr) const {
	return ipr.protocol_type() == IGP;
    }
};

/**
 * @short EGP Protocol Policy.
 *
 * Accepts route updates from Exterior Gateway Protocols.
 */
template <typename A>
class IsEGP : public RedistPolicy<A>
{
public:
    IsEGP() {}
    bool accept(const IPRouteEntry<A>& ipr) const {
	return ipr.protocol_type() == EGP;
    }
};

#endif // __RIB_REDIST_POLICY_HH__
