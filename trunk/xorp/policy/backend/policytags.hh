// vim:set sts=4 ts=8:

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

// $XORP$

#ifndef __POLICY_BACKEND_POLICYTAGS_HH__
#define __POLICY_BACKEND_POLICYTAGS_HH__

#include "policy/common/policy_exception.hh"
#include "policy/common/element_base.hh"

#include "libxipc/xrl_atom_list.hh"
#include <set>

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
	PolicyTagsError(const string& err) : PolicyException(err) {}
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
     * Attempt to create policy tags from ElemSet.
     *
     * @param e ElemSet to initialize from.
     */
    PolicyTags(const Element& e);

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

    /**
     * Check if intersection is not empty. 
     *
     * @return true if atleast one tag is contained in the other tags.
     * @param tags tags to check with.
     */
    bool contains_atleast_one(const PolicyTags& tags) const;

private:
    typedef set<uint32_t> Set;

    Set _tags;
};

#endif // __POLICY_BACKEND_POLICYTAGS_HH__
