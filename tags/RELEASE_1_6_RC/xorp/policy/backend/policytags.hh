// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/policy/backend/policytags.hh,v 1.8 2008/08/06 08:24:11 abittau Exp $

#ifndef __POLICY_BACKEND_POLICYTAGS_HH__
#define __POLICY_BACKEND_POLICYTAGS_HH__

#include <set>

#include "policy/common/policy_exception.hh"
#include "policy/common/element_base.hh"
#include "libxipc/xrl_atom_list.hh"

/**
 * @short A set of policy tags. A policytag is a marker for a route.
 *
 * A policytag marks a route. This is needed to match the source part of an
 * export policy. Export filters are in the destination protocol, so no
 * information about the origin of the route is available then. Thus, routes
 * need to be tagged in the source protocol, so the export filter may match
 * against tags.
 */
class PolicyTags {
public:
    /**
     * @short Exception thrown on failed initialization of tags.
     */
    class PolicyTagsError : public PolicyException {
    public:
        PolicyTagsError(const char* file, size_t line, 
			const string& init_why = "")   
	: PolicyException("PolicyTagsError", file, line, init_why) {} 
    };

    /**
     * Empty policytags may be safely created. No exception thrown
     */
    PolicyTags();

    /**
     * Attempt to create policy tags from an XrlAtomList.
     * It must contain unsigned 32bit integer atoms.
     *
     * @param xrlatoms list of xrlatom_uint32 atoms to initialize from.
     */
    PolicyTags(const XrlAtomList& xrlatoms);

    /**
     * @return string representation of policytags.
     */
    string str() const;

    /**
     * @return true if set is equal.
     * @param rhs PolicyTags to compare with.
     */
    bool operator==(const PolicyTags& rhs) const;

    /**
     * Convert to an ElemSet.
     *
     * @return ElemSet representation. Caller is responsible for delete.
     */
    Element* element() const;

    Element* element_tag() const;
    void     set_tag(const Element& e);
    void     set_ptags(const Element& e);

    /**
     * Convert to XrlAtomList of xrlatom_uint32's
     *
     * @return XrlAtomList representation.
     */
    XrlAtomList xrl_atomlist() const;

    /**
     * Insert policy tags from another PolicyTags.
     *
     * @param pt PolicyTags to insert.
     */
    void insert(const PolicyTags& pt);

    void insert(uint32_t tag);

    /**
     * Check if intersection is not empty. 
     *
     * @return true if atleast one tag is contained in the other tags.
     * @param tags tags to check with.
     */
    bool contains_atleast_one(const PolicyTags& tags) const;

private:
    typedef set<uint32_t> Set;

    Set		_tags;
    uint32_t	_tag;
};

#endif // __POLICY_BACKEND_POLICYTAGS_HH__
