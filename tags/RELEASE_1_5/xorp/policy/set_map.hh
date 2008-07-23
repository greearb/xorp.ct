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

// $XORP: xorp/policy/set_map.hh,v 1.8 2008/01/04 03:17:12 pavlin Exp $

#ifndef __POLICY_SET_MAP_HH__
#define __POLICY_SET_MAP_HH__

#include "policy/common/element_factory.hh"
#include "policy/common/policy_exception.hh"
#include "dependancy.hh"
#include <string>

/**
 * @short Container of all sets.
 *
 * The SetMap owns all sets in the policy configuration. It also tracks
 * dependancies between sets and policies.
 */
class SetMap {
public:
    /**
     * @short Exception thrown on error, such as deleting a set in use.
     */
    class SetMapError : public PolicyException {
    public:
        SetMapError(const char* file, size_t line, const string& init_why = "")
            : PolicyException("SetMapError", file, line, init_why) {}  

    };

    /**
     * Throws exception if set is not found.
     *
     * @return set requested.
     * @param name set name requested.
     */
    const Element& getSet(const string& name) const;

    /**
     * Create a new set.
     *
     * Throws exception if set exists.
     *
     * @param name name of the set.
     */
    void create(const string& name);

    /**
     * Replace the elements of a set.
     *
     * Throws an expcetion if set does not exist.
     *
     * @param type type of the set.
     * @param name name of the set.
     * @param elements the new elements comma separated.
     * @param modified set filled with policies which are now modified.
     */
    void update_set(const string& type,
		    const string& name, 
		    const string& elements, 
		    set<string>& modified);

    /**
     * Attempts to delete a set.
     *
     * Throws an exception if set is in use.
     *
     * @param name name of the set.
     */
    void delete_set(const string& name);

    /**
     * Add an element to a set.
     *
     * Throws an expcetion if set does not exist.
     *
     * @param type type of the set.
     * @param name name of the set.
     * @param element the element to add.
     * @param modified set filled with policies which are now modified.
     */
    void add_to_set(const string& type,
		    const string& name,
		    const string& element,
		    set<string>& modified);

    /**
     * Delete an element from a set.
     *
     * Throws an expcetion if set does not exist.
     *
     * @param type type of the set.
     * @param name name of the set.
     * @param element the element to delete.
     * @param modified set filled with policies which are now modified.
     */
    void delete_from_set(const string& type,
			 const string& name,
			 const string& element,
			 set<string>& modified);

    /**
     * Add a dependancy of a policy using a set.
     *
     * Throws an exception if set is not found.
     *
     * @param setname name of set in which dependancy should be added.
     * @param policyname name of policy which uses the set.
     */
    void add_dependancy(const string& setname, const string& policyname);

    /**
     * Delete a dependancy of a policy using a set.
     *
     * Throws an exception if set or policy is not found.
     *
     * @param setname name of set in which dependancy should be removed.
     * @param policyname name of policy which no longer uses the set.
     */
    void del_dependancy(const string& setname, const string& policyname);

    /**
     * @return string representation of all sets.
     */
    string str() const;

private:
    typedef Dependancy<Element> Dep;
    Dep    _deps;
    ElementFactory _ef;

};

#endif // __POLICY_SET_MAP_HH__
