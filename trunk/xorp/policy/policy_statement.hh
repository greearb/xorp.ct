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

#ifndef __POLICY_POLICY_STATEMENT_HH__
#define __POLICY_POLICY_STATEMENT_HH__

#include "policy/common/policy_exception.hh"

#include "set_map.hh"
#include "term.hh"
#include <list>
#include <set>
#include <string>


/**
 * @short A policy statement is a collection of terms.
 */
class PolicyStatement {

public:
    /**
     * @short Exception thrown on error such as when no term is found.
     */
    class PolicyStatementErr : public PolicyException {
    public:
	PolicyStatementErr(const string& err) : PolicyException(err) {}
    };

    
    typedef list<Term*> TermContainer;

    /**
     * @param name the name of the policy.
     * @param smap the SetMap. Used for dependancy tracking.
     */
    PolicyStatement(const string& name, SetMap& smap);
    ~PolicyStatement();

    /**
     * Append a term at the end of the policy.
     *
     * Caller must not delete / modify pointer.
     *
     * @param term term to append to policy.
     */
    void add_term(Term* term);
  
    /**
     * Throws exception if no term is found.
     *
     * @return term requested.
     * @param name name of term to find.
     */
    Term& find_term(const string& name) const;

    /**
     * Checks if a term already exists.
     *
     * @return true if term exists, false otherwise.
     * @param name term name.
     */
    bool term_exists(const string& name) const;
     
    /**
     * Attempts to delete a term.
     *
     * @return true on successful delete, false otherwise.
     * @param name name of term to delete.
     */
    bool delete_term(const string& name);

    /**
     * @return string representation of policy.
     */
    string str();

    /**
     * @return name of policy.
     */
    const string& name() const;

    /**
     * Visitor implementation.
     *
     * @param v visitor to visit policy.
     */
    bool accept(Visitor& v);

    /**
     * @return terms of this policy
     */
    TermContainer& terms();

    /**
     * Replace the set dependancies.
     *
     * @param sets the new sets this policy is dependant on.
     */
    void set_dependancy(const set<string>& sets);
    
private:
    /**
     * Delete all set dependancies of this policy.
     */
    void del_dependancies();

    /**
     * Get the itherator for a specific term.
     *
     * @return iterator for term.
     * @param name name of the term.
     */
    TermContainer::iterator get_term_iter(const string& name);
    TermContainer::const_iterator get_term_iter(const string& name) const;


    string _name;
    TermContainer _terms;

    set<string> _sets;

    SetMap& _smap;

    // not impl
    PolicyStatement(const PolicyStatement&);
    PolicyStatement& operator=(const PolicyStatement&);

};

#endif // __POLICY_POLICY_STATEMENT_HH__
