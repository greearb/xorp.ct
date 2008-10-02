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

// $XORP: xorp/policy/set_map.hh,v 1.11 2008/08/06 08:28:01 abittau Exp $

#ifndef __POLICY_SET_MAP_HH__
#define __POLICY_SET_MAP_HH__

#include <string>
#include <vector>

#include "policy/common/element_factory.hh"
#include "policy/common/policy_exception.hh"
#include "dependency.hh"

typedef vector<string>	SETS;

/**
 * @short Container of all sets.
 *
 * The SetMap owns all sets in the policy configuration. It also tracks
 * dependencies between sets and policies.
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
     * Add a dependency of a policy using a set.
     *
     * Throws an exception if set is not found.
     *
     * @param setname name of set in which dependency should be added.
     * @param policyname name of policy which uses the set.
     */
    void add_dependency(const string& setname, const string& policyname);

    /**
     * Delete a dependency of a policy using a set.
     *
     * Throws an exception if set or policy is not found.
     *
     * @param setname name of set in which dependency should be removed.
     * @param policyname name of policy which no longer uses the set.
     */
    void del_dependency(const string& setname, const string& policyname);

    /**
     * @return string representation of all sets.
     */
    string str() const;

    void sets_by_type(SETS& s, const string& type) const;

private:
    typedef Dependency<Element> Dep;

    Dep		    _deps;
    ElementFactory  _ef;
};

#endif // __POLICY_SET_MAP_HH__
