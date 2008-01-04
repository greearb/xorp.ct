// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/policy/policy_map.hh,v 1.7 2007/02/16 22:46:54 pavlin Exp $

#ifndef __POLICY_POLICY_MAP_HH__
#define __POLICY_POLICY_MAP_HH__

#include <string>

#include "policy/common/policy_exception.hh"
#include "policy_statement.hh"
#include "dependancy.hh"

/**
 * @short Container of all policies.
 *
 * It relates policy names with actual policies and deals with dependancies.
 */
class PolicyMap {
public:
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
     * Attempts to create a policy and update depndancies.
     *
     * Throws an exception on error.
     *
     * @param name name of policy.
     * @param smap SetMap used for updating dependancies.
     */
    void create(const string& name,SetMap& smap);

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
    void add_dependancy(const string& policyname, const string& protocol);

    /**
     * Remove the use of a policy by a protocol.
     *
     * @param policyname policy name.
     * @param protocol name of protocol which no longer uses policy.
     */
    void del_dependancy(const string& policyname, const string& protocol);

    /**
     * Dumps all policies in human readable format.
     *
     * @return string representation of all policies.
     */
    string str();

    void clear() { _deps.clear(); }

private:
    // internally, policystatements are held as pointers.
    typedef Dependancy<PolicyStatement> Dep;

    Dep _deps;
};

#endif // __POLICY_POLICY_MAP_HH__
