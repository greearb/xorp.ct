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

#ifndef __POLICY_POLICY_LIST_HH__
#define __POLICY_POLICY_LIST_HH__

#include "code_list.hh"
#include "set_map.hh"
#include "var_map.hh"
#include "policy_map.hh"
#include "visitor_semantic.hh"

/**
 * @short The list of policies associated with a protocol.
 *
 * This relates to the import/export directives of protocols.
 *
 * Depending on what protocols support, they will normally have a single
 * import/export policy list associated with them.
 *
 * Each policy list is an instantiation of a policy, and thus it hold the
 * specific code for this instantiation.
 */
class PolicyList {
public:
    typedef set<uint32_t> TagSet;
    typedef map<string,TagSet*> TagMap;

    enum PolicyType {
	IMPORT,
	EXPORT
    };

    /**
     * @param p protocol for which this list applies.
     * @param pt the type of policy list [import/export].
     * @param pmap the container of all policies.
     * @param smap the container of all sets.
     * @param vmap the VarMap.
     */
    PolicyList(const string& p, PolicyType pt, 
	       PolicyMap& pmap,
	       SetMap& smap, VarMap& vmap);

    ~PolicyList();

    /**
     * Append a policy to the list.
     *
     * @param policyname the name of the policy
     */
    void push_back(const string& policyname);

    /**
     * Compiles a specific policy.
     *
     * Throws an exception on semantic / compile errors.
     *
     * @param ps policy to compile.
     * @param mod set filled with targets which are modified by compilation.
     * @param tagstart first policy tag available.
     */
    void compile_policy(PolicyStatement& ps,
			Code::TargetSet& mod, 
			uint32_t& tagstart);

    /**
     * Compile all policies which were not previously compiled.
     *
     * Throws an exception on semantic / compile errors.
     *
     * @param mod set filled with targets which are modified by compilation.
     * @param tagstart first policy tag available.
     */
    void compile(Code::TargetSet& mod, uint32_t& tagstart);
   
    /**
     * @return string representation of list
     */
    string str();

    /**
     * Link the all the code avialable in this policy list with code supplied.
     *
     * The code supplied will normally contain a target, so only relevant code
     * is linked.
     *
     * @param ret code to link with.
     */
    void link_code(Code& ret);

    /**
     * Return all targets in this policy list.
     *
     * @param targets set filled with all targets in this policy instantiation.
     */
    void get_targets(Code::TargetSet& targets);

    /**
     * Return all policy tags for a specific protocol.
     *
     * @param protocol protocol for which tags are requesdted.
     * @param ts set filled with policy-tags used by the protocol.
     */
    void get_tags(const string& protocol, Code::TagSet& ts);

private:
    typedef pair<string,CodeList*> PolicyCode;

    typedef list<PolicyCode> PolicyCodeList;


    /**
     * Semantically check the policy for this instantiation.
     *
     * Throws an exception on error.
     *
     * @param ps policy to check.
     * @param type type of policy [import/export].
     */
    void semantic_check(PolicyStatement& ps, VisitorSemantic::PolicyType type);

    /**
     * Compile an import policy.
     *
     * @param iter position in code list which needs to be replaced with code.
     * @param ps policy to compile.
     * @param modified_targets set filled with targets modified by compilation.
     */
    void compile_import(PolicyCodeList::iterator& iter, PolicyStatement& ps,
			Code::TargetSet& modified_targets);

    /**
     * Compile an export policy.
     *
     * @param iter position in code list which needs to be replaced with code.
     * @param ps policy to compile.
     * @param modified_targets set filled with targets modified by compilation.
     * @param tagstart first policy tag available.
     */
    void compile_export(PolicyCodeList::iterator& iter, PolicyStatement& ps,
			Code::TargetSet& modified_targets, uint32_t& tagstart);


			
    string _protocol;
    PolicyType _type;

    PolicyCodeList _policies;



    PolicyMap& _pmap;

    SetMap& _smap;
    VarMap& _varmap;


    // not impl
    PolicyList(const PolicyList&);
    PolicyList& operator=(const PolicyList&);
};

#endif // __POLICY_POLICY_LIST_HH__
