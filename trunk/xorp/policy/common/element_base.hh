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

// $XORP: xorp/policy/common/element_base.hh,v 1.8 2008/08/06 08:10:18 abittau Exp $

#ifndef __POLICY_COMMON_ELEMENT_BASE_HH__
#define __POLICY_COMMON_ELEMENT_BASE_HH__

#include <string>

/**
 * @short Basic object type used by policy engine.
 *
 * This element hierarchy is similar to XrlAtom's but exclusive to policy
 * components.
 */
class Element {
public:
    typedef unsigned char Hash;

    Element(Hash hash);
    virtual ~Element();

    /**
     * Every element must be representable by a string. This is a requirement
     * to enable the policy manager to send elements to the backend filters via
     * XRL calls for example.
     *
     * @return string representation of the element.
     */
    virtual std::string str() const = 0;

    /**
     * @return string representation of element type.
     */
    virtual const char* type() const = 0;

    Hash hash() const;

    // XXX don't use for now... not implemented
    void	ref() const;
    void	unref();
    uint32_t	refcount() const;

private:
    mutable uint32_t	_refcount;
    Hash		_hash;
};

#endif // __POLICY_COMMON_ELEMENT_BASE_HH__
