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

#ifndef __POLICY_TERM_HH__
#define __POLICY_TERM_HH__

#include "policy/common/policy_exception.hh"

#include "parser.hh"

#include <vector>
#include <string>

/**
 * @short A term is an atomic policy unit. 
 *
 * It is a complete specification of how a route needs to be matched, and what
 * actions must be taken.
 */
class Term {
public:
    typedef vector<Node*> Nodes;

    /**
     * @short Exception thrown on a syntax error while parsing configuration.
     */
    class term_syntax_error :  public PolicyException {
    public:
	term_syntax_error(const string& r) : PolicyException(r) {}
    };


    /**
     * @param name term name.
     */
    Term(const string& name);
    ~Term();
   
    /**
     * @return name of the term.
     */
    const string& name() const { return _name; }
    
    /**
     * @return the original user source block configuration.
     */
    const string& source() const { return _source; }

    /**
     * @return the original user dest block configuration.
     */
    const string& dest() const { return _dest; }

    /**
     * @return the original user action block configuration.
     */
    const string& action() const { return _action; }
   

    /**
     * @param src the un-parsed source block configuration.
     */
    void set_source(const string& src);

    /**
     * @param dst the un-parsed dest block configuration.
     */
    void set_dest(const string& dst); 

    /**
     * @param act the un-parsed action block configuration.
     */
    void set_action(const string& act); 

    /**
     * @return string representation of term.
     */
    string str(); 

    /**
     * Visitor implementation.
     *
     * @param v visitor used to visit this term.
     */
    const Element* accept(Visitor& v) {
	return v.visit(*this);
    }


    /**
     * @return parse tree of source block.
     */
    Nodes& source_nodes() { return *_source_nodes; }

    /**
     * @return parse tree of dest block.
     */
    Nodes& dest_nodes() { return *_dest_nodes; }

    /**
     * @return parse tree of action block.
     */
    Nodes& action_nodes() { return *_action_nodes; }

private:
    string _name;
    
    string _source;
    Nodes* _source_nodes; 
    
    string _dest;
    Nodes* _dest_nodes;
    
    string _action;
    Nodes* _action_nodes;

    Parser _parser;

    // not impl
    Term(const Term&);
    Term& operator=(const Term&);
};

#endif // __POLICY_TERM_HH__
