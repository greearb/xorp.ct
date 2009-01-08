// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/policy/policy_map.hh,v 1.14 2008/10/02 21:57:59 bms Exp $

#ifndef __POLICY_POLICY_MAP_HH__
#define __POLICY_POLICY_MAP_HH__

#include <string>
#include <vector>

#include "policy/common/policy_exception.hh"
#include "policy_statement.hh"
#include "dependency.hh"

/**
 * @short Container of all policies.
 *
 * It relates policy names with actual policies and deals with dependencies.
 */
class PolicyMap {
public:
    typedef Dependency<PolicyStatement>::KEYS    KEYS;

    /**
     * @short Exception thrown on errors such as when a policy is not found.
     */
    class PolicyMapError : public PolicyException {
    public:
        PolicyMapError(const char* file, size_t line, 
		       const string& init_why = "")   
	: PolicyException("PolicyMapError", file, line, init_why) {}   
    };

    /**
     * Find a policy.
     *
     * Throws an exception if not found.
     *
     * @return policy requested.
     * @param name name of policy wanted.
     */
    PolicyStatement& find(const string& name) const;

    /**
     * Checks if a policy exists.
     *
     * @return true if policy exists.
     * @param name name of policy.
     */
    bool exists(const string& name);

    /**
     * Attempts to create a policy and update depndencies.
     *
     * Throws an exception on error.
     *
     * @param name name of policy.
     * @param smap SetMap used for updating dependencies.
     */
    void create(const string& name, SetMap& smap);

    /**
     * Attempts to delete a policy.
     *
     * Throws an exception on error.
     *
     * @param name policy name.
     */
    void delete_policy(const string& name);

    /**
     * Indicates the use of a policy by a protocol.
     *
     * @param policyname policy name.
     * @param protocol name of protocol which uses policy.
     */
    void add_dependency(const string& policyname, const string& protocol);

    /**
     * Remove the use of a policy by a protocol.
     *
     * @param policyname policy name.
     * @param protocol name of protocol which no longer uses policy.
     */
    void del_dependency(const string& policyname, const string& protocol);

    /**
     * Dumps all policies in human readable format.
     *
     * @return string representation of all policies.
     */
    string str();

    void clear() { _deps.clear(); }

    void policy_deps(const string& policy, DEPS& deps);
    void policies(KEYS& out);

private:
    // internally, policystatements are held as pointers.
    typedef Dependency<PolicyStatement> Dep;

    Dep _deps;
};

#endif // __POLICY_POLICY_MAP_HH__
