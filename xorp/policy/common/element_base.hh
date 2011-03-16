// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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

// $XORP: xorp/policy/common/element_base.hh,v 1.10 2008/10/02 21:58:06 bms Exp $

#ifndef __POLICY_COMMON_ELEMENT_BASE_HH__
#define __POLICY_COMMON_ELEMENT_BASE_HH__



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
    virtual string str() const = 0;

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
