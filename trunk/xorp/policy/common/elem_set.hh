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

#ifndef __POLICY_COMMON_ELEM_SET_HH__
#define __POLICY_COMMON_ELEM_SET_HH__

#include "element_base.hh"

#include <string>
#include <set>

/**
 * @short A set of elements.
 *
 * All sets hold a string representation of the elements. All type information
 * will be lost, as elements will all be promoted to strings.
 */
class ElemSet : public Element {
public:
    static const char* id;
    ElemSet(const set<string>& val);

    /**
     * @param c_str initialize from string in the form element1,element2,...
     */
    ElemSet(const char* c_str);
    ElemSet();

    /**
     * @return string representation of set.
     */
    string str() const;

    /**
     * @param s string representation of element to insert
     */
    void insert(const string& s);


    /**
     * Left and right sets are identical [same elements and size].
     *
     * @param rhs set to compare with
     */
    bool operator==(const ElemSet& rhs) const;
    
    /**
     * Left and right are not identical
     *
     * @param rhs set to compare with
     */
    bool operator!=(const ElemSet& rhs) const;

    /**
     * All elements on left match, but right has more elments.
     *
     * @param rhs set to compare with
     */
    bool operator<(const ElemSet& rhs) const;

    /**
     * All elements on right match, but left has more elements.
     *
     * @param rhs set to compare with
     */
    bool operator>(const ElemSet& rhs) const;

    /**
     * Left is a subset of right.
     *
     * @param rhs set to compare with
     */
    bool operator<=(const ElemSet& rhs) const;

    /**
     * Right is a subset of left.
     *
     * @param rhs set to compare with
     */
    bool operator>=(const ElemSet& rhs) const;

    /**
     * All elements on left match, but right has more.
     *
     * May only be true if left is an empty set.
     *
     * @param rhs element to compare with.
     */
    bool operator<(const Element& rhs) const;
    
    /**
     * All elements on on right match, but left has more.
     *
     * Will be true if the element is present in the set, and the set contains
     * at least one more element.
     *
     * @param rhs element to compare with.
     */
    bool operator>(const Element& rhs) const;

    /**
     * Left and right are identical.
     *
     * Will be true in a single element set which contains the rhs element.
     *
     * @param rhs element to compare with.
     */
    bool operator==(const Element& rhs) const;

    /**
     * Disjoint sets.
     *
     * Will be true if element is not contained in set.
     *
     * @param rhs element to compare with.
     */
    bool operator!=(const Element& rhs) const;

    /**
     * Left is a subset of right.
     *
     * Will be true if set is empty or contains rhs.
     *
     * @param rhs element to compare with.
     */
    bool operator<=(const Element& rhs) const;

    /**
     * Right is a subset of left.
     *
     * Will be true if element is contained in set.
     *
     * @param rhs element to compare with.
     */
    bool operator>=(const Element& rhs) const;

    /**
     * @return reference to the actual set.
     */
    const set<string>& get_set() const { return _val; }

private:
    set<string> _val;
};


#endif // __POLICY_COMMON_ELEM_SET_HH__
