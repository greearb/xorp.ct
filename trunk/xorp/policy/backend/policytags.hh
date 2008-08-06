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

// $XORP: xorp/policy/backend/policytags.hh,v 1.7 2008/07/23 05:11:24 pavlin Exp $

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
