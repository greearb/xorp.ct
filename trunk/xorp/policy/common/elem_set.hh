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

// $XORP: xorp/policy/common/elem_set.hh,v 1.11 2008/07/23 05:11:26 pavlin Exp $

#ifndef __POLICY_COMMON_ELEM_SET_HH__
#define __POLICY_COMMON_ELEM_SET_HH__

#include "element_base.hh"
#include "element.hh"
#include <string>
#include <set>

/**
 * @short A set of elements.
 *
 * All sets hold a string representation of the elements. All type information
 * will be lost, as elements will all be promoted to strings.
 */
template <class T> 
class ElemSetAny : public Element {
public:
    typedef set<T> Set;
    typedef typename Set::iterator iterator;
    typedef typename Set::const_iterator const_iterator;

    static const char* id;
    static Hash _hash;

    ElemSetAny(const Set& val);

    /**
     * @param c_str initialize from string in the form element1,element2,...
     */
    ElemSetAny(const char* c_str);
    ElemSetAny();

    /**
     * @return string representation of set.
     */
    string str() const;

    /**
     * @param s element to insert.
     */
    void insert(const T& s);

    /**
     * Insert all elements of other set.
     *
     * @param s set to insert.
     */
    void insert(const ElemSetAny<T>& s);

    /**
     * Left and right sets are identical [same elements and size].
     *
     * @param rhs set to compare with
     */
    bool operator==(const ElemSetAny<T>& rhs) const;
    
    /**
     * Left and right are not identical
     *
     * @param rhs set to compare with
     */
    bool operator!=(const ElemSetAny<T>& rhs) const;

    /**
     * All elements on left match, but right has more elments.
     *
     * @param rhs set to compare with
     */
    bool operator<(const ElemSetAny<T>& rhs) const;

    /**
     * All elements on right match, but left has more elements.
     *
     * @param rhs set to compare with
     */
    bool operator>(const ElemSetAny<T>& rhs) const;

    /**
     * Left is a subset of right.
     *
     * @param rhs set to compare with
     */
    bool operator<=(const ElemSetAny<T>& rhs) const;

    /**
     * Right is a subset of left.
     *
     * @param rhs set to compare with
     */
    bool operator>=(const ElemSetAny<T>& rhs) const;

    /**
     * All elements on left match, but right has more.
     *
     * May only be true if left is an empty set.
     *
     * @param rhs element to compare with.
     */
    bool operator<(const T& rhs) const;
    
    /**
     * All elements on on right match, but left has more.
     *
     * Will be true if the element is present in the set, and the set contains
     * at least one more element.
     *
     * @param rhs element to compare with.
     */
    bool operator>(const T& rhs) const;

    /**
     * Left and right are identical.
     *
     * Will be true in a single element set which contains the rhs element.
     *
     * @param rhs element to compare with.
     */
    bool operator==(const T& rhs) const;

    /**
     * Disjoint sets.
     *
     * Will be true if element is not contained in set.
     *
     * @param rhs element to compare with.
     */
    bool operator!=(const T& rhs) const;

    /**
     * Left is a subset of right.
     *
     * Will be true if set is empty or contains rhs.
     *
     * @param rhs element to compare with.
     */
    bool operator<=(const T& rhs) const;

    /**
     * Right is a subset of left.
     *
     * Will be true if element is contained in set.
     *
     * @param rhs element to compare with.
     */
    bool operator>=(const T& rhs) const;

    /**
     * @return true if intersection is not empty
     */
    bool nonempty_intersection(const ElemSetAny<T>& rhs) const;

    /**
     * Removes elements in set.
     *
     * @param s elements to remove.
     */
    void erase(const ElemSetAny<T>& rhs);

    /**
     * Obtain iterator for set.
     *
     * @return iterator for the set.
     */
    iterator begin();

    /**
     * Obtain an iterator for the end.
     *
     * @return iterator for the end of the set.
     */
    iterator end(); 
    
    /**
     * Obtain const iterator for set.
     *
     * @return const iterator for the set.
     */
    const_iterator begin() const;

    /**
     * Obtain an const iterator for the end.
     *
     * @return const iterator for the end of the set.
     */
    const_iterator end() const;

    const char* type() const;

private:
    Set _val;
};

// define set types
typedef ElemSetAny<ElemU32> ElemSetU32;
typedef ElemSetAny<ElemCom32> ElemSetCom32;
typedef ElemSetAny<ElemIPv4Net> ElemSetIPv4Net;
typedef ElemSetAny<ElemIPv6Net> ElemSetIPv6Net;
typedef ElemSetAny<ElemStr> ElemSetStr;

#endif // __POLICY_COMMON_ELEM_SET_HH__
